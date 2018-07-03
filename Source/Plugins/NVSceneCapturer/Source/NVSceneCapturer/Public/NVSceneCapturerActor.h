/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "NVImageExporter.h"
#include "NVSceneDataHandler.h"
#include "NVSceneCapturerActor.generated.h"

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNCapturerSettingExportedActorData
{
    GENERATED_BODY();

public:
    UPROPERTY()
    FString Class;

	UPROPERTY()
	uint32 segmentation_class_id;

	UPROPERTY()
	uint32 segmentation_instance_id;

    UPROPERTY()
    FMatrix fixed_model_transform;

    UPROPERTY()
    FVector cuboid_dimensions;

    UPROPERTY(Transient)
    FVector CuboidCenterLocal;

    UPROPERTY(Transient)
    AActor* ActorRef;
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVSceneAnnotatedActorData
{
    GENERATED_BODY();

public:
    UPROPERTY()
    TArray<FString> exported_object_classes;

    UPROPERTY()
    TArray<FNCapturerSettingExportedActorData> exported_objects;
};


USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVCapturerSettingExportData
{
    GENERATED_BODY();

public:
    UPROPERTY()
    FNVSceneCapturerSettings CapturersSettings;

    UPROPERTY()
    uint32 ExportedObjectCount;

    UPROPERTY()
    TArray<FNCapturerSettingExportedActorData> ExportedObjects;
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVViewpointSettingExportData
{
    GENERATED_BODY();

public:
    UPROPERTY()
    FString Name;

    /// Horizontal field-of-view
    UPROPERTY()
    float horizontal_fov;

    UPROPERTY()
    FCameraIntrinsicSettings intrinsic_settings;

    UPROPERTY()
    FNVImageSize captured_image_size;

    UPROPERTY(Transient)
    FMatrix CameraProjectionMatrix;
};

USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVCameraSettingExportData
{
    GENERATED_BODY();

public:
    UPROPERTY()
    TArray<FNVViewpointSettingExportData> camera_settings;
};

class UNVSceneCapturerViewpointComponent;
class ANVSceneCapturerActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNVSceneCapturer_Started, ANVSceneCapturerActor*, SceneCapturer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNVSceneCapturer_Stopped, ANVSceneCapturerActor*, SceneCapturer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNVSceneCapturer_Completed, ANVSceneCapturerActor*, SceneCapturer, bool, bIsSucceeded);

///
/// The scene exporter actor.
///
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config=Engine,
       HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering, Collision, Physics, Navigation, Shape, Cooking, HLOD, Mobile))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API ANVSceneCapturerActor : public AActor
{
    GENERATED_BODY()

public:
    /// Use SpawnActor() to create this instance.
    ANVSceneCapturerActor(const FObjectInitializer& ObjectInitializer);

    /// Setter and Getter for Number of scene to capture
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void SetNumberOfFramesToCapture(int32 NewSceneCount);
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    int32 GetNumberOfFramesToCapture() const
    {
        return NumberOfFramesToCapture;
    }

    /// Capture controls
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void StartCapturing();
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void StopCapturing();
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void PauseCapturing();
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void ResumeCapturing();

    /// return false means PlayerController view.
    ///        true means Viewport is taken over.
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    bool ToggleTakeOverViewport();

    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void TakeOverViewport();
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void ReturnViewportToPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Capturer")
	ENVSceneCapturerState GetCurrentState() const
    {
        return CurrentState;
    }

    /// Frame counters
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    FNVFrameCounter GetCapturedFrameCounter() const
    {
        return CapturedFrameCounter;
    }

    /// Capturing information
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    float GetCapturedFPS() const;
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    int32 GetNumberOfFramesLeftToCapture() const;
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    float GetEstimatedTimeUntilFinishCapturing() const;
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    float GetCaptureProgressFraction() const;
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    float GetCapturedDuration() const;

    UFUNCTION(BlueprintCallable, Category = "Capturer")
    TArray<UNVSceneCapturerViewpointComponent*> GetViewpointList();

    /// Control what to do with the captured scene data
	UFUNCTION(BlueprintCallable, Category = "Capturer")
	UNVSceneDataHandler* GetSceneDataHandler() const;

    /// Control what to do with the captured scene data
	UNVSceneDataVisualizer* GetSceneDataVisualizer() const;

    static TArray<FNVNamedImageSizePreset> const& GetImageSizePresets();

    /// Event properties
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNVSceneCapturer_Started OnStartedEvent;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNVSceneCapturer_Stopped OnStoppedEvent;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNVSceneCapturer_Completed OnCompletedEvent;

protected:
    virtual void PostLoad() final;
    virtual void BeginPlay() final;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) final;
    virtual void PostInitializeComponents() final;
    virtual void Tick(float DeltaTime) final;
#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    void OnActorSelected(UObject* Object);
#endif //WITH_EDITORONLY_DATA

    void ResetCounter();
    void UpdateSettingsFromCommandLine();
    void UpdateViewpointList();
    void StartCapturing_Internal();
    void CaptureSceneToPixelsData();
    void CheckCaptureScene();
    void UpdateCapturerSettings();
    void OnCompleted();
    bool CanHandleMoreSceneData() const;

public: // Editor properties
	/// Whether this capturer actor is active and can start capturing or not
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture")
    bool bIsActive;

	UPROPERTY(EditAnywhere, SimpleDisplay, Category = Settings, meta = (ShowOnlyInnerProperties))
    FNVSceneCapturerSettings CapturerSettings;

    /// List of the feature extractors this capturer support
    UPROPERTY(EditAnywhere, SimpleDisplay, Category = FeatureExtraction, meta = (ShowOnlyInnerProperties))
    TArray<FNVFeatureExtractorSettings> FeatureExtractorSettings;

protected: // Editor properties
    /// Collision of the capturer actor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class USphereComponent* CollisionComponent;

    /// If true, this capturer will automatically start capturing the scene right when the game start (every TimeBetweenSceneExport seconds)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture")
    bool bAutoStartCapturing;

    /// NOTE: TimeBetweenSceneExport <= 0 mean export every frame
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture")
    float TimeBetweenSceneCapture;

    /// Maximum number of scenes (>= 0) to export before stopping
    /// NOTE: If TotalNumberOfScenesToExport == 0 then the exporter will keep exporting without limit until told to stop
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture", meta=(UIMin=0))
    int32 MaxNumberOfFramesToCapture;

    /// If true, the player's camera will be tied to this exporter's location and rotation
    UPROPERTY(EditAnywhere, Category = "Capture")
    bool bTakeOverGameViewport;

    /// Control what to do with the captured scene data
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Capture")
    class UNVSceneDataHandler* SceneDataHandler;

    /// Control what to do with the captured scene data
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Capture")
    class UNVSceneDataVisualizer* SceneDataVisualizer;

    /// If true, the capturer will pause the game logic when it's trying to flushing - handle the scene data from previous frame
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
    bool bPauseGameLogicWhenFlushing;

    /// List of available image size presets
    UPROPERTY(config)
    TArray<FNVNamedImageSizePreset> ImageSizePresets;

protected: // Transient properties
    UPROPERTY(Transient)
    float StartCapturingTimestamp;

    UPROPERTY(Transient)
    float CapturedDuration;

    UPROPERTY(Transient)
    float StartCapturingDuration;

    UPROPERTY(Transient)
    float LastCaptureTimestamp;

    UPROPERTY(Transient)
    FNVFrameCounter CapturedFrameCounter;

    UPROPERTY(Transient)
    AActor* CachedPlayerControllerViewTarget;

    UPROPERTY(Transient)
    ENVSceneCapturerState CurrentState;

    UPROPERTY(Transient)
    int32 NumberOfFramesToCapture;

    UPROPERTY(Transient)
    bool bNeedToExportScene;

    UPROPERTY(Transient)
    bool bTakingOverViewport;

    UPROPERTY(Transient)
    bool bSkipFirstFrame;

    UPROPERTY(Transient)
    FTimerHandle TimeHandle_StartCapturingDelay;

	UPROPERTY(Transient)
	TArray<UNVSceneCapturerViewpointComponent*> ViewpointList;
};