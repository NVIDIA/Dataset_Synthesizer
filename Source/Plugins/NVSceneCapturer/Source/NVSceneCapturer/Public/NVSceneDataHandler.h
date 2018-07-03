/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneCapturerUtils.h"
#include "NVImageExporter.h"
#include "NVSceneDataHandler.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVSceneDataHandler, Log, All)

///
/// Base interface for serializing/visualizing captured pixel and annotation data.
///
UCLASS(NotBlueprintable, Abstract, DefaultToInstanced, editinlinenew, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVSceneDataHandler : public UObject
{
    GENERATED_BODY()

public:
    UNVSceneDataHandler() { };

    /// Check whether this handler can process more scene data
    /// If it can't then we should stop getting more data until it's available again
    virtual bool CanHandleMoreData() const PURE_VIRTUAL(UNVSceneDataHandler::CanHandleMoreData, return false; );

    virtual bool IsHandlingData() const PURE_VIRTUAL(UNVSceneDataHandler::IsHandlingData, return false; );

    /// Handle the pixels data captured from the scene
    /// @param CapturedPixelData - The scene's pixels data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleScenePixelsData(const FNVTexturePixelData& CapturedPixelData,
        class UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor,
        class UNVSceneCapturerViewpointComponent* CapturedViewpoint,
        int32 FrameIndex) PURE_VIRTUAL(UNVSceneDataHandler::HandleScenePixelsData, return false; );

    /// Handle the annotation data captured from the scene
    /// @param CapturedData  - The scene's annotated data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleSceneAnnotationData(const TSharedPtr<FJsonObject>& CapturedData,
        class UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor,
        class UNVSceneCapturerViewpointComponent* CapturedViewpoint,
        int32 FrameIndex) PURE_VIRTUAL(UNVSceneDataHandler::HandleSceneAnnotationData, return false; );

    virtual void OnStartCapturingSceneData() PURE_VIRTUAL(UNVSceneDataHandler::OnStartCapturingSceneData, return; );
    virtual void OnStopCapturingSceneData() PURE_VIRTUAL(UNVSceneDataHandler::OnStopCapturingSceneData, return; );
    virtual void OnCapturingCompleted() PURE_VIRTUAL(UNVSceneDataHandler::OnCapturingCompleted, return; );
};

//=================================== UNVSceneDataExporter ===================================
UENUM(BlueprintType)
enum class ENVCaptureDirectoryConflictHandleType : uint8
{
    /// Just overwrite existing files in the same directory
    OverwriteExistingFiles UMETA(DisplayName = "Overwrite existing files"),

    /// Clean the directory by removing all files in it
    CleanDirectory UMETA(DisplayName = "Remove existing files"),

    /// Create new directory with timestamp as postfix in name
    CreateNewDirectoryWithTimestampPostfix UMETA(DisplayName = "Create new directory with timestamp as postfix in name"),

    CaptureDirectoryConflictHandleType_MAX UMETA(Hidden)
};

///
/// NVSceneDataExporter - export all the captured data (image buffer and object annotation info) to files on disk
///
UCLASS(Blueprintable, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVSceneDataExporter : public UNVSceneDataHandler
{
    GENERATED_BODY()

public:
    UNVSceneDataExporter();

    /// Check whether this handler can process more scene data
    /// If it can't then we should stop getting more data until it's available again
    virtual bool CanHandleMoreData() const override;
    virtual bool IsHandlingData() const override;

    /// Handle the pixels data captured from the scene
    /// @param CapturedPixelData - The scene's pixels data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleScenePixelsData(const FNVTexturePixelData& CapturedPixelData,
                                       UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor,
                                       UNVSceneCapturerViewpointComponent* CapturedViewpoint,
                                       int32 FrameIndex) override;

    /// Handle the annotation data captured from the scene
    /// @param CapturedData  - The scene's annotated data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleSceneAnnotationData(const TSharedPtr<FJsonObject>& CapturedData,
                                           class UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor,
                                           class UNVSceneCapturerViewpointComponent* CapturedViewpoint,
                                           int32 FrameIndex) override;

    virtual void OnStartCapturingSceneData() override;
    virtual void OnStopCapturingSceneData() override;
    virtual void OnCapturingCompleted() override;

    UFUNCTION(BlueprintCallable, Category = "Exporter")
    FString GetRootCaptureDirectoryPath() const;
    UFUNCTION(BlueprintCallable, Category = "Exporter")
    virtual FString GetExportFolderName() const;

    UFUNCTION(BlueprintCallable, Category = "Exporter")
    FString GetConfiguredOutputDirectoryPath() const;
    UFUNCTION(BlueprintCallable, Category = "Exporter")
    FString GetConfiguredOutputDirectoryName() const;
    UFUNCTION(BlueprintCallable, Category = "Exporter")
    FString GetFullOutputDirectoryPath() const;

    UFUNCTION(BlueprintCallable, Category = "Exporter")
    virtual FString GetSubFolderName() const;
    UFUNCTION(BlueprintCallable, Category = "Exporter")
    void SetSubFolderName(const FString& NewSubFolderName);

    UFUNCTION(BlueprintCallable, Category = "Exporter")
    FString GetExportFilePath(class UNVSceneFeatureExtractor* CapturedFeatureExtractor,
                              UNVSceneCapturerViewpointComponent* CapturedViewpoint,
                              int32 FrameIndex,
                              const FString& FileExtension) const;

protected:
    void ExportCapturerSettings();

public: // Editor properties
    // ToDo: move to protected.
    /// If true, the exporter will use the current map's name for the export folder, otherwise it will use the ExportFolderName
    /// NOTE: If ExportFolderName is empty, it will fallback to use the map's name
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save Path")
    bool bUseMapNameForCapturedDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Save Path", meta = (EditCondition = "!bUseMapNameForCapturedDirectory"))
    FString CustomDirectoryName;

protected: // Editor properties
    /// Path to the directory where to save captured data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Path")
    FDirectoryPath RootCapturedDirectoryPath;

    /// How to handle conflict files
    UPROPERTY(EditAnywhere, Category = "Save Path")
    ENVCaptureDirectoryConflictHandleType DirectoryConflictHandleType;

    /// If true, this exporter will automatically open the exported directory after it finish exporting
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture")
    bool bAutoOpenExportedDirectory;

    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Capture")
    uint32 MaxSaveImageAsyncCount;

protected: // Transient
    UPROPERTY(Transient)
    FString SubFolderName;

    UPROPERTY(Transient)
    FString FullOutputDirectoryPath;

    TUniquePtr<FNVImageExporter_Thread> ImageExporterThread;
    IImageWrapperModule* ImageWrapperModule;

    static const FString DefaultDataOutputFolder;
};

//=================================== UNVSceneDataVisualizer ===================================
///
/// NVSceneDataVisualizer - visualize all the captured data (image buffer and object annotation info) using material, UI
///
UCLASS(Blueprintable, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVSceneDataVisualizer : public UNVSceneDataHandler
{
    GENERATED_BODY()

public:
    UNVSceneDataVisualizer();

    virtual void Init();
    virtual bool CanHandleMoreData() const override;
    virtual bool IsHandlingData() const override;

    /// Handle the pixels data captured from the scene
    /// @param CapturedPixelData - The scene's pixels data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleScenePixelsData(const FNVTexturePixelData& CapturedPixelData,
                                       UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor,
                                       UNVSceneCapturerViewpointComponent* CapturedViewpoint,
                                       int32 FrameIndex) override;

    /// Handle the annotation data captured from the scene
    /// @param CapturedData  - The scene's annotated data
    /// @param CapturedFeatureExtractor - The feature extractor which captured the data
    /// @param CapturedViewpoint - The viewpoint which captured the data
    /// @param FrameIndex - The frame when the data is captured
    virtual bool HandleSceneAnnotationData(const TSharedPtr<FJsonObject>& CapturedData,
        class UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor,
        class UNVSceneCapturerViewpointComponent* CapturedViewpoint,
        int32 FrameIndex) override;

    virtual void OnCapturingCompleted() override;

    TArray<FString> GetVizNameList() const;
    UTextureRenderTarget2D* GetTexture(const FString& VizName);
    
protected: // Transient
	/// Map between a feature extractor name and its visualized texture
    UPROPERTY(Transient)
    TMap<FString, UTextureRenderTarget2D*> VizTextureMap;
};