/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneCapturerActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/CollisionProfile.h"
#include "Components/DrawFrustumComponent.h"


DEFINE_LOG_CATEGORY(LogNVSceneCapturerViewpointComponent);

UNVSceneCapturerViewpointComponent::UNVSceneCapturerViewpointComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
    if (!IsRunningCommandlet())
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> EditorCameraMesh(TEXT("/Engine/EditorMeshes/MatineeCam_SM"));
        CameraMesh = EditorCameraMesh.Object;
    }
#endif

    bAutoActivate = true;
}

void UNVSceneCapturerViewpointComponent::SetupFeatureExtractors()
{
    UpdateCapturerSettings();
    const auto& FeatureExtractorSettings = GetFeatureExtractorSettings();
    for (const auto& CheckFeatureExtractorSetting : FeatureExtractorSettings)
    {
        const auto& FeatureExtractor = CheckFeatureExtractorSetting.FeatureExtractorRef;
        if (FeatureExtractor && FeatureExtractor->IsEnabled())
        {
            UClass* ExtractorClass = FeatureExtractor->GetClass();
            FName NewExtractorName = FName(*FString::Printf(TEXT("%s.%s"), *this->GetName(), *FeatureExtractor->GetDisplayName()));
            NewExtractorName = MakeUniqueObjectName(this, ExtractorClass, NewExtractorName);
            UNVSceneFeatureExtractor* NewSubFeatureExtractor = NewObject<UNVSceneFeatureExtractor>(GetOwner(),
                    ExtractorClass, NewExtractorName, EObjectFlags::RF_Transient, FeatureExtractor);
            if (NewSubFeatureExtractor)
            {
                NewSubFeatureExtractor->Init(this);
            }
        }
    }
}

void UNVSceneCapturerViewpointComponent::UpdateCapturerSettings()
{
    if (Settings.bOverrideCaptureSettings)
    {
        Settings.CaptureSettings.RandomizeSettings();
    }

    for (auto SceneFeatureExtractor : FeatureExtractorList)
    {
        if (SceneFeatureExtractor)
        {
            SceneFeatureExtractor->UpdateCapturerSettings();
        }
    }
}

const FNVSceneCapturerViewpointSettings& UNVSceneCapturerViewpointComponent::GetSettings() const
{
    return Settings;
}

const FNVSceneCapturerSettings& UNVSceneCapturerViewpointComponent::GetCapturerSettings() const
{
    if (!OwnerSceneCapturer || Settings.bOverrideCaptureSettings)
    {
        return Settings.CaptureSettings;
    }

    return OwnerSceneCapturer->CapturerSettings;
}

const TArray<FNVFeatureExtractorSettings>& UNVSceneCapturerViewpointComponent::GetFeatureExtractorSettings() const
{
    if (!OwnerSceneCapturer || Settings.bOverrideFeatureExtractorSettings)
    {
        return Settings.FeatureExtractorSettings;
    }

    return OwnerSceneCapturer->FeatureExtractorSettings;
}

bool UNVSceneCapturerViewpointComponent::IsEnabled() const
{
    return Settings.bIsEnabled;
}

FString UNVSceneCapturerViewpointComponent::GetDisplayName() const
{
    return (Settings.DisplayName.IsEmpty() ? GetName() : Settings.DisplayName);
}

bool UNVSceneCapturerViewpointComponent::CaptureSceneToPixelsData(UNVSceneCapturerViewpointComponent::OnFinishedCaptureScenePixelsDataCallback ViewpointCallback)
{
    bool bResults = false;

    ensure(ViewpointCallback);
    if (!ViewpointCallback)
    {
        UE_LOG(LogNVSceneCapturerViewpointComponent, Error, TEXT("invalid argument."));
    }
    else
    {
        bResults = true;
        for (auto SceneFeatureExtractor : FeatureExtractorList)
        {
            UNVSceneFeatureExtractor_PixelData* FeatureExtractorScenePixels = Cast<UNVSceneFeatureExtractor_PixelData>(SceneFeatureExtractor);
            if (FeatureExtractorScenePixels)
            {
                bResults = bResults && FeatureExtractorScenePixels->CaptureSceneToPixelsData(
                               [this, Callback = ViewpointCallback](const FNVTexturePixelData& CapturedPixelData, UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor)
                {
                    Callback(CapturedPixelData, CapturedFeatureExtractor, this);
                });
            }
        }
    }

    return bResults;
}

bool UNVSceneCapturerViewpointComponent::CaptureSceneAnnotationData(UNVSceneCapturerViewpointComponent::OnFinishedCaptureSceneAnnotationDataCallback ViewpointCallback)
{
	bool bResults = false;
	ensure(ViewpointCallback);
    if (!ViewpointCallback)
    {
        UE_LOG(LogNVSceneCapturerViewpointComponent, Error, TEXT("invalid argument."));
    }
    else
    {
		bResults = true;
		for (auto SceneFeatureExtractor : FeatureExtractorList)
        {
            UNVSceneFeatureExtractor_AnnotationData* FeatureExtractorAnnotationData = Cast<UNVSceneFeatureExtractor_AnnotationData>(SceneFeatureExtractor);
            if (FeatureExtractorAnnotationData)
            {
				bResults = bResults && FeatureExtractorAnnotationData->CaptureSceneAnnotationData(
                               [this, Callback = ViewpointCallback](TSharedPtr<FJsonObject> CapturedData, UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor)
                {
                    Callback(CapturedData, CapturedFeatureExtractor, this);
                });
            }
        }
    }
    return bResults;
}

void UNVSceneCapturerViewpointComponent::StartCapturing()
{
    for (auto SceneFeatureExtractor : FeatureExtractorList)
    {
        if (SceneFeatureExtractor)
        {
            SceneFeatureExtractor->StartCapturing();
        }
    }
}

void UNVSceneCapturerViewpointComponent::StopCapturing()
{
    for (auto SceneFeatureExtractor : FeatureExtractorList)
    {
        if (SceneFeatureExtractor)
        {
            SceneFeatureExtractor->StopCapturing();
        }
    }
}

#if WITH_EDITOR
bool UNVSceneCapturerViewpointComponent::GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut)
{
    ViewOut.Location = this->GetComponentLocation();
    ViewOut.Rotation = this->GetComponentRotation();

    const auto& CapturerSettings = GetCapturerSettings();
    ANVSceneCapturerActor* OwnerCapturer = Cast<ANVSceneCapturerActor>(GetOwner());
    if (OwnerCapturer)
    {
        // For the preview viewport, use the middle of the fov range
        ViewOut.FOV = CapturerSettings.FOVAngleRange.Interpolate(0.5f);

        // TODO: Get the post process materials from a feature extractor and apply it to the view
        //ViewOut.PostProcessSettings = OwnerCapturer->PostProcessSettings;
        ViewOut.PostProcessBlendWeight = 1.f;

        ViewOut.AspectRatio = (CapturerSettings.CapturedImageSize.Height > 0) ? (float(CapturerSettings.CapturedImageSize.Width) / CapturerSettings.CapturedImageSize.Height) : 1.f;
        ViewOut.bConstrainAspectRatio = true;
    }
    ViewOut.ProjectionMode = ECameraProjectionMode::Perspective;

    return true;
}
#endif // WITH_EDITOR

void UNVSceneCapturerViewpointComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UNVSceneCapturerViewpointComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UNVSceneCapturerViewpointComponent::OnRegister()
{
#if WITH_EDITORONLY_DATA
    if (AActor* MyOwner = GetOwner())
    {
        if (ProxyMeshComponent == nullptr)
        {
            ProxyMeshComponent = NewObject<UStaticMeshComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
            ProxyMeshComponent->SetupAttachment(this);
            ProxyMeshComponent->bIsEditorOnly = true;
            ProxyMeshComponent->SetStaticMesh(CameraMesh);
            ProxyMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            ProxyMeshComponent->bHiddenInGame = true;
            ProxyMeshComponent->CastShadow = false;
            //ProxyMeshComponent->PostPhysicsComponentTick.bCanEverTick = false;
            ProxyMeshComponent->CreationMethod = CreationMethod;
            ProxyMeshComponent->RegisterComponentWithWorld(GetWorld());
        }

        if (DrawFrustum == nullptr)
        {
            DrawFrustum = NewObject<UDrawFrustumComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
            DrawFrustum->SetupAttachment(this);
            DrawFrustum->bIsEditorOnly = true;
            DrawFrustum->CreationMethod = CreationMethod;
            DrawFrustum->RegisterComponentWithWorld(GetWorld());
        }
    }

    RefreshVisualRepresentation();
#endif

    Super::OnRegister();

    OwnerSceneCapturer = Cast<ANVSceneCapturerActor>(GetOwner());
}

void UNVSceneCapturerViewpointComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);
#if WITH_EDITORONLY_DATA
    if (ProxyMeshComponent)
    {
        ProxyMeshComponent->DestroyComponent();
    }
    if (DrawFrustum)
    {
        DrawFrustum->DestroyComponent();
    }
#endif //WITH_EDITORONLY_DATA
}

void UNVSceneCapturerViewpointComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
#if WITH_EDITORONLY_DATA
    UpdateProxyMeshTransform();
#endif

    Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

#if WITH_EDITORONLY_DATA
void UNVSceneCapturerViewpointComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
    UNVSceneCapturerViewpointComponent* This = CastChecked<UNVSceneCapturerViewpointComponent>(InThis);
    Collector.AddReferencedObject(This->ProxyMeshComponent);
    Collector.AddReferencedObject(This->DrawFrustum);

    Super::AddReferencedObjects(InThis, Collector);
}

void UNVSceneCapturerViewpointComponent::SetCameraMesh(UStaticMesh* Mesh)
{
    if (Mesh != CameraMesh)
    {
        CameraMesh = Mesh;

        if (ProxyMeshComponent)
        {
            ProxyMeshComponent->SetStaticMesh(Mesh);
        }
    }
}

void UNVSceneCapturerViewpointComponent::ResetProxyMeshTransform()
{
    if (ProxyMeshComponent != nullptr)
    {
        ProxyMeshComponent->ResetRelativeTransform();
    }
}


void UNVSceneCapturerViewpointComponent::RefreshVisualRepresentation()
{
    // TODO
    //if (DrawFrustum != nullptr)
    //{
    //  const float FrustumDrawDistance = 1000.0f;
    //  if (ProjectionMode == ECameraProjectionMode::Perspective)
    //  {
    //      DrawFrustum->FrustumAngle = FieldOfView;
    //      DrawFrustum->FrustumStartDist = 10.f;
    //      DrawFrustum->FrustumEndDist = DrawFrustum->FrustumStartDist + FrustumDrawDistance;
    //  }
    //  else
    //  {
    //      DrawFrustum->FrustumAngle = -OrthoWidth;
    //      DrawFrustum->FrustumStartDist = OrthoNearClipPlane;
    //      DrawFrustum->FrustumEndDist = FMath::Min(OrthoFarClipPlane - OrthoNearClipPlane, FrustumDrawDistance);
    //  }
    //  DrawFrustum->FrustumAspectRatio = AspectRatio;
    //  DrawFrustum->MarkRenderStateDirty();
    //}

    ResetProxyMeshTransform();
}

void UNVSceneCapturerViewpointComponent::UpdateProxyMeshTransform()
{
    if (ProxyMeshComponent)
    {
        FTransform OffsetCamToWorld = GetComponentToWorld();

        ResetProxyMeshTransform();

        FTransform LocalTransform = ProxyMeshComponent->GetRelativeTransform();
        FTransform WorldTransform = LocalTransform * OffsetCamToWorld;

        ProxyMeshComponent->SetWorldTransform(WorldTransform);
    }
}

void UNVSceneCapturerViewpointComponent::OverrideFrustumColor(FColor OverrideColor)
{
    if (DrawFrustum != nullptr)
    {
        DrawFrustum->FrustumColor = OverrideColor;
    }
}

void UNVSceneCapturerViewpointComponent::RestoreFrustumColor()
{
    if (DrawFrustum != nullptr)
    {
        //@TODO:
        const FColor DefaultFrustumColor(255, 0, 255, 255);
        DrawFrustum->FrustumColor = DefaultFrustumColor;
        //ACameraActor* DefCam = Cam->GetClass()->GetDefaultObject<ACameraActor>();
        //Cam->DrawFrustum->FrustumColor = DefCam->DrawFrustum->FrustumColor;
    }
}
#endif  // WITH_EDITORONLY_DATA

//========================================= FNVSceneCapturerViewpointSettings =========================================
FNVSceneCapturerViewpointSettings::FNVSceneCapturerViewpointSettings()
{
    bIsEnabled = true;
    DisplayName = TEXT("Viewpoint");
}