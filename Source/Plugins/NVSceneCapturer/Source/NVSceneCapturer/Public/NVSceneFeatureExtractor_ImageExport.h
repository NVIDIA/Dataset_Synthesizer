/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "NVSceneCapturerUtils.h"
#include "NVTextureReader.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneFeatureExtractor_ImageExport.generated.h"

class UNVSceneCapturerViewpointComponent;
class ANVSceneCapturerActor;
class UNVSceneCaptureComponent2D;
class UNVSceneFeatureExtractor;

/// Base class for all the feature extractors that capture the scene view in pixel data format
UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_PixelData : public UNVSceneFeatureExtractor
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_PixelData(const FObjectInitializer& ObjectInitializer);

    /// Callback function get called after the scene capture component finished capturing scene and read back its pixels data
    /// FNVTexturePixelData - The struct contain the captured scene's pixels data
    /// UNVSceneFeatureExtractor_PixelData* - Reference to the feature extractor that captured the scene pixels data
    typedef TFunction<void(const FNVTexturePixelData&, UNVSceneFeatureExtractor_PixelData*)> OnFinishedCaptureScenePixelsDataCallback;

    virtual bool CaptureSceneToPixelsData(UNVSceneFeatureExtractor_PixelData::OnFinishedCaptureScenePixelsDataCallback Callback);

    virtual void StartCapturing() override;
    virtual void StopCapturing() override;
    virtual void UpdateCapturerSettings() override;

    UNVSceneCaptureComponent2D* CreateSceneCaptureComponent2d(UMaterialInstance* PostProcessingMaterial = nullptr, const FString& ComponentName = TEXT(""));

    virtual class UTextureRenderTarget2D* GetRenderTarget() const;

protected:
    virtual void UpdateSettings() override;
    virtual void UpdateMaterial();

protected: // Editor properties
    /// If true, only show the training actors in the exported images
    UPROPERTY(EditDefaultsOnly, Category = Config)
    bool bOnlyShowTrainingActors;

    /// List of actors to ignore when capturing image
    UPROPERTY(EditInstanceOnly, Category = Config)
    TArray<AActor*> IgnoreActors;

    UPROPERTY(EditDefaultsOnly, Category = Config, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bOverrideShowFlagSettings;

    /// ShowFlags for the SceneCapture's ViewFamily, to control rendering settings for this view. Hidden but accessible through details customization 
    UPROPERTY(EditDefaultsOnly, interp, Category = Config, meta = (editcondition = bOverrideShowFlagSettings))
    TArray<struct FEngineShowFlagsSetting> OverrideShowFlagSettings;

    UPROPERTY(EditDefaultsOnly, AdvancedDisplay)
    float PostProcessBlendWeight;

    UPROPERTY(EditDefaultsOnly, SimpleDisplay, Category = Config)
    class UMaterialInterface* PostProcessMaterial;

    /// If true, this feature extractors have its own exported image type and doesn't use the owner capturer's image type
    UPROPERTY(EditDefaultsOnly, Category = Config, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bOverrideExportImageType;

    UPROPERTY(EditDefaultsOnly, Category = Config, meta = (editcondition = "bOverrideExportImageType"))
    ENVImageFormat ExportImageFormat;

	UPROPERTY(EditDefaultsOnly, Category = Config)
	TEnumAsByte<ENVCapturedPixelFormat> CapturedPixelFormat;

    UPROPERTY(EditDefaultsOnly, AdvancedDisplay)
    TEnumAsByte<EPixelFormat> OverrideTexturePixelFormat;

    UPROPERTY(EditDefaultsOnly, AdvancedDisplay)
    TEnumAsByte<enum ESceneCaptureSource> CaptureSource;

    UPROPERTY(EditDefaultsOnly, AdvancedDisplay)
    bool bUpdateContinuously;

protected: // Transient properties
    UPROPERTY(Transient)
    TArray<FNVSceneCaptureComponentData> SceneCaptureComp2DDataList;

    UPROPERTY(Transient)
    class UMaterialInstanceDynamic* PostProcessMaterialInstance;

    UPROPERTY(Transient)
    UNVSceneCaptureComponent2D* SceneCaptureComponent;
};

/// Base class for all the feature extractors that export the scene's depth buffer
UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_SceneDepth : public UNVSceneFeatureExtractor_PixelData
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_SceneDepth(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void UpdateMaterial() override;

public: // Editor properties
    /// The furthest distance to quantize when capturing the scene's depth
    UPROPERTY(EditAnywhere, SimpleDisplay, Category=Config)
    float MaxDepthDistance;
};

UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_ScenePixelVelocity : public UNVSceneFeatureExtractor_PixelData
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_ScenePixelVelocity(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void UpdateSettings() override;
};

/// Base class for all the feature extractors that export the scene's stencil mask buffer
UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_StencilMask : public UNVSceneFeatureExtractor_PixelData
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_StencilMask(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void UpdateSettings() override;
};


/// Base class for all the feature extractors that export the scene's vertex color buffer
UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_VertexColorMask : public UNVSceneFeatureExtractor_PixelData
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_VertexColorMask(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void UpdateSettings() override;
};
