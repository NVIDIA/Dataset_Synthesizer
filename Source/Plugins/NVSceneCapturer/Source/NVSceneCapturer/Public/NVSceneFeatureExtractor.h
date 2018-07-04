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
#include "NVSceneFeatureExtractor.generated.h"

class UNVSceneCapturerViewpointComponent;
class ANVSceneCapturerActor;
class UNVSceneCaptureComponent2D;
class UNVSceneFeatureExtractor;

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVFeatureExtractorSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Instanced, Category = FeatureExtraction, meta = (ShowOnlyInnerProperties))
    UNVSceneFeatureExtractor* FeatureExtractorRef;
};

///
///
///
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, Abstract, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVSceneFeatureExtractor : public UObject
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor(const FObjectInitializer& ObjectInitializer);

    virtual class UWorld* GetWorld() const override;

    bool IsEnabled() const;
    FString GetDisplayName() const;

    void Init(UNVSceneCapturerViewpointComponent* InOwnerViewpoint);

    virtual void StartCapturing();
    virtual void StopCapturing();

    virtual void UpdateCapturerSettings() PURE_VIRTUAL(UNVSceneFeatureExtractor::UpdateCapturerSettings,);
protected:
    virtual void UpdateSettings() PURE_VIRTUAL(UNVSceneFeatureExtractor::UpdateSettings,);

public: // Editor properties
    /// If true, the feature extractor will capture otherwise it won't
    /// ToDo: Move to protected.
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = Config)
    bool bIsEnabled;
    /// Name of the feature extractor to show
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = Config)
    FString DisplayName;
    /// 
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = Config)
    FString Description;
    /// The string to add to the end of the exported file's name captured from this feature extractor. e.g: "depth", "mask" ...
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = Config)
    FString ExportFileNamePostfix;

protected: // Transient properties
    UPROPERTY(Transient)
    ANVSceneCapturerActor* OwnerCapturer;
    UPROPERTY(Transient)
    UNVSceneCapturerViewpointComponent* OwnerViewpoint;
    UPROPERTY(Transient)
    bool bCapturing;
};

USTRUCT()
struct FNVSceneCaptureComponentData
{
    GENERATED_BODY()

public:
    UPROPERTY()
    UNVSceneCaptureComponent2D* SceneCaptureComp2D;

    UPROPERTY()
    FString ComponentName;
};