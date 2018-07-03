/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NVSceneCapturerHUD_Overlay.generated.h"

/**
 *
 */
UCLASS()
class NVSCENECAPTURERGAME_API UNVSceneCapturerHUD_Overlay : public UUserWidget
{
    GENERATED_BODY()

public:
    UNVSceneCapturerHUD_Overlay(const FObjectInitializer& ObjectInitializer);

    void Update();

    void ToggleExporterControlPanel();

protected: // Transient properties
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
