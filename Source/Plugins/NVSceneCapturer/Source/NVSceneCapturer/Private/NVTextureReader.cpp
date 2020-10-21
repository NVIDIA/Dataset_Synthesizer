/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVTextureReader.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ScreenRendering.h"
#include "PipelineStateCache.h"
#include "CommonRenderResources.h"
#include "RenderingThread.h"
#include "RendererInterface.h"
#include "RenderTargetPool.h"
#include "StaticBoundShaderState.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY(LogNVTextureReader);

//======================= FNVTextureReader =======================//
FNVTextureReader::FNVTextureReader()
{
    SourceTexture = nullptr;
}

FNVTextureReader::~FNVTextureReader()
{
    SourceTexture = nullptr;
}

FNVTextureReader& FNVTextureReader::operator=(const FNVTextureReader& OtherReader)
{
    SourceTexture = OtherReader.SourceTexture;
    SourceRect = OtherReader.SourceRect;
    ReadbackPixelFormat = OtherReader.ReadbackPixelFormat;
    ReadbackSize = OtherReader.ReadbackSize;

    return (*this);
}

void FNVTextureReader::SetSourceTexture(FTexture2DRHIRef NewSourceTexture,
                                        const FIntRect& NewSourceRect /*= FIntRect()*/,
                                        EPixelFormat NewReadbackPixelFormat /*= EPixelFormat::PF_Unknown*/,
                                        const FIntPoint& NewReadbackSize /*= FIntPoint::ZeroValue*/)
{
    // SourceTexture can be null by design.
    // so we don't check NewSourceTexture here.
    SourceTexture = NewSourceTexture;
    SourceRect = NewSourceRect;
    ReadbackPixelFormat = NewReadbackPixelFormat;
    ReadbackSize = NewReadbackSize;

    if (SourceTexture)
    {
        if (ReadbackSize == FIntPoint::ZeroValue)
        {
            ReadbackSize = SourceTexture->GetSizeXY();
        }
        if (ReadbackPixelFormat == EPixelFormat::PF_Unknown)
        {
            ReadbackPixelFormat = SourceTexture->GetFormat();

            if (GDynamicRHI)
            {
                const FString RHIName = GDynamicRHI->GetName();
                // NOTE: UE4's D3D11 implement of the RHI doesn't support all the pixel formats so we must change it to be another format with the same pixel size
                if (RHIName.Contains(TEXT("D3D11")))
                {
                    if ((ReadbackPixelFormat == PF_R16F) || (ReadbackPixelFormat == PF_R16_UINT))
                    {
                        ReadbackPixelFormat = PF_ShadowDepth;
                    }
                }
                // TODO: Should we ignore non-supported pixel format?
				// NOTE: Since we read back the pixel in bytes, we need to change the format to be uint mode instead of float
                if (ReadbackPixelFormat == PF_R32_FLOAT)
                {
                    ReadbackPixelFormat = PF_R32_UINT;
                }
            }
        }
        if (SourceRect.IsEmpty())
        {
            SourceRect = FIntRect(FIntPoint::ZeroValue, SourceTexture->GetSizeXY());
        }
    }
}

// Read back the pixels data from the current source texture
// NOTE: This function is sync, the pixels data is returned right away but it may cause the game to hitches since it flush the rendering commands
bool FNVTextureReader::ReadPixelsData(FNVTexturePixelData& OutPixelsData)
{
    bool bResult = false;
    if (SourceTexture)
    {
        ENQUEUE_RENDER_COMMAND(ReadPixelsFromTexture)(
            [this, &OutPixelsData = OutPixelsData](FRHICommandListImmediate& RHICmdList)
            {
                void* PixelDataBuffer = nullptr;
                FIntPoint PixelSize = FIntPoint::ZeroValue;
                RHICmdList.MapStagingSurface(SourceTexture, PixelDataBuffer, PixelSize.X, PixelSize.Y);

                BuildPixelData(OutPixelsData, (uint8*)PixelDataBuffer, ReadbackPixelFormat, PixelSize, ReadbackSize);

                RHICmdList.UnmapStagingSurface(SourceTexture);
            });

        FlushRenderingCommands();
        bResult = true;
    }
    return bResult; 
}

// Read back the pixels data from the current source texture
// NOTE: This function is sync, the pixels data is returned right away but it may cause the game to hitches since it flush the rendering commands
bool FNVTextureReader::ReadPixelsData(
    FNVTexturePixelData& OutPixelsData,
    const FTexture2DRHIRef& NewSourceTexture,
    const FIntRect& NewSourceRect /* = FIntRect()*/,
    EPixelFormat NewReadbackPixelFormat /*= EPixelFormat::PF_Unknown*/,
    const FIntPoint& NewReadbackSize /*= FIntPoint::ZeroValue*/)
{
    SetSourceTexture(NewSourceTexture, NewSourceRect, NewReadbackPixelFormat, NewReadbackSize);
    return ReadPixelsData(OutPixelsData);
}

bool FNVTextureReader::ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback, bool bIgnoreAlpha/*= false*/)
{
    bool bResult = false;

    ensure(Callback);
    if (!Callback)
    {
        UE_LOG(LogNVTextureReader, Error, TEXT("invalid argument."));
    }
    else
    {
        if (SourceTexture)
        {
            bResult = ReadPixelsRaw(SourceTexture,
                SourceRect, ReadbackPixelFormat, ReadbackSize, bIgnoreAlpha,
                [=](uint8* PixelData, EPixelFormat PixelFormat, FIntPoint PixelSize)
            {
                Callback(BuildPixelData(PixelData, PixelFormat, PixelSize, ReadbackSize));
            });
        }
    }
    return bResult;
}

bool FNVTextureReader::ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback,
                                      const FTexture2DRHIRef& NewSourceTexture,
                                      const FIntRect& NewSourceRect /*= FIntRect()*/,
                                      EPixelFormat NewReadbackPixelFormat /*= EPixelFormat::PF_Unknown*/,
                                      const FIntPoint& NewReadbackSize /*= FIntPoint::ZeroValue*/,
                                      bool bIgnoreAlpha/*= false*/)
{
    ensure(Callback);
    SetSourceTexture(NewSourceTexture, NewSourceRect, NewReadbackPixelFormat, NewReadbackSize);
    return ReadPixelsData(Callback, bIgnoreAlpha);
}

bool FNVTextureReader::ReadPixelsRaw(const FTexture2DRHIRef& NewSourceTexture, const FIntRect& SourceRect,
                                     EPixelFormat TargetPixelFormat, const FIntPoint& TargetSize, bool bIgnoreAlpha, OnFinishedReadingRawPixelsCallback Callback)
{
    bool bResult = false;

    // Make sure the source render target and area settings are valid
    ensure(NewSourceTexture);
    ensure(SourceRect.Area() != 0);
    ensure(TargetSize != FIntPoint::ZeroValue);
    ensure(Callback);
    if (!NewSourceTexture
            || (SourceRect.Area() == 0)
            || (TargetSize == FIntPoint::ZeroValue)
            || (!Callback))
    {
        UE_LOG(LogNVTextureReader, Error, TEXT("invalid argument."));
    }
    else
    {
        static const FName RendererModuleName("Renderer");
        // Load the renderer module on the main thread, as the module manager is not thread-safe, and copy the ptr into the render command, along with 'this' (which is protected by BlockUntilAvailable in ~FViewportSurfaceReader())
        IRendererModule* RendererModule = &FModuleManager::GetModuleChecked<IRendererModule>(RendererModuleName);
        check(RendererModule);

        // NOTE: This approach almost identical to function FViewportSurfaceReader::ResolveRenderTarget in FrameGrabber.cpp
        // The main different is we reading the pixels back from a render target instead of a viewport
        ENQUEUE_RENDER_COMMAND(ReadPixelsFromTexture)(
            [=](FRHICommandListImmediate& RHICmdList)
            {
                FRHIResourceCreateInfo CreateInfo(FClearValueBinding::None);
                // TODO: Can cache the ReadbackTexture and reuse it if the format and size doesn't change instead of creating it everytime we read like this
                // Need to be careful with reusing the ReadbackTexture: need to make sure the texture is already finished reading
                FTexture2DRHIRef ReadbackTexture = RHICreateTexture2D(
                                                       TargetSize.X,
                                                       TargetSize.Y,
                                                       TargetPixelFormat,
                                                       1,
                                                       1,
                                                       TexCreate_CPUReadback,
                                                       CreateInfo
                                                   );

                bool bOverwriteAlpha = !bIgnoreAlpha;
                // Copy the source texture to the readback texture so we can read it back later even after the source texture is modified
                CopyTexture2d(RendererModule, RHICmdList, NewSourceTexture, SourceRect, ReadbackTexture, FIntRect(FIntPoint::ZeroValue, TargetSize), bOverwriteAlpha);

                // Stage the texture to read back its pixels data
                FIntPoint PixelSize = FIntPoint::ZeroValue;
                void* PixelDataBuffer = nullptr;
                RHICmdList.MapStagingSurface(ReadbackTexture, PixelDataBuffer, PixelSize.X, PixelSize.Y);

                if (PixelDataBuffer)
                {
                    Callback((uint8*)PixelDataBuffer, TargetPixelFormat, PixelSize);
                }

                RHICmdList.UnmapStagingSurface(ReadbackTexture);

                ReadbackTexture.SafeRelease();
            });
        bResult = true;
    }

    return bResult;
}

void FNVTextureReader::CopyTexture2d(class IRendererModule* RendererModule, FRHICommandListImmediate& RHICmdList,
                                     const FTexture2DRHIRef& NewSourceTexture, const FIntRect& SourceRect,
                                     FTexture2DRHIRef& ReadbackTexture, const FIntRect& TargetRect, bool bOverwriteAlpha/*= true*/)
{
    ensure(RendererModule);
    ensure(NewSourceTexture);
    ensure(ReadbackTexture);
    if ((!RendererModule) || (!NewSourceTexture) || (!ReadbackTexture))
    {
        UE_LOG(LogNVTextureReader, Error, TEXT("invalid argument."));
    }
    else
    {
        const FIntPoint TargetSize = TargetRect.Size();
        FPooledRenderTargetDesc OutputDesc = FPooledRenderTargetDesc::Create2DDesc(
            TargetSize,
            ReadbackTexture->GetFormat(),
            FClearValueBinding::None,
            TexCreate_None,
            TexCreate_RenderTargetable,
            false);

        TRefCountPtr<IPooledRenderTarget> ResampleTexturePooledRenderTarget;
        GRenderTargetPool.FindFreeElement(RHICmdList, OutputDesc, ResampleTexturePooledRenderTarget, TEXT("ResampleTexture"));
        check(ResampleTexturePooledRenderTarget);
        // Get a temporary render target from the render thread's pool to draw the source render target on
        const FSceneRenderTargetItem& DestRenderTarget = ResampleTexturePooledRenderTarget->GetRenderTargetItem();

        FRHIRenderPassInfo RPInfo(DestRenderTarget.TargetableTexture, ERenderTargetActions::Load_Store, ReadbackTexture);
        RHICmdList.BeginRenderPass(RPInfo, TEXT("TextureReaderResolveRenderTarget"));
        {
            RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
            //GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
            // NOTE: Render the source texture as is on to the target:
            // RGB = src.rgb * 1 + dst.rgb * 0
            // A = src.a * 1 + dst.a * 0
            if (bOverwriteAlpha)
            {
                GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
            }
            else
            {
                // A = MAX(src.a, dst.a)
                GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Max, BF_One, BF_One>::GetRHI();
            }
            GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
            GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

            const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;

            //TShaderMap<FGlobalShaderType>* ShaderMap = GetGlobalShaderMap(FeatureLevel);
            TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap(FeatureLevel));
            TShaderMapRef<FScreenPS> PixelShader(GetGlobalShaderMap(FeatureLevel));



            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
            //GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
            //GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;

            SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

            // NOTE: Render the render target's texture to the temp texture in the GPU so we can copy the temp texture to the read-back texture in the CPU
            const FIntPoint& FullSourceSize = NewSourceTexture->GetSizeXY();
            const FIntPoint& SourceSize = SourceRect.Size();

            ensure(FullSourceSize != FIntPoint::ZeroValue);
            ensure(SourceSize != FIntPoint::ZeroValue);

            if (TargetSize == SourceSize)
            {
                PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), NewSourceTexture);
            }
            // Must use Bilinear sampling if the size of the source and target regions are not the same
            else
            {
                PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), NewSourceTexture);
            }

            const float U = float(SourceRect.Min.X) / float(FullSourceSize.X);
            const float V = float(SourceRect.Min.Y) / float(FullSourceSize.Y);
            const float SizeU = float(SourceRect.Max.X) / float(FullSourceSize.X) - U;
            const float SizeV = float(SourceRect.Max.Y) / float(FullSourceSize.Y) - V;

            // Render the source texture to the target render target
            RendererModule->DrawRectangle(
                RHICmdList,
                0, 0,                                   // Dest X, Y
                TargetSize.X,                           // Dest Width
                TargetSize.Y,                           // Dest Height
                U, V,                                   // Source U, V
                1, 1,                                   // Source USize, VSize
                SourceRect.Max - SourceRect.Min,        // Target buffer size
                FIntPoint(1, 1),                        // Source texture size
                VertexShader,
                EDRF_Default);
        }
        RHICmdList.EndRenderPass();

        // Asynchronously copy render target from GPU to CPU
        const bool bKeepOriginalSurface = false;
        const FResolveParams ResolveParams;
        RHICmdList.CopyToResolveTarget(
            DestRenderTarget.TargetableTexture,
            ReadbackTexture,
            ResolveParams);
    }
}

FNVTexturePixelData FNVTextureReader::BuildPixelData(uint8* PixelsData, EPixelFormat PixelFormat, const FIntPoint& ImageSize, const FIntPoint& TargetSize)
{
    FNVTexturePixelData NewPixelData;
    BuildPixelData(NewPixelData, PixelsData, PixelFormat, ImageSize, TargetSize);

    return NewPixelData;
}

void FNVTextureReader::BuildPixelData(FNVTexturePixelData& OutPixelsData, uint8* RawPixelsData, EPixelFormat PixelFormat, const FIntPoint& ImageSize, const FIntPoint& TargetSize)
{
    ensure(RawPixelsData);
    if (!RawPixelsData)
    {
        UE_LOG(LogNVTextureReader, Error, TEXT("invalid argument."));
    }
    else
    {
        const uint32 PixelCount = TargetSize.X * TargetSize.Y;

        OutPixelsData.PixelSize = TargetSize;
        OutPixelsData.PixelFormat = PixelFormat;

        // NOTE: We don't support pixel format that use less than 1 byte for now, e.g: grayscale 1, 2 or 4 bit
        const uint8 PixelByteSize = NVSceneCapturerUtils::GetPixelByteSize(PixelFormat);
        const uint32 PixelBufferSize = PixelByteSize * PixelCount;
        OutPixelsData.PixelData.InsertUninitialized(0, PixelBufferSize);

        uint8* SrcPixelBuffer = RawPixelsData;
        uint8* Dest = &OutPixelsData.PixelData[0];
        // NOTE: The 2d size of the read back pixel buffer (PixelSize) may be different than the target size that we want (ReadbackSize)
        // So we must make sure to only copy the minimum part of it
        const int32 MinWidth = FMath::Min(TargetSize.X, ImageSize.X);
        const int32 MinHeight = FMath::Min(TargetSize.Y, ImageSize.Y);
        const int32 TargetWidthByteSize = MinWidth * PixelByteSize;
        const int32 SourceWidthByteSize = ImageSize.X * PixelByteSize;
        OutPixelsData.RowStride = TargetWidthByteSize;
        for (int32 Row = 0; Row < MinHeight; ++Row)
        {
            FMemory::Memcpy(Dest, SrcPixelBuffer, TargetWidthByteSize);

            SrcPixelBuffer += SourceWidthByteSize;
            Dest += TargetWidthByteSize;
        }
    }
}

//======================= FNVTextureRenderTargetReader =======================//
FNVTextureRenderTargetReader::FNVTextureRenderTargetReader(UTextureRenderTarget2D* InRenderTarget) : FNVTextureReader()
{
    SourceRenderTarget = InRenderTarget;
}

FNVTextureRenderTargetReader::~FNVTextureRenderTargetReader()
{
    SourceRenderTarget = nullptr;
}

void FNVTextureRenderTargetReader::SetTextureRenderTarget(UTextureRenderTarget2D* NewRenderTarget)
{
    if (NewRenderTarget != SourceRenderTarget)
    {
        SourceRenderTarget = NewRenderTarget;
        UpdateTextureFromRenderTarget();
    }
}

bool FNVTextureRenderTargetReader::ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback, bool bIgnoreAlpha /*= false*/)
{
    UpdateTextureFromRenderTarget();
    return FNVTextureReader::ReadPixelsData(Callback, bIgnoreAlpha);
}

bool FNVTextureRenderTargetReader::ReadPixelsData(FNVTexturePixelData& OutPixelsData)
{
    ENQUEUE_RENDER_COMMAND(ReadPixelsFromTexture)(
    [this, &OutPixelsData = OutPixelsData](FRHICommandListImmediate& RHICmdList)
    {
        const FTextureRenderTargetResource* RenderTargetResource = SourceRenderTarget->GetRenderTargetResource();
        ensure(RenderTargetResource);
        if (RenderTargetResource)
        {
            SourceTexture = RenderTargetResource->GetRenderTargetTexture();
            ensure(SourceTexture);
            if (SourceTexture)
            {
                void* PixelDataBuffer = nullptr;
                FIntPoint PixelSize = FIntPoint::ZeroValue;
                RHICmdList.MapStagingSurface(SourceTexture, PixelDataBuffer, PixelSize.X, PixelSize.Y);

                BuildPixelData(OutPixelsData, (uint8*)PixelDataBuffer, ReadbackPixelFormat, PixelSize, ReadbackSize);

                RHICmdList.UnmapStagingSurface(SourceTexture);
            }
        }
    });

    FlushRenderingCommands();

    return true;
}

void FNVTextureRenderTargetReader::UpdateTextureFromRenderTarget()
{
    // Update the texture reference from the render target
    // NOTE: Should check if the texture still belong to the render target before updating its reference
    if (SourceRenderTarget)
    {
        // NOTE: Because function GameThread_GetRenderTargetResource is not marked as const, we can't have SourceRenderTarget as const either
        const FTextureRenderTargetResource* RenderTargetResource = IsInGameThread() ? SourceRenderTarget->GameThread_GetRenderTargetResource()
                : SourceRenderTarget->GetRenderTargetResource();
        if (RenderTargetResource)
        {
            SetSourceTexture(RenderTargetResource->GetRenderTargetTexture());
        }
    }
    else
    {
        SetSourceTexture(nullptr);
    }
}