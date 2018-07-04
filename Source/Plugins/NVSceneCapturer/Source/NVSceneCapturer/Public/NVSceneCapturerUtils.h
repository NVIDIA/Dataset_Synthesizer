/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once
#include "NVSceneCapturerModule.h"
#include "IImageWrapper.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/JsonSerializerMacros.h"
#include "Serialization/JsonTypes.h"
#include "Paths.h"
#include "SharedPointer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "JsonObjectConverter.h"
#include "Json.h"
#include "NVCameraSettings.h"
#include "NVSceneCapturerUtils.generated.h"

// NOTE: Should remove this enum when the EImageFormat in IImageWrapper marked as UENUM
UENUM(BlueprintType)
enum class ENVImageFormat : uint8
{
    /// Portable Network Graphics.
    PNG               UMETA(DisplayName = "PNG (Portable Network Graphics)."),

    /// Joint Photographic Experts Group.
    JPEG              UMETA(DisplayName = "JPEG (Joint Photographic Experts Group)."),

    /// Single channel jpeg.
    GrayscaleJPEG     UMETA(DisplayName = "GrayscaleJPEG (Single channel jpeg"),

    /// Windows Bitmap.
    BMP               UMETA(DisplayName = "BMP (Windows Bitmap"),

    // OpenEXR (HDR) image file format
    // TODO: Support HDR format
    // EXR UMETA(DisplayName = "EXR (OpenEXR (HDR) image"),

    /// @cond DOXYGEN_SUPPRESSED_CODE
    NVImageFormat_MAX UMETA(Hidden)
    /// @endcond DOXYGEN_SUPPRESSED_CODE
};
EImageFormat ConvertExportFormatToImageFormat(ENVImageFormat ExportFormat);
FString GetExportImageExtension(ENVImageFormat ExportFormat);

/// The pixel format which can be captured
UENUM()
enum ENVCapturedPixelFormat
{
	/// R channel, 8 bit per channel fixed point, range [0, 1]
	/// Use this format for the grayscale 8 bit image type
	R8,
	
	/// RGBA channels, 8 bit per channel fixed point, range [0, 1]
	/// Use this format for the normal full color image type
	RGBA8,

	/// R channel, 16 bit per channel floating point, range [-65504, 65504]
	/// Use this format for the grayscale 16 bits image type
	R16f,

	/// R channel, 32 bit per channel floating point, range [-3.402823 x 10^38, 3.402823 x 10^38]
	/// NOTE: This format capture to 32 bits floating point value and can be exported to RGBA8 format
	R32f,

	/// @cond DOXYGEN_SUPPRESSED_CODE
	NVCapturedPixelFormat_MAX UMETA(Hidden)
	/// @endcond DOXYGEN_SUPPRESSED_CODE
};
ETextureRenderTargetFormat ConvertCapturedFormatToRenderTargetFormat(ENVCapturedPixelFormat PixelFormat);

USTRUCT()
struct FNVTexturePixelData
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TArray<uint8> PixelData;

    EPixelFormat PixelFormat;
    UPROPERTY(Transient)
    uint32 RowStride;
    UPROPERTY(Transient)
    FIntPoint PixelSize;
};

/// Data to be captured and exported for each socket
USTRUCT()
struct NVSCENECAPTURER_API FNVSocketData
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString SocketName;

    UPROPERTY()
    FVector2D SocketLocation;
};

/// This enum represent 8 corner vertexes of a rectangular cuboid
/// NOTE: The order of the enums here is what the researcher want for the training data.
/// If they want to change the exported order of these vertexes then we must update this order too
UENUM(BlueprintType)
enum class ENVCuboidVertexType : uint8
{
    FrontTopRight = 0,
    FrontTopLeft,
    FrontBottomLeft,
    FrontBottomRight,
    RearTopRight,
    RearTopLeft,
    RearBottomLeft,
    RearBottomRight,

    CuboidVertexType_MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVCuboidData
{
    GENERATED_BODY();

public:
    /// List of position for each vertexes in the cuboid
    UPROPERTY()
    FVector Vertexes[(uint8)ENVCuboidVertexType::CuboidVertexType_MAX];

    static uint8 TotalVertexesCount;

public:
    FNVCuboidData();
    FNVCuboidData(const FBox& AABB);
    FNVCuboidData(const FBox& LocalBox, const FTransform& LocalTransform);

    void BuildFromAABB(const FBox& AABB);
    void BuildFromOOBB(const FBox& OOBB, const FTransform& LocalTransform);

    FVector GetCenter() const
    {
        return Center;
    }
    FVector GetVertex(ENVCuboidVertexType VertexType) const
    {
        return Vertexes[(uint8)VertexType];
    }
    FVector GetExtent() const
    {
        return LocalBox.GetExtent();
    };
    FVector GetDimension() const
    {
        return LocalBox.GetExtent() * 2.f;
    }
    FVector GetDirection() const
    {
        return Rotation.Vector();
    }
    FQuat GetRotation() const
    {
        return Rotation;
    }
    bool IsValid() const
    {
        return (LocalBox.IsValid != 0);
    };

private:
    /// The center position of the cuboid
    UPROPERTY()
    FVector Center;

    UPROPERTY()
    FBox LocalBox;

    UPROPERTY()
    FQuat Rotation;
};

USTRUCT()
struct NVSCENECAPTURER_API FNVBox2D
{
    GENERATED_BODY()

    FNVBox2D(const FBox2D& box = FBox2D())
    {
        top_left = FVector2D(box.Min.Y, box.Min.X);
        bottom_right = FVector2D(box.Max.Y, box.Max.X);
    }

public:
    UPROPERTY()
    FVector2D top_left;

    UPROPERTY()
    FVector2D bottom_right;
};

USTRUCT()
struct NVSCENECAPTURER_API FCapturedObjectData
{
    GENERATED_BODY()

public:    // Properties
    /// Object's name
    UPROPERTY(Transient)
    FString Name;

    /// Name of the object's class
    UPROPERTY()
    FString Class;

    UPROPERTY(Transient)
    float truncated;

    UPROPERTY(Transient)
    uint32 occluded;

    /// Fraction of how much the object's 2d bounding box get occluded
    UPROPERTY(Transient)
    float occlusion;

    UPROPERTY()
    float visibility;

    UPROPERTY(Transient)
    FVector dimensions_worldspace;

    UPROPERTY(Transient)
    FVector location_worldspace;

    UPROPERTY()
    FVector location;

    UPROPERTY(Transient)
    FRotator rotation_worldspace;

    UPROPERTY(Transient)
    FQuat quaternion_worldspace;

    UPROPERTY(Transient)
    FRotator rotation;

    UPROPERTY()
    FQuat quaternion_xyzw;

    UPROPERTY(Transient)
    FMatrix actor_to_world_matrix_ue4;

    UPROPERTY(Transient)
    FMatrix actor_to_world_matrix_opencv;

    UPROPERTY(Transient)
    FMatrix actor_to_camera_matrix;

    UPROPERTY()
    FMatrix pose_transform;

    UPROPERTY(Transient)
    FVector bounding_box_center_worldspace;

    UPROPERTY()
    FVector cuboid_centroid;

    UPROPERTY()
    FVector2D projected_cuboid_centroid;

    UPROPERTY(Transient)
    FVector bounding_box_forward_direction;

    UPROPERTY(Transient)
    FVector2D bounding_box_forward_direction_imagespace;

    UPROPERTY(Transient)
    float viewpoint_azimuth_angle;

    UPROPERTY(Transient)
    float viewpoint_altitude_angle;

    UPROPERTY(Transient)
    float distance_scale;

    UPROPERTY()
    FNVBox2D bounding_box;

    // TODO: Create a struct for the cuboid since it must have exactly 8 corner vertexes
    UPROPERTY()
    TArray<FVector> cuboid;

    UPROPERTY()
    TArray<FVector2D> projected_cuboid;

    UPROPERTY(Transient)
    TArray<FNVSocketData> socket_data;

    TSharedPtr<FJsonObject> custom_data;
};

USTRUCT()
struct NVSCENECAPTURER_API FCapturedViewpointData
{
    GENERATED_BODY()


public: // Editor properties
    UPROPERTY()
    FVector location_worldframe;

    UPROPERTY()
    FQuat quaternion_xyzw_worldframe;

    UPROPERTY(Transient)
    FMatrix ProjectionMatrix;

    UPROPERTY(Transient)
    FMatrix ViewProjectionMatrix;

    UPROPERTY(Transient)
    FCameraIntrinsicSettings CameraSettings;

    UPROPERTY(Transient)
    float fov;

    // TODO: Add the view projection matrix
};

USTRUCT()
struct NVSCENECAPTURER_API FCapturedSceneData
{
    GENERATED_BODY()

public: // Editor properties
    UPROPERTY()
    FCapturedViewpointData camera_data;

    UPROPERTY()
    TArray<FCapturedObjectData> Objects;
};

USTRUCT()
struct NVSCENECAPTURER_API FCapturedFrameData
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FCapturedSceneData Data;

    // Pixels data of the frame
    UPROPERTY()
    TArray<FColor> SceneBitmap;
};


UCLASS(ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
class NVSCENECAPTURER_API UNVCapturableActorTag : public UActorComponent
{
    GENERATED_BODY()
public:
    UNVCapturableActorTag() : bIncludeMe(true) {};

	bool IsValid() const { return bIncludeMe && !Tag.IsEmpty();  }

public: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    FString Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    bool bIncludeMe;

    /// If true, find all the socket in the owner's meshes and export all of their name and transform data
    /// Otherwise only export socket in the list
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    bool bExportAllMeshSocketInfo;

    /// List of the name of the sockets from the owner's mesh need to be exported
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (editcondition = "!bExportAllMeshSocketInfo"))
    TArray<FName> SocketNameToExportList;
};

UENUM(BlueprintType)
enum class ENVIncludeObjects : uint8
{
    AllTaggedObjects,
    MatchesTag
};

UENUM(BlueprintType)
enum class ENVBoundsGenerationType : uint8
{
    /// World space AABB
    VE_AABB         UMETA(DisplayName = "AABB"),
    /// Object Space AABB, scaled, rotated and translated
    VE_OOBB         UMETA(DisplayName = "OOBB"),
    /// Arbitrary tight fitting bounds generated from mesh vertices.
    /// REMOVE: May not support this
    VE_TightOOBB    UMETA(DisplayName = "Tight Arbitrary OOBB")
};

UENUM(BlueprintType)
enum class ENVBoundBox2dGenerationType : uint8
{
    /// Generate the 2d bounding box from the mesh's 3d bounding box vertexes
    From3dBoundingBox,

    /// Generate the 2d bounding box from the mesh's body collision
    FromMeshBodyCollision,
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVSceneExporterConfig
{
    GENERATED_BODY()

public:
    FNVSceneExporterConfig();

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = "Export")
    bool bExportObjectData;

    UPROPERTY(EditAnywhere, Category = "Export")
    bool bExportScreenShot;

    UPROPERTY(EditAnywhere, Category = "Export")
    ENVIncludeObjects IncludeObjectsType;

    /// If true, the exporter will ignore all the hidden actors in game
    // TODO (TT): Should remove ENVIncludeObjects::AllVisibleObjects and keep this flag option independently from the InludeObjectsType
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

    // TODO: Should move this to the data feature extractors
    UPROPERTY(EditAnywhere, Category = "Export")
    FFloatInterval DistanceScaleRange;
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVSceneCapturerSettings
{
    GENERATED_BODY();

public:
    FNVSceneCapturerSettings();

    float GetFOVAngle() const;
    void RandomizeSettings();
    FCameraIntrinsicSettings GetCameraIntrinsicSettings() const;

#if WITH_EDITORONLY_DATA
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif //WITH_EDITORONLY_DATA

public: // Editor properties
    UPROPERTY(VisibleAnywhere, Category = CapturerSettings)
    ENVImageFormat ExportImageFormat;

    UPROPERTY(Transient, VisibleAnywhere, Category = CapturerSettings, meta = (EditCondition = "!bUseExplicitCameraIntrinsic"))
    float FOVAngle;

    UPROPERTY(EditAnywhere, Category = CapturerSettings, meta = (DisplayName = "Field of View", UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0", EditCondition = "!bUseExplicitCameraIntrinsic"))
    FFloatInterval FOVAngleRange;

    UPROPERTY(EditAnywhere, Category = CapturerSettings)
    FNVImageSize CapturedImageSize;

    /// NOTE: Only advance user who need to change this
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = CapturerSettings)
    int32 MaxSaveImageAsyncTaskCount;

    UPROPERTY(EditAnywhere, Category = CapturerSettings)
    bool bUseExplicitCameraIntrinsic;

    UPROPERTY(EditAnywhere, Category = CapturerSettings, meta = (EditCondition=bUseExplicitCameraIntrinsic))
    FCameraIntrinsicSettings CameraIntrinsicSettings;

    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = CapturerSettings)
    FMatrix CameraIntrinsicMatrix;

    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = CapturerSettings)
    FMatrix CameraProjectionMatrix;
};

UENUM()
enum class ENVSceneCapturerState : uint8
{
    NotActive                UMETA(DisplayName = "NotActive. The capturer is not active."),
    Active                   UMETA(DisplayName = "Active.    The capturer is active. but not started."),
    Running                  UMETA(DisplayName = "Running.   The capturer is running/exporting."),
    Paused                   UMETA(DisplayName = "Paused.    The capturer is paused, can be resumed."),
    Completed                UMETA(DisplayName = "Completed. The capturer finished exporting a batch."),

    NVSceneCapturerState_MAX UMETA(Hidden)
};

/// Name of parameters that used in message format FText
namespace NVSceneCapturerStateString
{
    extern const FString NotStarted_Str;
    extern const FString Running_Str;
    extern const FString Paused_Str;
    extern const FString Completed_Str;

    NVSCENECAPTURER_API FString ConvertExporterStateToString(ENVSceneCapturerState ExporterState);
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVFrameCounter
{
    GENERATED_BODY()

public:
    FNVFrameCounter();

    float GetFPS() const
    {
        return CachedFPS;
    }
    int32 GetTotalFrameCount() const
    {
        return TotalFrameCount;
    }

    void Reset();
    void IncreaseFrameCount(int AdditionalFrameCount = 1);
    void SetFrameCount(int NewFrameCount);
    void AddFrameDuration(float NewDuration, bool bIncreaseFrame = false);

protected:
    void UpdateFPS();

    int32 TotalFrameCount;
    float CachedFPS;
    int FPSAccumulatedFrames;
    float FPSAccumulatedDuration;
};

namespace NVSceneCapturerUtils
{
    extern const FMatrix UE4ToOpenCVMatrix;
    extern const FMatrix OpenCVToUE4Matrix;
    extern const FMatrix ObjToUE4Matrix;
	extern const uint32 MaxVertexColorID;


    //================ Helper functions ================
    NVSCENECAPTURER_API FQuat ConvertQuaternionToOpenCVCoordinateSystem(const FQuat& InQuat);
    NVSCENECAPTURER_API FVector ConvertDimensionToOpenCVCoordinateSystem(const FVector& InDimension);

    NVSCENECAPTURER_API FString GetGameDataOutputFolder();
    NVSCENECAPTURER_API FString GetDefaultDataOutputFolder();
    NVSCENECAPTURER_API FString GetOutputFileFullPath(uint32 Index, FString Extension, const FString& Subfolder, FString Filename = TEXT(""), uint8 ZeroPad = 6);

    NVSCENECAPTURER_API TSharedPtr<FJsonValue> CustomPropertyToJsonValueFunc(UProperty* PropertyType, const void* Value);

    /// Convert a property to json value use our own shorthand format, e.g: for Vector3: [x, y, z] instead of {"x": x, "y": y, "z": z}
    NVSCENECAPTURER_API TSharedPtr<FJsonValue> CustomPropertyToJsonValueFunc(UProperty* PropertyType, const void* Value);

    template<typename InStructType> TSharedPtr<FJsonObject> UStructToJsonObject(const InStructType& InStructData, int64 CheckFlags=0, int64 SkipFlags=0)
    {
        FJsonObjectConverter::CustomExportCallback CustomPropertyToJsonValue;
        if (!CustomPropertyToJsonValue.IsBound())
        {
            CustomPropertyToJsonValue.BindStatic(&NVSceneCapturerUtils::CustomPropertyToJsonValueFunc);
        }

        TSharedPtr<FJsonObject> JsonObj = FJsonObjectConverter::UStructToJsonObject(InStructData, CheckFlags, SkipFlags, &CustomPropertyToJsonValue);
        return JsonObj;
    }

    NVSCENECAPTURER_API bool SaveJsonObjectToFile(const TSharedPtr<FJsonObject>& JsonObjData, const FString& Filename);

    NVSCENECAPTURER_API FString GetExportImageExtension(EImageFormat ImageFormat);

    NVSCENECAPTURER_API UMeshComponent* GetFirstValidMeshComponent(const AActor* CheckActor);
    /// Get the list of vertexes from the mesh's simple collision
    NVSCENECAPTURER_API TArray<FVector> GetSimpleCollisionVertexes(const class UMeshComponent* MeshComp);

    /// Get the number of bit in each pixel
    NVSCENECAPTURER_API uint8 GetBitCountPerChannel(EPixelFormat PixelFormat);
    /// Get the number of channel in each pixel
    NVSCENECAPTURER_API uint8 GetColorChannelCount(EPixelFormat PixelFormat);
    NVSCENECAPTURER_API uint8 GetPixelByteSize(EPixelFormat PixelFormat);

    NVSCENECAPTURER_API FColor ConvertByteIndexToColor(uint8 Index);
    NVSCENECAPTURER_API FColor ConvertInt32ToRGB(uint32 Value);
	NVSCENECAPTURER_API FColor ConvertInt32ToRGBA(uint32 Value);
	NVSCENECAPTURER_API FColor ConvertInt32ToVertexColor(uint32 Value);

    /// Set the vertexes of the meshes in an actor to use the same color
    NVSCENECAPTURER_API void SetMeshVertexColor(AActor* MeshOwnerActor, const FColor& VertexColor);
    NVSCENECAPTURER_API void ClearMeshVertexColor(AActor* MeshOwnerActor);

    /// Calculate the spherical coordinate of a target compare to an origin point.
    /// Ref: https://en.wikipedia.org/wiki/Azimuth
    /// NOTE: Assume the horizontal plane is the XY plane in the world coordinate
    NVSCENECAPTURER_API void CalculateSphericalCoordinate(const FVector& TargetLocation, const FVector& SourceLocation, const FVector& ForwardDirection,
            float& OutTargetAzimuthAngle, float& OutTargetAltitudeAngle);

    //================ Calculate 3D bounding box ================
    /// Get the mesh's bound cuboid using axis-aligned bounding box
    NVSCENECAPTURER_API FNVCuboidData GetMeshCuboid_AABB(const class UMeshComponent* MeshComp);

    /// Get the mesh's bound cuboid using object-oriented bounding box
    /// NOTE: This 'simple' approach calculate the mesh's AABB in its local space then transform it using the component's local-to-world transform
    /// @param bInWorldSpace if true, the result is location in world space, otherwise it's in the mesh's local space
    /// @param bCheckMeshCollision If true, the function check the mesh's collision vertices when finding the bounding box instead of just checking all of its vertices
    NVSCENECAPTURER_API FNVCuboidData GetMeshCuboid_OOBB_Simple(const class UMeshComponent* MeshComp, bool bInWorldSpace = true, bool bCheckMeshCollision = true);

    /// Get the mesh's bound cuboid using object-oriented bounding box
    /// NOTE: This 'complex' approach calculate the OOBB base on 'Principal Component Analysis':
    /// http://www.inf.fu-berlin.de/users/rote/Papers/pdf/On+the+bounding+boxes+obtained+by+principal+component+analysis.pdf
    NVSCENECAPTURER_API FNVCuboidData GetMeshCuboid_OOBB_Complex(const class UMeshComponent* MeshComp);

    NVSCENECAPTURER_API FNVCuboidData GetActorCuboid_AABB(const AActor* CheckActor);
    NVSCENECAPTURER_API FNVCuboidData GetActorCuboid_OOBB_Simple(const AActor* CheckActor, bool bCheckMeshCollision = true);
    NVSCENECAPTURER_API FNVCuboidData GetActorCuboid_OOBB_Complex(const AActor* CheckActor);
};