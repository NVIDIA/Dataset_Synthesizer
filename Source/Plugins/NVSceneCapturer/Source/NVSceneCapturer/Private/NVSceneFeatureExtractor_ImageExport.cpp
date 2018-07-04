/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor_ImageExport.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneCaptureComponent2D.h"
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

#include "Kismet/KismetSystemLibrary.h"

//========================================== UNVSceneFeatureExtractor_ImageExport ==========================================
UNVSceneFeatureExtractor_PixelData::UNVSceneFeatureExtractor_PixelData(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DisplayName = TEXT("Scene View Image");
    bOnlyShowTrainingActors = false;
    PostProcessMaterial = nullptr;
    bOverrideExportImageType = false;
    ExportImageFormat = ENVImageFormat::PNG;
	CapturedPixelFormat = ENVCapturedPixelFormat::RGBA8;
    OverrideTexturePixelFormat = EPixelFormat::PF_Unknown;
    PostProcessBlendWeight = 1.f;
    CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SceneCaptureComponent = nullptr;
}

void UNVSceneFeatureExtractor_PixelData::UpdateSettings()
{
    UpdateMaterial();

    SceneCaptureComponent = CreateSceneCaptureComponent2d(PostProcessMaterialInstance);
}

UNVSceneCaptureComponent2D* UNVSceneFeatureExtractor_PixelData::CreateSceneCaptureComponent2d(UMaterialInstance* PostProcessingMaterial, const FString& ComponentName)
{
    UNVSceneCaptureComponent2D* NewSceneCaptureComp2D = nullptr;
    if (OwnerViewpoint)
    {
        const FName NewSceneCaptureComponentName = MakeUniqueObjectName(OwnerViewpoint, GetClass(),
            *FString::Printf(TEXT("%s_%s"), *GetDisplayName(), *ComponentName));

        AActor* OwnerActor = OwnerCapturer ? OwnerCapturer : OwnerViewpoint->GetOwner();
        NewSceneCaptureComp2D = NewObject<UNVSceneCaptureComponent2D>(OwnerActor,
            UNVSceneCaptureComponent2D::StaticClass(), NewSceneCaptureComponentName, EObjectFlags::RF_Transient);

        if (NewSceneCaptureComp2D)
        {
            if (bOverrideShowFlagSettings)
            {
                NewSceneCaptureComp2D->ShowFlagSettings = OverrideShowFlagSettings;
            }

            const auto& CapturerSettings = OwnerViewpoint->GetCapturerSettings();

            FAttachmentTransformRules AttachmentTransformRule = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
            NewSceneCaptureComp2D->AttachToComponent(OwnerViewpoint, AttachmentTransformRule);

            NewSceneCaptureComp2D->TextureTargetSize = CapturerSettings.CapturedImageSize;
            NewSceneCaptureComp2D->FOVAngle = CapturerSettings.GetFOVAngle();
            if (CapturerSettings.bUseExplicitCameraIntrinsic)
            {
                FCameraIntrinsicSettings CameraIntrinsicSettings = CapturerSettings.GetCameraIntrinsicSettings();
                CameraIntrinsicSettings.UpdateSettings();
                NewSceneCaptureComp2D->bUseCustomProjectionMatrix = true;
                NewSceneCaptureComp2D->CustomProjectionMatrix = CameraIntrinsicSettings.GetProjectionMatrix();
            }

            NewSceneCaptureComp2D->OverrideTexturePixelFormat = OverrideTexturePixelFormat;
			NewSceneCaptureComp2D->TextureTargetFormat = ConvertCapturedFormatToRenderTargetFormat(CapturedPixelFormat);

            NewSceneCaptureComp2D->CaptureSource = CaptureSource;

            if (PostProcessMaterialInstance)
            {
                FPostProcessSettings& SceneCapturePPS = NewSceneCaptureComp2D->PostProcessSettings;
                SceneCapturePPS.AddBlendable(PostProcessMaterialInstance, 1.f);
            }
            NewSceneCaptureComp2D->PostProcessBlendWeight = PostProcessBlendWeight;

            if (bOnlyShowTrainingActors)
            {
                UWorld* World = GetWorld();

                // TODO: Should create a TrainingActor class to handle actors we want to export
                // Let those actor register with the exporter so we don't need to do a loop through all the actor like this every time we export
                // NOTE: Can keep 1 relevant actor list on the exporter actor and all the exporter components can use it too
                for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
                {
                    AActor* CheckActor = *ActorIt;
                    if (CheckActor)
                    {
                        UNVCapturableActorTag* Tag = Cast<UNVCapturableActorTag>(CheckActor->GetComponentByClass(UNVCapturableActorTag::StaticClass()));
                        if (Tag && Tag->bIncludeMe)
                        {
                            NewSceneCaptureComp2D->ShowOnlyActorComponents(CheckActor);
                        }
                    }
                }
            }

            // Hide actors in the list of ignored actors
            for (AActor* CheckActor : IgnoreActors)
            {
                if (!CheckActor)
                {
                    NewSceneCaptureComp2D->HideActorComponents(CheckActor);
                }
            }

            FNVSceneCaptureComponentData NewSceneCaptureCompData;
            NewSceneCaptureCompData.SceneCaptureComp2D = NewSceneCaptureComp2D;
            NewSceneCaptureCompData.ComponentName = ComponentName;
            SceneCaptureComp2DDataList.Add(NewSceneCaptureCompData);

            NewSceneCaptureComp2D->RegisterComponent();
        }
    }
    return NewSceneCaptureComp2D;
}

UTextureRenderTarget2D* UNVSceneFeatureExtractor_PixelData::GetRenderTarget() const
{
    return SceneCaptureComponent ? SceneCaptureComponent->TextureTarget : nullptr;
}

void UNVSceneFeatureExtractor_PixelData::UpdateMaterial()
{
    PostProcessMaterialInstance = nullptr;
    if (PostProcessMaterial)
    {
        PostProcessMaterialInstance = UMaterialInstanceDynamic::Create(PostProcessMaterial, this, TEXT("PostProcessMaterialInstance"));
    }
}

bool UNVSceneFeatureExtractor_PixelData::CaptureSceneToPixelsData(UNVSceneFeatureExtractor_PixelData::OnFinishedCaptureScenePixelsDataCallback InCallback)
{
    bool bIsSucceeded = false;

    if (InCallback)
    {
        for (auto& SceneCaptureComp2DData : SceneCaptureComp2DDataList)
        {
            auto CheckSceneCaptureComp2D = SceneCaptureComp2DData.SceneCaptureComp2D;
            if (CheckSceneCaptureComp2D)
            {
                CheckSceneCaptureComp2D->CaptureSceneToPixelsData(
                    [this, Callback = InCallback](const FNVTexturePixelData& CapturedPixelData)
                {
                    Callback(CapturedPixelData, this);
                });
            }
        }
        bIsSucceeded = true;
    }
    return bIsSucceeded;
}

void UNVSceneFeatureExtractor_PixelData::UpdateCapturerSettings()
{
    if (OwnerViewpoint)
    {
        for (auto& SceneCaptureComp2DData : SceneCaptureComp2DDataList)
        {
            auto& SceneCaptureComp2D = SceneCaptureComp2DData.SceneCaptureComp2D;
            if (SceneCaptureComp2D)
            {
                SceneCaptureComp2D->FOVAngle = OwnerViewpoint->GetCapturerSettings().FOVAngle;
            }
        }
    }
}

void UNVSceneFeatureExtractor_PixelData::StartCapturing()
{
    Super::StartCapturing();

    if (bUpdateContinuously)
    {
        for (auto& SceneCaptureComp2DData : SceneCaptureComp2DDataList)
        {
            if (SceneCaptureComp2DData.SceneCaptureComp2D)
            {
                SceneCaptureComp2DData.SceneCaptureComp2D->StartCapturing();
            }
        }
    }
}

void UNVSceneFeatureExtractor_PixelData::StopCapturing()
{
    if (bUpdateContinuously)
    {
        for (auto& SceneCaptureComp2DData : SceneCaptureComp2DDataList)
        {
            if (SceneCaptureComp2DData.SceneCaptureComp2D)
            {
                SceneCaptureComp2DData.SceneCaptureComp2D->StopCapturing();
            }
        }
    }
    Super::StopCapturing();
}

//========================================== UNVSceneFeatureExtractor_SceneDepth ==========================================
UNVSceneFeatureExtractor_SceneDepth::UNVSceneFeatureExtractor_SceneDepth(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DisplayName = TEXT("Depth");
    MaxDepthDistance = 3000.f;
	CapturedPixelFormat = ENVCapturedPixelFormat::R8;
}

void UNVSceneFeatureExtractor_SceneDepth::UpdateMaterial()
{
    Super::UpdateMaterial();

    if (PostProcessMaterialInstance)
    {
        static const FName MaxDepthParamName = FName(TEXT("MaxDepthDistance"));
        PostProcessMaterialInstance->SetScalarParameterValue(MaxDepthParamName, MaxDepthDistance);
    }
}

//========================================== UNVSceneFeatureExtractor_ScenePixelVelocity ==========================================
UNVSceneFeatureExtractor_ScenePixelVelocity::UNVSceneFeatureExtractor_ScenePixelVelocity(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    DisplayName = TEXT("PixelVelocity");
}

void UNVSceneFeatureExtractor_ScenePixelVelocity::UpdateSettings()
{
    Super::UpdateSettings();

    for (auto& SceneCaptureComp2DData : SceneCaptureComp2DDataList)
    {
        auto SceneCaptureComp2D = SceneCaptureComp2DData.SceneCaptureComp2D;
        if (SceneCaptureComp2D)
        {
#ifdef UE_SUPPORT_PIXEL_VELOCTY
            SceneCaptureComp2D->CaptureSource = ESceneCaptureSource::SCS_PixelVelocity;
#endif // UE_SUPPORT_PIXEL_VELOCTY
            SceneCaptureComp2D->TextureTargetFormat = ETextureRenderTargetFormat::RTF_RG32f;
        }
    }
}

//========================================== UNVSceneFeatureExtractor_StencilMask ==========================================
UNVSceneFeatureExtractor_StencilMask::UNVSceneFeatureExtractor_StencilMask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    DisplayName = TEXT("StencilMask");
	CapturedPixelFormat = ENVCapturedPixelFormat::R8;
}

void UNVSceneFeatureExtractor_StencilMask::UpdateSettings()
{
    Super::UpdateSettings();

    if (IsEnabled())
    {
        // Make sure the engine render to CustomDepth buffer
        UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepth 3"));
    }
}

//========================================== UNVSceneFeatureExtractor_ObjectSegmentation ==========================================
UNVSceneFeatureExtractor_VertexColorMask::UNVSceneFeatureExtractor_VertexColorMask(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DisplayName = TEXT("VertexColorMask");
}

void UNVSceneFeatureExtractor_VertexColorMask::UpdateSettings()
{
    Super::UpdateSettings();

    // TODO: Check if there's any NVObjectMaskManager_VertexColor set up in the level and give out warning when that happened
    if (SceneCaptureComponent)
    {
        // Since the VertexColor render mode doesn't use alpha and cleared it, we should ignore it when trying to read back pixels value
        SceneCaptureComponent->bIgnoreReadbackAlpha = true;
        SceneCaptureComponent->PostProcessBlendWeight = 0.f;
        SceneCaptureComponent->ShowFlags = FEngineShowFlags(EShowFlagInitMode::ESFIM_Game);
        // Check Engine\Source\Editor\MeshPaint\Private\MeshPaintHelpers.cpp
        // Use VertexColor to render objects, and disable the post-process and lighting ...
        FEngineShowFlags& OverrideShowFlags = SceneCaptureComponent->ShowFlags;
        OverrideShowFlags.SetMaterials(true);
        OverrideShowFlags.SetLighting(false);
        OverrideShowFlags.SetBSPTriangles(true);
        OverrideShowFlags.SetVertexColors(true);
        OverrideShowFlags.SetPostProcessing(false);
        OverrideShowFlags.SetHMDDistortion(false);
        OverrideShowFlags.SetAntiAliasing(false);
        OverrideShowFlags.SetMotionBlur(false);
        OverrideShowFlags.SetSeparateTranslucency(false);
        OverrideShowFlags.SetSelection(true);
        OverrideShowFlags.SetVignette(false);
        OverrideShowFlags.SetTonemapper(false);
        OverrideShowFlags.SetColorGrading(false);

        if (SceneCaptureComponent->TextureTarget)
        {
            SceneCaptureComponent->TextureTarget->TargetGamma = 1.f;
        }
    }
    GVertexColorViewMode = EVertexColorViewMode::Color;
}
