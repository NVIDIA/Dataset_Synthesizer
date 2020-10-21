/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneFeatureExtractor.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneManager.h"
#include "NVAnnotatedActor.h"
#include "NVSceneDataHandler.h"
#include "Engine.h"
#include "JsonObjectConverter.h"
#if WITH_EDITOR
#include "Factories/FbxAssetImportData.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif // WITH_EDITOR

const float MAX_StartCapturingDuration = 5.0f; // max duration to wait for ANVSceneCapturerActor::StartCapturing to successfully begin capturing before emitting warning messages

ANVSceneCapturerActor::ANVSceneCapturerActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
    CollisionComponent->SetSphereRadius(20.f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Ignore);
    RootComponent = CollisionComponent;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    PrimaryActorTick.bTickEvenWhenPaused = true;


    TimeBetweenSceneCapture = 0.f;
    LastCaptureTimestamp = 0.f;

    bIsActive = true;
    CurrentState = ENVSceneCapturerState::Active;
    bAutoStartCapturing = false;
    bPauseGameLogicWhenFlushing = true;

    MaxNumberOfFramesToCapture = 0;
    NumberOfFramesToCapture = MaxNumberOfFramesToCapture;

    CachedPlayerControllerViewTarget = nullptr;

    CapturedDuration = 0.f;
    StartCapturingDuration = 0.0f;
    StartCapturingTimestamp = 0.f;
    bNeedToExportScene = false;
    bTakingOverViewport = false;
    bSkipFirstFrame = false;

#if WITH_EDITORONLY_DATA
    USelection::SelectObjectEvent.AddUObject(this, &ANVSceneCapturerActor::OnActorSelected);
#endif //WITH_EDITORONLY_DATA
}

void ANVSceneCapturerActor::PostLoad()
{
    Super::PostLoad();

    NumberOfFramesToCapture = MaxNumberOfFramesToCapture;
}

void ANVSceneCapturerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CheckCaptureScene();
}

void ANVSceneCapturerActor::UpdateSettingsFromCommandLine()
{
    const auto CommandLine = FCommandLine::Get();

    // TODO: Scan the setting's properties and auto check for overrided value
    FString OutputPathOverride;
    if (FParse::Value(CommandLine, TEXT("-OutputPath="), OutputPathOverride))
    {
        // FIXME: Let the SceneDataExporter update the settings from the commandline itself
        UNVSceneDataExporter* CurrentSceneDataExporter = Cast<UNVSceneDataExporter>(SceneDataHandler);
        if (CurrentSceneDataExporter)
        {
            CurrentSceneDataExporter->CustomDirectoryName = OutputPathOverride;
            CurrentSceneDataExporter->bUseMapNameForCapturedDirectory = false;
        }
    }

    int32 NumberOfFrameToCaptureOverride = 0;
    if (FParse::Value(CommandLine, TEXT("-NumberOfFrame="), NumberOfFrameToCaptureOverride))
    {
        NumberOfFramesToCapture = NumberOfFrameToCaptureOverride;
        // TODO: Separated bAutoStartExporting to a different switch?
        bAutoStartCapturing = true;
    }

    FString SettingsFilePath;
    if (FParse::Value(CommandLine, TEXT("-SettingsPath="), SettingsFilePath))
    {
        const FString SettingsFullpath = FPaths::Combine(FPaths::ProjectDir(), SettingsFilePath);
        FString SettingsStr = TEXT("");
        if (FFileHelper::LoadFileToString(SettingsStr, *SettingsFullpath))
        {
            TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(SettingsStr);

            TSharedPtr<FJsonObject> SettingsJsonObject;
            if (FJsonSerializer::Deserialize(JsonReader, SettingsJsonObject) && SettingsJsonObject.IsValid())
            {
                FNVSceneCapturerSettings OverridedSettings;
                int64 CheckFlags = 0;
                int64 SkipFlags = 0;
                if (FJsonObjectConverter::JsonObjectToUStruct(SettingsJsonObject.ToSharedRef(), FNVSceneCapturerSettings::StaticStruct(), &OverridedSettings, CheckFlags, SkipFlags))
                {
                    CapturerSettings = OverridedSettings;
                }
            }
        }
    }
}

void ANVSceneCapturerActor::BeginPlay()
{
    Super::BeginPlay();

    UpdateSettingsFromCommandLine();

    UWorld* World = GetWorld();
#if WITH_EDITOR
    bool bIsSimulating = GUnrealEd ? (GUnrealEd->bIsSimulatingInEditor || GUnrealEd->bIsSimulateInEditorQueued) : false;
    if (!World || !World->IsGameWorld() || bIsSimulating)
    {
        return;
    }
#endif

    bTakingOverViewport = bTakeOverGameViewport;
    if (bTakeOverGameViewport)
    {
        TakeOverViewport();
    }

    bNeedToExportScene = false;
    bSkipFirstFrame = true;

    UpdateViewpointList();

    // Create the feature extractors for each viewpoint
    for (UNVSceneCapturerViewpointComponent* CheckViewpointComp : ViewpointList)
    {
        if (CheckViewpointComp && CheckViewpointComp->IsEnabled())
        {
            CheckViewpointComp->SetupFeatureExtractors();
        }
    }

    // bIsActive is public property for UI. so we need to copy to protected property
    // to avoid access from user while we are captuering.
    CurrentState = bIsActive? ENVSceneCapturerState::Active: ENVSceneCapturerState::NotActive;
    if (bAutoStartCapturing && (CurrentState == ENVSceneCapturerState::Active))
    {
        // NOTE: We need to delay the capturing so the scene have time to set up
        const float DelayDuration = 1.f;
        GetWorldTimerManager().SetTimer(TimeHandle_StartCapturingDelay,
                                        this,
                                        &ANVSceneCapturerActor::StartCapturing,
                                        DelayDuration,
                                        false, 
                                        DelayDuration);
        StartCapturingDuration = 0;
    }

    if (SceneDataVisualizer)
    {
        SceneDataVisualizer->Init();
    }
}

void ANVSceneCapturerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(TimeHandle_StartCapturingDelay);

    Super::EndPlay(EndPlayReason);
}

void ANVSceneCapturerActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    UpdateViewpointList();
}

#if WITH_EDITORONLY_DATA
void ANVSceneCapturerActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        if ((ChangedPropName == GET_MEMBER_NAME_CHECKED(ANVSceneCapturerActor, CapturerSettings)))
        {
            CapturerSettings.PostEditChangeProperty(PropertyChangedEvent);
        }

        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}

void ANVSceneCapturerActor::OnActorSelected(UObject* Object)
{
    if (Object == this)
    {
		TArray<UNVSceneCapturerViewpointComponent*> ChildComponents;
		GetComponents<UNVSceneCapturerViewpointComponent>(ChildComponents);
		for (auto CheckViewpointComp : ChildComponents)
        {
            if (CheckViewpointComp && CheckViewpointComp->IsEnabled())
            {
                GSelectedComponentAnnotation.Set(CheckViewpointComp);
                break;
            }
        }
    }
}

#endif //WITH_EDITORONLY_DATA

void ANVSceneCapturerActor::CheckCaptureScene()
{
#if WITH_EDITOR
    bool bIsSimulating = GUnrealEd ? (GUnrealEd->bIsSimulatingInEditor || GUnrealEd->bIsSimulateInEditorQueued) : false;
    if (bIsSimulating)
    {
        return;
    }
#endif

    if (CurrentState == ENVSceneCapturerState::Running)
    {
        const float CurrentTime = GetWorld()->GetTimeSeconds();
        const float TimeSinceLastCapture = CurrentTime - LastCaptureTimestamp;
        if (TimeSinceLastCapture >= TimeBetweenSceneCapture)
        {
            bNeedToExportScene = true;
        }
        CapturedDuration = GetWorld()->GetRealTimeSeconds() - StartCapturingTimestamp;
    }
    else
    {
        bNeedToExportScene = false;
    }

    ANVSceneManager* ANVSceneManagerPtr = ANVSceneManager::GetANVSceneManagerPtr();
    // if ANVSceneManagerPtr is nullptr, then there's no scene manager and it's assumed the scene is static and thus ready, else check with the scene manager
    const bool bSceneIsReady = !ANVSceneManagerPtr || ANVSceneManagerPtr->GetState()== ENVSceneManagerState::Ready;
    UWorld* World = GetWorld();
    AGameModeBase* CurrentGameMode = World ? World->GetAuthGameMode() : nullptr;

    if (bNeedToExportScene && bSceneIsReady)
    {
        if (CanHandleMoreSceneData())
        {
            // Must unpause the game in this frame first before resuming capturing it
            if (CurrentGameMode && CurrentGameMode->IsPaused())
            {
                CurrentGameMode->ClearPause();
            }
            else
            {
                CaptureSceneToPixelsData();
                // Update the capturer settings at the end of the frame after we already captured data of this frame
                UpdateCapturerSettings();
            }
        }
        if (!CanHandleMoreSceneData())
        {
            // Pause the game logic if we can't handle more scene data right now
            if (bPauseGameLogicWhenFlushing && CurrentGameMode && !CurrentGameMode->IsPaused())
            {
                CurrentGameMode->SetPause(World->GetFirstPlayerController());
            }
        }
    }
}

void ANVSceneCapturerActor::ResetCounter()
{
    CapturedFrameCounter.Reset();
}

void ANVSceneCapturerActor::CaptureSceneToPixelsData()
{
    const int32 CurrentFrameIndex = CapturedFrameCounter.GetTotalFrameCount();

    bool bFinishedCapturing = (NumberOfFramesToCapture > 0) && (CurrentFrameIndex >= NumberOfFramesToCapture);

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float TimePassSinceLastCapture = CurrentTime - LastCaptureTimestamp;

    // Let all the child exporter components know it need to export the scene
    if (!bFinishedCapturing)
    {
        for (UNVSceneCapturerViewpointComponent* ViewpointComp : ViewpointList)
        {
            if (ViewpointComp && ViewpointComp->IsEnabled())
            {
                ViewpointComp->CaptureSceneToPixelsData(
                    [this, CurrentFrameIndex](const FNVTexturePixelData& CapturedPixelData, UNVSceneFeatureExtractor_PixelData* CapturedFeatureExtractor, UNVSceneCapturerViewpointComponent* CapturedViewpoint)
                {
                    if (SceneDataHandler)
                    {
                        SceneDataHandler->HandleScenePixelsData(CapturedPixelData,
                                                                CapturedFeatureExtractor,
                                                                CapturedViewpoint,
                                                                CurrentFrameIndex);
                    }

                    if (SceneDataVisualizer)
                    {
                        SceneDataVisualizer->HandleScenePixelsData(CapturedPixelData,
                                CapturedFeatureExtractor,
                                CapturedViewpoint,
                                CurrentFrameIndex);
                    }
                });

                ViewpointComp->CaptureSceneAnnotationData(
                    [this, CurrentFrameIndex](const TSharedPtr<FJsonObject>& CapturedData, UNVSceneFeatureExtractor_AnnotationData* CapturedFeatureExtractor, UNVSceneCapturerViewpointComponent* CapturedViewpoint)
                {
                    if (SceneDataHandler)
                    {
                        SceneDataHandler->HandleSceneAnnotationData(CapturedData,
                                CapturedFeatureExtractor,
                                CapturedViewpoint,
                                CurrentFrameIndex);
                    }
                });
            }
        }

        if (bSkipFirstFrame)
        {
            bSkipFirstFrame = false;
        }
        else
        {
            CapturedFrameCounter.IncreaseFrameCount();
        }
        CapturedFrameCounter.AddFrameDuration(TimePassSinceLastCapture);
    }
    else
    {
        bool bFinishedProcessingData = true;
        // Make sure all the captured scene data are processed
        if (SceneDataHandler)
        {
            bFinishedProcessingData = !SceneDataHandler->IsHandlingData();
        }

        if (bFinishedProcessingData)
        {
            OnCompleted();
        }
    }

    LastCaptureTimestamp = CurrentTime;
    bNeedToExportScene = false;
}

void ANVSceneCapturerActor::UpdateCapturerSettings()
{
    CapturerSettings.RandomizeSettings();
    for (auto* ViewpointComp : ViewpointList)
    {
        ViewpointComp->UpdateCapturerSettings();
    }
}

void ANVSceneCapturerActor::SetNumberOfFramesToCapture(int32 NewSceneCount)
{
    NumberOfFramesToCapture = NewSceneCount;
}

void ANVSceneCapturerActor::StartCapturing()
{
    bNeedToExportScene = false;
    bSkipFirstFrame = true;
    ANVSceneManager* ANVSceneManagerPtr = ANVSceneManager::GetANVSceneManagerPtr();
    // if ANVSceneManagerPtr is nullptr, then there's no scene manager and it's assumed the scene is static and thus ready, else check with the scene manager
    const bool bSceneIsReady = !ANVSceneManagerPtr || ANVSceneManagerPtr->GetState() == ENVSceneManagerState::Ready;
    if (bSceneIsReady)
    {
        StartCapturing_Internal();
    }
    else if (!TimeHandle_StartCapturingDelay.IsValid())
    {
        const float DelayDuration = 1.f;
        // NOTE: We need to delay the capturing so the scene have time to set up
        GetWorldTimerManager().SetTimer(TimeHandle_StartCapturingDelay,
                                        this,
                                        &ANVSceneCapturerActor::StartCapturing,
                                        DelayDuration,
                                        false,
                                        DelayDuration);

        StartCapturingDuration += DelayDuration;
        if (StartCapturingDuration > MAX_StartCapturingDuration)
        {
            UE_LOG(LogNVSceneCapturer, Warning, TEXT("Capturing could not Start -- did you set up the Game Mode?\nStartCapturingDuration: %.6f"),
                StartCapturingDuration);
        }
    }
    else
    {
        UE_LOG(LogNVSceneCapturer, Error, TEXT("Capturing could not Start -- did you set up the Game Mode?"));
    }
}

void ANVSceneCapturerActor::StartCapturing_Internal()
{
    if (!bIsActive)
    {
        // bIsActive is public. we need to copy bIsActive state into the protected value.
        CurrentState = ENVSceneCapturerState::NotActive;
    }
    else
    {
        // To start capture, SceneDataHandler is requirement.
        if (!SceneDataHandler)
        {
            UE_LOG(LogNVSceneCapturer, Error, TEXT("SceneCapturer SceneDataHandler is empty. Please select data handler in details panel."));
        }
        else
        {
            ANVSceneManager* NVSceneManagerPtr = ANVSceneManager::GetANVSceneManagerPtr();
            if (ensure(NVSceneManagerPtr))
            {
                // Make sure the segmentation mask of objects in the scene are up-to-date before capturing them
                NVSceneManagerPtr->UpdateSegmentationMask();
            }

            // Now we can start capture.
            UpdateCapturerSettings();

            OnStartedEvent.Broadcast(this);
            SceneDataHandler->OnStartCapturingSceneData();

            GetWorldTimerManager().ClearTimer(TimeHandle_StartCapturingDelay);
            TimeHandle_StartCapturingDelay.Invalidate();

            // Reset the counter and stats
            ResetCounter();
            // bIsActive is public. we need to copy bIsActive state into the protected value.
            CurrentState = ENVSceneCapturerState::Running;
            // NOTE: Make it wait till the next frame to start exporting since the scene capturer only just start capturing now
            StartCapturingTimestamp = GetWorld()->GetRealTimeSeconds();
            LastCaptureTimestamp = StartCapturingTimestamp + 0.1f;

            // Let all the viewpoint component start capturing
            for (UNVSceneCapturerViewpointComponent* ViewpointComp : ViewpointList)
            {
                ViewpointComp->StartCapturing();
            }
        }
    }
}

void ANVSceneCapturerActor::StopCapturing()
{
    ensure(CurrentState != ENVSceneCapturerState::NotActive);
    ensure(CurrentState != ENVSceneCapturerState::Active);

    GetWorldTimerManager().ClearTimer(TimeHandle_StartCapturingDelay);
    TimeHandle_StartCapturingDelay.Invalidate();

    for (UNVSceneCapturerViewpointComponent* ViewpointComp : ViewpointList)
    {
        ViewpointComp->StopCapturing();
    }

    CurrentState = ENVSceneCapturerState::Active;

    OnStoppedEvent.Broadcast(this);

    if (SceneDataHandler)
    {
        SceneDataHandler->OnStopCapturingSceneData();
    }
    ResetCounter();
}

void ANVSceneCapturerActor::PauseCapturing()
{
    if (CurrentState == ENVSceneCapturerState::Running)
    {
        CurrentState = ENVSceneCapturerState::Paused;
    }
}

void ANVSceneCapturerActor::ResumeCapturing()
{
    if (CurrentState == ENVSceneCapturerState::Paused)
    {
        CurrentState = ENVSceneCapturerState::Running;
    }
}

void ANVSceneCapturerActor::OnCompleted()
{
    if (CurrentState == ENVSceneCapturerState::Running)
    {
        const float CompletedCapturingTimestamp = GetWorld()->GetRealTimeSeconds();
        const float CapturingDuration = CompletedCapturingTimestamp - StartCapturingTimestamp;

        UE_LOG(LogNVSceneCapturer, Warning, TEXT("Capturing completed!!!\nTotal capturing duration: %.6f\nStart time: %.6f\nCompleted time: %.6f"),
               CapturedDuration, StartCapturingTimestamp, CompletedCapturingTimestamp);

        for (UNVSceneCapturerViewpointComponent* ViewpointComp : ViewpointList)
        {
            ViewpointComp->StopCapturing();
        }

        CurrentState = ENVSceneCapturerState::Completed;

        if (SceneDataHandler)
        {
            SceneDataHandler->OnCapturingCompleted();
        }

        OnCompletedEvent.Broadcast(this, true);
    }
}

bool ANVSceneCapturerActor::ToggleTakeOverViewport()
{
    bTakingOverViewport = !bTakingOverViewport;
    if (bTakingOverViewport)
    {
        TakeOverViewport();
    }
    else
    {
        ReturnViewportToPlayerController();
    }
    return bTakingOverViewport;
}

void ANVSceneCapturerActor::TakeOverViewport()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        AActor* CurrentPCViewTarget = PlayerController->GetViewTarget();
        if (CurrentPCViewTarget != this)
        {
            CachedPlayerControllerViewTarget = CurrentPCViewTarget;
        }
        PlayerController->SetViewTarget(this);
    }
}

void ANVSceneCapturerActor::ReturnViewportToPlayerController()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        AActor* CurrentPCViewTarget = PlayerController->GetViewTarget();
        if (CurrentPCViewTarget == this)
        {
            PlayerController->SetViewTarget(CachedPlayerControllerViewTarget);
            CachedPlayerControllerViewTarget = nullptr;
        }
    }
}

float ANVSceneCapturerActor::GetCapturedFPS() const
{
    return CapturedFrameCounter.GetFPS();
}

int32 ANVSceneCapturerActor::GetNumberOfFramesLeftToCapture() const
{
    // NOTE: -1 mean there is infinite number of frames need to be captured
    return (NumberOfFramesToCapture > 0) ? (NumberOfFramesToCapture - CapturedFrameCounter.GetTotalFrameCount()) : -1;
}

float ANVSceneCapturerActor::GetEstimatedTimeUntilFinishCapturing() const
{
    float EstRemainTime = -1.f;

    const float CapturedProgressFraction = GetCaptureProgressFraction();
    if (CapturedProgressFraction < 1.f)
    {
        const float FullTimeEstimated = (CapturedProgressFraction > 0.f) ? (CapturedDuration / CapturedProgressFraction) : 0.f;
        EstRemainTime = FullTimeEstimated * (1 - CapturedProgressFraction);
    }

    return EstRemainTime;
}

float ANVSceneCapturerActor::GetCaptureProgressFraction() const
{
    return (NumberOfFramesToCapture > 0) ? (float(CapturedFrameCounter.GetTotalFrameCount()) / NumberOfFramesToCapture) : -1.f;
}

float ANVSceneCapturerActor::GetCapturedDuration() const
{
    return CapturedDuration;
}

int32 ANVSceneCapturerActor::GetExportedFrameCount() const
{
    UNVSceneDataExporter* CurrentSceneDataExporter = Cast<UNVSceneDataExporter>(SceneDataHandler);
    if (CurrentSceneDataExporter)
    {
        uint32 PendingToExportImagesCount = CurrentSceneDataExporter->GetPendingToExportImagesCount();
        uint32 PendingToExportFrameCount = (ImageToCapturePerFrame > 0) ? FMath::CeilToFloat(float(PendingToExportImagesCount) / ImageToCapturePerFrame) : PendingToExportImagesCount;
        uint32 CapturedFrameCount = GetCapturedFrameCounter().GetTotalFrameCount();
        return (PendingToExportFrameCount <= CapturedFrameCount) ? (CapturedFrameCount - PendingToExportFrameCount) : 0;
    }

    return 0;
}

TArray<FNVNamedImageSizePreset> const& ANVSceneCapturerActor::GetImageSizePresets()
{
    return GetDefault<ANVSceneCapturerActor>()->ImageSizePresets;
}

TArray<UNVSceneCapturerViewpointComponent*> ANVSceneCapturerActor::GetViewpointList()
{
    // TODO: Should only update viewpoint list if there are viewpoint components added or removed
    UpdateViewpointList();

    return ViewpointList;
}

UNVSceneDataHandler* ANVSceneCapturerActor::GetSceneDataHandler() const
{
	return SceneDataHandler;
}

UNVSceneDataVisualizer* ANVSceneCapturerActor::GetSceneDataVisualizer() const
{
	return SceneDataVisualizer;
}

void ANVSceneCapturerActor::UpdateViewpointList()
{
    ensure(CurrentState != ENVSceneCapturerState::Running);
    ensure(CurrentState != ENVSceneCapturerState::Paused);
    if ((CurrentState != ENVSceneCapturerState::Running) &&
            (CurrentState != ENVSceneCapturerState::Paused))
    {
        // Keep track of all the child viewpoint components
		TArray<UNVSceneCapturerViewpointComponent*> ChildComponents;
		this->GetComponents<UNVSceneCapturerViewpointComponent>(ChildComponents);
        ViewpointList.Reset(ChildComponents.Num());
        for (auto CheckViewpoint : ChildComponents)
        {
            if (CheckViewpoint)
            {
                ViewpointList.Add(CheckViewpoint);
            }
        }

        ViewpointList.Sort([](const UNVSceneCapturerViewpointComponent& A, const UNVSceneCapturerViewpointComponent& B)
        {
            return A.GetDisplayName() < B.GetDisplayName();
        });
    }

    // Count the number of images we need to capture and export every frame
    ImageToCapturePerFrame = 0;
    for (const auto& CheckViewpointComp : ViewpointList)
    {
        if (CheckViewpointComp)
        {
            for (const auto& CheckSceneFeatureExtractor : CheckViewpointComp->FeatureExtractorList)
            {
                const UNVSceneFeatureExtractor_PixelData* FeatureExtractorScenePixels = Cast<UNVSceneFeatureExtractor_PixelData>(CheckSceneFeatureExtractor);
                if (FeatureExtractorScenePixels && FeatureExtractorScenePixels->IsEnabled())
                {
                    ImageToCapturePerFrame++;
                }
            }
        }
    }
}

bool ANVSceneCapturerActor::CanHandleMoreSceneData() const
{
    return (SceneDataHandler && SceneDataHandler->CanHandleMoreData());
}