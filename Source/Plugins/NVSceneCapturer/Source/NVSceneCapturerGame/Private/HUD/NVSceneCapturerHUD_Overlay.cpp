/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerHUD_Overlay.h"
#include "NVSceneCapturerPlayerController.h"
#include "HUD/NVSceneCapturerUIUtils.h"
#include "HUD/NVSceneCapturerHUD.h"
#include "HUD/NVSceneCapturerHUD_ExporterControlPanel.h"
#include "HUD/NVSceneCapturerHUD_PIPPanel.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "EngineMinimal.h"
#include "Engine/TextureRenderTarget2D.h"

UNVSceneCapturerHUD_Overlay::UNVSceneCapturerHUD_Overlay(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNVSceneCapturerHUD_Overlay::Update()
{
    TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
    if (!SafeWidget.IsValid())
    {
        return;
    }

    DEFINE_UI_WIDGET(UNVSceneCapturerHUD_ExporterControlPanel, ExporterControlPanel);
    if (ExporterControlPanel)
    {
        ExporterControlPanel->Update();
    }

    DEFINE_UI_WIDGET(UNVSceneCapturerHUD_PIPPanel, PIPPanel);
    if (PIPPanel)
    {
        PIPPanel->Update();
    }
}

void UNVSceneCapturerHUD_Overlay::ToggleExporterControlPanel()
{
    DEFINE_UI_WIDGET(UPanelWidget, ExporterControlContainer);
    if (ExporterControlContainer)
    {
        const ESlateVisibility CurrentWidgetVisibility = ExporterControlContainer->GetVisibility();
        const ESlateVisibility NewWidgetVisibility = (CurrentWidgetVisibility == ESlateVisibility::Hidden) ?
                ESlateVisibility::Visible : ESlateVisibility::Hidden;
        ExporterControlContainer->SetVisibility(NewWidgetVisibility);
    }
}

FReply UNVSceneCapturerHUD_Overlay::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FReply Reply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

    FKey MouseKey = InMouseEvent.GetEffectingButton();
    if (MouseKey == EKeys::MiddleMouseButton)
    {
        ANVSceneCapturerPlayerController* PC = Cast<ANVSceneCapturerPlayerController>(GetOwningPlayer());
        if (PC)
        {
            PC->ToggleCursorMode();

            return FReply::Handled().ClearUserFocus();
        }
    }

    return Reply;
}
