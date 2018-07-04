/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneCapturerUtils.h"
#include "HAL/Runnable.h"
#include "IImageWrapper.h"
#include "NVImageExporter.generated.h"

USTRUCT()
struct NVSCENECAPTURER_API FNVImageExporterData
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FNVTexturePixelData PixelDataToBeExported;

    UPROPERTY()
    FString ExportFilePath;

	UPROPERTY()
	ENVImageFormat ExportImageFormat;

public:
	FNVImageExporterData();
    FNVImageExporterData(const FNVTexturePixelData& InPixelDataToBeExported,
						const FString InExportFilePath,
						ENVImageFormat InExportImageFormat = ENVImageFormat::PNG);
};

struct NVSCENECAPTURER_API FNVImageExporter
{
public:
    FNVImageExporter(IImageWrapperModule* InImageWrapperModule);
    ~FNVImageExporter();

    /// Compress a source image data to PNG format
    /// NOTE: PNG is lossless compression so we can't change the compression quality
    /// This function always use Z_BEST_SPEED, may be we want to customize the compression level
    /// result   The compressed data in bytes
    static TArray<uint8> CompressImagePNG(const FNVTexturePixelData& SourcePixelData);

    /// Compress a source image to a certain image type
    /// @param ImageWrapperModule    Reference to the ImageWrapper module
    /// @param SourcePixelData       The source, raw pixel data
    /// @param ImageFormat           The type of the image to compress to
    /// @param CompressionQuality    The quality of the compression
    /// result                       The compressed data in bytes
    static TArray<uint8> CompressImage(IImageWrapperModule* ImageWrapperModule,
                                       const FNVTexturePixelData& SourcePixelData,
                                       ENVImageFormat ImageFormat,
                                       uint8 CompressionQuality = 100);

    /// Export an in-memory image to file on disk
    static bool ExportImage(IImageWrapperModule* ImageWrapperModule, const FNVImageExporterData& ImageExporterData);

	bool ExportImage(const FNVImageExporterData& ImageExporterData);

protected:
    IImageWrapperModule* ImageWrapperModule;
};

struct NVSCENECAPTURER_API FNVImageExporter_Thread : public FRunnable
{
public:
    FNVImageExporter_Thread(IImageWrapperModule* InImageWrapperModule);
    ~FNVImageExporter_Thread();

    bool ExportImage(const FNVTexturePixelData& ExportPixelData,
                     const FString& ExportFilePath,
					 const ENVImageFormat ExportImageFormat = ENVImageFormat::PNG);

    virtual uint32 Run();
    virtual void Stop() override;

    uint32 GetPendingImagesCount() const;
    bool IsExportingImage() const;

protected:
    FRunnableThread* Thread;
    bool bIsRunning;

    TQueue<FNVImageExporterData> QueuedImageData;

    IImageWrapperModule* ImageWrapperModule;

    FEvent* HavePendingImageEvent;
    FThreadSafeCounter PendingImageCounter;
    TSharedPtr<FThreadSafeCounter, ESPMode::ThreadSafe> ExportingImageCounterPtr;
};