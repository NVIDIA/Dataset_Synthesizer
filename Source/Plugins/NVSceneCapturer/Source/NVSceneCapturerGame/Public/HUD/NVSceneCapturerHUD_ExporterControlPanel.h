/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerHUD_ExporterControlPanel.generated.h"

class UPanelWidget;
class ANVSceneCapturerActor;

/**
 *
 */
UCLASS()
class NVSCENECAPTURERGAME_API UNVSceneCapturerHUD_ExporterControlPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UNVSceneCapturerHUD_ExporterControlPanel(const FObjectInitializer& ObjectInitializer);

public:
    // UWidget interface
    virtual void SynchronizeProperties() override;
    // End of UWidget interface

    void Update();

    ANVSceneCapturerActor* GetActiveExporterActor() const;
    FString GetOutputDirectory() const;

protected:
	// NOTE: All these function need to be marked with UFUNCTION so they can be used with the Slate widget event
	UFUNCTION()
    void OnStartExporting_Clicked();
	UFUNCTION()
    void OnStopExporting_Clicked();
	UFUNCTION()
    void OnPauseExporting_Clicked();
	UFUNCTION()
    void OnResumeExporting_Clicked();
	UFUNCTION()
    void OnCompleted_Clicked();
	UFUNCTION()
    void OnToggleExporterViewport_Clicked();
	UFUNCTION()
    void OnOpenOutputDirectory_Clicked();

    UPanelWidget* GetPanelOfState(ENVSceneCapturerState ExporterState) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
    FText TitleText;

protected: // Transient properties
    UPROPERTY(Transient)
    ANVSceneCapturerActor* ActiveCapturerActor;

    UPROPERTY(Transient)
    class UNVSceneDataExporter* SceneDataExporter;

    UPROPERTY(Transient)
    UPanelWidget* CurrentActiveStatePanel;

    UPROPERTY(Transient)
    UPanelWidget* StatePanelList[(uint8)ENVSceneCapturerState::NVSceneCapturerState_MAX];
};
