/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerPlayerController.h"
#include "NVSceneCapturerGameModeBase.h"
#include "HUD/NVSceneCapturerHUD.h"
#include "HUD/NVSceneCapturerHUD_Overlay.h"
#include "NVSceneCapturerActor.h"
#include "EngineUtils.h"
#include "Engine/EngineTypes.h"

ANVSceneCapturerPlayerController::ANVSceneCapturerPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bShouldPerformFullTickWhenPaused = true;
}

void ANVSceneCapturerPlayerController::BeginPlay()
{
    Super::BeginPlay();
}

void ANVSceneCapturerPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);
    if (InputComponent)
    {
#define BIND_INPUT_ACTION(ActionName, BindInputEvent, FunctionToBind, ShouldExecuteWhenPaused) \
    {\
        FInputActionBinding& NewInputAction = InputComponent->BindAction(ActionName, BindInputEvent, this, &ANVSceneCapturerPlayerController::FunctionToBind);\
        NewInputAction.bExecuteWhenPaused = ShouldExecuteWhenPaused; \
    }

        BIND_INPUT_ACTION("ToggleHUDOverlay", IE_Released, ToggleHUDOverlay, true);
        BIND_INPUT_ACTION("ToggleCursorMode", IE_Released, ToggleCursorMode, true);
        BIND_INPUT_ACTION("ToggleExporterManagement", IE_Released, ToggleExporterManagement, true);
        BIND_INPUT_ACTION("ToggleShowExportActorDebug", IE_Released, ToggleShowExportActorDebug, true);
        BIND_INPUT_ACTION("ToggleTakeOverViewport", IE_Released, ToggleTakeOverViewport, true);
        BIND_INPUT_ACTION("TogglePause", IE_Released, TogglePause, true);
#undef BIND_INPUT_ACTION
    }
}

void ANVSceneCapturerPlayerController::ToggleHUDOverlay()
{
    ANVSceneCapturerHUD* GameHUD = Cast<ANVSceneCapturerHUD>(GetHUD());
    if (GameHUD)
    {
        GameHUD->ToggleOverlay();
    }
}

void ANVSceneCapturerPlayerController::ToggleShowExportActorDebug()
{
    ANVSceneCapturerHUD* GameHUD = Cast<ANVSceneCapturerHUD>(GetHUD());
    if (GameHUD)
    {
        GameHUD->ToggleShowExportActorDebug();
    }
}

void ANVSceneCapturerPlayerController::ToggleTakeOverViewport()
{
    UWorld* World = GetWorld();
    if (World)
    {
        for (TActorIterator<ANVSceneCapturerActor> It(World); It; ++It)
        {
            ANVSceneCapturerActor* CheckExporter = *It;
            if (CheckExporter)
            {
                CheckExporter->ToggleTakeOverViewport();
                break;
            }
        }
    }
}

void ANVSceneCapturerPlayerController::TogglePause()
{
    Pause();
}

void ANVSceneCapturerPlayerController::ToggleCursorMode()
{
    bShowMouseCursor = !bShowMouseCursor;

    if (bShowMouseCursor)
    {
        FInputModeUIOnly UIInputMode;
        UIInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
        SetInputMode(UIInputMode);
    }
    else
    {
        FInputModeGameOnly GameInputMode;
        GameInputMode.SetConsumeCaptureMouseDown(true);
        SetInputMode(GameInputMode);
    }
}

void ANVSceneCapturerPlayerController::ToggleExporterManagement()
{
    ANVSceneCapturerHUD* GameHUD = Cast<ANVSceneCapturerHUD>(GetHUD());
    UNVSceneCapturerHUD_Overlay* HUDOverlay = (GameHUD ? GameHUD->GetHUDOverlay() : nullptr);
    if (HUDOverlay)
    {
        HUDOverlay->ToggleExporterControlPanel();
    }
}
