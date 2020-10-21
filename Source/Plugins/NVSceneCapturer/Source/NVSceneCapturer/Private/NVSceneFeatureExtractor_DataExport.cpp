/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor_DataExport.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneCaptureComponent2D.h"
#include "NVAnnotatedActor.h"
#include "NVSceneManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/CollisionProfile.h"
#include "Components/DrawFrustumComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Components/SkeletalMeshComponent.h"

//========================================== UNVSceneFeatureExtractor_DataExport ==========================================
UNVSceneFeatureExtractor_AnnotationData::UNVSceneFeatureExtractor_AnnotationData(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Description = TEXT("Calculate the annotation data of the objects in the scene, e.g: location, rotation, bounding box ...");
}

void UNVSceneFeatureExtractor_AnnotationData::StartCapturing()
{
    Super::StartCapturing();
    ProtectedDataExportSettings = DataExportSettings;
}

void UNVSceneFeatureExtractor_AnnotationData::UpdateSettings()
{
}

void UNVSceneFeatureExtractor_AnnotationData::UpdateCapturerSettings()
{
}

bool UNVSceneFeatureExtractor_AnnotationData::CaptureSceneAnnotationData(UNVSceneFeatureExtractor_AnnotationData::OnFinishedCaptureSceneAnnotationDataCallback Callback)
{
    if (Callback)
    {
        TSharedPtr<FJsonObject> CapturedData = CaptureSceneAnnotationData();
        if (CapturedData.IsValid())
        {
            Callback(CapturedData, this);
            return true;
        }
    }
    return false;
}

TSharedPtr<FJsonObject> UNVSceneFeatureExtractor_AnnotationData::CaptureSceneAnnotationData()
{
    TSharedPtr<FJsonObject> SceneDataJsonObj = nullptr;
    if (OwnerViewpoint)
    {
        const auto& CapturerSettings = OwnerViewpoint->GetCapturerSettings();
        const float FOVAngle = CapturerSettings.GetFOVAngle();
        const FTransform& ViewTransform = OwnerViewpoint->GetComponentTransform();

        FCapturedSceneData SceneData;

        FCapturedViewpointData& ViewpointData = SceneData.camera_data;
        const FVector& ViewLocation = ViewTransform.GetLocation();
        const FQuat& ViewRotation = ViewTransform.GetRotation();
        ViewpointData.fov = FOVAngle;
        ViewpointData.location_worldframe = ViewLocation;
        ViewpointData.quaternion_xyzw_worldframe = ViewRotation;
        ViewpointData.CameraSettings = CapturerSettings.GetCameraIntrinsicSettings();

        UpdateProjectionMatrix();
        ViewpointData.ProjectionMatrix = ProjectionMatrix;
        ViewpointData.ViewProjectionMatrix = ViewProjectionMatrix;

        UWorld* World = GetWorld();
        ensure(World);
        if (World)
        {
            // TODO: Should create a TrainingActor class to handle actors we want to export
            // Let those actor register with the exporter so we don't need to do a loop through all the actor like this every time we export
            // NOTE: Can keep 1 relevant actor list on the exporter actor and all the exporter components can use it too
            for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
            {
                const AActor* CheckActor = *ActorIt;
                FCapturedObjectData ActorData;
                if (GatherActorData(CheckActor, ActorData))
                {
                    SceneData.Objects.Add(ActorData);
                }
            }
        }

        SceneDataJsonObj = NVSceneCapturerUtils::UStructToJsonObject(SceneData, 0, 0);
        // TODO: This code is a bit messy. Should implement a ToJsonObject function in the FCapturedSceneData struct
        for (int i = 0; i < SceneData.Objects.Num(); i++)
        {
            const FCapturedObjectData& CheckObjectData = SceneData.Objects[i];
            const TArray< TSharedPtr<FJsonValue> >& JsonObjectArrayData = SceneDataJsonObj->GetArrayField(TEXT("objects"));
            const TSharedPtr<FJsonObject>& JsonObjectData = JsonObjectArrayData[i]->AsObject();
            const TSharedPtr<FJsonObject>& CustomDataJsonObj = CheckObjectData.custom_data;
            if (CustomDataJsonObj.IsValid() && JsonObjectData.IsValid())
            {
                JsonObjectData->SetObjectField(TEXT("custom_data"), CustomDataJsonObj);
            }
        }
    }
    return SceneDataJsonObj;
}

void UNVSceneFeatureExtractor_AnnotationData::UpdateProjectionMatrix()
{
    if (OwnerViewpoint)
    {
        const auto& CapturerSettings = OwnerViewpoint->GetCapturerSettings();
        const FNVImageSize& CaptureImageSize = CapturerSettings.CapturedImageSize;
        const float FOVAngle = CapturerSettings.GetFOVAngle();
        const ECameraProjectionMode::Type ProjectionMode = ECameraProjectionMode::Perspective; // NOTE: May need to add option for orthogonal mode
        const float OrthoWidth = CaptureImageSize.Width;
        const FTransform& ViewTransform = OwnerViewpoint->GetComponentTransform();

        if (CapturerSettings.bUseExplicitCameraIntrinsic)
        {
            FCameraIntrinsicSettings CurrentIntrinsicSettings = CapturerSettings.CameraIntrinsicSettings;
            CurrentIntrinsicSettings.UpdateSettings();
            ProjectionMatrix = CapturerSettings.CameraIntrinsicSettings.GetProjectionMatrix();
            ViewProjectionMatrix = UNVSceneCaptureComponent2D::BuildViewProjectionMatrix(ViewTransform, ProjectionMatrix);
        }
        else
        {
            ViewProjectionMatrix = UNVSceneCaptureComponent2D::BuildViewProjectionMatrix(ViewTransform, CaptureImageSize, ProjectionMode, FOVAngle, OrthoWidth, ProjectionMatrix);
        }
    }
}

bool UNVSceneFeatureExtractor_AnnotationData::GatherActorData(const AActor* CheckActor, FCapturedObjectData& ActorData)
{
    if (!OwnerViewpoint || !CheckActor || !ShouldExportActor(CheckActor))
    {
        return false;
    }

    const FString& ObjectName = CheckActor->GetName();

    UWorld* World = GetWorld();
    ensure(World);
    if (World)
    {
        const FVector& ViewLocation = OwnerViewpoint->GetComponentLocation();
        const FTransform& WorldToCameraTransform = OwnerViewpoint->GetComponentToWorld().Inverse();
        const FMatrix& WorldToCameraMatrix_UE4 = WorldToCameraTransform.ToMatrixNoScale();
        const FMatrix& WorldToCameraMatrix_OpenCV = WorldToCameraMatrix_UE4 * NVSceneCapturerUtils::UE4ToOpenCVMatrix;

        const FTransform& ActorToWorldTransform = CheckActor->GetActorTransform();
        const FMatrix& ActorToWorldMatrix_UE4 = ActorToWorldTransform.ToMatrixWithScale();
        const FMatrix& ActorToWorldMatrix_OpenCV = ActorToWorldMatrix_UE4 * NVSceneCapturerUtils::UE4ToOpenCVMatrix;
        const FMatrix& ActorToCameraMatrix_UE4 = ActorToWorldMatrix_UE4 * WorldToCameraMatrix_UE4;
        const FMatrix& ActorToCameraMatrix_OpenCV = ActorToWorldMatrix_UE4 * WorldToCameraMatrix_UE4 * NVSceneCapturerUtils::UE4ToOpenCVMatrix;

        const FTransform& ActorToWorldTransform_OpenCV = FTransform(ActorToWorldMatrix_OpenCV);
        const FTransform& ActorToCameraTransform_OpenCV = FTransform(ActorToCameraMatrix_OpenCV);


        const FVector& ActorLocation = CheckActor->GetActorLocation();
        const FVector& ActorForwardDir = ActorToWorldTransform.GetRotation().Vector();

        ActorData.location_worldspace = NVSceneCapturerUtils::UE4ToOpenCVMatrix.TransformPosition(ActorLocation);
        ActorData.location = WorldToCameraMatrix_OpenCV.TransformPosition(ActorLocation);

        const UNVCapturableActorTag* Tag = Cast<UNVCapturableActorTag>(CheckActor->GetComponentByClass(UNVCapturableActorTag::StaticClass()));
        // Fill in actor's data
        ActorData.Name = ObjectName;
        ActorData.Class = Tag ? Tag->Tag : ObjectName;
        ANVSceneManager* NVSceneManagerPtr = ANVSceneManager::GetANVSceneManagerPtr();
        if (NVSceneManagerPtr)
        {
            ActorData.instance_id = NVSceneManagerPtr->ObjectInstanceSegmentation.GetInstanceId(CheckActor);
        }
        else
        {
            ActorData.instance_id = 0;
        }
        const FQuat& ActorQuaternion_UE4 = ActorToWorldTransform.GetRotation();
        const FRotator& ActorRotator_UE4 = ActorToWorldTransform.Rotator();

        // OpenCV coordinate system
        // NOTE: The actor transform matrix may included scaling, if we just use ToQuat(), UE4 will just return [0, 0, 0, 1] when the rotation matrix is not an identity matrix
        // => we need to convert it first using GetMatrixWithoutScale
        ActorData.quaternion_worldspace = NVSceneCapturerUtils::ConvertQuaternionToOpenCVCoordinateSystem(ActorToWorldMatrix_UE4.GetMatrixWithoutScale().ToQuat());
        ActorData.rotation_worldspace = ActorData.quaternion_worldspace.Rotator();

        const FQuat& ActorCamQuat_OpenCV = ActorToCameraMatrix_OpenCV.GetMatrixWithoutScale().ToQuat();
        const FVector& ActorCamLocation_OpenCV = ActorToCameraMatrix_OpenCV.GetOrigin();
        ActorData.quaternion_xyzw = NVSceneCapturerUtils::ConvertQuaternionToOpenCVCoordinateSystem(ActorToCameraMatrix_UE4.GetMatrixWithoutScale().ToQuat());
        ActorData.rotation = ActorToCameraMatrix_UE4.Rotator();

        ActorData.actor_to_camera_matrix = ActorToCameraMatrix_OpenCV;
        ActorData.pose_transform = ActorToCameraMatrix_OpenCV;
        ActorData.actor_to_world_matrix_ue4 = ActorToWorldMatrix_UE4;
        ActorData.actor_to_world_matrix_opencv = ActorToWorldMatrix_OpenCV;

        UMeshComponent* ValidMeshComp = NVSceneCapturerUtils::GetFirstValidMeshComponent(CheckActor);
        if (!ValidMeshComp)
        {
            return false;
        }

        // Find the actor's cuboid
        FNVCuboidData ActorCuboid;
        switch (ProtectedDataExportSettings.BoundsType)
        {
            case ENVBoundsGenerationType::VE_OOBB:
                // TODO: Right now some of the mesh's collision components are quite different from its mesh => just don't use the collision component for now
                // May be later we can just use the cuboid from the AAnnotatedActor which are only calculated once
                ActorCuboid = NVSceneCapturerUtils::GetActorCuboid_OOBB_Simple(CheckActor, false);
                break;
            case ENVBoundsGenerationType::VE_TightOOBB:
                ActorCuboid = NVSceneCapturerUtils::GetActorCuboid_OOBB_Complex(CheckActor);
                break;
            default:
            case ENVBoundsGenerationType::VE_AABB:
                ActorCuboid = NVSceneCapturerUtils::GetActorCuboid_AABB(CheckActor);
                break;
        }
        for (const FVector& CuboidVertex : ActorCuboid.Vertexes)
        {
            const FVector VertexImgPoint = ProjectWorldPositionToImagePosition(CuboidVertex);
            // TODO: Should check VertexImgPoint.Z > 0 to see if the location is in front of the camera or not
            ActorData.projected_cuboid.Add(FVector2D(VertexImgPoint.X, VertexImgPoint.Y));

            const FVector& VertexCameraSpace = WorldToCameraMatrix_OpenCV.TransformPosition(CuboidVertex);
            ActorData.cuboid.Add(VertexCameraSpace);
        }
        ActorData.dimensions_worldspace = NVSceneCapturerUtils::ConvertDimensionToOpenCVCoordinateSystem(ActorCuboid.GetDimension());

        const FVector& BoundingBoxCenter_WorldUE4 = ActorCuboid.GetCenter();
        ActorData.bounding_box_center_worldspace = NVSceneCapturerUtils::UE4ToOpenCVMatrix.TransformPosition(BoundingBoxCenter_WorldUE4);
        ActorData.cuboid_centroid = WorldToCameraMatrix_OpenCV.TransformPosition(BoundingBoxCenter_WorldUE4);
        ActorData.projected_cuboid_centroid = FVector2D(ProjectWorldPositionToImagePosition(BoundingBoxCenter_WorldUE4));

        //ActorData.bounding_box_forward_direction = ActorCuboid.GetDirection().GetSafeNormal();
        ActorData.bounding_box_forward_direction = ActorForwardDir;

        // Calculate the forward direction of the cuboid projected to the 2d screen
        const FVector& CuboidForwardLocation = ActorData.bounding_box_center_worldspace + ActorData.bounding_box_forward_direction * 10.f;
        const FVector2D& CuboidForwardLocation2D = FVector2D(ProjectWorldPositionToImagePosition(CuboidForwardLocation));
        ActorData.bounding_box_forward_direction_imagespace = (CuboidForwardLocation2D - ActorData.projected_cuboid_centroid).GetSafeNormal();

        // TODO: Calculate the azimuth and altitude of the object in the camera space using OpenCV coordinate system
        // Calculate Azimuth
        float ViewpointAzimuthAngle, ViewpointAltitudeAngle;
        NVSceneCapturerUtils::CalculateSphericalCoordinate(ViewLocation, ActorLocation, ActorForwardDir, ViewpointAzimuthAngle, ViewpointAltitudeAngle);
        ActorData.viewpoint_azimuth_angle = ViewpointAzimuthAngle;
        ActorData.viewpoint_altitude_angle = ViewpointAltitudeAngle;

        // Calculate the actor's scale of distance to the view point
        const float ActorDistanceToViewpoint = FVector::Dist(ActorLocation, ViewLocation);
        const float MinDist = ProtectedDataExportSettings.DistanceScaleRange.Min;
        const float MaxDist = ProtectedDataExportSettings.DistanceScaleRange.Max;
        const float DistRange = MaxDist - MinDist;
        if (DistRange > 0.f)
        {
            ActorData.distance_scale = (ActorDistanceToViewpoint - MinDist) / DistRange;
        }
        else
        {
            ActorData.distance_scale = (ActorDistanceToViewpoint >= MaxDist) ? 1.f : 0.f;
        }

        FBox2D ActorBB2D = GetBoundingBox2D(CheckActor, false);
        // Calculate Truncated
        FBox2D ClampedActorBB2D = ActorBB2D;
        ClampedActorBB2D.Min.X = FMath::Clamp(ActorBB2D.Min.X, 0.f, 1.f);
        ClampedActorBB2D.Min.Y = FMath::Clamp(ActorBB2D.Min.Y, 0.f, 1.f);
        ClampedActorBB2D.Max.X = FMath::Clamp(ActorBB2D.Max.X, 0.f, 1.f);
        ClampedActorBB2D.Max.Y = FMath::Clamp(ActorBB2D.Max.Y, 0.f, 1.f);
        ActorData.bounding_box = ActorBB2D;

        // Calculate Occluded
        // count how many of the points are occluded
        int OccludedPointsCount = 0;    // how many bbox corners were occluded by a ray trace?
        for (FVector CheckVertex : ActorCuboid.Vertexes)
        {
            FHitResult TraceHitResult;
            FCollisionObjectQueryParams objectQueryParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;
            FCollisionQueryParams queryParams = FCollisionQueryParams::DefaultQueryParam;
            queryParams.AddIgnoredActor(CheckActor);

            if (World->LineTraceSingleByObjectType(TraceHitResult, ViewLocation, CheckVertex, objectQueryParams, queryParams))
            {
                OccludedPointsCount++;  // keep track of how many of the bound corners are blocked.
            }
        }

        ActorData.occluded = 0;
        if (OccludedPointsCount > 0)
        {
            // more than half means "largely occluded"
            // TODO: Create enum for 'occluded type' instead of using number directly like this
            ActorData.occluded = (OccludedPointsCount > 4) ? 2 : 1;
        }

        // TODO: Trace against a 3d voxelized volume of the target actor
        // Find the 3d bounding box in the camera coordinate
        const UMeshComponent* ActorMesh = NVSceneCapturerUtils::GetFirstValidMeshComponent(CheckActor);
        TArray<FVector> MeshBoundVertexes = NVSceneCapturerUtils::GetSimpleCollisionVertexes(ActorMesh);
        FBox CameraSpaceBoundingBox(EForceInit::ForceInitToZero);
        const FTransform& CameraTransform = OwnerViewpoint->GetComponentTransform();
        // Find the nearest and farthest vertexes
        for (const FVector& CheckVertex : MeshBoundVertexes)
        {
            const FVector VertexCameraSpace = CameraTransform.InverseTransformPosition(CheckVertex);
            CameraSpaceBoundingBox += VertexCameraSpace;
        }
        const FVector& CamSpaceBBSize = CameraSpaceBoundingBox.GetSize();

        ActorData.occlusion = 0.f;
        //FHitResult LastBlockedHitResult;
        if (CameraSpaceBoundingBox.IsValid)
        {
            // Calculate the sampling rate in each direction
            static const int BB2dOcclusionSamplingRes = 10;
            FVector VoxelSamplingRate;
            // Use higher sampling rate for the image space X, Z
            // Use a lower sampling rate for the depth X
            VoxelSamplingRate.X = FMath::Min(BB2dOcclusionSamplingRes / 2, FMath::RoundToInt(CamSpaceBBSize.X));
            VoxelSamplingRate.Z = FMath::Min(BB2dOcclusionSamplingRes, FMath::RoundToInt(CamSpaceBBSize.Z));
            VoxelSamplingRate.Y = FMath::Min(BB2dOcclusionSamplingRes, FMath::RoundToInt(CamSpaceBBSize.Y));
            FVector VoxelSamplingStep(1.f / VoxelSamplingRate.X, 1.f / VoxelSamplingRate.Y, 1.f / VoxelSamplingRate.Z);

            int TotalSampledCellCount = 0;
            int OccludedCellCount = 0;
            // Loop through all the cell in the 3d voxel grid
            for (int y = 0; y < VoxelSamplingRate.Y; y++)
            {
                for (int z = 0; z < VoxelSamplingRate.Y; z++)
                {
                    for (int x = 0; x < VoxelSamplingRate.X; x++)
                    {
                        FVector CellCenterCameraSpace;
                        CellCenterCameraSpace.X = FMath::Lerp(CameraSpaceBoundingBox.Min.X, CameraSpaceBoundingBox.Max.X, (x + 0.5f) * VoxelSamplingStep.X);
                        CellCenterCameraSpace.Y = FMath::Lerp(CameraSpaceBoundingBox.Min.Y, CameraSpaceBoundingBox.Max.Y, (y + 0.5f) * VoxelSamplingStep.Y);
                        CellCenterCameraSpace.Z = FMath::Lerp(CameraSpaceBoundingBox.Min.Z, CameraSpaceBoundingBox.Max.Z, (z + 0.5f) * VoxelSamplingStep.Z);

                        const FVector& CellCenterWorld = CameraTransform.TransformPosition(CellCenterCameraSpace);

                        const FVector& TraceStart = ViewLocation;
                        const FVector& TraceEnd = CellCenterWorld;

                        FHitResult TraceHitResult;
                        const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam;
                        FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
                        //QueryParams.AddIgnoredActor(CheckActor);

                        if (World->LineTraceSingleByChannel(TraceHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams, ResponseParam))
                        {
                            // Only care about the cell that actually have something there
                            TotalSampledCellCount++;

                            // If the trace hit other actor instead the target one then it mean it's occluded
                            if (TraceHitResult.Actor.IsValid() && (TraceHitResult.Actor != CheckActor))
                            {
                                OccludedCellCount++;
                            }

                            // No need to go deeper when we already hit something
                            break;
                        }
                    }
                }
            }

            if (TotalSampledCellCount > 0)
            {
                ActorData.occlusion = float(OccludedCellCount) / TotalSampledCellCount;
            }
        }
        else
        {
            ActorData.occlusion = 1.f;
        }

        ActorData.visibility = FMath::Clamp(1.f - ActorData.occlusion, 0.f, 1.f);

        const float ClampedArea = ClampedActorBB2D.GetArea();
        const float FullArea = ActorBB2D.GetArea();
        ActorData.truncated = (FullArea > 0.f) ? (1.f - (ClampedArea / FullArea)) : 1.f;

        // Gather the socket data
        if (Tag)
        {
            TArray<UMeshComponent*> MeshComponents;
            CheckActor->GetComponents(MeshComponents);

            bool bNeedExportSockets = Tag->bExportAllMeshSocketInfo || (Tag->SocketNameToExportList.Num() > 0);
            if (bNeedExportSockets)
            {
                for (UMeshComponent* CheckMeshComp : MeshComponents)
                {
                    if (CheckMeshComp)
                    {
                        const TArray<FName>& AllSocketNames = CheckMeshComp->GetAllSocketNames();
                        for (const FName CheckSocketName : AllSocketNames)
                        {
                            bool bShouldExportSocket = Tag->bExportAllMeshSocketInfo || Tag->SocketNameToExportList.Contains(CheckSocketName);
                            if (bShouldExportSocket)
                            {
                                const FVector& SocketWorldLocation = CheckMeshComp->GetSocketLocation(CheckSocketName);
                                const FVector& SocketScreenPosition = ProjectWorldPositionToImagePosition(SocketWorldLocation);
                                const FVector2D& SocketLocation = FVector2D(SocketScreenPosition.X, SocketScreenPosition.Y);

                                FNVSocketData NewSocketData;
                                NewSocketData.SocketName = CheckSocketName.ToString();
                                NewSocketData.SocketLocation = SocketLocation;

                                ActorData.socket_data.Add(NewSocketData);
                            }
                        }
                    }
                }
            }
        }

        const ANVAnnotatedActor* AnnotatedActor = Cast<ANVAnnotatedActor>(CheckActor);
        if (AnnotatedActor)
        {
            ActorData.custom_data = AnnotatedActor->GetCustomAnnotatedData();
        }
    }
    return true;
}

bool UNVSceneFeatureExtractor_AnnotationData::ShouldExportActor(const AActor* CheckActor) const
{
    bool bShouldExport = false;
    const auto& CapturerSettings = OwnerViewpoint->GetCapturerSettings();

    // Only care about valid actor
    if (CheckActor)
    {
        if (ProtectedDataExportSettings.bIgnoreHiddenActor)
        {
            FConvexVolume ViewFrustum;
            GetViewFrustumBounds(ViewFrustum, ViewProjectionMatrix, true);

            // The actor is considered as hidden if it's not rendered in the game
            // or it doesn't appear on the viewport
            if (CheckActor->IsHidden() || !IsActorInViewFrustum(ViewFrustum, CheckActor))
            {
                return bShouldExport;
            }
        }

        //Check if it's flagged
        //TODO (OS): Implement ENVIncludeObjects::MatchesTag
        UNVCapturableActorTag* Tag = Cast<UNVCapturableActorTag>(CheckActor->GetComponentByClass(UNVCapturableActorTag::StaticClass()));
        bShouldExport |= (!ProtectedDataExportSettings.bIgnoreHiddenActor) && (!CheckActor->IsHidden());
        bShouldExport |= ((ProtectedDataExportSettings.IncludeObjectsType == ENVIncludeObjects::AllTaggedObjects) && Tag && (Tag->bIncludeMe /* || IncludeAll*/));

        if (bShouldExport)
        {
            bShouldExport = false;
            // Ensure the actor have a mesh
            TArray<UMeshComponent*> MeshComponents;
            CheckActor->GetComponents(MeshComponents);
            if (MeshComponents.Num() != 0)
            {
                // Check if the actor actually have a valid bound
                const FBox ActorBounds = CheckActor->GetComponentsBoundingBox(true); // true means all subcomponents
                const FVector BoxExtent = ActorBounds.GetExtent();
                if (!BoxExtent.IsZero())
                {
                    bShouldExport = true;
                }
            }
        }
    }

    return bShouldExport;
}

bool UNVSceneFeatureExtractor_AnnotationData::IsActorInViewFrustum(const FConvexVolume& ViewFrustum, const AActor* CheckActor) const
{
    if (!CheckActor)
    {
        return false;
    }

    //Check if Bounds are > 0
    const FBox ActorBounds = CheckActor->GetComponentsBoundingBox(true); //true means all non-colliding subcomponents
    const FVector Origin = ActorBounds.GetCenter();
    const FVector BoxExtent = ActorBounds.GetExtent();

    //Skip 0-extent objects
    if (BoxExtent.X == 0.f || BoxExtent.Y == 0.f || BoxExtent.Z == 0.f)
    {
        return false;
    }

    // NOTE: The Frustum's intersect test may not be too cheap, can just get the cuboid of the actor and project them into the 2d image to see if any of them in range
    // NOTE: this test is conservative.  If any part of the box extents intersect the frustum, actor is considered as in the view frustum
    return ViewFrustum.IntersectBox(Origin, BoxExtent);
}

FVector UNVSceneFeatureExtractor_AnnotationData::ProjectWorldPositionToImagePosition(const FVector& WorldPosition) const
{
    // This Plane calculation is from FSceneView::Project
    FPlane ProjectedPlane = ViewProjectionMatrix.TransformFVector4(FVector4(WorldPosition, 1));
    if (ProjectedPlane.W == 0)
    {
        ProjectedPlane.W = KINDA_SMALL_NUMBER;
    }
    const float RHW = 1.0f / ProjectedPlane.W;
    ProjectedPlane.X *= RHW;
    ProjectedPlane.Y *= RHW;
    ProjectedPlane.Z *= RHW;

    FVector PlanePos = FVector(ProjectedPlane);
    if (ProjectedPlane.W <= 0.f)
    {
        PlanePos.Z = 0.f;
    }

    // Convert the position to be in range of [0, 1] from [-1, 1]
    FVector ImagePos = PlanePos;
    ImagePos.X = 0.5f * (PlanePos.X + 1.f);
    ImagePos.Y = 0.5f * (-PlanePos.Y + 1.f);

    if (ProtectedDataExportSettings.bExportImageCoordinateInPixel)
    {
        const auto& CapturerSettings = OwnerViewpoint->GetCapturerSettings();
        const FNVImageSize& CaptureImageSize = CapturerSettings.CapturedImageSize;
        ImagePos.X = ImagePos.X * CaptureImageSize.Width;
        ImagePos.Y = ImagePos.Y * CaptureImageSize.Height;
    }

    return ImagePos;
}

FBox2D UNVSceneFeatureExtractor_AnnotationData::GetBoundingBox2D(const AActor* CheckActor, bool bClampToImage /*= true*/) const
{
    FBox2D ActorBB2D(EForceInit::ForceInitToZero);

    if (CheckActor)
    {
        TArray<UMeshComponent*> MeshComponents;
        CheckActor->GetComponents(MeshComponents);

        for (const UMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp)
            {
                ActorBB2D += Calculate2dAABB_MeshComplexCollision(MeshComp, bClampToImage);
            }
        }

        // TODO: Fallback to use the actor's cuboid vertexes in case the mesh doesn't have valid collision body set up
    }

    // Mark the bounding box as invalid if its area is empty
    if (ActorBB2D.GetArea() <= 0.f)
    {
        ActorBB2D.bIsValid = false;
    }

    return ActorBB2D;
}

FBox2D UNVSceneFeatureExtractor_AnnotationData::Calculate2dAABB(TArray<FVector> Vertexes, bool bClampToImage /*= true*/) const
{
    FBox2D BBox2D(EForceInit::ForceInitToZero);

    for (const FVector& VertexLoc : Vertexes)
    {
        FVector ProjectedVertexLoc = ProjectWorldPositionToImagePosition(VertexLoc);
        if (bClampToImage)
        {
            ProjectedVertexLoc.X = FMath::Clamp(ProjectedVertexLoc.X, 0.f, 1.f);
            ProjectedVertexLoc.Y = FMath::Clamp(ProjectedVertexLoc.Y, 0.f, 1.f);
        }

        BBox2D += FVector2D(ProjectedVertexLoc.X, ProjectedVertexLoc.Y);
    }

    return BBox2D;
}

FBox2D UNVSceneFeatureExtractor_AnnotationData::Calculate2dAABB_MeshComplexCollision(const class UMeshComponent* CheckMeshComp, bool bClampToImage /*= true*/) const
{
    FBox2D BBox2D(EForceInit::ForceInitToZero);

    TArray<FVector> BoundVertexes;

    const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(CheckMeshComp);
    if (StaticMeshComp)
    {
        const FTransform& MeshTransform = StaticMeshComp->GetComponentTransform();
        const UStaticMesh* CheckMesh = StaticMeshComp->GetStaticMesh();
        if (CheckMesh && CheckMesh->BodySetup)
        {
            const FKAggregateGeom& MeshGeom = CheckMesh->BodySetup->AggGeom;

            for (const FKConvexElem& ConvexElem : MeshGeom.ConvexElems)
            {
                for (const FVector& CheckVertex : ConvexElem.VertexData)
                {
                    const FVector& VertexWorldLocation = MeshTransform.TransformPosition(CheckVertex);
                    BoundVertexes.Add(VertexWorldLocation);
                }
            }
        }

        bool bHaveBodyVertexData = (BoundVertexes.Num() > 0);
        // Fallback to use the mesh's render data if it doesn't have a valid body setup
        if (!bHaveBodyVertexData && CheckMesh && CheckMesh->RenderData)
        {
            const FPositionVertexBuffer& MeshVertexBuffer = CheckMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
            const uint32 VertexesCount = MeshVertexBuffer.GetNumVertices();
            for (uint32 i = 0; i < VertexesCount; i++)
            {
                const FVector& VertexPosition = MeshTransform.TransformPosition(MeshVertexBuffer.VertexPosition(i));
                BoundVertexes.Add(VertexPosition);
            }
        }
    }
    else
    {
        const USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(CheckMeshComp);
        if (SkeletalMeshComp)
        {
            const USkeletalMesh* SkeletalMesh = SkeletalMeshComp ? SkeletalMeshComp->SkeletalMesh : nullptr;
            if (SkeletalMesh)
            {
                const UPhysicsAsset* MeshPhysicsAsset = SkeletalMesh->PhysicsAsset;
                if (MeshPhysicsAsset)
                {
                    const FTransform& MeshTransform = SkeletalMeshComp->GetComponentTransform();
                    for (const USkeletalBodySetup* CheckSkeletalBodySetup : MeshPhysicsAsset->SkeletalBodySetups)
                    {
                        if (CheckSkeletalBodySetup)
                        {
                            const FKAggregateGeom& MeshGeom = CheckSkeletalBodySetup->AggGeom;
                            for (const FKConvexElem& ConvexElem : MeshGeom.ConvexElems)
                            {
                                for (const FVector& CheckVertex : ConvexElem.VertexData)
                                {
                                    const FVector& VertexWorldLocation = MeshTransform.TransformPosition(CheckVertex);
                                    BoundVertexes.Add(VertexWorldLocation);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (BoundVertexes.Num() > 0)
    {
        BBox2D = Calculate2dAABB(BoundVertexes, bClampToImage);
    }

    return BBox2D;
}

//=========================================== FNVDataExportSettings ===========================================
FNVDataExportSettings::FNVDataExportSettings()
{
    IncludeObjectsType = ENVIncludeObjects::AllTaggedObjects;
    bIgnoreHiddenActor = true;
    BoundsType = ENVBoundsGenerationType::VE_OOBB;
    BoundingBox2dType = ENVBoundBox2dGenerationType::FromMeshBodyCollision;
    bOutputEvenIfNoObjectsAreInView = true;
    DistanceScaleRange = FFloatInterval(100.f, 1000.f);
    bExportImageCoordinateInPixel = true;
}
