/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerHUD_ExporterControlPanel.h"
#include "NVSceneCapturerGameModeBase.h"
#include "NVSceneCapturerPlayerController.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneManager.h"
#include "HUD/NVSceneCapturerUIUtils.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetStringLibrary.h"
#include "EngineMinimal.h"
#include "Rendering/DrawElements.h"

UNVSceneCapturerHUD_ExporterControlPanel::UNVSceneCapturerHUD_ExporterControlPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CurrentActiveStatePanel = nullptr;
}

void UNVSceneCapturerHUD_ExporterControlPanel::SynchronizeProperties()
{
    TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
    if (!SafeWidget.IsValid())
    {
        return;
    }

    Super::SynchronizeProperties();

    // Select a valid exporter
    ANVSceneCapturerGameModeBase* NVGameMode = Cast<ANVSceneCapturerGameModeBase>(GetWorld()->GetAuthGameMode());
    if (NVGameMode)
    {
        ActiveCapturerActor = NVGameMode->GetFirstActiveCapturer();
        if (ActiveCapturerActor)
        {
            SceneDataExporter = Cast<UNVSceneDataExporter>(ActiveCapturerActor->GetSceneDataHandler());
        }
    }

    // Set the number of scenes to export to the text box
    DEFINE_UI_WIDGET(USpinBox, spbTotalNumberOfScenesToExport);
    if (spbTotalNumberOfScenesToExport)
    {
        int32 TotalNumberOfScenesToCapture = ActiveCapturerActor ? ActiveCapturerActor->GetNumberOfFramesToCapture() : 0;
        spbTotalNumberOfScenesToExport->SetValue(TotalNumberOfScenesToCapture);
    }

    // Handle button events
    DEFINE_UI_WIDGET(UButton, btnStartExporting);
    if (btnStartExporting)
    {
        btnStartExporting->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnStartExporting_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnStopExporting);
    if (btnStopExporting)
    {
        btnStopExporting->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnStopExporting_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnPauseExporting);
    if (btnPauseExporting)
    {
        btnPauseExporting->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnPauseExporting_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnResumeExporting);
    if (btnResumeExporting)
    {
        btnResumeExporting->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnResumeExporting_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnCompleted);
    if (btnCompleted)
    {
        btnCompleted->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnCompleted_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnToggleExporterViewport);
    if (btnToggleExporterViewport)
    {
        btnToggleExporterViewport->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnToggleExporterViewport_Clicked);
    }

    DEFINE_UI_WIDGET(UButton, btnOpenOutputDir);
    if (btnOpenOutputDir)
    {
        btnOpenOutputDir->OnClicked.AddDynamic(this, &UNVSceneCapturerHUD_ExporterControlPanel::OnOpenOutputDirectory_Clicked);
    }

    DEFINE_UI_WIDGET(UPanelWidget, pnNotStartedState);
    DEFINE_UI_WIDGET(UPanelWidget, pnRunningState);
    DEFINE_UI_WIDGET(UPanelWidget, pnPausedState);
    DEFINE_UI_WIDGET(UPanelWidget, pnCompletedState);
    StatePanelList[(uint8)ENVSceneCapturerState::Active] = pnNotStartedState;
    StatePanelList[(uint8)ENVSceneCapturerState::Running] = pnRunningState;
    StatePanelList[(uint8)ENVSceneCapturerState::Paused] = pnPausedState;
    StatePanelList[(uint8)ENVSceneCapturerState::Completed] = pnCompletedState;
}

void UNVSceneCapturerHUD_ExporterControlPanel::Update()
{
    TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
    if (!SafeWidget.IsValid())
    {
        return;
    }

    // TODO: Should support multiple exporters, make different tab for different exporters
    if (!ActiveCapturerActor)
    {
        return;
    }

	const FString& FullOutputDirPath = GetOutputDirectory();;
    const FString ExportedFolderPath = SceneDataExporter ? SceneDataExporter->GetConfiguredOutputDirectoryName(): TEXT("");

    const uint64 CapturerCounter = ActiveCapturerActor->GetCapturedFrameCounter().GetTotalFrameCount();
	// TODO: Get the exported frame count from the UNVSceneDataExporter
	const int32 ExportedFrameCount = 0;
    const int32 NumberOfScenesToExport = ActiveCapturerActor->GetNumberOfFramesToCapture();
    const ENVSceneCapturerState ExporterState = ActiveCapturerActor->GetCurrentState();
    const float CapturedFPS = ActiveCapturerActor->GetCapturedFPS();
    const float EstRemainTime = ActiveCapturerActor->GetEstimatedTimeUntilFinishCapturing();
    const FString RemainTimeStr = NVSceneCapturerUIUtils::ConvertTimeSecondsToString(EstRemainTime);
    const float CapturingDuration = ActiveCapturerActor->GetCapturedDuration();
    const FString CapturingDurationStr = NVSceneCapturerUIUtils::ConvertTimeSecondsToString(CapturingDuration);

    // Update visibility of state panel
    UPanelWidget* NewActiveStatePanel = GetPanelOfState(ExporterState);
    if (NewActiveStatePanel != CurrentActiveStatePanel)
    {
        // Hide old panel
        if (CurrentActiveStatePanel)
        {
            CurrentActiveStatePanel->SetVisibility(ESlateVisibility::Hidden);
        }

        CurrentActiveStatePanel = NewActiveStatePanel;

        // Show new panel
        if (CurrentActiveStatePanel)
        {
            CurrentActiveStatePanel->SetVisibility(ESlateVisibility::Visible);
        }
    }

    DEFINE_UI_WIDGET(UTextBlock, txtSceneExporterInfo);
    if (txtSceneExporterInfo)
    {
        // TODO: Use FText and parameters
        FString SceneExporterInfoStr = FString::Printf(
                                           TEXT("Export folder: %s\nCaptured frame count: %d / %d\nExported frame count: %d\nFPS: %.1f\nCapturing time: %s\nEstimated remain time: %s\n"),
                                           *ExportedFolderPath, CapturerCounter, NumberOfScenesToExport, ExportedFrameCount, CapturedFPS, *CapturingDurationStr, *RemainTimeStr);

        FString ExporterStateStr = FString::Printf(TEXT("State: %s\n"), *NVSceneCapturerStateString::ConvertExporterStateToString(ExporterState));

        FString InfoStr = SceneExporterInfoStr + ExporterStateStr;

        txtSceneExporterInfo->SetText(FText::FromString(InfoStr));
    }

    DEFINE_UI_WIDGET(UTextBlock, txtFullOutputDirPath);
    if (txtFullOutputDirPath)
    {
        txtFullOutputDirPath->SetText(FText::FromString(FullOutputDirPath));
    }

    DEFINE_UI_WIDGET(UProgressBar, pbCaptureProgress);
    if (pbCaptureProgress)
    {
        const float CapturePercent = ActiveCapturerActor->GetCaptureProgressFraction();

        if (CapturePercent > 0.f)
        {
            pbCaptureProgress->SetPercent(CapturePercent);
            pbCaptureProgress->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            pbCaptureProgress->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

ANVSceneCapturerActor* UNVSceneCapturerHUD_ExporterControlPanel::GetActiveExporterActor() const
{
    return ActiveCapturerActor;
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnStartExporting_Clicked()
{
    if (ActiveCapturerActor)
    {
        DEFINE_UI_WIDGET(USpinBox, spbTotalNumberOfScenesToExport);
        if (spbTotalNumberOfScenesToExport)
        {
            int32 NumberOfScenesToExport = FMath::FloorToInt(spbTotalNumberOfScenesToExport->GetValue());
            ActiveCapturerActor->SetNumberOfFramesToCapture(NumberOfScenesToExport);
        }
        ANVSceneManager* ANVSceneManagerPtr = ANVSceneManager::GetANVSceneManagerPtr();
        if (ANVSceneManagerPtr)
        {
            ANVSceneManagerPtr->ResetState();
        }
        ActiveCapturerActor->StartCapturing();
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnStopExporting_Clicked()
{
    if (ActiveCapturerActor)
    {
        ActiveCapturerActor->StopCapturing();
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnPauseExporting_Clicked()
{
    if (ActiveCapturerActor)
    {
        ActiveCapturerActor->PauseCapturing();
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnResumeExporting_Clicked()
{
    if (ActiveCapturerActor)
    {
        ActiveCapturerActor->ResumeCapturing();
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnCompleted_Clicked()
{
    if (ActiveCapturerActor)
    {
        ActiveCapturerActor->StopCapturing();
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnToggleExporterViewport_Clicked()
{
    if (ActiveCapturerActor)
    {
        ANVSceneCapturerActor* AsyncExporterActor = Cast<ANVSceneCapturerActor>(ActiveCapturerActor);
        if (AsyncExporterActor)
        {
            AsyncExporterActor->ToggleTakeOverViewport();
        }
    }
}

void UNVSceneCapturerHUD_ExporterControlPanel::OnOpenOutputDirectory_Clicked()
{
    if (ActiveCapturerActor)
    {
        const FString& FullOutputDirPath = GetOutputDirectory();
        FPlatformProcess::ExploreFolder(*FullOutputDirPath);
    }
}

FString UNVSceneCapturerHUD_ExporterControlPanel::GetOutputDirectory() const
{
    if (SceneDataExporter)
    {
		const auto FullPath = SceneDataExporter->GetFullOutputDirectoryPath();
		if (!FullPath.IsEmpty())
		{
			return FPaths::ConvertRelativePathToFull(FullPath);
		}
		else
		{
			return FullPath;
		}
    }
    return TEXT("");
}

UPanelWidget* UNVSceneCapturerHUD_ExporterControlPanel::GetPanelOfState(ENVSceneCapturerState ExporterState) const
{
    uint8 StateIndex = (uint8)ExporterState;
    if (StateIndex < (uint8)ENVSceneCapturerState::NVSceneCapturerState_MAX)
    {
        return StatePanelList[StateIndex];
    }

    return nullptr;
}
