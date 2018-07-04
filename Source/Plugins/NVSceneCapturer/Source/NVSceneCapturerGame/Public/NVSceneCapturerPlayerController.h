/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NVSceneCapturerPlayerController.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class NVSCENECAPTURERGAME_API ANVSceneCapturerPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ANVSceneCapturerPlayerController(const FObjectInitializer& ObjectInitializer);

    virtual void BeginPlay() final;

public: // Editor properties
    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void ToggleCursorMode();

    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void ToggleExporterManagement();

    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void ToggleHUDOverlay();

    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void ToggleShowExportActorDebug();

    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void ToggleTakeOverViewport();

    UFUNCTION(exec, BlueprintCallable, Category = "NVSceneCapturerPlayerController")
    void TogglePause();

protected: // Override
    virtual void SetupInputComponent() override;
};
