/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneCapturerUtils.h"
#include "NVTextureReader.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVTextureReader, Log, All)

// This class read the pixels data from a texture target
USTRUCT()
struct NVSCENECAPTURER_API FNVTextureReader
{
    GENERATED_BODY()

public:
    FNVTextureReader();
    virtual ~FNVTextureReader();
    FNVTextureReader& operator=(const FNVTextureReader& OtherReader);

    /// Callback function get called after finish reading pixels data
    /// uint8* - pointer to pixel array buffer
    /// EPixelFormat - format of the read back pixels
    /// FIntPoint - 2d size of the pixels image
    typedef TFunction<void(uint8*, EPixelFormat, FIntPoint)> OnFinishedReadingRawPixelsCallback;

    /// Callback function get called after finish reading pixels data
    /// FNVTexturePixelData - The struct contain the texture's pixels data
    typedef TFunction<void(const FNVTexturePixelData&)> OnFinishedReadingPixelsDataCallback;

    /// Read back the pixels data from the current source texture
    /// @param Callback  The function to call after all the pixels data are read from the source texture
    /// @param bIgnoreAlpha          If true, just set the alpha value of the readback pixels to 1, otherwise read it correctly
    /// NOTE: This function is async, the reading process will run on the rendering thread in parallel with the game thread
    virtual bool ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback, bool bIgnoreAlpha = false);

    /// Read back the pixels data from the current source texture
    /// NOTE: This function is sync, the pixels data is returned right away but it may cause the game to hitches since it flush the rendering commands
    virtual bool ReadPixelsData(FNVTexturePixelData& OutPixelsData);

protected:
    /// Change the information of the texture to read from
    /// @param NewSourceTexture          The texture to read from
    /// @param NewSourceRect             The region to read from the source texture. If the region is empty, the whole source texture will be read
    /// @param NewReadbackPixelFormat    The pixel format of the read back pixels. If the pixel format is Unknown, the read back pixels will have the same format as the source texture
    /// @param NewReadbackSize           The 2d size of the read back pixels. If the size is zero, the read back size will be the same as the size of the source texture
    void SetSourceTexture(FTexture2DRHIRef NewSourceTexture,
        const FIntRect& NewSourceRect = FIntRect(),
        EPixelFormat NewReadbackPixelFormat = EPixelFormat::PF_Unknown,
        const FIntPoint& NewReadbackSize = FIntPoint::ZeroValue);

    /// Read back the pixels data from the current source texture
    /// @param Callback  The function to call after all the pixels data are read from the source texture
    /// @param NewSourceTexture          The texture to read from
    /// @param NewSourceRect             The region to read from the source texture. If the region is empty, the whole source texture will be read
    /// @param NewReadbackPixelFormat    The pixel format of the read back pixels. If the pixel format is Unknown, the read back pixels will have the same format as the source texture
    /// @param NewReadbackSize           The 2d size of the read back pixels. If the size is zero, the read back size will be the same as the size of the source texture
    /// @param bIgnoreAlpha          If true, just set the alpha value of the readback pixels to 1, otherwise read it correctly
    /// NOTE: This function is async, the reading process will run on the rendering thread in parallel with the game thread
    // TODO: Group these property into a settings struct to make the read pixels function more simple
    bool ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback,
        const FTexture2DRHIRef& NewSourceTexture,
        const FIntRect& NewSourceRect = FIntRect(),
        EPixelFormat NewReadbackPixelFormat = EPixelFormat::PF_Unknown,
        const FIntPoint& NewReadbackSize = FIntPoint::ZeroValue,
        bool bIgnoreAlpha = false);

    /// Read back the pixels data from the current source texture
    /// NOTE: This function is sync, the pixels data is returned right away but it may cause the game to hitches since it flush the rendering commands
    bool ReadPixelsData(FNVTexturePixelData& OutPixelsData,
        const FTexture2DRHIRef& NewSourceTexture,
        const FIntRect& NewSourceRect = FIntRect(),
        EPixelFormat NewReadbackPixelFormat = EPixelFormat::PF_Unknown,
        const FIntPoint& NewReadbackSize = FIntPoint::ZeroValue);

    /// Read the pixel data from a render target
    /// @param SourceTexture         The texture to read from
    /// @param SourceRect            The area where to read from the SourceRenderTarget
    /// @param TargetPixelFormat     The pixel format of the read back pixels data
    /// @param TargetSize            The size of the read back pixels area
    /// @param bIgnoreAlpha          If true, just set the alpha value of the readback pixels to 1, otherwise read it correctly
    /// @param Callback              Function to call after finished reading pixels data
    static bool ReadPixelsRaw(const FTexture2DRHIRef& SourceTexture,
                              const FIntRect& SourceRect,
                              EPixelFormat TargetPixelFormat,
                              const FIntPoint& TargetSize,
                              bool bIgnoreAlpha,
                              OnFinishedReadingRawPixelsCallback Callback);

    static FNVTexturePixelData BuildPixelData(uint8* PixelsData, EPixelFormat PixelFormat, const FIntPoint& ImageSize, const FIntPoint& TargetSize);
    static void BuildPixelData(FNVTexturePixelData& OutPixelsData, uint8* PixelsData, EPixelFormat PixelFormat, const FIntPoint& ImageSize, const FIntPoint& TargetSize);

    /// Copy the pixels data from a texture to another one
    /// NOTE: The function return back right away but the action is running in the GPU
    /// @param RendererModule  Reference to the Renderer module
    /// @param RHICmdList      The RHI command list used to copy texture. This parameter can be used to wait for the action to be done
    /// @param SourceTexture   The original texture
    /// @param SourceRect      The region to copy from the SourceTexture
    /// @param TargetTexture   The target texture to copy pixels to
    /// @param TargetRect      The region in the TargetTexture to copy pixels to
    /// @param bOverwriteAlpha   If true, overwrite the alpha of the target using the source texture's alpha
    static void CopyTexture2d(class IRendererModule* RendererModule, FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& SourceTexture, const FIntRect& SourceRect,
                              FTexture2DRHIRef& TargetTexture, const FIntRect& TargetRect, bool bOverwriteAlpha = true);

protected:
    FTexture2DRHIRef SourceTexture;
    FIntRect SourceRect;
    EPixelFormat ReadbackPixelFormat;
    FIntPoint ReadbackSize;
};

class UTextureRenderTarget2D;
USTRUCT()
struct NVSCENECAPTURER_API FNVTextureRenderTargetReader : public FNVTextureReader
{
    GENERATED_BODY()

public:
    FNVTextureRenderTargetReader(UTextureRenderTarget2D* InRenderTarget = nullptr);
    virtual ~FNVTextureRenderTargetReader();

    const UTextureRenderTarget2D* GetTextureRenderTarget() const
    {
        return SourceRenderTarget;
    }
    void SetTextureRenderTarget(UTextureRenderTarget2D* NewRenderTarget);

    virtual bool ReadPixelsData(FNVTexturePixelData& OutPixelData) final;
    virtual bool ReadPixelsData(OnFinishedReadingPixelsDataCallback Callback, bool bIgnoreAlpha = false) final;

protected:
    void UpdateTextureFromRenderTarget();

protected:
    UPROPERTY(Transient)
    UTextureRenderTarget2D* SourceRenderTarget;
};
