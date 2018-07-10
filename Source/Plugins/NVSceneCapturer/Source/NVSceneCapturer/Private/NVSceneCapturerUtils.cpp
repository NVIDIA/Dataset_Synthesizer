/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "IImageWrapper.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "JsonObjectConverter.h"
#include "Json.h"
#include "MeshVertexPainter/MeshVertexPainter.h"
#include "SkeletalMeshRenderData.h"
#include "SkeletalMeshLODRenderData.h"

//================================== FNVSceneExporterConfig ==================================
FNVSceneExporterConfig::FNVSceneExporterConfig()
{
    bExportObjectData = true;
    bExportScreenShot = true;

    IncludeObjectsType = ENVIncludeObjects::AllTaggedObjects;
    bIgnoreHiddenActor = true;
    BoundsType = ENVBoundsGenerationType::VE_OOBB;
    BoundingBox2dType = ENVBoundBox2dGenerationType::FromMeshBodyCollision;
    bOutputEvenIfNoObjectsAreInView = true;

    DistanceScaleRange = FFloatInterval(100.f, 1000.f);
}

//================================== NVSceneCapturerStateString ==================================
namespace NVSceneCapturerStateString
{
    const FString NotStarted_Str = TEXT("Idle");
    const FString Running_Str = TEXT("Running");
    const FString Paused_Str = TEXT("Paused");
    const FString Completed_Str = TEXT("Completed");

    FString ConvertExporterStateToString(ENVSceneCapturerState ExporterState)
    {
        switch (ExporterState)
        {
            case ENVSceneCapturerState::Active:
                return NotStarted_Str;
            case ENVSceneCapturerState::Running:
                return Running_Str;
            case ENVSceneCapturerState::Paused:
                return Paused_Str;
            case ENVSceneCapturerState::Completed:
                return Completed_Str;
            default:
                return TEXT("");
        }
    }
}

//================================== FNVCuboidData ==================================
uint8 FNVCuboidData::TotalVertexesCount = (uint8)ENVCuboidVertexType::CuboidVertexType_MAX;

FNVCuboidData::FNVCuboidData()
{
    for (int i = 0; i < TotalVertexesCount; i++)
    {
        Vertexes[i] = FVector::ZeroVector;
    }

    Center = FVector::ZeroVector;
    Rotation = FQuat::Identity;
    LocalBox = FBox();
}

FNVCuboidData::FNVCuboidData(const FBox& AABB)
{
    BuildFromAABB(AABB);
}

FNVCuboidData::FNVCuboidData(const FBox& LocalBox, const FTransform& LocalTransform)
{
    BuildFromOOBB(LocalBox, LocalTransform);
}

void FNVCuboidData::BuildFromAABB(const FBox& AABB)
{
    LocalBox = AABB;

    // Front == +X, Rear == -X, Left == -Y, Right == +Y, Top == +Z, Bottom == -Z
    Vertexes[(uint8)ENVCuboidVertexType::FrontTopLeft] = FVector(LocalBox.Max.X, LocalBox.Min.Y, LocalBox.Max.Z);
    Vertexes[(uint8)ENVCuboidVertexType::FrontTopRight] = FVector(LocalBox.Max.X, LocalBox.Max.Y, LocalBox.Max.Z);
    Vertexes[(uint8)ENVCuboidVertexType::FrontBottomRight] = FVector(LocalBox.Max.X, LocalBox.Max.Y, LocalBox.Min.Z);
    Vertexes[(uint8)ENVCuboidVertexType::FrontBottomLeft] = FVector(LocalBox.Max.X, LocalBox.Min.Y, LocalBox.Min.Z);
    Vertexes[(uint8)ENVCuboidVertexType::RearTopLeft] = FVector(LocalBox.Min.X, LocalBox.Min.Y, LocalBox.Max.Z);
    Vertexes[(uint8)ENVCuboidVertexType::RearTopRight] = FVector(LocalBox.Min.X, LocalBox.Max.Y, LocalBox.Max.Z);
    Vertexes[(uint8)ENVCuboidVertexType::RearBottomRight] = FVector(LocalBox.Min.X, LocalBox.Max.Y, LocalBox.Min.Z);
    Vertexes[(uint8)ENVCuboidVertexType::RearBottomLeft] = FVector(LocalBox.Min.X, LocalBox.Min.Y, LocalBox.Min.Z);

    Center = AABB.GetCenter();
    Rotation = FQuat::Identity;
}

void FNVCuboidData::BuildFromOOBB(const FBox& OOBB, const FTransform& LocalTransform)
{
    // TODO: May need to scale up using LocalTransform.GetScale3D
    BuildFromAABB(OOBB);

    // Convert all the vertexes to the world space
    for (uint8 i = 0; i < TotalVertexesCount; i++)
    {
        Vertexes[i] = LocalTransform.TransformPosition(Vertexes[i]);
    }

    Center = LocalTransform.TransformPosition(Center);
    Rotation = LocalTransform.GetRotation();
}

//================================== ENVImageFormat ==================================
EImageFormat ConvertExportFormatToImageFormat(ENVImageFormat ExportFormat)
{
    switch (ExportFormat)
    {
        case ENVImageFormat::BMP:
            return EImageFormat::BMP;
        case ENVImageFormat::JPEG:
            return EImageFormat::JPEG;
        case ENVImageFormat::GrayscaleJPEG:
            return EImageFormat::GrayscaleJPEG;
        case ENVImageFormat::PNG:
            return EImageFormat::PNG;
        default:
            return EImageFormat::BMP;
    }
}

FString GetExportImageExtension(ENVImageFormat ExportFormat)
{
    static const FString BMP_Extension = TEXT(".bmp");
    static const FString JPEG_Extension = TEXT(".jpg");
    static const FString PNG_Extension = TEXT(".png");

    switch (ExportFormat)
    {
        case ENVImageFormat::BMP:
            return BMP_Extension;
        case ENVImageFormat::JPEG:
        case ENVImageFormat::GrayscaleJPEG:
            return JPEG_Extension;
        case ENVImageFormat::PNG:
            return PNG_Extension;
        default:
            return BMP_Extension;
    }
}

//================================== ENVCapturedPixelFormat ==================================
ETextureRenderTargetFormat ConvertCapturedFormatToRenderTargetFormat(ENVCapturedPixelFormat PixelFormat)
{
	switch (PixelFormat)
	{
	case ENVCapturedPixelFormat::R8:
		return ETextureRenderTargetFormat::RTF_R8;
	case ENVCapturedPixelFormat::R16f:
		return ETextureRenderTargetFormat::RTF_R16f;
	case ENVCapturedPixelFormat::R32f:
		return ETextureRenderTargetFormat::RTF_R32f;
	case ENVCapturedPixelFormat::RGBA8:
	default:
		return ETextureRenderTargetFormat::RTF_RGBA8;
		break;
	}
}

//================================ FNVFrameCounter ================================
FNVFrameCounter::FNVFrameCounter()
{
    Reset();
}

void FNVFrameCounter::Reset()
{
    TotalFrameCount = 0;
    CachedFPS = 0.f;
    FPSAccumulatedFrames = 0;
    FPSAccumulatedDuration = 0;
}

void FNVFrameCounter::IncreaseFrameCount(int AdditionalFrameCount/*= 1*/)
{
    TotalFrameCount += AdditionalFrameCount;

    FPSAccumulatedFrames += AdditionalFrameCount;

    UpdateFPS();
}

void FNVFrameCounter::SetFrameCount(int NewFrameCount)
{
    int FrameDelta = NewFrameCount - TotalFrameCount;
    if (FrameDelta > 0)
    {
        IncreaseFrameCount(FrameDelta);
    }
}

void FNVFrameCounter::AddFrameDuration(float NewDuration, bool bIncreaseFrame)
{
    FPSAccumulatedDuration += NewDuration;
    if (bIncreaseFrame)
    {
        IncreaseFrameCount();
    }
    UpdateFPS();

    if (FPSAccumulatedDuration >= 1.f)
    {
        FPSAccumulatedDuration = FMath::Frac(FPSAccumulatedDuration);
        FPSAccumulatedFrames = (FPSAccumulatedDuration > 0.f) ? 1 : 0;
    }
}

void FNVFrameCounter::UpdateFPS()
{
    if (FPSAccumulatedDuration >= 1.f)
    {
        CachedFPS = FPSAccumulatedFrames / FPSAccumulatedDuration;
    }
}

//=========================== FNVSceneCapturerSettings ===========================
FNVSceneCapturerSettings::FNVSceneCapturerSettings()
{
    FOVAngle = -1.f;
    FOVAngleRange = FFloatInterval(90.f, 90.f);

    CapturedImageSize = FNVImageSize(512, 512);
    ExportImageFormat = ENVImageFormat::PNG;
    MaxSaveImageAsyncTaskCount = 100;
    bUseExplicitCameraIntrinsic = false;
}

float FNVSceneCapturerSettings::GetFOVAngle() const
{
    return FOVAngle;
}

void FNVSceneCapturerSettings::RandomizeSettings()
{
    FOVAngle = FMath::RandRange(FOVAngleRange.Min, FOVAngleRange.Max);
}

FCameraIntrinsicSettings FNVSceneCapturerSettings::GetCameraIntrinsicSettings() const
{
    return bUseExplicitCameraIntrinsic ?
             CameraIntrinsicSettings :
             FCameraIntrinsicSettings(CapturedImageSize.Width, CapturedImageSize.Height, GetFOVAngle());
}

#if WITH_EDITORONLY_DATA
void FNVSceneCapturerSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        if (bUseExplicitCameraIntrinsic)
        {
            CameraIntrinsicMatrix = CameraIntrinsicSettings.GetIntrinsicMatrix();
            CameraProjectionMatrix = CameraIntrinsicSettings.GetProjectionMatrix();
        }
    }
}
#endif //WITH_EDITORONLY_DATA

//================================== Helper functions ==================================
namespace NVSceneCapturerUtils
{
    // Transform matrix from the UE4 to OpenCV coordination system
    const FMatrix UE4ToOpenCVMatrix = FMatrix(
                                          FPlane(0.f, 0.f, 1.f, 0.f), // X - forward in UE4 - Z in OpenCV
                                          FPlane(1.f, 0.f, 0.f, 0.f), // Y - right in UE4 - X in OpenCV
                                          FPlane(0.f, -1.f, 0.f, 0.f),  // Z - up in UE4 - -Y in OpenCV
                                          FPlane(0.f, 0.f, 0.f, 1.f)  // W
                                      );

    // Transform matrix from the OpenCV to UE4 coordination system
    const FMatrix OpenCVToUE4Matrix = FMatrix(
                                          FVector(0.f, 1.f, 0.f), // X - right in OpenCV - Y in UE4
                                          FVector(0.f, 0.f, -1.f), // Y - down in OpenCV - -Z in UE4
                                          FVector(1.f, 0.f, 0.f), // Z - forward in OpenCV - X in UE4
                                          FVector(0.f, 0.f, 0.f)  // W
                                      );

    const FMatrix ObjToUE4Matrix = FMatrix(
                                       FVector(0.f, 1.f, 0.f), // X - right in OpenGL - Y in UE4
                                       FVector(0.f, 0.f, 1.f), // Y - up in OpenGL - Z in UE4
                                       FVector(-1.f, 0.f, 0.f), // Z - backward in OpenCV - -X in UE4
                                       FVector(0.f, 0.f, 0.f)  // W
                                   );

	const uint32 MaxVertexColorID = (1 << 24) - 1;

    FQuat ConvertQuaternionToOpenCVCoordinateSystem(const FQuat& InQuat)
    {
        FQuat OutQuat;
        OutQuat.X = -InQuat.Y;
        OutQuat.Y = InQuat.Z;
        OutQuat.Z = -InQuat.X;
        OutQuat.W = InQuat.W;

        return OutQuat;
    }

    FVector ConvertDimensionToOpenCVCoordinateSystem(const FVector& InDimension)
    {
        FVector OutDimension;
        OutDimension.X = InDimension.Y;
        OutDimension.Y = InDimension.Z;
        OutDimension.Z = InDimension.X;

        return OutDimension;
    }

    float ConvertFloatWithPrecision4(const float InValue)
    {
        const float ModValue = FMath::RoundToDouble(double(InValue) * 10000.f) / 10000.f;
        return ModValue;
    }

    TSharedPtr<FJsonValue> CustomPropertyToJsonValueFunc(UProperty* PropertyType, const void* Value)
    {
        if (!PropertyType || !Value)
        {
            return nullptr;
        }

        const FString& PropCppType = PropertyType->GetCPPType();
        const UStructProperty* StructProperty = Cast<UStructProperty>(PropertyType);
        if (StructProperty)
        {
            UScriptStruct* ScriptStruct = StructProperty->Struct;
            if (PropCppType == TEXT("FMatrix"))
            {
                FMatrix* MatrixValue = (FMatrix*)Value;

                TArray< TSharedPtr<FJsonValue> > MatrixJsonPtrArray;
                for (int i = 0; i < 4; i++)
                {
                    TArray< TSharedPtr<FJsonValue> > MatrixRow;
                    for (int j = 0; j < 4; j++)
                    {
                        const float CellValue = MatrixValue->M[i][j];
                        const float ModValue = ConvertFloatWithPrecision4(CellValue);
                        TSharedPtr<FJsonValue> Elem = MakeShareable(new FJsonValueNumber(ModValue));
                        MatrixRow.Add(Elem);
                    }
                    TSharedPtr<FJsonValue> RowJsonValue = MakeShareable(new FJsonValueArray(MatrixRow));
                    MatrixJsonPtrArray.Add(RowJsonValue);
                }
                TSharedPtr<FJsonValue> MatrixJsonValue = MakeShareable(new FJsonValueArray(MatrixJsonPtrArray));
                return MatrixJsonValue;
            }
            else if (PropCppType == TEXT("FVector"))
            {
                FVector* VectorValue = (FVector*)Value;
                TArray< TSharedPtr<FJsonValue> > VectorJsonPtrArray;

                TSharedPtr<FJsonValue> VectorJsonValue_X = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(VectorValue->X)));
                VectorJsonPtrArray.Add(VectorJsonValue_X);
                TSharedPtr<FJsonValue> VectorJsonValue_Y = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(VectorValue->Y)));
                VectorJsonPtrArray.Add(VectorJsonValue_Y);
                TSharedPtr<FJsonValue> VectorJsonValue_Z = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(VectorValue->Z)));
                VectorJsonPtrArray.Add(VectorJsonValue_Z);

                TSharedPtr<FJsonValue> VectorJsonValue = MakeShareable(new FJsonValueArray(VectorJsonPtrArray));
                return VectorJsonValue;
            }
            else if (PropCppType == TEXT("FVector2D"))
            {
                FVector2D* Vector2dValue = (FVector2D*)Value;
                TArray< TSharedPtr<FJsonValue> > Vector2dJsonPtrArray;

                TSharedPtr<FJsonValue> VectorJsonValue_X = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(Vector2dValue->X)));
                Vector2dJsonPtrArray.Add(VectorJsonValue_X);
                TSharedPtr<FJsonValue> VectorJsonValue_Y = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(Vector2dValue->Y)));
                Vector2dJsonPtrArray.Add(VectorJsonValue_Y);

                TSharedPtr<FJsonValue> Vector2dJsonValue = MakeShareable(new FJsonValueArray(Vector2dJsonPtrArray));
                return Vector2dJsonValue;
            }
            else if (PropCppType == TEXT("FQuat"))
            {
                FQuat* QuatValue = (FQuat*)Value;
                TArray< TSharedPtr<FJsonValue> > QuatJsonPtrArray;

                TSharedPtr<FJsonValue> QuatJsonValue_X = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(QuatValue->X)));
                QuatJsonPtrArray.Add(QuatJsonValue_X);
                TSharedPtr<FJsonValue> QuatJsonValue_Y = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(QuatValue->Y)));
                QuatJsonPtrArray.Add(QuatJsonValue_Y);
                TSharedPtr<FJsonValue> QuatJsonValue_Z = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(QuatValue->Z)));
                QuatJsonPtrArray.Add(QuatJsonValue_Z);
                TSharedPtr<FJsonValue> QuatJsonValue_W = MakeShareable(new FJsonValueNumber(ConvertFloatWithPrecision4(QuatValue->W)));
                QuatJsonPtrArray.Add(QuatJsonValue_W);

                TSharedPtr<FJsonValue> QuatJsonValue = MakeShareable(new FJsonValueArray(QuatJsonPtrArray));
                return QuatJsonValue;
            }
        }
        else
        {
            const UNumericProperty* NumericProperty = Cast<UNumericProperty>(PropertyType);
            if (NumericProperty && NumericProperty->IsFloatingPoint())
            {
                const float FloatValue = *((float*)Value);
                const float ModValue = ConvertFloatWithPrecision4(FloatValue);
                TSharedPtr<FJsonValue> ExportJsonValue = MakeShareable(new FJsonValueNumber(ModValue));
                return ExportJsonValue;
            }
        }

        return nullptr;
    }

    bool SaveJsonObjectToFile(const TSharedPtr<FJsonObject>& JsonObjData, const FString& Filename)
    {
        bool bResult = false;

        if (JsonObjData.IsValid())
        {
            FString OutJsonString = TEXT("");
            auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonString, 0);
            bool bSuccess = FJsonSerializer::Serialize(JsonObjData.ToSharedRef(), JsonWriter);
            JsonWriter->Close();

            if (!FFileHelper::SaveStringToFile(OutJsonString, *Filename))
            {
                UE_LOG(LogNVSceneCapturer, Error, TEXT("Unable to open file for writing.  Check permissions. File is %s"), *Filename);
            }
            else
            {
                bResult = true;
            }
        }

        return bResult;
    }

    FString GetExportImageExtension(EImageFormat ImageFormat)
    {
        static const FString BMP_Extension = TEXT(".bmp");
        static const FString JPEG_Extension = TEXT(".jpg");
        static const FString PNG_Extension = TEXT(".png");

        switch (ImageFormat)
        {
            case EImageFormat::JPEG:
            case EImageFormat::GrayscaleJPEG:
                return JPEG_Extension;
            case EImageFormat::PNG:
                return PNG_Extension;
            case EImageFormat::BMP:
            default:
                return BMP_Extension;
        }
    }

    UMeshComponent* GetFirstValidMeshComponent(const AActor* CheckActor)
    {
        auto ChildActorComponents = CheckActor->GetComponents();
        UMeshComponent* ValidMeshComp = nullptr;
        for (UActorComponent* CheckComp : ChildActorComponents)
        {
            if (CheckComp)
            {
                UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(CheckComp);
                if (StaticMeshComp)
                {
                    if (StaticMeshComp->GetStaticMesh())
                    {
                        ValidMeshComp = StaticMeshComp;
                        break;
                    }
                }
                else
                {
                    USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(CheckComp);
                    if (SkeletalMeshComp)
                    {
                        if (SkeletalMeshComp->SkeletalMesh)
                        {
                            ValidMeshComp = SkeletalMeshComp;
                            break;
                        }
                    }
                }
            }
        }

        return ValidMeshComp;
    }

    TArray<FVector> GetSimpleCollisionVertexes(const class UMeshComponent* MeshComp)
    {
        TArray<FVector> OutVertexes;
        OutVertexes.Reset();

        if (MeshComp)
        {
            const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(MeshComp);
            if (StaticMeshComp)
            {
                const UStaticMesh* CheckMesh = StaticMeshComp->GetStaticMesh();
                if (CheckMesh && CheckMesh->BodySetup)
                {
                    const FTransform& MeshTransform = StaticMeshComp->GetComponentTransform();
                    const FKAggregateGeom& MeshGeom = CheckMesh->BodySetup->AggGeom;

                    for (const FKConvexElem& ConvexElem : MeshGeom.ConvexElems)
                    {
                        for (const FVector& CheckVertex : ConvexElem.VertexData)
                        {
                            const FVector& VertexWorldLocation = MeshTransform.TransformPosition(CheckVertex);
                            OutVertexes.Add(VertexWorldLocation);
                        }
                    }
                }
                // If the mesh doesn't have a collision body setup then just use the mesh's vertexes themselves
                if ((OutVertexes.Num() == 0) && CheckMesh && CheckMesh->RenderData)
                {
                    const FPositionVertexBuffer& MeshVertexBuffer = CheckMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
                    const uint32 VertexesCount = MeshVertexBuffer.GetNumVertices();

                    for (uint32 i = 0; i < VertexesCount; i++)
                    {
                        OutVertexes.Add(MeshVertexBuffer.VertexPosition(i));
                    }
                }
            }
            else
            {
                const USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComp);
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
                                        OutVertexes.Add(VertexWorldLocation);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return OutVertexes;
    }

    uint8 GetBitCountPerChannel(EPixelFormat PixelFormat)
    {
        switch (PixelFormat)
        {
            case EPixelFormat::PF_A8:
            case EPixelFormat::PF_R8_UINT:
            case EPixelFormat::PF_G8:
            case EPixelFormat::PF_R8G8:
            case EPixelFormat::PF_A8R8G8B8:
            case EPixelFormat::PF_B8G8R8A8:
            case EPixelFormat::PF_R8G8B8A8:
                return 8;
            case EPixelFormat::PF_R16F:
            case EPixelFormat::PF_R16_SINT:
            case EPixelFormat::PF_R16_UINT:
            case EPixelFormat::PF_A16B16G16R16:
            case EPixelFormat::PF_ShadowDepth:
                return 16;
            case EPixelFormat::PF_R32_FLOAT:
            case EPixelFormat::PF_R32_SINT:
            case EPixelFormat::PF_R32_UINT:
            case EPixelFormat::PF_G32R32F:
                return 32;
        }

        return 0;
    }

    uint8 GetColorChannelCount(EPixelFormat PixelFormat)
    {
        switch (PixelFormat)
        {
            case EPixelFormat::PF_A8:
            case EPixelFormat::PF_R8_UINT:
            case EPixelFormat::PF_G8:
            case EPixelFormat::PF_R16F:
            case EPixelFormat::PF_R16_SINT:
            case EPixelFormat::PF_R16_UINT:
            case EPixelFormat::PF_R32_FLOAT:
            case EPixelFormat::PF_R32_SINT:
            case EPixelFormat::PF_R32_UINT:
            case EPixelFormat::PF_ShadowDepth:
                return 1;
            case EPixelFormat::PF_R8G8:
            case EPixelFormat::PF_G32R32F:
                return 2;
            case EPixelFormat::PF_A8R8G8B8:
            case EPixelFormat::PF_B8G8R8A8:
            case EPixelFormat::PF_R8G8B8A8:
            case EPixelFormat::PF_A16B16G16R16:
                return 4;
        }

        return 0;
    }

    uint8 GetPixelByteSize(EPixelFormat PixelFormat)
    {
        // NOTE: This calculation only work if each channel of the pixel have the same size
        return (GetBitCountPerChannel(PixelFormat) * GetColorChannelCount(PixelFormat)) / 8;
    }

    FColor ConvertByteIndexToColor(uint8 ColorIndex)
    {
        FColor OutColor;
        OutColor.R = FMath::RoundToInt(((ColorIndex >> 5) / 7.0) * 255.f);
        OutColor.G = FMath::RoundToInt((((ColorIndex >> 2) & 7) / 7.0) * 255.f);
        OutColor.B = FMath::RoundToInt(((ColorIndex & 3) / 3.0) * 255.f);
        OutColor.A = 255;
        return OutColor;
    }

    FColor ConvertInt32ToRGB(uint32 Value)
    {
        FColor OutColor;
        // 32 bit RGB
        // R: 11 bit
        // G: 11 bit
        // B: 10 bit
        OutColor.R = FMath::RoundToInt(((Value >> 21) / float((1 << 11) - 1)) * 255.f);
        OutColor.G = FMath::RoundToInt(((Value >> 10) & ((1 << 11) - 1)) / float((1 << 11) - 1) * 255.f);
        OutColor.B = FMath::RoundToInt((Value & ((1 << 10) - 1)) / float((1 << 10) - 1) * 255.f);

        OutColor.A = 255;

        return OutColor;
    }

    FColor ConvertInt32ToRGBA(uint32 Value)
    {
        FColor OutColor;
        // From left to right, each 8 bit is used for each channel: R, G, B and A
        OutColor.R = (Value >> 24);
        OutColor.G = (Value >> 16) & 255;
        OutColor.B = (Value >> 8) & 255;
        OutColor.A = Value & 255;

        return OutColor;
    }

	FColor ConvertInt32ToVertexColor(uint32 Value)
	{
		// Ignore the mask if the id pass the max ID value
		ensure(Value <= MaxVertexColorID);
		if (Value > MaxVertexColorID)
		{
			Value = 0;
		}

		FColor OutColor;
		OutColor.R = (Value >> 16) & 255;
		OutColor.G = (Value >> 8) & 255;
		OutColor.B = Value & 255;
		OutColor.A = 255;

		return OutColor;
	}


    void SetMeshVertexColor(AActor* MeshOwnerActor, const FColor& VertexColor)
    {
        if (MeshOwnerActor)
        {
            const FLinearColor& MeshVertexLinearColor = VertexColor.ReinterpretAsLinear();

            TArray<UStaticMeshComponent*> StaticMeshComps;
            MeshOwnerActor->GetComponents<UStaticMeshComponent>(StaticMeshComps, true);
            for (UStaticMeshComponent* CheckStaticMeshComp : StaticMeshComps)
            {
                if (CheckStaticMeshComp)
                {
                    FMeshVertexPainter::PaintVerticesSingleColor(CheckStaticMeshComp, MeshVertexLinearColor, false);
                }
            }

            // Check: USkinnedMeshComponent::SetVertexColorOverride(int32 LODIndex, const TArray<FColor>& VertexColors)
            // Must build a list of color for each vertexes in this LOD in order to change their color
            TArray<FColor> AllVertexInLODColors;
            TArray<USkinnedMeshComponent*> SkinnedMeshComps;
            MeshOwnerActor->GetComponents<USkinnedMeshComponent>(SkinnedMeshComps, true);
            for (USkinnedMeshComponent* CheckSkinnedMeshComp : SkinnedMeshComps)
            {
                if (CheckSkinnedMeshComp)
                {
                    FSkeletalMeshRenderData* Resource = CheckSkinnedMeshComp->GetSkeletalMeshRenderData();

                    for (int i = 0; i < CheckSkinnedMeshComp->LODInfo.Num(); i++)
                    {
                        // NOTE: We need to paint all the vertexes in the skeletal mesh
                        FSkelMeshComponentLODInfo& Info = CheckSkinnedMeshComp->LODInfo[i];
                        FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[i];
                        const int32 ExpectedNumVerts = LODModel.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
                        AllVertexInLODColors.Reset();
                        AllVertexInLODColors.SetNum(ExpectedNumVerts);
                        for (int j = 0; j < ExpectedNumVerts; j++)
                        {
                            AllVertexInLODColors[j] = VertexColor;
                        }

                        CheckSkinnedMeshComp->SetVertexColorOverride(i, AllVertexInLODColors);
                    }
                }
            }
        }
    }

    void ClearMeshVertexColor(AActor* MeshOwnerActor)
    {
        if (MeshOwnerActor)
        {
            TArray<UStaticMeshComponent*> StaticMeshComps;
            MeshOwnerActor->GetComponents<UStaticMeshComponent>(StaticMeshComps, true);
            for (UStaticMeshComponent* CheckStaticMeshComp : StaticMeshComps)
            {
                if (CheckStaticMeshComp)
                {
                    CheckStaticMeshComp->RemoveInstanceVertexColors();
                }
            }

            TArray<USkinnedMeshComponent*> SkinnedMeshComps;
            MeshOwnerActor->GetComponents<USkinnedMeshComponent>(SkinnedMeshComps, true);
            for (USkinnedMeshComponent* CheckSkinnedMeshComp : SkinnedMeshComps)
            {
                if (CheckSkinnedMeshComp)
                {
                    for (int i = 0; i < CheckSkinnedMeshComp->LODInfo.Num(); i++)
                    {
                        CheckSkinnedMeshComp->ClearVertexColorOverride(i);
                    }
                }
            }
        }
    }

    void CalculateSphericalCoordinate(const FVector& TargetLocation, const FVector& SourceLocation, const FVector& SourceForwardDirection, float& OutTargetAzimuthAngle, float& OutTargetAltitudeAngle)
    {
        // NOTE: Assume the XY plane in the world coordinate is the horizontal plane
        static const FVector& HorizontalPlaneNormalVector = FVector::UpVector;

        const FVector& TargetOffset = (TargetLocation - SourceLocation).GetSafeNormal();
        // Get the projection of the target offset on the XY plane
        const FVector& TargetOffsetProjected = FVector::VectorPlaneProject(TargetOffset, HorizontalPlaneNormalVector).GetSafeNormal();
        const float TargetAltitudeAngleCos = FVector::DotProduct(TargetOffset, TargetOffsetProjected);
        OutTargetAltitudeAngle = FMath::RadiansToDegrees(FMath::Acos(TargetAltitudeAngleCos));
        // If the target is lower than the source then the altitude angle is negative
        if (TargetOffset.Z < 0.f)
        {
            OutTargetAltitudeAngle = -OutTargetAltitudeAngle;
        }

        const FVector& SourceForwardDir = FVector::VectorPlaneProject(SourceForwardDirection, HorizontalPlaneNormalVector).GetSafeNormal();
        const FVector& TargetOffsetForwardCross = FVector::CrossProduct(TargetOffsetProjected, SourceForwardDir);
        const float TargetAzimuthAngleSin = TargetOffsetForwardCross.Size();
        OutTargetAzimuthAngle = FMath::RadiansToDegrees(FMath::Asin(TargetAzimuthAngleSin));
        // Since both forward vector and the target offset vector are on the same plane (XY), the cross product will go along the Z axis
        // If the cross product vector have negative Z => the target is on the left of the forward direction
        if (TargetOffsetForwardCross.Z < 0.f)
        {
            OutTargetAzimuthAngle = -OutTargetAzimuthAngle;
        }
    }

    //================================== Calculate bounding box ==================================
    // Get the mesh's bound cuboid using axis-aligned bounding box
    FNVCuboidData GetMeshCuboid_AABB(const class UMeshComponent* MeshComp)
    {
        FNVCuboidData MeshAACuboid;

        if (MeshComp)
        {
            FBox AABB = MeshComp->Bounds.GetBox();
            MeshAACuboid.BuildFromAABB(AABB);
        }

        return MeshAACuboid;
    }

    // Get the mesh's object-oriented bounding box cuboid
    FNVCuboidData GetMeshCuboid_OOBB_Simple(const class UMeshComponent* MeshComp, bool bInWorldSpace/*= true*/, bool bCheckMeshCollision/*= true*/)
    {
        FNVCuboidData MeshOOCuboid;

        if (MeshComp)
        {
            // The 3d bounding box of the mesh in its local component space
            FBox LocalOOBB(EForceInit::ForceInitToZero);

            const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(MeshComp);
            if (StaticMeshComp)
            {
                UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
                if (StaticMesh)
                {
                    // If the static mesh have body setup with its convex collision then use it
                    // NOTE: This approach is a lot cheaper than checking all of the mesh's vertexes but the convex body set up may not as tight as the raw vertexes list itself
                    if (bCheckMeshCollision && StaticMesh->BodySetup)
                    {
                        const FKAggregateGeom& MeshGeom = StaticMesh->BodySetup->AggGeom;
                        for (const FKConvexElem& ConvexElem : MeshGeom.ConvexElems)
                        {
                            for (const FVector& CheckVertex : ConvexElem.VertexData)
                            {
                                LocalOOBB += CheckVertex;
                            }
                        }
                    }

                    static const int MAX_VERTEXES_COUNT_MESH_FOR_BOUNDING_BOX = 1000;

                    bool bHaveBodyVertexData = (LocalOOBB.IsValid != 0);
                    // Otherwise use the mesh's raw triangle vertexes
                    // NOTE: This approach is way slower than using the simplified collision especially with complicated mesh with a lot of vertexes
                    // Always fallback to use the Render Data if the mesh doesn't have correct body setup
                    if (!bHaveBodyVertexData && StaticMesh->RenderData)
                    {
                        const FPositionVertexBuffer& MeshVertexBuffer = StaticMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
                        const uint32 VertexesCount = MeshVertexBuffer.GetNumVertices();

                        // Warn user when the mesh have a lot of vertexes since it can slow down the bounding box calculation
                        if (VertexesCount > MAX_VERTEXES_COUNT_MESH_FOR_BOUNDING_BOX)
                        {
                            UE_LOG(LogNVSceneCapturer, VeryVerbose, TEXT("Mesh '%s' have '%d' vertexes => calculating the bounding box for it using its render data will be slow => Recommend generate simple collision for this mesh."),
                                   *StaticMesh->GetName(), VertexesCount);
                        }

                        for (uint32 i = 0; i < VertexesCount; i++)
                        {
                            const FVector& VertexPosition = MeshVertexBuffer.VertexPosition(i);
                            LocalOOBB += VertexPosition;
                        }
                    }
                }
            }
            else
            {
                const USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComp);
                const USkeletalMesh* SkeletalMesh = SkeletalMeshComp ? SkeletalMeshComp->SkeletalMesh : nullptr;
                if (SkeletalMesh)
                {
                    UPhysicsAsset* MeshPhysicsAsset = SkeletalMesh->PhysicsAsset;
                    if (MeshPhysicsAsset)
                    {
                        LocalOOBB = MeshPhysicsAsset->CalcAABB(SkeletalMeshComp, FTransform::Identity);
                    }
                    else
                    {
                        // TODO: Check the skeletal's vertexes location using its render data similar to how we did with the static mesh?
                    }
                }
            }

            if (LocalOOBB.IsValid)
            {
                const FTransform& MeshCompTransform = bInWorldSpace ? MeshComp->GetComponentTransform() : FTransform::Identity;

                MeshOOCuboid.BuildFromOOBB(LocalOOBB, MeshCompTransform);
            }
        }

        return MeshOOCuboid;
    }

    FNVCuboidData GetMeshCuboid_OOBB_Complex(const class UMeshComponent* MeshComp)
    {
        FNVCuboidData MeshOOCuboid;

        if (MeshComp)
        {
            // List of the vertexes used to calculate the OOBB
            TArray<FVector> BoundingVertexes;
            BoundingVertexes.Reset();

            const FTransform& MeshTransform = MeshComp->GetComponentTransform();
            const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(MeshComp);
            if (StaticMeshComp)
            {
                const UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
                if (StaticMesh)
                {
                    // If the static mesh have body setup with its convex collision then use it
                    if (StaticMesh->BodySetup)
                    {
                        const FKAggregateGeom& MeshGeom = StaticMesh->BodySetup->AggGeom;
                        for (const FKConvexElem& ConvexElem : MeshGeom.ConvexElems)
                        {
                            BoundingVertexes.Append(ConvexElem.VertexData);
                        }
                    }
                    // Otherwise use the mesh's raw triangle vertexes
                    else if (StaticMesh->RenderData)
                    {
                        const FPositionVertexBuffer& MeshVertexBuffer = StaticMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
                        const uint32 VertexesCount = MeshVertexBuffer.GetNumVertices();
                        BoundingVertexes.Reserve(BoundingVertexes.Num() + VertexesCount);
                        for (uint32 i = 0; i < VertexesCount; i++)
                        {
                            const FVector& VertexPosition = MeshVertexBuffer.VertexPosition(i);
                            BoundingVertexes.Add(VertexPosition);
                        }
                    }
                }
            }
            else
            {
                const USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComp);
                const USkeletalMesh* SkeletalMesh = SkeletalMeshComp ? SkeletalMeshComp->SkeletalMesh : nullptr;
                if (SkeletalMesh)
                {
                    UPhysicsAsset* MeshPhysicsAsset = SkeletalMesh->PhysicsAsset;
                    if (MeshPhysicsAsset)
                    {
                        // TODO: Collect all the vertexes from the skeletal's body setups
                        //MeshPhysicsAsset->SkeletalBodySetups
                    }
                    else
                    {
                        // TODO: Check the skeletal's vertexes location using its render data similar to how we did with the static mesh?
                    }
                }
            }

            if (BoundingVertexes.Num() > 0)
            {
                // TODO:
                // NOTE: This 'complex' approach calculate the OOBB base on 'Principal Component Analysis':
                // http://www.inf.fu-berlin.de/users/rote/Papers/pdf/On+the+bounding+boxes+obtained+by+principal+component+analysis.pdf
                // We may not want this approach since it doesn't maintain the direction of the cuboid (front-face may be different)
            }
        }

        return MeshOOCuboid;
    }

    FNVCuboidData GetActorCuboid_AABB(const AActor* CheckActor)
    {
        FNVCuboidData ActorCuboidAABB;

        if (CheckActor)
        {
            const FBox& ActorBounds = CheckActor->GetComponentsBoundingBox(true); //true means all non-colliding subcomponents
            ActorCuboidAABB.BuildFromAABB(ActorBounds);
        }

        return ActorCuboidAABB;
    }

    FNVCuboidData GetActorCuboid_OOBB_Simple(const AActor* CheckActor, bool bCheckMeshCollision/*= true*/)
    {
        FNVCuboidData ActorCuboidOOBB;

        if (CheckActor)
        {
            // TODO: Right now we only calculate the OOBB from the first mesh component in of the actor
            // Need to merge all of the OOBB of all the child mesh components together
            const UMeshComponent* ValidMeshComp = GetFirstValidMeshComponent(CheckActor);
            ActorCuboidOOBB = GetMeshCuboid_OOBB_Simple(ValidMeshComp, true, bCheckMeshCollision);
        }

        return ActorCuboidOOBB;
    }

    FNVCuboidData GetActorCuboid_OOBB_Complex(const AActor* CheckActor)
    {
        FNVCuboidData ActorCuboidOOBB;

        if (CheckActor)
        {
            // NOTE: Right now we only calculate the OOBB from the first mesh component in of the actor
            // Need to merge all of the OOBB of all the child mesh components together
            const UMeshComponent* ValidMeshComp = GetFirstValidMeshComponent(CheckActor);
            ActorCuboidOOBB = GetMeshCuboid_OOBB_Complex(ValidMeshComp);
        }

        return ActorCuboidOOBB;
    }
}
