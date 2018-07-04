/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "Components/SceneComponent.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneFeatureExtractor_DataExport.h"
#include "NVSceneFeatureExtractor_ImageExport.h"
#include "NVTextureReader.h"
#include "NVSceneCapturerViewpointComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVSceneCapturerViewpointComponent, Log, All)

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVSceneCapturerViewpointSettings
{
    GENERATED_BODY()

public:
    FNVSceneCapturerViewpointSettings();

public: // Editor properties
    /// If true, the viewpoint will capture otherwise it won't
    UPROPERTY(EditAnywhere, Category = Config)
    bool bIsEnabled;

    /// Name of the viewpoint to show
    UPROPERTY(EditAnywhere, Category = Config)
    FString DisplayName;

    /// The string to add to the end of the exported file's name captured from this viewpoint. e.g: "infrared", "left", "right" ...
    UPROPERTY(EditAnywhere, Category = Config)
    FString ExportFileNamePostfix;

    /// If true, the viewpoint have its own feature extractors settings and doesn't use the owner capturer's feature extractor settings
    UPROPERTY(EditAnywhere, Category = FeatureExtraction, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bOverrideFeatureExtractorSettings;

    UPROPERTY(EditAnywhere, Category = FeatureExtraction, meta = (editcondition = "bOverrideFeatureExtractorSettings"))
    TArray<FNVFeatureExtractorSettings> FeatureExtractorSettings;

    /// If true, the viewpoint have its own capture settings and doesn't use the owner capturer's feature extractor settings
    UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bOverrideCaptureSettings;

    UPROPERTY(EditAnywhere, Category = Settings, meta = (editcondition="bOverrideCaptureSettings"))
    FNVSceneCapturerSettings CaptureSettings;
};


///
/// UNVSceneCapturerViewpointComponent: Represents each viewpoint from where the capturer captures data
///
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent),
       HideCategories = (Replication, ComponentReplication, Cooking, Events, ComponentTick, Actor, Input, Collision,
                         Physics, PhysX, Activation, Sockets, Rendering, LOD, Tags))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API UNVSceneCapturerViewpointComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UNVSceneCapturerViewpointComponent(const FObjectInitializer& ObjectInitializer);

    /// Callback function get called after the scene capture component finished capturing scene and read back its pixels data
    /// FNVTexturePixelData - The struct contain the captured scene's pixels data
    /// UNVSceneFeatureExtractor_PixelData* - Reference to the feature extractor that captured the scene pixels data
    /// UNVSceneCapturerViewpointComponent* - Reference to the viewpoint that captured the scene pixels data
    typedef TFunction<void(const FNVTexturePixelData&, UNVSceneFeatureExtractor_PixelData*, UNVSceneCapturerViewpointComponent*)> OnFinishedCaptureScenePixelsDataCallback;

    bool CaptureSceneToPixelsData(UNVSceneCapturerViewpointComponent::OnFinishedCaptureScenePixelsDataCallback Callback);

    /// Callback function get called after the scene capture component finished capturing scene's annotation data
    /// TSharedPtr<FJsonObject> - The JSON object contain the annotation data
    /// UNVSceneFeatureExtractor_AnnotationData* - Reference to the feature extractor that captured the scene annotation data
    /// UNVSceneCapturerViewpointComponent* - Reference to the viewpoint that captured the scene pixels data
    typedef TFunction<void(TSharedPtr<FJsonObject>, UNVSceneFeatureExtractor_AnnotationData*, UNVSceneCapturerViewpointComponent*)> OnFinishedCaptureSceneAnnotationDataCallback;

    bool CaptureSceneAnnotationData(UNVSceneCapturerViewpointComponent::OnFinishedCaptureSceneAnnotationDataCallback Callback);

    void SetupFeatureExtractors();
    void UpdateCapturerSettings();
    const FNVSceneCapturerViewpointSettings& GetSettings() const;
    const FNVSceneCapturerSettings& GetCapturerSettings() const;
    const TArray<FNVFeatureExtractorSettings>& GetFeatureExtractorSettings() const;

    bool IsEnabled() const;
    FString GetDisplayName() const;

    void StartCapturing();
    void StopCapturing();

#if WITH_EDITOR
    virtual bool GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut) override;
#endif // WITH_EDITOR

protected:
    virtual void BeginPlay() final;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) final;
    virtual void OnRegister() final;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) final;
    virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) final;

public: // Editor properties
    UPROPERTY(EditAnywhere, Category = Config, meta = (ShowOnlyInnerProperties))
    FNVSceneCapturerViewpointSettings Settings;

public: // Transient properties
    UPROPERTY(Transient)
    TArray<class UNVSceneFeatureExtractor*> FeatureExtractorList;
protected: // Transient properties
    UPROPERTY(Transient)
    class ANVSceneCapturerActor* OwnerSceneCapturer;

#if WITH_EDITORONLY_DATA
protected: // Proxy editor mesh
    /// The frustum component used to show visually where the camera field of view is
    class UDrawFrustumComponent* DrawFrustum;

    UPROPERTY(transient)
    class UStaticMesh* CameraMesh;

    /// The camera mesh to show visually where the camera is placed
    class UStaticMeshComponent* ProxyMeshComponent;

    static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

    void SetCameraMesh(UStaticMesh* Mesh);

    /// Refreshes the visual components to match the component state
    void RefreshVisualRepresentation();

    void OverrideFrustumColor(FColor OverrideColor);
    void RestoreFrustumColor();

    void ResetProxyMeshTransform();
    /// Ensure the proxy mesh is in the correct place
    void UpdateProxyMeshTransform();
#endif
};
