/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVImageExporter.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Async/Async.h"
#include "FileManager.h"
#include "ImageUtils.h"
#include "IImageWrapperModule.h"
#if WITH_UNREALPNG
THIRD_PARTY_INCLUDES_START
#include "ThirdParty/zlib/zlib-1.2.5/Inc/zlib.h"
#include "ThirdParty/libPNG/libPNG-1.5.2/png.h"
#include "ThirdParty/libPNG/libPNG-1.5.2/pnginfo.h"
#include <setjmp.h>
THIRD_PARTY_INCLUDES_END
// Disable warning "interaction between '_setjmp' and C++ object destruction is non-portable"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4611)
#endif
#endif  // WITH_UNREALPNG

// Convert a pixel format to an exportable one
EPixelFormat GetExportImagePixelFormat(EPixelFormat TexturePixelFormat)
{
	switch (TexturePixelFormat)
	{
	case EPixelFormat::PF_R32_FLOAT:
	case EPixelFormat::PF_R32_UINT:
		return EPixelFormat::PF_B8G8R8A8;
	case EPixelFormat::PF_G32R32F:
		return EPixelFormat::PF_A16B16G16R16;
	default:
		return TexturePixelFormat;
	}
}

bool CanPixelFormatBeExported(EPixelFormat CheckPixelFormat)
{
	switch (CheckPixelFormat)
	{
	case EPixelFormat::PF_B8G8R8A8:
	case EPixelFormat::PF_R32_FLOAT:
	case EPixelFormat::PF_R32_UINT:
	case EPixelFormat::PF_A16B16G16R16:
	case EPixelFormat::PF_G32R32F:
	case EPixelFormat::PF_G8:
	case EPixelFormat::PF_ShadowDepth:
		return true;
	default:
		return false;
	}
}

bool GetExportedImageSettings(EPixelFormat ImgPixelFormat, uint8& ImageBitDepth, ERGBFormat& ImageRGBFormat)
{
	if (!CanPixelFormatBeExported(ImgPixelFormat))
	{
		return false;
	}

	// NOTE: We need to map some of the internal pixel format type into exportable ones
	if ((ImgPixelFormat == PF_R32_FLOAT) || (ImgPixelFormat == PF_R32_UINT))
	{
		ImgPixelFormat = PF_B8G8R8A8;
	}
	else if (ImgPixelFormat == PF_G32R32F)
	{
		ImgPixelFormat = PF_A16B16G16R16;
	}

	ImageBitDepth = NVSceneCapturerUtils::GetBitCountPerChannel(ImgPixelFormat);
	const uint8 ImgColorChannelCount = NVSceneCapturerUtils::GetColorChannelCount(ImgPixelFormat);
	ImageRGBFormat = (ImgColorChannelCount == 1) ? ERGBFormat::Gray : ERGBFormat::BGRA;

	return true;
}

FNVImageExporter::FNVImageExporter(IImageWrapperModule* InImageWrapperModule)
    : ImageWrapperModule(InImageWrapperModule)
{
	check(InImageWrapperModule != nullptr);
}

FNVImageExporter::~FNVImageExporter()
{
    ImageWrapperModule = nullptr;
}

#if WITH_UNREALPNG
/**
* Guard that safely releases PNG Writer resources
*/
class PNGWriteGuard
{
public:

    PNGWriteGuard(png_structp* InWriteStruct, png_infop* InInfo)
        : PNGWriteStruct(InWriteStruct)
        , info_ptr(InInfo)
        , PNGRowPointers(NULL)
    {
    }

    ~PNGWriteGuard()
    {
        if (PNGRowPointers != NULL)
        {
            png_free(*PNGWriteStruct, PNGRowPointers);
        }
        png_destroy_write_struct(PNGWriteStruct, info_ptr);
    }

    void SetRowPointers(png_bytep* InRowPointers)
    {
        ensure(InRowPointers!=nullptr);
        PNGRowPointers = InRowPointers;
    }

private:

    png_structp* PNGWriteStruct;
    png_infop* info_ptr;
    png_bytep* PNGRowPointers;
};

void PngArrayWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    TArray<uint8>* CompressedData = (TArray<uint8>*)png_get_io_ptr(png_ptr);
    ensure(CompressedData);
    int32 Offset = CompressedData->AddUninitialized(length);
    FMemory::Memcpy(&((*CompressedData)[Offset]), data, length);
}
#endif // WITH_UNREALPNG

TArray<uint8> FNVImageExporter::CompressImagePNG(const FNVTexturePixelData& SourcePixelData)
{
    TArray<uint8> CompressedData;
    CompressedData.Reset(0);

    // Currently, supported pixel format is limited.
    EPixelFormat ImgPixelFormat = SourcePixelData.PixelFormat;
	if (ensure(CanPixelFormatBeExported(ImgPixelFormat)))
    {
        // NOTE: This code is similar to FImageWrapperBase::SetRaw
        TArray<uint8> RawData = SourcePixelData.PixelData;
        if (RawData.Num() <= 0)
        {
            UE_LOG(LogNVSceneCapturer, Error, TEXT("Number of Pixels is 0."));
        }
        else
        {
			uint8 RawBitDepth = 32;
			ERGBFormat RawFormat = ERGBFormat::BGRA;
			if (GetExportedImageSettings(ImgPixelFormat, RawBitDepth, RawFormat))
			{
				const auto& ImageSize = SourcePixelData.PixelSize;
				int32 Width = ImageSize.X;
				int32 Height = ImageSize.Y;

				// NOTE: This code is similar to FPngImageWrapper::Compress without the scope lock so we can run this in parallel in multiple threads
	#if WITH_UNREALPNG

				jmp_buf SetjmpBuffer;

				// Reset to the beginning of file so we can use png_read_png(), which expects to start at the beginning.
				int32 ReadOffset = 0;

				png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
				check(png_ptr);
				if (png_ptr == nullptr)
				{
					UE_LOG(LogNVSceneCapturer, Error, TEXT("png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr) is failed"));
				}
				else
				{
					png_infop info_ptr = png_create_info_struct(png_ptr);
					check(info_ptr);
					if (info_ptr == nullptr)
					{
						UE_LOG(LogNVSceneCapturer, Error, TEXT("png_create_info_struct() is failed"));
					}
					else
					{
						png_bytep* row_pointers = (png_bytep*)png_malloc(png_ptr, Height * sizeof(png_bytep));
						PNGWriteGuard PNGGuard(&png_ptr, &info_ptr);
						PNGGuard.SetRowPointers(row_pointers);
						{
							png_set_compression_level(png_ptr, Z_BEST_SPEED);
							png_set_IHDR(png_ptr, info_ptr, Width, Height, RawBitDepth, (RawFormat == ERGBFormat::Gray) ?
								PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
							png_set_write_fn(png_ptr, &CompressedData, PngArrayWriteCallback, nullptr);

							const uint32 PixelChannels = (RawFormat == ERGBFormat::Gray) ? 1 : 4;
							const uint32 BytesPerPixel = (RawBitDepth * PixelChannels) / 8;
							const uint32 BytesPerRow = BytesPerPixel * Width;

							for (int32 i = 0; i < Height; i++)
							{
								row_pointers[i] = &RawData[i * BytesPerRow];
							}
							png_set_rows(png_ptr, info_ptr, row_pointers);

							uint32 Transform = (RawFormat == ERGBFormat::BGRA) ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

							// PNG files store 16-bit pixels in network byte order (big-endian, ie. most significant bits first).
#if PLATFORM_LITTLE_ENDIAN
						// We're little endian so we need to swap
							if (RawBitDepth == 16)
							{
								Transform |= PNG_TRANSFORM_SWAP_ENDIAN;
							}
#endif
							if (!setjmp(SetjmpBuffer))
							{
								png_write_png(png_ptr, info_ptr, Transform, NULL);
							}
						}
#endif // WITH_UNREALPNG
					}
				}
            }
			else
			{
				UE_LOG(LogNVSceneCapturer, Error, TEXT("Unsupported pixel format."));
			}
        }
    }
	else
	{
		UE_LOG(LogNVSceneCapturer, Error, TEXT("Unsupported pixel format."));
	}

    return CompressedData;
}

TArray<uint8>  FNVImageExporter::CompressImage(IImageWrapperModule* ImageWrapperModule, const FNVTexturePixelData& SourcePixelData,
        ENVImageFormat ImageFormat, uint8 CompressionQuality/*= 100*/)
{
    TArray<uint8> CompressedData;
    CompressedData.Reset();
    const auto& PixelData = SourcePixelData.PixelData;
    const uint32 PixelCount = PixelData.Num();

    if ((PixelCount == 0) ||                    // The source pixels data must be valid
            (ImageWrapperModule == nullptr) ||      // We need a valid image wrapper module reference
            (ImageFormat == ENVImageFormat::BMP))   // Don't handle compression for BMP format
    {
        return CompressedData;
    }

    if (ImageFormat == ENVImageFormat::PNG)
    {
        return CompressImagePNG(SourcePixelData);
    }

    const EImageFormat ImageFormatType = ConvertExportFormatToImageFormat(ImageFormat);
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule->CreateImageWrapper(ImageFormatType);
    if (!ImageWrapper.IsValid())
    {
        return CompressedData;
    }

    EPixelFormat ImgPixelFormat = SourcePixelData.PixelFormat;
	uint8 ImgBitDepth = 32;
	ERGBFormat ImgRGBFormat = ERGBFormat::BGRA;
	if (GetExportedImageSettings(ImgPixelFormat, ImgBitDepth, ImgRGBFormat))
	{
		const auto& ImageSize = SourcePixelData.PixelSize;

		const void* RawData = (void*)PixelData.GetData();
		int32 AllocatedSize = PixelData.GetAllocatedSize();
		ImageWrapper->SetRaw(RawData, AllocatedSize, ImageSize.X, ImageSize.Y, ImgRGBFormat, ImgBitDepth);
		CompressedData = ImageWrapper->GetCompressed(CompressionQuality);

		ImageWrapper.Reset();
	}
	else
	{
		UE_LOG(LogNVSceneCapturer, Error, TEXT("Unsupported pixel format."));
	}
    return CompressedData;
}

bool FNVImageExporter::ExportImage(IImageWrapperModule* ImageWrapperModule, const FNVImageExporterData& ImageExporterData)
{
	bool bResult = false;
	const auto& ExportedPixelData = ImageExporterData.PixelDataToBeExported;
	const auto& PixelData = ExportedPixelData.PixelData;
	const auto& ExportFilePath = ImageExporterData.ExportFilePath;
	const auto& ExportImageFormat = ImageExporterData.ExportImageFormat;
	uint32 PixelCount = PixelData.Num();

	if ((PixelCount != 0) && (ImageWrapperModule != nullptr))
	{
		if (ExportImageFormat == ENVImageFormat::BMP)
		{
			const auto& ImageSize = ExportedPixelData.PixelSize;
			bResult = FFileHelper::CreateBitmap(*ExportFilePath, ImageSize.X, ImageSize.Y, (FColor*)((void*)PixelData.GetData()));
		}
		else
		{
			const uint8 CompressedQuality = 100;
			const TArray<uint8>& CompressedBitmap = CompressImage(ImageWrapperModule, ExportedPixelData, ExportImageFormat, CompressedQuality);
			bResult = FFileHelper::SaveArrayToFile(CompressedBitmap, *ExportFilePath);
		}
		if (!bResult)
		{
			UE_LOG(LogNVSceneCapturer, Error, TEXT("Unable to save image to file.  Check permissions. File is %s"), *ExportFilePath);
		}
	}
	return bResult;
}

bool FNVImageExporter::ExportImage(const FNVImageExporterData& ImageExporterData)
{
	return FNVImageExporter::ExportImage(ImageWrapperModule, ImageExporterData);
}

//====================================== FNVSaveImageToFileThread ==========================================
FNVImageExporter_Thread::FNVImageExporter_Thread(IImageWrapperModule* InImageWrapperModule)
    : ImageWrapperModule(InImageWrapperModule)
{
    ensure(ImageWrapperModule);

    bIsRunning = false;

    PendingImageCounter.Reset();

    ExportingImageCounterPtr = MakeShareable(new FThreadSafeCounter());
    ExportingImageCounterPtr->Reset();

    QueuedImageData.Empty();

    HavePendingImageEvent = FPlatformProcess::GetSynchEventFromPool();

    // TODO: May move this Thread to a Start function
    // Create the thread for this task to run
    static int32 ThreadIndex = 0;
    const FString& ThreadName = FString::Printf(TEXT("NVSaveImageToFileThread_%d"), ++ThreadIndex);
    const uint32 ThreadStackSize = 0;
    const EThreadPriority& ThreadPriority = EThreadPriority::TPri_TimeCritical;
    const uint64 ThreadAffinityMask = FPlatformAffinity::GetNoAffinityMask();
    Thread = FRunnableThread::Create(this, *ThreadName, ThreadStackSize, ThreadPriority, ThreadAffinityMask);
}

FNVImageExporter_Thread::~FNVImageExporter_Thread()
{
    ImageWrapperModule = nullptr;
    Kill();
}

bool FNVImageExporter_Thread::ExportImage(const FNVTexturePixelData& ExportPixelData, const FString& ExportFilePath, const ENVImageFormat ExportImageFormat/*= ENVImageFormat::PNG*/)
{
    FNVImageExporterData NewImageExporterData = FNVImageExporterData(ExportPixelData, ExportFilePath, ExportImageFormat);
    QueuedImageData.Enqueue(MoveTemp(NewImageExporterData));
    PendingImageCounter.Increment();

    if (HavePendingImageEvent)
    {
        HavePendingImageEvent->Trigger();
    }

    // TODO: Check whether the image data are valid or not
    return true;
}

uint32 FNVImageExporter_Thread::Run()
{
    bIsRunning = true;

    // How to run each exporting tasks
    const EAsyncExecution AsyncExecution = EAsyncExecution::ThreadPool;

    while (bIsRunning)
    {
        // Process all the queued frame data
        while (!QueuedImageData.IsEmpty())
        {
            FNVImageExporterData TmpImageData;
            QueuedImageData.Dequeue(TmpImageData);

            PendingImageCounter.Decrement();

            // TODO: Limit the number of exporting threads running at the same time
            // Need to check if ExportingImageCounterPtr < ThreadThreshold
            ExportingImageCounterPtr->Increment();
            auto TempExportingImageCounterPtr = ExportingImageCounterPtr;
            auto TempImageWrapperModule = ImageWrapperModule;
            Async<void>(AsyncExecution, [TempExportingImageCounterPtr, TempImageWrapperModule, CheckImageData = MoveTemp(TmpImageData)]
            {
                FNVImageExporter::ExportImage(TempImageWrapperModule, CheckImageData);
                if (TempExportingImageCounterPtr.IsValid())
                {
                    TempExportingImageCounterPtr->Decrement();
                }
            });
        }

        if (HavePendingImageEvent)
        {
            HavePendingImageEvent->Wait();
        }
        else
        {
            FPlatformProcess::Sleep(0.1f);
        }
    }

    return 0;
}

void FNVImageExporter_Thread::Stop()
{
    bIsRunning = false;
    QueuedImageData.Empty();
    if (HavePendingImageEvent)
    {
        // Trigger the event so the thread doesn't wait anymore
        HavePendingImageEvent->Trigger();
        HavePendingImageEvent->Reset();
    }
}

void FNVImageExporter_Thread::Kill()
{
    if (Thread)
    {
        Thread->Kill(true);
        Thread = nullptr;
    }
}

uint32 FNVImageExporter_Thread::GetPendingImagesCount() const
{
    return PendingImageCounter.GetValue() + (ExportingImageCounterPtr.IsValid() ? ExportingImageCounterPtr->GetValue() : 0);
}

bool FNVImageExporter_Thread::IsExportingImage() const
{
    return ExportingImageCounterPtr.IsValid() && (ExportingImageCounterPtr->GetValue() > 0);
}

//====================================== FNVImageExporterData ==========================================
FNVImageExporterData::FNVImageExporterData()
{
    ExportFilePath = TEXT("");
	ExportImageFormat = ENVImageFormat::PNG;
}

FNVImageExporterData::FNVImageExporterData(const FNVTexturePixelData& InPixelDataToBeExported, const FString InExportFilePath, ENVImageFormat InExportImageFormat /*= ENVImageFormat::PNG*/)
	: PixelDataToBeExported(InPixelDataToBeExported),
	ExportFilePath(InExportFilePath),
	ExportImageFormat(InExportImageFormat)
{
}
