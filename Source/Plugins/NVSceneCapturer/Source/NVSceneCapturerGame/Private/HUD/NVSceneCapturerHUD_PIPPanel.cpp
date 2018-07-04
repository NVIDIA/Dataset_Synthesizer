/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerHUD_PIPPanel.h"
#include "NVSceneCapturerGameModeBase.h"
#include "NVSceneCapturerPlayerController.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerActor.h"
#include "HUD/NVSceneCapturerUIUtils.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/ComboBoxString.h"
#include "Kismet/KismetStringLibrary.h"
#include "EngineMinimal.h"
#include "Rendering/DrawElements.h"

UNVSceneCapturerHUD_PIPPanel::UNVSceneCapturerHUD_PIPPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNVSceneCapturerHUD_PIPPanel::SynchronizeProperties()
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
        SceneDataVisualizer = ActiveCapturerActor ? ActiveCapturerActor->GetSceneDataVisualizer() : nullptr;
    }

    if (!SceneDataVisualizer)
    {
        SetVisibility(ESlateVisibility::Hidden);
    }

    DEFINE_UI_WIDGET(UImage, imgViz);
    if (imgViz)
    {
        if (!VizMaterialDynamic && VisualizerMaterial)
        {
            VizMaterialDynamic = UMaterialInstanceDynamic::Create(VisualizerMaterial, this, FName(TEXT("VizMaterialDynamic")));
            imgViz->SetBrushFromMaterial(VisualizerMaterial);
        }
    }

    DEFINE_UI_WIDGET(UComboBoxString, cbChannel);
    if (cbChannel)
    {
        cbChannel->OnSelectionChanged.AddDynamic(this, &UNVSceneCapturerHUD_PIPPanel::OnChannelSelected);

        UpdateChannels();
    }

    // NOTE: Disable the PIP panel for now since we have some problem with the render target textures
    SetVisibility(ESlateVisibility::Hidden);
    //SetVisibility(ESlateVisibility::Visible);
}

void UNVSceneCapturerHUD_PIPPanel::UpdateChannels()
{
    if (!SceneDataVisualizer)
    {
        return;
    }

    DEFINE_UI_WIDGET(UComboBoxString, cbChannel);
    if (cbChannel)
    {
        const FString CurrentSelectedOption = cbChannel->GetSelectedOption();
        TArray<FString> VizNameList = SceneDataVisualizer->GetVizNameList();
        for (auto VizName : VizNameList)
        {
            int32 OptionIndex = cbChannel->FindOptionIndex(VizName);
            if (OptionIndex < 0)
            {
                cbChannel->AddOption(VizName);
            }
        }

        // Automatically select the first option
        if ((VizNameList.Num() > 0) && CurrentSelectedOption.IsEmpty())
        {
            const FString& FirstOption = cbChannel->GetOptionAtIndex(0);
            cbChannel->SetSelectedOption(FirstOption);
        }
        else
        {
            cbChannel->RefreshOptions();
        }
    }
}

void UNVSceneCapturerHUD_PIPPanel::Update()
{
    TSharedPtr<SWidget> SafeWidget = GetCachedWidget();
    if (!SafeWidget.IsValid())
    {
        return;
    }

    // TODO: Should support multiple exporters, make different tab for different exporters
    if (!SceneDataVisualizer)
    {
        return;
    }

    UpdateChannels();
}

void UNVSceneCapturerHUD_PIPPanel::OnChannelSelected(FString SelectedValue, ESelectInfo::Type type)
{
    if (!SceneDataVisualizer)
    {
        return;
    }

    UTextureRenderTarget2D* SelectedVizTexture = SceneDataVisualizer->GetTexture(SelectedValue);
    if (SelectedVizTexture)
    {
        DEFINE_UI_WIDGET(UImage, imgViz);
        if (imgViz)
        {
            if (VizMaterialDynamic)
            {
                static const FName TextureParam_Name = FName(TEXT("SourceTexture"));
                VizMaterialDynamic->SetTextureParameterValue(TextureParam_Name, SelectedVizTexture);
                imgViz->SetBrushFromMaterial(VisualizerMaterial);
            }
            else
            {
                FSlateBrush NewBrush;
                NewBrush.SetResourceObject(SelectedVizTexture);
                NewBrush.ImageSize.X = SelectedVizTexture->GetSurfaceWidth();
                NewBrush.ImageSize.Y = SelectedVizTexture->GetSurfaceHeight();
                imgViz->SetBrush(NewBrush);
            }
        }
    }
}
