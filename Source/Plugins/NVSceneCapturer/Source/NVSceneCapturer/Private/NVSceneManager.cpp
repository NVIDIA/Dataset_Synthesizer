/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerActor.h"
#include "NVObjectMaskManager.h"
#include "NVSceneManager.h"
#include "NVSceneMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "Factories/FbxAssetImportData.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

namespace
{
    static TWeakObjectPtr<ANVSceneManager> globalANVSceneManagerPtr=nullptr;
}

ANVSceneManager::ANVSceneManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    globalANVSceneManagerPtr = nullptr;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    bIsActive = true;
    bCaptureAtAllMarkers = true;
    SceneManagerState = ENVSceneManagerState::NotActive;
    CurrentSceneMarker = nullptr;
    bAutoExitAfterExportingComplete = true;
}

ANVSceneManager* ANVSceneManager::GetANVSceneManagerPtr()
{
    return globalANVSceneManagerPtr.Get();
}

ENVSceneManagerState ANVSceneManager::GetState() const
{
    return SceneManagerState;
}

void ANVSceneManager::ResetState()
{
    if (SceneManagerState == ENVSceneManagerState::Captured)
    {
        SceneManagerState = ENVSceneManagerState::Ready;
    }
}

bool ANVSceneManager::IsAllSceneCaptured() const
{
    return !bCaptureAtAllMarkers || (CurrentMarkerIndex >= SceneMarkers.Num() - 1);
}

void ANVSceneManager::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    globalANVSceneManagerPtr = nullptr;
}

void ANVSceneManager::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // bIsActive is public for UI, we need to copy the state to a protected value.
    // because we need a state which user can not change.

    // ANVSceneManager should be singleton. but currentry ANVSceneManager is also used for UI.
    // we need to decide which ANVSceneManager to use.
    // this selects one ANVSceneManager which is called PostInitializeComponents() first and it is active.
    // now only one ANVSceneManager will be active and only one instance will be used.
    if (bIsActive)
    {
        if (globalANVSceneManagerPtr == nullptr)
        {
            SceneManagerState = ENVSceneManagerState::Active;
            globalANVSceneManagerPtr = this;
        }
        else
        {
            SceneManagerState = ENVSceneManagerState::NotActive;

            // If user put multiple ANVSceneManagers,
            // We need disable bIsActive to show user which ANVSceneManagers is not used.
            bIsActive = false;
        }
    }
    else
    {
        SceneManagerState =  ENVSceneManagerState::NotActive;
    }
}

void ANVSceneManager::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
#if WITH_EDITOR
    bool bIsSimulating = GUnrealEd ? (GUnrealEd->bIsSimulatingInEditor || GUnrealEd->bIsSimulateInEditorQueued) : false;
    if (!World || !World->IsGameWorld() || bIsSimulating)
    {
        return;
    }
#endif
    ensure(World);
    if (World)
    {
        for (int i = SceneMarkers.Num() - 1; i >= 0; i--)
        {
            if (!SceneMarkers[i])
            {
                SceneMarkers.RemoveAt(i);
            }
        }
        if (SceneMarkers.Num() <= 0)
        {
            bCaptureAtAllMarkers = false;
        }

        if (SceneManagerState == ENVSceneManagerState::Active)
        {
            SceneCapturers.Reset();
            for (TActorIterator<ANVSceneCapturerActor> It(World); It; ++It)
            {
                ANVSceneCapturerActor* CheckCapturer = *It;
                if (CheckCapturer)
                {
                    SceneCapturers.Add(CheckCapturer);
                    // FIXME
                    //SceneCaptureExportDirNames.Add(CheckCapturer->GetExportFolderName());

                    CheckCapturer->OnCompletedEvent.AddDynamic(this, &ANVSceneManager::OnCapturingCompleted);
                }
            }

            UpdateSettingsFromCommandLine();

            if (bCaptureAtAllMarkers)
            {
                CurrentMarkerIndex = -1;
                FocusNextMarker();
            }
            else
            {
                CurrentMarkerIndex = 0;
                SetupScene();
            }

			ObjectClassSegmentation.Init(this);
			ObjectInstanceSegmentation.Init(this);

			ObjectClassSegmentation.ScanActors(World);
			ObjectInstanceSegmentation.ScanActors(World);
        }
    }
}

void ANVSceneManager::UpdateSettingsFromCommandLine()
{
    const auto CommandLine = FCommandLine::Get();

    FString OverrideCapturers = TEXT("");
    if (FParse::Value(CommandLine, TEXT("-Capturers="), OverrideCapturers))
    {
        TArray<FString> CapturerNames;
        OverrideCapturers.ParseIntoArray(CapturerNames, TEXT(","));
        if (CapturerNames.Num() > 0)
        {
            // Make sure all the capturers are deactivated first
            for (ANVSceneCapturerActor* CheckCapturer : SceneCapturers)
            {
                if (CheckCapturer)
                {
                    CheckCapturer->StopCapturing();
                    CheckCapturer->bIsActive = false;
                }
            }

            // Only activate the capturers specified in the command line
            for (const FString& CheckCapturerName : CapturerNames)
            {
                for (ANVSceneCapturerActor* CheckCapturer : SceneCapturers)
                {
                    if (CheckCapturer)
                    {
                        const FString& CapturerName = CheckCapturer->GetName();
                        const FString& CapturerHumanReadableName = CheckCapturer->GetHumanReadableName();
                        if ((CapturerHumanReadableName == CheckCapturerName) || (CapturerName == CheckCapturerName))
                        {
                            CheckCapturer->bIsActive = true;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void ANVSceneManager::SetupScene()
{
    UWorld* World = GetWorld();
    if (World)
    {
        const int32 POICount = SceneMarkers.Num();
        if ((POICount > 0) && (CurrentMarkerIndex < POICount) && (CurrentMarkerIndex >= 0))
        {
            INVSceneMarkerInterface* SceneMarker = Cast<INVSceneMarkerInterface>(CurrentSceneMarker);
            if (SceneMarker)
            {
                SceneMarker->RemoveAllObservers();
            }

            CurrentSceneMarker = SceneMarkers[CurrentMarkerIndex];
        }

        SetupSceneInternal();

        // TODO: Broadcast Ready event
        SceneManagerState = ENVSceneManagerState::Ready;

        OnSetupCompleted.Broadcast(this, SceneManagerState == ENVSceneManagerState::Ready);
    }
}

void ANVSceneManager::SetupSceneInternal()
{
    INVSceneMarkerInterface* SceneMarker = Cast<INVSceneMarkerInterface>(CurrentSceneMarker);
    if (SceneMarker)
    {
        for (ANVSceneCapturerActor* CheckCapturer : SceneCapturers)
        {
            // ToDo: Use GetCurrentState() instead of bIsActive.
            // CheckCapturer->GetCurrentState() is available after BeginPlay life cycle.
            if (CheckCapturer && CheckCapturer->bIsActive)
            {
                SceneMarker->AddObserver(CheckCapturer);
            }
        }
    }
}

void ANVSceneManager::FocusNextMarker()
{
    const int32 POICount = SceneMarkers.Num();

    if (bCaptureAtAllMarkers && (CurrentMarkerIndex < POICount - 1))
    {
        CurrentMarkerIndex++;
        SetupScene();

        for (int i = 0; i < SceneCapturers.Num(); i++)
        {
            ANVSceneCapturerActor* CheckCapturer = SceneCapturers[i];
            if (CheckCapturer && (CheckCapturer->GetCurrentState() != ENVSceneCapturerState::NotActive))
            {
                if (bUseMarkerNameAsPostfix)
                {
                    const FString CurrentExportFolderName = SceneCaptureExportDirNames[i];
                    const FString NewExportFolderName = FString::Printf(TEXT("%s_%d"), *CurrentExportFolderName, CurrentMarkerIndex);
                    // FIXME: Let the SceneDataExporter handle the directory
                    //CheckCapturer->CustomDirectoryName = NewExportFolderName;
                }

                CheckCapturer->StartCapturing();
            }
        }
    }
}

void ANVSceneManager::OnCapturingCompleted(ANVSceneCapturerActor* SceneCapturer, bool bIsSucceeded)
{
    if (bIsActive)
    {
        bool bAllCapturerCompleted = true;
        for (ANVSceneCapturerActor* CheckCapturer : SceneCapturers)
        {
            if (CheckCapturer && !(CheckCapturer->GetCurrentState()== ENVSceneCapturerState::Completed))
            {
                bAllCapturerCompleted = false;
                break;
            }
        }

        if (bAllCapturerCompleted)
        {
            if (IsAllSceneCaptured())
            {
                SceneManagerState = ENVSceneManagerState::Captured;
                if (bAutoExitAfterExportingComplete)
                {
                    UWorld* World = GetWorld();
                    if (World && GEngine)
                    {
                        GEngine->Exec(World, TEXT("exit"));
                    }
                }
            }
            else
            {
                SceneManagerState = ENVSceneManagerState::Active;
                FocusNextMarker();
            }
        }
    }
}

#if WITH_EDITORONLY_DATA
void ANVSceneManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    // Don't add ensure(PropertyThatChanged) here. 
    // When ANVSceneManagerActor is deplicated, PropertyThatChanged can be null.
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}
#endif // WITH_EDITORONLY_DATA

