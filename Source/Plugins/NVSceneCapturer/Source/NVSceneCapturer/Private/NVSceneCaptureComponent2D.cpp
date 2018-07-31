/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine.h"

DEFINE_LOG_CATEGORY(LogNVSceneCapturerComponent2D);

UNVSceneCaptureComponent2D::UNVSceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bCaptureEveryFrame = false;
    // NOTE: We already managing capturing either manually or through flag bCaptureEveryFrame => no need to capture on movement
    bCaptureOnMovement = false;
    bAlwaysPersistRenderingState = true;
    CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;

    TextureTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    OverrideTexturePixelFormat = EPixelFormat::PF_Unknown;
    bIgnoreReadbackAlpha = false;
}

void UNVSceneCaptureComponent2D::BeginPlay()
{
    Super::BeginPlay();

    InitTextureRenderTarget();
    RenderTargetReader.SetTextureRenderTarget(TextureTarget);
}

void UNVSceneCaptureComponent2D::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UNVSceneCaptureComponent2D::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    // NOTE: We don't call the Super function so we can override the conditions to whether we should capture this frame or not
    USceneCaptureComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ShouldCaptureCurrentFrame())
    {
        CaptureSceneDeferred();
    }
}

#if WITH_EDITOR
bool UNVSceneCaptureComponent2D::GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut)
{
    ViewOut.Location = this->GetComponentLocation();
    ViewOut.Rotation = this->GetComponentRotation();
    ViewOut.FOV = this->FOVAngle;
    ViewOut.ProjectionMode = this->ProjectionType;
    ViewOut.PostProcessSettings = this->PostProcessSettings;
    ViewOut.PostProcessBlendWeight = this->PostProcessBlendWeight;

    ViewOut.bConstrainAspectRatio = true;
    if (TextureTarget)
    {
        ViewOut.AspectRatio = (TextureTarget->SizeY > 0) ? (TextureTarget->SizeX / float(TextureTarget->SizeY)) : 1.f;
    }
    else
    {
        ViewOut.AspectRatio = TextureTargetSize.GetAspectRatio();
    }

    return true;
}
#endif //WITH_EDITOR

void UNVSceneCaptureComponent2D::InitTextureRenderTarget()
{
    if (!TextureTarget)
    {
        // Automatically create a new texture if there's no valid TextureTarget specified
        const FName TextureTargetName = MakeUniqueObjectName(this, UTextureRenderTarget2D::StaticClass(), TEXT("SceneCaptureTextureTarget"));
        TextureTarget = NewObject<UTextureRenderTarget2D>(this, TextureTargetName);
        ensure(TextureTarget);
        if (TextureTarget)
        {
            TextureTarget->TargetGamma = GEngine ? GEngine->GetDisplayGamma() : 0.f;
            TextureTarget->bForceLinearGamma = false;
            TextureTarget->SRGB = false;
            TextureTarget->bAutoGenerateMips = false;
            TextureTarget->bNeedsTwoCopies = true;
            TextureTarget->bGPUSharedFlag = true;
            if (OverrideTexturePixelFormat == EPixelFormat::PF_Unknown)
            {
                TextureTarget->RenderTargetFormat = TextureTargetFormat;
            }
            else
            {
                TextureTarget->OverrideFormat = OverrideTexturePixelFormat;
            }

            TextureTarget->InitAutoFormat(TextureTargetSize.Width, TextureTargetSize.Height);
            TextureTarget->ClearColor = FLinearColor::Black;

            // TODO: May need to keep 2 different pixel formats for the captured texture in the memory and the exported image on disk
            //ExportPixelFormat = TextureTarget->GetFormat();
        }
    }
}

void UNVSceneCaptureComponent2D::UpdateSceneCaptureContents(FSceneInterface* Scene)
{
    Super::UpdateSceneCaptureContents(Scene);

    // After the render commands to capture the scene are issued, it's safe to start issues pixels readback command for the texture
    OnSceneCaptured();
}

// NOTE: This function is copied from SceneCaptureRendering.cpp, from unexported UE engine code
FMatrix UNVSceneCaptureComponent2D::BuildProjectionMatrix(const FNVImageSize& RenderTargetSize, ECameraProjectionMode::Type ProjectionType, float FOV, float InOrthoWidth)
{
    float XAxisMultiplier;
    float YAxisMultiplier;
    FMatrix ProjectionMatrix;

    if (RenderTargetSize.Width > RenderTargetSize.Height)
    {
        // if the viewport is wider than it is tall
        XAxisMultiplier = 1.0f;
        YAxisMultiplier = RenderTargetSize.Width / (float)RenderTargetSize.Height;
    }
    else
    {
        // if the viewport is taller than it is wide
        XAxisMultiplier = RenderTargetSize.Height / (float)RenderTargetSize.Width;
        YAxisMultiplier = 1.0f;
    }

    if (ProjectionType == ECameraProjectionMode::Orthographic)
    {
        check((int32)ERHIZBuffer::IsInverted);
        const float OrthoWidth = InOrthoWidth / 2.0f;
        const float OrthoHeight = InOrthoWidth / 2.0f * YAxisMultiplier;

        const float NearPlane = 0;
        const float FarPlane = WORLD_MAX / 8.0f;

        const float ZScale = 1.0f / (FarPlane - NearPlane);
        const float ZOffset = -NearPlane;

        ProjectionMatrix = FReversedZOrthoMatrix(
                               OrthoWidth,
                               OrthoHeight,
                               ZScale,
                               ZOffset
                           );
    }
    else
    {
        // NOTE (TT): The input FOV is in degree => must convert to radian
        const float MatrixFOV = FMath::DegreesToRadians(FMath::Max(0.001f, FOV)) / 2.f;

        if ((int32)ERHIZBuffer::IsInverted)
        {
            ProjectionMatrix = FReversedZPerspectiveMatrix(
                                   MatrixFOV,
                                   MatrixFOV,
                                   XAxisMultiplier,
                                   YAxisMultiplier,
                                   GNearClippingPlane,
                                   GNearClippingPlane
                               );
        }
        else
        {
            ProjectionMatrix = FPerspectiveMatrix(
                                   MatrixFOV,
                                   MatrixFOV,
                                   XAxisMultiplier,
                                   YAxisMultiplier,
                                   GNearClippingPlane,
                                   GNearClippingPlane
                               );
        }
    }

    return ProjectionMatrix;
}

FMatrix UNVSceneCaptureComponent2D::BuildViewProjectionMatrix(const FTransform& ViewTransform, const FNVImageSize& RenderTargetSize, ECameraProjectionMode::Type ProjectionType,
        float FOVAngle, float OrthoWidth, FMatrix& ProjectionMatrix)
{
    ProjectionMatrix = BuildProjectionMatrix(RenderTargetSize, ProjectionType, FOVAngle, OrthoWidth);

    return BuildViewProjectionMatrix(ViewTransform, ProjectionMatrix);
}

FMatrix UNVSceneCaptureComponent2D::BuildViewProjectionMatrix(const FTransform& ViewTransform, const FMatrix& ProjectionMatrix)
{
    const FVector&  ViewLocation = ViewTransform.GetLocation();
    const FRotator& ViewRotation = ViewTransform.GetRotation().Rotator();

    const FMatrix& ViewTranslationMatrix = FTranslationMatrix(-ViewLocation);
    const FMatrix ViewPlanesMatrix = FMatrix(
                                         FPlane(0, 0, 1, 0),
                                         FPlane(1, 0, 0, 0),
                                         FPlane(0, 1, 0, 0),
                                         FPlane(0, 0, 0, 1));
    const FMatrix& ViewRotationMatrix = FInverseRotationMatrix(ViewRotation) * ViewPlanesMatrix;

    const FMatrix& ViewProjectionMatrix = ViewTranslationMatrix * ViewRotationMatrix * ProjectionMatrix;

    return ViewProjectionMatrix;
}

FMatrix UNVSceneCaptureComponent2D::GetProjectionMatrix() const
{
    return bUseCustomProjectionMatrix? 
                CustomProjectionMatrix : 
                BuildProjectionMatrix(TextureTargetSize, ProjectionType, FOVAngle, OrthoWidth);
}

void UNVSceneCaptureComponent2D::CaptureSceneToTexture()
{
    // TODO: Should use a counter/flag to see if the capture scene command already issued
    // This call still will not duplicate the call because SceneCapturesToUpdateMap use AddUnique function
    CaptureSceneDeferred();
}

bool UNVSceneCaptureComponent2D::ReadPixelsDataFromTexture(FNVTexturePixelData& OutPixelsData)
{
    return RenderTargetReader.ReadPixelsData(OutPixelsData);
}

void UNVSceneCaptureComponent2D::ReadPixelsDataFromTexture(OnFinishedCaptureScenePixelsDataCallback Callback)
{
    ensure(Callback);
    if (!Callback)
    {
        UE_LOG(LogNVSceneCapturerComponent2D, Error, TEXT("invalid argument."));
    }
    else
    {
        ReadbackCallbackList.Add(Callback);
    }
}

void UNVSceneCaptureComponent2D::CaptureSceneToPixelsData(UNVSceneCaptureComponent2D::OnFinishedCaptureScenePixelsDataCallback Callback)
{
    ensure(Callback);
    if (!Callback)
    {
        UE_LOG(LogNVSceneCapturerComponent2D, Error, TEXT("invalid argument."));
    }
    else
    {
        CaptureSceneDeferred();
        ReadPixelsDataFromTexture(Callback);
    }
}

void UNVSceneCaptureComponent2D::OnSceneCaptured()
{
    if (ShouldReadbackPixelsData())
    {
        // NOTE: Need to check the case where we want to capture the render target but doesn't want to read back the pixels

        RenderTargetReader.ReadPixelsData(
            [this, TempCallbackList=ReadbackCallbackList](const FNVTexturePixelData& CapturedPixelData)
        {
            // Trigger all the waiting callback, pass the captured pixel data and its context data to it
            for (auto WaitingCallback : TempCallbackList)
            {
                if (WaitingCallback)
                {
                    WaitingCallback(CapturedPixelData);
                }
            }
        }, bIgnoreReadbackAlpha);

        ReadbackCallbackList.Reset();
    }
}

void UNVSceneCaptureComponent2D::StartCapturing()
{
    bCaptureEveryFrame = true;
}

void UNVSceneCaptureComponent2D::StopCapturing()
{
    bCaptureEveryFrame = false;
}

bool UNVSceneCaptureComponent2D::ShouldCaptureCurrentFrame() const
{
    // TODO: Add more condition check here
    return (bCaptureEveryFrame);
}

bool UNVSceneCaptureComponent2D::ShouldReadbackPixelsData() const
{
    // Only automatically read back the pixel data if there are callbacks listening to it
    return (ReadbackCallbackList.Num() > 0);
}
