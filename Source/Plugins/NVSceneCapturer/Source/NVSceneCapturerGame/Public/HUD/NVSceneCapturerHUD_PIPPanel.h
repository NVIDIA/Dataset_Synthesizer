/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NVSceneCapturerHUD_PIPPanel.generated.h"

/**
 *
 */
UCLASS()
class NVSCENECAPTURERGAME_API UNVSceneCapturerHUD_PIPPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UNVSceneCapturerHUD_PIPPanel(const FObjectInitializer& ObjectInitializer);

    // UWidget interface
    virtual void SynchronizeProperties() override;
    // End of UWidget interface

    void Update();

protected:
    void UpdateChannels();

	UFUNCTION()
    void OnChannelSelected(FString SelectedValue, ESelectInfo::Type type);

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = "PIP Panel")
    bool bUseRenderTargetForVisualization;

    /** The material used for visualizing the captured image buffer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneDataVisualizer")
    UMaterialInterface* VisualizerMaterial;

protected: // Transient properties
    UPROPERTY(Transient)
    class ANVSceneCapturerActor* ActiveCapturerActor;

    UPROPERTY(Transient)
    class UNVSceneDataVisualizer* SceneDataVisualizer;

    UPROPERTY(Transient)
    class UMaterialInstanceDynamic* VizMaterialDynamic;
};
