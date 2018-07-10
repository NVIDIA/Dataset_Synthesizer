/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVAnnotatedActor.h"
#include "NVCoordinateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Factories/FbxAssetImportData.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

ANVAnnotatedActor::ANVAnnotatedActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Root"));
    BoxComponent->SetMobility(EComponentMobility::Movable);

    RootComponent = BoxComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);

    CoordComponent = CreateDefaultSubobject<UNVCoordinateComponent>(TEXT("CoordComponent"));
    CoordComponent->SetupAttachment(RootComponent);

    AnnotationTag = CreateDefaultSubobject<UNVCapturableActorTag>(TEXT("AnnotationTag"));

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

#if WITH_EDITORONLY_DATA
    USelection::SelectObjectEvent.AddUObject(this, &ANVAnnotatedActor::OnActorSelected);
#endif //WITH_EDITORONLY_DATA

    bForceCenterOfBoundingBoxAtRoot = true;
    bSetClassNameFromMesh = false;
}

void ANVAnnotatedActor::PostLoad()
{
    Super::PostLoad();
}

void ANVAnnotatedActor::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    ensure(World);
#if WITH_EDITOR
    bool bIsSimulating = GUnrealEd ? (GUnrealEd->bIsSimulatingInEditor || GUnrealEd->bIsSimulateInEditorQueued) : false;
    if (!World || !World->IsGameWorld() || bIsSimulating)
    {
        return;
    }
#endif

    UpdateStaticMesh();
}

void ANVAnnotatedActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    UpdateStaticMesh();
}

#if WITH_EDITORONLY_DATA
void ANVAnnotatedActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        if ((ChangedPropName == GET_MEMBER_NAME_CHECKED(ANVAnnotatedActor, MeshComponent)))
        {
            UpdateStaticMesh();
        }

        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}

void ANVAnnotatedActor::OnActorSelected(UObject* Object)
{
    // TODO: Show the  3d bounding box and facing direction
}
#endif //WITH_EDITORONLY_DATA


void ANVAnnotatedActor::SetStaticMesh(class UStaticMesh* NewMesh)
{
    ensure(NewMesh);
    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh(NewMesh);
        UpdateStaticMesh();
    }
}

TSharedPtr<FJsonObject> ANVAnnotatedActor::GetCustomAnnotatedData() const
{
    return nullptr;
}

void ANVAnnotatedActor::SetClassSegmentationId(uint32 NewId)
{
    if (MeshComponent)
    {
        MeshComponent->CustomDepthStencilValue = (int32)NewId;
        MeshComponent->SetRenderCustomDepth(true);
    }
}

void ANVAnnotatedActor::UpdateMeshCuboid()
{
    UStaticMesh* ActorStaticMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
    if (ActorStaticMesh)
    {
        const FMatrix& PCATransformMat = CalculatePCA(ActorStaticMesh);
        PCACenter = PCATransformMat.GetOrigin();
        const FVector& TempPCADirection = PCATransformMat.GetUnitAxis(EAxis::Z);
        PCARotation = (-TempPCADirection).ToOrientationRotator();

        CoordComponent->SetRelativeRotation(PCARotation);

        // NOTE: We need to check all the mesh's vertices here for accuracy
        const bool bCheckMeshCollision = false;
        MeshCuboid = NVSceneCapturerUtils::GetMeshCuboid_OOBB_Simple(MeshComponent, false, bCheckMeshCollision);

        const FVector& MeshCuboidSize = MeshCuboid.GetDimension();
        BoxComponent->SetBoxExtent(MeshCuboidSize * 0.5f);
        CoordComponent->SetSize(MeshCuboidSize);

        CuboidDimension = MeshCuboidSize;
        CuboidCenterLocal = MeshCuboid.GetCenter();
    }
}

FMatrix ANVAnnotatedActor::CalculatePCA(const class UStaticMesh* Mesh)
{
    FMatrix TransformMatrix = FMatrix::Identity;
    if (Mesh)
    {
        TArray<FVector> MeshVertices;
        MeshVertices.Reset();

        const FPositionVertexBuffer& MeshVertexBuffer = Mesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
        const uint32 VertexesCount = MeshVertexBuffer.GetNumVertices();
        if (VertexesCount != 0)
        {
            MeshVertices.Reserve(VertexesCount);
            for (uint32 i = 0; i < VertexesCount; i++)
            {
                const FVector& VertexPosition = MeshVertexBuffer.VertexPosition(i);
                MeshVertices.Add(VertexPosition);
            }

            // Find the mean vertex
            FVector MeanVertex = FVector::ZeroVector;
            for (uint32 i = 0; i < VertexesCount; i++)
            {
                MeanVertex += MeshVertices[i];
            }
            MeanVertex /= VertexesCount;

            // Calculate the covariance matrix
            FMatrix CMat = FMatrix::Identity;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    float Cij = 0.f;
                    for (uint32 k = 0; k < VertexesCount; k++)
                    {
                        Cij += (MeshVertices[k][i] - MeanVertex[i]) * (MeshVertices[k][j] - MeanVertex[j]);
                    }
                    Cij /= VertexesCount;

                    CMat.M[i][j] = Cij;
                }
            }

            // Find the EigenVector of the matrix
            FVector ZAxis = ComputeEigenVector(CMat);
            FVector XAxis, YAxis;
            ZAxis.FindBestAxisVectors(YAxis, XAxis);

            TransformMatrix = FMatrix(XAxis, YAxis, ZAxis, MeanVertex);
        }
    }
    return TransformMatrix;
}

FVector ANVAnnotatedActor::ComputeEigenVector(const FMatrix& Mat)
{
    // NOTES: Copied from function ComputeEigenVector in PhysicsASsetUtils.cpp
    //using the power method: this is ok because we only need the dominate eigenvector and speed is not critical:
    // http://en.wikipedia.org/wiki/Power_iteration
    FVector EVector = FVector(0, 0, 1.f);
    for (int32 i = 0; i < 32; ++i)
    {
        float Length = EVector.Size();
        if (Length > 0.f)
        {
            EVector = Mat.TransformVector(EVector) / Length;
        }
    }

    return EVector.GetSafeNormal();
}

void ANVAnnotatedActor::UpdateStaticMesh()
{
    UpdateMeshCuboid();

    if (bForceCenterOfBoundingBoxAtRoot)
    {
        MeshComponent->SetRelativeLocation(-CuboidCenterLocal);
        CuboidCenterLocal = FVector::ZeroVector;
    }

    if (bSetClassNameFromMesh)
    {
        if (AnnotationTag)
        {
            const UStaticMesh* ActorStaticMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
            if (ActorStaticMesh)
            {
                AnnotationTag->Tag = ActorStaticMesh->GetName();
            }
        }
    }
}

FMatrix ANVAnnotatedActor::GetMeshInitialMatrix() const
{
    FMatrix ResultFMatrix = FMatrix::Identity;

    UStaticMesh* ActorStaticMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
    if (ActorStaticMesh)
    {
        UFbxAssetImportData* FbxAssetImportData = Cast<UFbxAssetImportData>(ActorStaticMesh->AssetImportData);
        FMatrix MeshImportMatrix = FMatrix::Identity;

        if (FbxAssetImportData)
        {
            FTransform AssetImportTransform(
                FbxAssetImportData->ImportRotation.Quaternion(),
                FbxAssetImportData->ImportTranslation,
                FVector(FbxAssetImportData->ImportUniformScale)
            );

            MeshImportMatrix = AssetImportTransform.ToMatrixWithScale();
        }

        const FTransform& MeshRelativeTransform = MeshComponent->GetRelativeTransform();
        const FMatrix& MeshRelativeMatrix = MeshRelativeTransform.ToMatrixWithScale();

        const FMatrix& MeshInitialMatrix = MeshImportMatrix * MeshRelativeMatrix;
        FMatrix MeshInitialMatrix_OpenCV = MeshInitialMatrix * NVSceneCapturerUtils::UE4ToOpenCVMatrix;

        // HACK: Invert the Y axis - need to figure out why?
        FVector MatX, MatY, MatZ;
        MeshInitialMatrix_OpenCV.GetScaledAxes(MatX, MatY, MatZ);
        MatY = -MatY;
        MeshInitialMatrix_OpenCV.SetAxes(nullptr, &MatY, nullptr);
        ResultFMatrix = MeshInitialMatrix_OpenCV;
    }
    return ResultFMatrix;
}
