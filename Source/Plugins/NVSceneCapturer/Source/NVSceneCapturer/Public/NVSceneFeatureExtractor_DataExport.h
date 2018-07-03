/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneFeatureExtractor.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor_DataExport.generated.h"

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVDataExportSettings
{
    GENERATED_BODY()

public:
    FNVDataExportSettings();

public: // Editor properties
    UPROPERTY(EditAnywhere, Category = "Export")
    ENVIncludeObjects IncludeObjectsType;

    /// If true, the exporter will ignore all the hidden actors in game
    UPROPERTY(EditAnywhere, Category = "Export")
    bool bIgnoreHiddenActor;

    /// How to generate 3d bounding box for each exported actor mesh
    UPROPERTY(EditAnywhere, Category = "Export")
    ENVBoundsGenerationType BoundsType;

    /// How to generate the 2d bounding box for each exported actor mesh
    UPROPERTY(EditAnywhere, Category = "Export")
    ENVBoundBox2dGenerationType BoundingBox2dType;

    UPROPERTY(EditAnywhere, Category = "Export")
    bool bOutputEvenIfNoObjectsAreInView;

    UPROPERTY(EditAnywhere, Category = "Export")
    FFloatInterval DistanceScaleRange;

    /// If true, all the image space coordinates are exported in absolute pixel
    /// Otherwise the coordinates are in  ratio between the position and the image size
    UPROPERTY(EditAnywhere, Category = "Export")
    bool bExportImageCoordinateInPixel;
};

// Base class for all the feature extractors that export the scene data to json file
UCLASS(Abstract)
class NVSCENECAPTURER_API UNVSceneFeatureExtractor_AnnotationData : public UNVSceneFeatureExtractor
{
    GENERATED_BODY()

public:
    UNVSceneFeatureExtractor_AnnotationData(const FObjectInitializer& ObjectInitializer);

    virtual void StartCapturing() override;
    virtual void UpdateCapturerSettings() override;

    /// Callback function get called after capturing scene's annotation data
    /// TSharedPtr<FJsonObject> - The JSON object contain the annotation data
    /// UNVSceneFeatureExtractor_AnnotationData* - Reference to the feature extractor that captured the scene annotation data
    typedef TFunction<void(TSharedPtr<FJsonObject>, UNVSceneFeatureExtractor_AnnotationData*)> OnFinishedCaptureSceneAnnotationDataCallback;

    /// Capture the annotation data of the scene and return it in JSON format
    bool CaptureSceneAnnotationData(UNVSceneFeatureExtractor_AnnotationData::OnFinishedCaptureSceneAnnotationDataCallback Callback);

protected:
    TSharedPtr<FJsonObject> CaptureSceneAnnotationData();
    virtual void UpdateSettings() override;

    void UpdateProjectionMatrix();
    /// NOTE: May make this function static
    bool GatherActorData(const AActor* CheckActor, FCapturedObjectData& ActorData);
    bool ShouldExportActor(const AActor* CheckActor) const;
    bool IsActorInViewFrustum(const FConvexVolume& ViewFrustum, const AActor* CheckActor) const;

    FVector ProjectWorldPositionToImagePosition(const FVector& WorldPosition) const;

    FBox2D GetBoundingBox2D(const AActor* CheckActor, bool bClampToImage = true) const;
    /// Calculate a 2D axis-aligned bounding box of a 3d shape knowing its vertexes on the viewport
    FBox2D Calculate2dAABB(TArray<FVector> Vertexes, bool bClampToImage = true) const;
    /// Calculate a 2D axis-aligned bounding box of a static mesh on the viewport
    FBox2D Calculate2dAABB_MeshComplexCollision(const class UMeshComponent* CheckMeshComp, bool bClampToImage = true) const;

protected: // Editor properties
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = Config, meta=(ShowOnlyInnerProperties))
    FNVDataExportSettings DataExportSettings;

    UPROPERTY(Transient)
    FMatrix ViewProjectionMatrix;

    UPROPERTY(Transient)
    FMatrix ProjectionMatrix;

protected: // Transient properties
    FNVDataExportSettings ProtectedDataExportSettings;
};
