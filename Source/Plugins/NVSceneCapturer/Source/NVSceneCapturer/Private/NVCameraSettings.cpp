/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVCameraSettings.h"

//=========================== FCameraIntrinsicSettings ===========================
FCameraIntrinsicSettings::FCameraIntrinsicSettings(int ResWidth, int ResHeight, float InFx, float InFy, float InCx, float InCy, float InS/*= 0.f*/)
    : ResX(ResWidth), ResY(ResHeight), Fx(InFx), Fy(InFy), Cx(InCx), Cy(InCy), S(InS)
{
}

FCameraIntrinsicSettings::FCameraIntrinsicSettings(int ResWidth, int ResHeight, float HFOV)
    : ResX(ResWidth), ResY(ResHeight)
{
    Cx = ResX / 2.f;
    Cy = ResY / 2.f;
    float d = FMath::Tan(FMath::DegreesToRadians(HFOV) / 2.f);
    Fx = (d != 0.f) ? (Cx / d) : ResX / 2.f;
    Fy = Fx;
    S = 0.f;
}

FMatrix FCameraIntrinsicSettings::GetIntrinsicMatrix() const
{
    return IntrinsicMatrix;
}

FMatrix FCameraIntrinsicSettings::GetProjectionMatrix() const
{
    return ProjectionMatrix;
}

void FCameraIntrinsicSettings::UpdateSettings()
{
    // Build the intrinsic matrix
    IntrinsicMatrix = FMatrix(
                          FPlane(Fx, S, Cx, 0.f),
                          FPlane(0.f, Fy, Cy, 0.f),
                          FPlane(0.f, 0.f, 1.f, 0.f),
                          FPlane(0.f, 0.f, 0.f, 0.f)
                      );

    // TODO: May move the resolution and the clip distance to a separated struct?
    const float ZNear = 1.f;
    // NOTE: the following code was copied from the UE base engine and is purposely used here for the subcase where ZFar = ZNear
    //const float ZFar = 10000.f;
    const float ZFar = ZNear;
    const float ZDiff = ZFar - ZNear;

    const float a = (2.f * Fx) / ResX;
    const float b = (2.f * Fy) / ResY;
    const float c = (ZDiff <= 0.f) ? 0.f : (ZNear / -ZDiff);
    const float d = (ZDiff <= 0.f) ? ZNear : (-ZFar * ZNear / -ZDiff);
    const float c1 = (2.f * Cx) / ResX - 1.f;
    const float c2 = 1.f - (2.f * Cy) / ResY;

    ProjectionMatrix = FMatrix(
                           FPlane(a, 0.f, 0.f, 0.f),
                           FPlane(0.f, b, 0.f, 0.f),
                           FPlane(c1, c2, c, d),
                           FPlane(0.f, 0.f, 1.f, 0.f)
                       );
}

//=========================== FNVImageSize ===========================
FNVImageSize::FNVImageSize(int32 InWidth /*= 0*/, int32 InHeight /*= 0*/)
{
    Width = InWidth;
    Height = InHeight;
}

FIntPoint FNVImageSize::ConvertToIntPoint() const
{
    FIntPoint OutIntPoint(Width, Height);
    return OutIntPoint;
}

float FNVImageSize::GetAspectRatio() const
{
    return (Height > 0) ? (Width / float(Height)) : 1.f;
}

//=========================== UCameraSettingsFactoryBase ===========================
#if WITH_EDITORONLY_DATA
void UCameraSettingsFactoryBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    if (PropertyChangedEvent.MemberProperty)
    {
        UpdateCameraSettings();
        CameraSettings.UpdateSettings();
    }
}

#endif //WITH_EDITORONLY_DATA

void UCameraSettingsFactoryBase::UpdateCameraSettings()
{
    CameraSettings.ResX = Resolution.Width;
    CameraSettings.ResY = Resolution.Height;
}

//=========================== UCameraSettingFactory_HFOV ===========================
UCameraSettingFactory_HFOV::UCameraSettingFactory_HFOV() : Super()
{
    HFOV = 90.f;
}

void UCameraSettingFactory_HFOV::UpdateCameraSettings()
{
    Super::UpdateCameraSettings();

    // Update the focal length from the FOV
    const float d = FMath::Tan(FMath::DegreesToRadians(HFOV) / 2.f);
    CameraSettings.Fx = (d != 0.f) ? (CameraSettings.Cx / d) : CameraSettings.ResX / 2.f;
    CameraSettings.Fy = CameraSettings.Fx;
}
