/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "Components/SceneCaptureComponent2D.h"
#include "NVSceneCapturerUtils.h"
#include "NVTextureReader.h"
#include "Engine/TextureRenderTarget2D.h"
#include "NVSceneCaptureComponent2D.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVSceneCapturerComponent2D, Log, All)

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent),
       HideCategories = (Replication, ComponentReplication, Cooking, Events, ComponentTick,
                         Actor, Input, Collision, PhysX, Activation, Sockets,
                         MobileTonemapper, PostProcessVolume,
                         Projection, Rendering, Transform, PlanarReflection, LOD))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API UNVSceneCaptureComponent2D : public USceneCaptureComponent2D
{
    GENERATED_BODY()

    /// Callback function get called after the scene capture component finished capturing scene and read back its pixels data
    /// FNVTexturePixelData - The struct contain the captured scene's pixels data
    typedef TFunction<void(const FNVTexturePixelData&)> OnFinishedCaptureScenePixelsDataCallback;


public:
    UNVSceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer);

    virtual void UpdateSceneCaptureContents(FSceneInterface* Scene) override;

    /// Build a projection matrix (instrinsic) base on the camera settings (aspect ratio, projection type, fov and orthogonal width)
    static FMatrix BuildProjectionMatrix(const FNVImageSize& RenderTargetSize, ECameraProjectionMode::Type ProjectionType,
        float FOV, float OrthoWidth);

    /// Build a projection matrix from a specific view location and rotation
    static FMatrix BuildViewProjectionMatrix(const FTransform& ViewTransform,
            const FNVImageSize& RenderTargetSize,
            ECameraProjectionMode::Type ProjectionType,
            float FOVAngle,
            float OrthoWidth,
            FMatrix& ProjectionMatrix);

    /// Build a projection matrix from a specific view transform and projection matrix
    static FMatrix BuildViewProjectionMatrix(const FTransform& ViewTransform, const FMatrix& ProjectionMatrix);

    FMatrix GetProjectionMatrix() const;

    /// Order the component to capture the scene to the render target texture, after the scene is captured, read back the pixels data
    /// NOTE: This function run asynchronously, the callback function should use the context data to decide what to do with the pixels data
    /// This function is combination of CaptureSceneToTexture and ReadPixelsDataFromTexture
    void CaptureSceneToPixelsData(UNVSceneCaptureComponent2D::OnFinishedCaptureScenePixelsDataCallback Callback);

    UFUNCTION(BlueprintCallable, Category = "Exporter")
    void StartCapturing();
    UFUNCTION(BlueprintCallable, Category = "Exporter")
    void StopCapturing();

protected:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
    virtual bool GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut) override;
#endif //WITH_EDITOR

    /// Command tell this scene capturer to capture the scene into its render target texture later in the rendering phase
    void CaptureSceneToTexture();

    /// Read back the pixels data from the captured texture
    /// NOTE: This function run synchronously (it flush rendering commands) which can cause hitches on the game thread
    bool ReadPixelsDataFromTexture(FNVTexturePixelData& OutPixelsData);
    /// Async function to read back the pixels data from the captured texture
    /// NOTE: This function run asynchronously, the callback function should use the context data to decide what to do with the pixels data
    void ReadPixelsDataFromTexture(OnFinishedCaptureScenePixelsDataCallback Callback);

    bool ShouldCaptureCurrentFrame() const;
    bool ShouldReadbackPixelsData() const;

    void OnSceneCaptured();
    void InitTextureRenderTarget();

public: // Editor properties
    /// The size (width x height in pixels) of the captured TextureTarget
    // NOTE: If a valid TextureTarget is specified then this property will be ignored
    UPROPERTY(VisibleAnywhere, Category = "SceneCapture")
    FNVImageSize TextureTargetSize;

    /// Pixel format of the captured TextureTarget
    /// NOTE: If a valid TextureTarget is specified then this property will be ignored
    UPROPERTY(EditAnywhere, Category = "SceneCapture")
    TEnumAsByte<ETextureRenderTargetFormat> TextureTargetFormat;

    // Pixel format of the texture used to capture the scene
    // NOTE: This can be different from the TextureTargetFormat
    UPROPERTY(EditAnywhere, Category = "SceneCapture")
    TEnumAsByte<EPixelFormat> OverrideTexturePixelFormat;

    /// If true, don't read back the raw alpha value from the render target but set it to 1
    UPROPERTY(EditAnywhere, Category = "SceneCapture")
    bool bIgnoreReadbackAlpha;
protected: // Transient properties
    FNVTextureRenderTargetReader RenderTargetReader;
    TArray<UNVSceneCaptureComponent2D::OnFinishedCaptureScenePixelsDataCallback> ReadbackCallbackList;
};
