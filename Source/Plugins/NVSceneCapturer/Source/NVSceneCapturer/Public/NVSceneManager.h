/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneMarker.h"
#include "NVObjectMaskManager.h"
#include "NVSceneManager.generated.h"

class ANVSceneManager;
class ANVSceneCapturerActor;

/**
 *  Capturing State.
 */
UENUM(BlueprintType)
enum class ENVSceneManagerState : uint8
{
    /** This Scene manager is not used. */
    NotActive UMETA(DisplayName = "This SceneManager is not active."),

    /** This Scene manager is active to capture the scene. */
    Active UMETA(DisplayName = "This SceneManager is active."),

    /** This Scene manager is active and ready to capture or it is capturing now.*/
    Ready UMETA(DisplayName = "Ready to capture."),

    /** All scenes are captured. */
    Captured UMETA(DisplayName = "Capturing is done."),
};

/**
 * Declaration for SetupCompleted multicast.
 * This event happens, when all instances are ready to capture.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNVSceneManger_SetupCompleted, ANVSceneManager*, SceneManager, bool, bIsSucceeded);

/**
 * Actor representing the scene to be annotated and have its info captured and exported.
 * Although multiple instances can be created, only one can be active for scene capturing at a time.
 * All other instances' 'bIsActive' property will be disabled when capturing starts.
 */
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config=Engine, HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API ANVSceneManager : public AActor
{
    GENERATED_BODY()

public:
    /// Use SpawnActor() to create this instance.
    ANVSceneManager(const FObjectInitializer& ObjectInitializer);

    /// Get singleton this instance.
    /// After lifecycle "PostInitializeComponents" is finished, this method is available.
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    static ANVSceneManager* GetANVSceneManagerPtr();

    /// Get scene capturing state.
    ENVSceneManagerState GetState() const;

    /// if state is CAPTURED, this change the state to READY.
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    void ResetState();

	UPROPERTY(EditAnywhere, Category = CapturerScene)
	FNVObjectSegmentation_Class ObjectClassSegmentation;

	UPROPERTY(EditAnywhere, Category = CapturerScene)
	FNVObjectSegmentation_Instance ObjectInstanceSegmentation;

protected:
    virtual void PreInitializeComponents() override;
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    virtual void UpdateSettingsFromCommandLine();
    virtual void SetupSceneInternal();

    void SetupScene();
    void FocusNextMarker();
    bool IsAllSceneCaptured() const;

	UFUNCTION()
    virtual void OnCapturingCompleted(ANVSceneCapturerActor* SceneCapturer, bool bIsSucceeded);

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

protected: // Editor properties
    /// Whether this scene manager actively change the scene or not
    UPROPERTY(EditAnywhere)
    bool bIsActive;

    /// List of anchors for point-of-interests
    UPROPERTY(EditInstanceOnly)
    TArray<AActor*> SceneMarkers;

    UPROPERTY(EditAnywhere)
    bool bCaptureAtAllMarkers;

    /// If true, this exporter will automatically shutdown the game after it finish exporting
    UPROPERTY(EditAnywhere)
    bool bAutoExitAfterExportingComplete;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FNVSceneManger_SetupCompleted OnSetupCompleted;

    UPROPERTY(EditAnywhere)
    bool bUseMarkerNameAsPostfix;

protected: // Transient properties
    /// Keep track of all the scene capturers in the map
    UPROPERTY(Transient)
    TArray<class ANVSceneCapturerActor*> SceneCapturers;

    UPROPERTY(Transient)
    TArray<FString> SceneCaptureExportDirNames;

    UPROPERTY(Transient)
    AActor* CurrentSceneMarker;

    UPROPERTY(Transient)
    ENVSceneManagerState SceneManagerState;

    UPROPERTY(Transient)
    int32 CurrentMarkerIndex;
};
