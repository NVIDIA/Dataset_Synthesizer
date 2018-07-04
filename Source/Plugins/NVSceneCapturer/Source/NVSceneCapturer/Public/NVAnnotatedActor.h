/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneCapturerUtils.h"
#include "NVAnnotatedActor.generated.h"

/**
 * The new actor which get annotated and have its info captured and exported
 */
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config = Engine, HideCategories = (Replication, Tick, Tags, Input))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API ANVAnnotatedActor : public AActor
{
    GENERATED_BODY()

public:
    ANVAnnotatedActor(const FObjectInitializer& ObjectInitializer);

    void SetStaticMesh(class UStaticMesh* NewMesh);

    virtual TSharedPtr<FJsonObject> GetCustomAnnotatedData() const;

    FNVCuboidData GetCuboidData() const
    {
        return MeshCuboid;
    }
    FVector GetCuboidCenterLocal() const
    {
        return CuboidCenterLocal;
    }
    void SetClassSegmentationId(uint32 NewId);

protected:
    virtual void PostLoad() override;
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    void OnActorSelected(UObject* Object);
#endif //WITH_EDITORONLY_DATA

    void UpdateStaticMesh();
    void UpdateMeshCuboid();

    FMatrix GetMeshInitialMatrix() const;

    FMatrix CalculatePCA(const class UStaticMesh* Mesh);
    FVector ComputeEigenVector(const FMatrix& Mat);

public: // Editor properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UNVCapturableActorTag* AnnotationTag;

protected: // Editor properties
    // TODO: Need to support both static mesh and skeletal mesh types
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UBoxComponent* BoxComponent;


    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UNVCoordinateComponent* CoordComponent;

    // If true, the mesh of this actor will be placed so its center of 3d bounding box is at the root of the actor
    // Otherwise user can place the actor manually
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnnotatedActor")
    bool bForceCenterOfBoundingBoxAtRoot;

    // If true, this actor's annotation tag will be the same as the mesh
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnnotatedActor")
    bool bSetClassNameFromMesh;

protected: // Transient properties
    UPROPERTY(VisibleAnywhere)
    FNVCuboidData MeshCuboid;

    UPROPERTY(VisibleAnywhere)
    FVector CuboidDimension;

    UPROPERTY(VisibleAnywhere)
    FVector CuboidCenterLocal;

    UPROPERTY(VisibleAnywhere)
    FVector PCACenter;

    UPROPERTY(VisibleAnywhere)
    FRotator PCARotation;

    UPROPERTY(VisibleAnywhere)
    FVector PCADirection;
};
