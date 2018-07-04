/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVCameraSettings.generated.h"

USTRUCT(Blueprintable)
struct FNVImageSize
{
    GENERATED_BODY()

public:
    /// The image's width (in pixels).
    UPROPERTY(EditAnywhere, meta = (UIMin = "1", ClampMin = "1"))
    int32 Width;

    /// The image's height (in pixels). 
    UPROPERTY(EditAnywhere, meta = (UIMin = "1", ClampMin = "1"))
    int32 Height;

public:
    FNVImageSize(int32 InWidth = 0, int32 InHeight = 0);
    FIntPoint ConvertToIntPoint() const;
    float GetAspectRatio() const;
};

USTRUCT()
struct NVSCENECAPTURER_API FNVNamedImageSizePreset
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere)
    FString Name;

    UPROPERTY(VisibleAnywhere)
    FNVImageSize ImageSize;
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FCameraIntrinsicSettings
{
    GENERATED_BODY()

public:
    /// Create the camera intrinsic settings from the horizontal field of view
    FCameraIntrinsicSettings(int ResWidth = 640, int ResHeight = 480, float HFOV = 90.f);
    FCameraIntrinsicSettings(int ResWidth, int ResHeight, float Fx, float Fy, float Cx, float Cy, float S = 0.f);

    FMatrix GetIntrinsicMatrix() const;
    FMatrix GetProjectionMatrix() const;

    void UpdateSettings();

public: // Editor properties
    /// Resolution's width
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Resolution - X"))
    int32 ResX;
    /// Resolution's height
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Resolution - Y"))
    int32 ResY;

    /// Focal length along X axis
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Focal length - X"))
    float Fx;
    /// Focal length along Y axis
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Focal length - Y"))
    float Fy;

    /// Principal point offset
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Principal point - X"))
    float Cx;
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Principal point - Y"))
    float Cy;

    /// Skew coefficient
    UPROPERTY(EditAnywhere, meta = (DisplayName = "Skew coefficient"))
    float S;

    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = "Matrix", meta = (DisplayName = "Intrinsic matrix"))
    FMatrix IntrinsicMatrix;

    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = "Matrix", meta = (DisplayName = "Projection matrix"))
    FMatrix ProjectionMatrix;
};

//============================= Camera settings factories =============================
/// Base class for all the different ways to create a camera settings
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, Abstract, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UCameraSettingsFactoryBase : public UObject
{
    GENERATED_BODY()
public:
    FCameraIntrinsicSettings GetCameraSettings() const
    {
        return CameraSettings;
    }

protected:
#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

    virtual void UpdateCameraSettings();

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = Settings)
    FNVImageSize Resolution;

protected:
    /// The camera settings which this factory present
    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = "Settings")
    FCameraIntrinsicSettings CameraSettings;
};

/// Create camera settings using horizontal field of view
UCLASS(Blueprintable, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UCameraSettingFactory_HFOV : public UCameraSettingsFactoryBase
{
    GENERATED_BODY()

public:
    UCameraSettingFactory_HFOV();

protected:
    virtual void UpdateCameraSettings() override;

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = Settings, meta=(DisplayName="Horizontal Field-Of-View"))
    float HFOV;
};
