/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneDataHandler.h"
#include "NVImageExporter.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneCapturerActor.h"
#include "NVAnnotatedActor.h"
#include "NVSceneManager.h"
#include "Engine.h"
#include "JsonObjectConverter.h"
#if WITH_EDITOR
#include "Factories/FbxAssetImportData.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

//================================== UNVSceneDataExporter ==================================//
DEFINE_LOG_CATEGORY(LogNVSceneDataHandler);
const FString UNVSceneDataExporter::DefaultDataOutputFolder = TEXT("NVCapturedData/");

UNVSceneDataExporter::UNVSceneDataExporter() : Super(),
    ImageWrapperModule(&FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper")))
{
    RootCapturedDirectoryPath.Path = DefaultDataOutputFolder;
    DirectoryConflictHandleType = ENVCaptureDirectoryConflictHandleType::CleanDirectory;
    CustomDirectoryName = TEXT("Custom");
    bUseMapNameForCapturedDirectory = true;
    bAutoOpenExportedDirectory = false;
    MaxSaveImageAsyncCount = 100;
}

bool UNVSceneDataExporter::CanHandleMoreData() const
{
    return ImageExporterThread &&
            ((MaxSaveImageAsyncCount <= 0) || (ImageExporterThread->GetPendingImagesCount() <= MaxSaveImageAsyncCount / 2));
}

bool UNVSceneDataExporter::IsHandlingData() const
{
    return ImageExporterThread && ImageExporterThread->IsExportingImage();
}

bool UNVSceneDataExporter::HandleScenePixelsData(const FNVTexturePixelData& CapturedPixelData, UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor, UNVSceneCapturerViewpointComponent* CapturedViewpoint, int32 FrameIndex)
{
    bool bResult = false;
    if (ImageExporterThread && CapturedFeatureExtractor && CapturedViewpoint)
    {
		ENVImageFormat ExportImageFormat = ENVImageFormat::PNG;

        const FString NewExportFilePath = GetExportFilePath(CapturedFeatureExtractor, CapturedViewpoint, FrameIndex, GetExportImageExtension(ExportImageFormat));
        ImageExporterThread->ExportImage(CapturedPixelData, NewExportFilePath, ExportImageFormat);
        bResult = true;
    }
    return bResult;
}

bool UNVSceneDataExporter::HandleSceneAnnotationData(const TSharedPtr<FJsonObject>& CapturedData, class UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor, class UNVSceneCapturerViewpointComponent* CapturedViewpoint, int32 FrameIndex)
{
    bool bResult = false;
    if (CapturedFeatureExtractor && CapturedViewpoint)
    {
        static const FString JsonExtension = TEXT(".json");

        const FString NewExportFilePath = GetExportFilePath(CapturedFeatureExtractor, CapturedViewpoint, FrameIndex, JsonExtension);
        return NVSceneCapturerUtils::SaveJsonObjectToFile(CapturedData, NewExportFilePath);
        bResult = true;
    }
    return bResult;
}

void UNVSceneDataExporter::OnStartCapturingSceneData()
{
    if (!ImageExporterThread.IsValid())
    {
        ImageExporterThread = TUniquePtr<FNVImageExporter_Thread>(new FNVImageExporter_Thread(ImageWrapperModule));
    }

    // Prepare the output directory before capturing
    FullOutputDirectoryPath = GetConfiguredOutputDirectoryPath();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.DirectoryExists(*FullOutputDirectoryPath))
    {
        switch (DirectoryConflictHandleType)
        {
            case ENVCaptureDirectoryConflictHandleType::CleanDirectory:
            {
                PlatformFile.DeleteDirectoryRecursively(*FullOutputDirectoryPath);
                break;
            }
            case ENVCaptureDirectoryConflictHandleType::CreateNewDirectoryWithTimestampPostfix:
            {
                // Create new directory with the current time as postfix
                const FString ConfigOutputDirName = GetConfiguredOutputDirectoryName();
                const FString CurrentTimeStr = FDateTime::Now().ToString();
                const FString NewOutputDirName = FString::Printf(TEXT("%s_%s"), *ConfigOutputDirName, *CurrentTimeStr);
                FullOutputDirectoryPath = FPaths::Combine(GetRootCaptureDirectoryPath(), NewOutputDirName);
                break;
            }
            case ENVCaptureDirectoryConflictHandleType::OverwriteExistingFiles:
            default:
                // Don't need to do anything
                break;
        }
    }

    ExportCapturerSettings();
}

void UNVSceneDataExporter::ExportCapturerSettings()
{
    ANVSceneCapturerActor* OwnerSceneCapturer = Cast<ANVSceneCapturerActor>(GetOuter());
    if (!OwnerSceneCapturer)
    {
        return;
    }

    // TODO: Right now we only export the capturer's settings data to the file
    // but we need to also export the map set up and training objects' settings later too

    FNVCapturerSettingExportData SettingExportData;
    SettingExportData.CapturersSettings = OwnerSceneCapturer->CapturerSettings;
    SettingExportData.ExportedObjects.Reset();

    FNVSceneAnnotatedActorData SceneAnnotatedActorData;
    SceneAnnotatedActorData.exported_objects.Reset();
    SceneAnnotatedActorData.exported_object_classes.Reset();

    // TODO: Right now we only export the capturer's settings data to the file
    // but we need to also export the map set up and training objects' settings later too

    UWorld* World = GetWorld();
    ensure(World);
    if (World)
    {
        TArray<FString> ActorClassNames;
        ActorClassNames.Reset();

	    ANVSceneManager* SceneManager = ANVSceneManager::GetANVSceneManagerPtr();
        // NOTE: We should keep a list of all the actors that we want to annotate
        for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
        {
            AActor* CheckActor = *ActorIt;
            if (CheckActor)
            {
                UNVCapturableActorTag* Tag = Cast<UNVCapturableActorTag>(CheckActor->GetComponentByClass(UNVCapturableActorTag::StaticClass()));
                bool bShouldExport = (Tag && Tag->bIncludeMe);
                if (bShouldExport)
                {
                    const FString& ActorClassName = Tag->Tag;
                    // Don't process the instanced actor of the same class more than once
                    if (ActorClassNames.Contains(ActorClassName))
                    {
                        continue;
                    }

                    FNCapturerSettingExportedActorData ExportActorData;
                    ExportActorData.Class = Tag->Tag;
                    ExportActorData.ActorRef = CheckActor;

                    ExportActorData.CuboidCenterLocal = FVector::ZeroVector;
                    ANVAnnotatedActor* AnnotatedActor = Cast<ANVAnnotatedActor>(CheckActor);
                    if (AnnotatedActor)
                    {
                        ExportActorData.CuboidCenterLocal = AnnotatedActor->GetCuboidCenterLocal();
                    }

                    UStaticMeshComponent* ActorMeshComp = Cast<UStaticMeshComponent>(NVSceneCapturerUtils::GetFirstValidMeshComponent(CheckActor));
                    if (ActorMeshComp)
                    {
                        const FNVCuboidData& MeshCuboid_Local = NVSceneCapturerUtils::GetMeshCuboid_OOBB_Simple(ActorMeshComp, false, false);
                        ExportActorData.cuboid_dimensions = NVSceneCapturerUtils::ConvertDimensionToOpenCVCoordinateSystem(MeshCuboid_Local.GetDimension());

                        UStaticMesh* ActorStaticMesh = ActorMeshComp->GetStaticMesh();
                        if (ActorStaticMesh)
                        {
#if WITH_EDITOR
                            UFbxAssetImportData* FbxAssetImportData = Cast<UFbxAssetImportData>(ActorStaticMesh->AssetImportData);
                            FMatrix ImportMatrix = FMatrix::Identity;

                            if (FbxAssetImportData)
                            {
                                FTransform AssetImportTransform(
                                    FbxAssetImportData->ImportRotation.Quaternion(),
                                    FbxAssetImportData->ImportTranslation,
                                    FVector(FbxAssetImportData->ImportUniformScale)
                                );

                                ImportMatrix = AssetImportTransform.ToMatrixWithScale();
                            }

                            const FTransform& RelativeTransform = ActorMeshComp->GetRelativeTransform();
                            const FMatrix& RelativeToActorMatrix = RelativeTransform.ToMatrixWithScale();
                            const FTransform& ComponentTransform = ActorMeshComp->GetComponentTransform();
                            const FMatrix& ComponentToWorldMatrix = ComponentTransform.ToMatrixWithScale();

                            const FMatrix& MeshInitialMatrix = ImportMatrix * RelativeToActorMatrix;
                            ExportActorData.fixed_model_transform = MeshInitialMatrix * NVSceneCapturerUtils::UE4ToOpenCVMatrix;

                            // HACK: Invert the Y axis - need to figure out why?
                            FVector MatX, MatY, MatZ;
                            ExportActorData.fixed_model_transform.GetScaledAxes(MatX, MatY, MatZ);
                            MatY = -MatY;
                            ExportActorData.fixed_model_transform.SetAxes(nullptr, &MatY, nullptr);

					        ExportActorData.segmentation_class_id = SceneManager ? SceneManager->ObjectClassSegmentation.GetInstanceId(CheckActor) : 0;
					        ExportActorData.segmentation_instance_id = SceneManager ? SceneManager->ObjectInstanceSegmentation.GetInstanceId(CheckActor) : 0;

                            SettingExportData.ExportedObjects.Add(ExportActorData);
                            SceneAnnotatedActorData.exported_objects.Add(ExportActorData);
                            SceneAnnotatedActorData.exported_object_classes.Add(ActorClassName);

                            ActorClassNames.Add(ActorClassName);
#endif // WITH_EDITOR
                        }
                    }
                }
            }
        }
    }
    SettingExportData.ExportedObjectCount = SettingExportData.ExportedObjects.Num();

    const FString& OutputDirectoryPath = GetFullOutputDirectoryPath();
    static const FString& CapturerSettingFileName = TEXT("_settings.json");
    const FString& SettingFilePath = FPaths::Combine(OutputDirectoryPath, CapturerSettingFileName);
    static const FString& ObjectSettingsFileName = TEXT("_object_settings.json");
    const FString& ObjectSettingsFilePath = FPaths::Combine(OutputDirectoryPath, ObjectSettingsFileName);
    static const FString& CameraSettingsFileName = TEXT("_camera_settings.json");
    const FString& CameraSettingsFilePath = FPaths::Combine(OutputDirectoryPath, CameraSettingsFileName);

    FString OutputString;
    int64 CheckFlags = 0;
    int64 SkipFlags = CPF_AdvancedDisplay | CPF_Transient;
    bool result = false;
    // TODO: May want to include the launch parameters in the _settings.json later but now just _object_settings and _camera_settings are enough
    //result = FJsonObjectConverter::UStructToJsonObjectString<FNVCapturerSettingExportData>(SettingExportData, OutputString, CheckFlags, SkipFlags, 0, nullptr);
    //if (!result || !FFileHelper::SaveStringToFile(OutputString, *SettingFilePath))
    //{
    //  UE_LOG(LogNVSceneCapturer, Error, TEXT("Can't export the capturer settings to file: '%s'"), *SettingFilePath);
    //}

    TSharedPtr<FJsonObject> SceneAnnotatedDataJsonObj = NVSceneCapturerUtils::UStructToJsonObject(SceneAnnotatedActorData, CheckFlags, SkipFlags);
    if (SceneAnnotatedDataJsonObj.IsValid())
    {
        NVSceneCapturerUtils::SaveJsonObjectToFile(SceneAnnotatedDataJsonObj, ObjectSettingsFilePath);
    }

    // Export the camera settings
    FNVCameraSettingExportData CameraSettingsExportData;
    CameraSettingsExportData.camera_settings.Reset();
    // Create the feature extractors for each viewpoint
    for (UNVSceneCapturerViewpointComponent* CheckViewpointComp : OwnerSceneCapturer->GetViewpointList())
    {
        if (CheckViewpointComp && CheckViewpointComp->IsEnabled())
        {
            FNVViewpointSettingExportData ViewpointSettingsData;
            ViewpointSettingsData.Name = CheckViewpointComp->GetDisplayName();
            FNVSceneCapturerSettings CapturerSettings = CheckViewpointComp->GetCapturerSettings();
            CapturerSettings.RandomizeSettings();
            ViewpointSettingsData.horizontal_fov = CapturerSettings.GetFOVAngle();
            ViewpointSettingsData.intrinsic_settings = CapturerSettings.GetCameraIntrinsicSettings();
            ViewpointSettingsData.captured_image_size.Width = ViewpointSettingsData.intrinsic_settings.ResX;
            ViewpointSettingsData.captured_image_size.Height = ViewpointSettingsData.intrinsic_settings.ResY;
            ViewpointSettingsData.CameraProjectionMatrix = ViewpointSettingsData.intrinsic_settings.GetProjectionMatrix();
            CameraSettingsExportData.camera_settings.Add(ViewpointSettingsData);
        }
    }

    TSharedPtr<FJsonObject> CamSettingsJsonObj = NVSceneCapturerUtils::UStructToJsonObject(CameraSettingsExportData, CheckFlags, SkipFlags);
    if (CamSettingsJsonObj.IsValid())
    {
        NVSceneCapturerUtils::SaveJsonObjectToFile(CamSettingsJsonObj, CameraSettingsFilePath);
    }
}

void UNVSceneDataExporter::OnStopCapturingSceneData()
{
	if (ImageExporterThread.IsValid())
	{
		ImageExporterThread->Stop();
	}
}

void UNVSceneDataExporter::OnCapturingCompleted()
{
    // TODO: Bugfix. SceneManager state will be change in OnCapturingCompleted().
    //               We are not sure that SceneManager OnCapturingCompleted() is called, after UNVSceneDataExporter::OnCapturingCompleted().

    ANVSceneManager* SceneManager = ANVSceneManager::GetANVSceneManagerPtr();
    const bool bIsSceneCompleted = !SceneManager || SceneManager->GetState() == ENVSceneManagerState::Captured;

    if (bIsSceneCompleted && bAutoOpenExportedDirectory)
    {
        // Open the output path when the capturing process is completed
        const FString& FullOutputDirPath = GetFullOutputDirectoryPath();
        FPlatformProcess::ExploreFolder(*FullOutputDirPath);
    }
}

FString UNVSceneDataExporter::GetRootCaptureDirectoryPath() const
{
    FString FullRootCaptureDirectoryPath = RootCapturedDirectoryPath.Path;
    if (FPaths::IsRelative(FullRootCaptureDirectoryPath))
    {
        FullRootCaptureDirectoryPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FullRootCaptureDirectoryPath);
    }
    return FullRootCaptureDirectoryPath;
}

FString UNVSceneDataExporter::GetExportFolderName() const
{
    return (bUseMapNameForCapturedDirectory || CustomDirectoryName.IsEmpty()) ? GetNameSafe(GetWorld()) : CustomDirectoryName;
}

FString UNVSceneDataExporter::GetConfiguredOutputDirectoryName() const
{
    const FString OutputFolderName = GetExportFolderName();
    const FString OutputSubFolderName = GetSubFolderName();

    if (OutputSubFolderName.IsEmpty())
    {
        return OutputFolderName;
    }

    return FPaths::Combine(OutputFolderName, OutputSubFolderName);
}

FString UNVSceneDataExporter::GetConfiguredOutputDirectoryPath() const
{
    const FString OutputDir = GetConfiguredOutputDirectoryName();
    FString FullOutputDirPath;
    if (FPaths::IsRelative(OutputDir))
    {
        const FString RootDataOutputDir = GetRootCaptureDirectoryPath();
        FullOutputDirPath = FPaths::Combine(RootDataOutputDir, OutputDir);
    }
    else
    {
        FullOutputDirPath = OutputDir;
    }

    return FullOutputDirPath;
}

FString UNVSceneDataExporter::GetFullOutputDirectoryPath() const
{
    return FullOutputDirectoryPath;
}

FString UNVSceneDataExporter::GetSubFolderName() const
{
    return SubFolderName;
}

void UNVSceneDataExporter::SetSubFolderName(const FString& NewSubFolderName)
{
    SubFolderName = NewSubFolderName;
}

FString UNVSceneDataExporter::GetExportFilePath(UNVSceneFeatureExtractor* CapturedFeatureExtractor,
        UNVSceneCapturerViewpointComponent* CapturedViewpoint,
        int32 FrameIndex,
        const FString& FileExtension) const
{
    const FString OutputFolderPath = GetFullOutputDirectoryPath();
    FString OutputFileName = FString::Printf(TEXT("%06i"), FrameIndex);
    const auto& ViewpointSettings = CapturedViewpoint->GetSettings();
    if (!ViewpointSettings.ExportFileNamePostfix.IsEmpty())
    {
        OutputFileName += TEXT(".") + ViewpointSettings.ExportFileNamePostfix;
    }
    if (!CapturedFeatureExtractor->ExportFileNamePostfix.IsEmpty())
    {
        OutputFileName += TEXT(".") + CapturedFeatureExtractor->ExportFileNamePostfix;
    }
    OutputFileName += FileExtension;

    const FString ExportFilePath = FPaths::Combine(OutputFolderPath, OutputFileName);
    return ExportFilePath;
}

//=================================== UNVSceneDataVisualizer ===================================
UNVSceneDataVisualizer::UNVSceneDataVisualizer()
{
}

void UNVSceneDataVisualizer::Init()
{
    ANVSceneCapturerActor* OwnerCapturer = Cast<ANVSceneCapturerActor>(GetOuter());
    if (OwnerCapturer)
    {
        for (UNVSceneCapturerViewpointComponent* ViewpointComp : OwnerCapturer->GetViewpointList())
        {
            if (ViewpointComp && ViewpointComp->IsEnabled())
            {
                for (auto SceneFeatureExtractor : ViewpointComp->FeatureExtractorList)
                {
                    UNVSceneFeatureExtractor_PixelData* ImageFeatureExtractor = Cast<UNVSceneFeatureExtractor_PixelData>(SceneFeatureExtractor);
                    if (ImageFeatureExtractor && ImageFeatureExtractor->IsEnabled())
                    {
                        const FString ChannelName = ViewpointComp->GetDisplayName() + TEXT("/") + SceneFeatureExtractor->DisplayName;
                        UTextureRenderTarget2D* SourceTexture2D = ImageFeatureExtractor->GetRenderTarget();
                        if (!SourceTexture2D)
                        {
                            UE_LOG(LogNVSceneDataHandler, Warning, TEXT("GetRenderTarget() is failed."));
                        }
                        else 
                        {
                            VizTextureMap.Add(ChannelName, SourceTexture2D);
                        }
                    }
                }
            }
        }
    }
}

bool UNVSceneDataVisualizer::CanHandleMoreData() const
{
    // We only keep reference to the latest captured data so we can always handle more, newest ones
    return true;
}

bool UNVSceneDataVisualizer::IsHandlingData() const
{
    return false;
}

bool UNVSceneDataVisualizer::HandleScenePixelsData(const FNVTexturePixelData& CapturedPixelData, UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor, UNVSceneCapturerViewpointComponent* CapturedViewpoint, int32 FrameIndex)
{
    if (!CapturedFeatureExtractor || !CapturedViewpoint)
    {
        return false;
    }

    // NOTE: May want to update debug text on the HUD?

    return true;
}

bool UNVSceneDataVisualizer::HandleSceneAnnotationData(const TSharedPtr<FJsonObject>& CapturedData, class UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor, class UNVSceneCapturerViewpointComponent* CapturedViewpoint, int32 FrameIndex)
{
    // TODO: Need to handle general data, annotation, not just the pixels data
    return false;
}

void UNVSceneDataVisualizer::OnCapturingCompleted()
{
}

TArray<FString> UNVSceneDataVisualizer::GetVizNameList() const
{
    // TODO: Only update this list when the feature extractors changed
    TArray<FString> VizNameList;
    VizTextureMap.GenerateKeyArray(VizNameList);
    return VizNameList;
}

UTextureRenderTarget2D* UNVSceneDataVisualizer::GetTexture(const FString& VizName)
{
    UTextureRenderTarget2D** VizTexturePtr = VizTextureMap.Find(VizName);
    if (VizTexturePtr)
    {
        return (*VizTexturePtr);
    }

    return nullptr;
}
