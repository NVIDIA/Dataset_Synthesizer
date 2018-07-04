/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerCustomization.h"
#include "NVSceneCapturerEditorModule.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "Engine.h"
#include "Core.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "ObjectEditorUtils.h"
#include "EditorCategoryUtils.h"
#include "ScopedTransaction.h"
#include "SlateExtras.h"
#include "PropertyHandle.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "ImageSizeCustomization"

//======================================== FNVSceneCapturerDetails ========================================
FNVSceneCapturerDetails::FNVSceneCapturerDetails()
{
    // TODO:
}

TSharedRef<IDetailCustomization> FNVSceneCapturerDetails::MakeInstance()
{
    return MakeShareable(new FNVSceneCapturerDetails);
}

void FNVSceneCapturerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> WeakObjectsBeingCustomized;
    DetailBuilder.GetObjectsBeingCustomized(WeakObjectsBeingCustomized);

    for (TWeakObjectPtr<UObject>& WeakObjectBeingCustomized : WeakObjectsBeingCustomized)
    {
        if (UObject* ObjectBeingCustomized = WeakObjectBeingCustomized.Get())
        {
            if (ANVSceneCapturerActor* CapturerActor = Cast<ANVSceneCapturerActor>(ObjectBeingCustomized))
            {
                // TODO:
                TArray<UNVSceneCapturerViewpointComponent*> ViewpointList = CapturerActor->GetViewpointList();
                if (ViewpointList.Num() > 0)
                {

                    IDetailCategoryBuilder& ViewpointCategory = DetailBuilder.EditCategory("Viewpoint");
                    ViewpointCategory.InitiallyCollapsed(false);
                    TArray<UObject*> ViewpointObjList;
                    ViewpointObjList.SetNum(ViewpointList.Num());
                    for (int i = 0; i < ViewpointList.Num(); i++)
                    {
                        ViewpointObjList[i] = Cast<UObject>(ViewpointList[i]);
                    }
                }
                break;
            }
        }
    }
}

void FNVSceneCapturerDetails::GenerateViewpointInfo(TSharedRef<IPropertyHandle> InPropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
    IDetailPropertyRow& ViewpointInfoRow = ChildrenBuilder.AddProperty(InPropertyHandle);
    ViewpointInfoRow.ShowPropertyButtons(false);

    UObject* ViewpointObj = nullptr;
    InPropertyHandle->GetValue(ViewpointObj);
    UNVSceneCapturerViewpointComponent* ViewpointComp = Cast<UNVSceneCapturerViewpointComponent>(ViewpointObj);
    if (ViewpointComp)
    {
        // ToDo: implementation.
    }
}

UNVSceneCapturerViewpointComponent* FNVSceneCapturerDetails::GetViewpointComponent(TSharedRef<IPropertyHandle> InPropertyHandle) const
{
    UObject* ViewpointCompObj = nullptr;
    InPropertyHandle->GetValue(ViewpointCompObj);
    UNVSceneCapturerViewpointComponent* ViewpointComponent = Cast<UNVSceneCapturerViewpointComponent>(ViewpointCompObj);

    return ViewpointComponent;
}

ECheckBoxState FNVSceneCapturerDetails::IsViewpointEnabled(TSharedRef<IPropertyHandle> InPropertyHandle) const
{
    ECheckBoxState ResultState = ECheckBoxState::Undetermined;

    const UNVSceneCapturerViewpointComponent* ViewpointComp = GetViewpointComponent(InPropertyHandle);
    if (ViewpointComp)
    {
        ResultState = ViewpointComp->Settings.bIsEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }

    return ResultState;
}

void FNVSceneCapturerDetails::HandleViewpointEnabledStateChanged(ECheckBoxState NewValue, TSharedRef<IPropertyHandle> InPropertyHandle)
{
    UNVSceneCapturerViewpointComponent* ViewpointComp = GetViewpointComponent(InPropertyHandle);
    if (ViewpointComp)
    {
        ViewpointComp->Settings.bIsEnabled = (NewValue == ECheckBoxState::Checked) ? true : false;
    }
}

FText FNVSceneCapturerDetails::GetViewpointName(TSharedRef<IPropertyHandle> InPropertyHandle) const
{
    FText ViewpointName = FText::GetEmpty();
    UNVSceneCapturerViewpointComponent* ViewpointComp = GetViewpointComponent(InPropertyHandle);
    if (ViewpointComp)
    {
        ViewpointName = FText::FromString(ViewpointComp->GetDisplayName());
    }

    return ViewpointName;
}

void FNVSceneCapturerDetails::OnSelectViewpointComponentClicked(TSharedRef<IPropertyHandle> InPropertyHandle)
{
    UNVSceneCapturerViewpointComponent* ViewpointComp = GetViewpointComponent(InPropertyHandle);
    if (ViewpointComp)
    {
        GEditor->GetSelectedComponents()->Select(ViewpointComp, true);
        GSelectedComponentAnnotation.Set(ViewpointComp);
    }
}

//======================================== FNVFeatureExtractorSettingsCustomization ========================================
FNVFeatureExtractorSettingsCustomization::FNVFeatureExtractorSettingsCustomization()
{
}

TSharedRef<IPropertyTypeCustomization> FNVFeatureExtractorSettingsCustomization::MakeInstance()
{
    return MakeShareable(new FNVFeatureExtractorSettingsCustomization);
}

void FNVFeatureExtractorSettingsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    TSharedPtr<IPropertyHandle> FeatureExtractorPropertyHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNVFeatureExtractorSettings, FeatureExtractorRef));

    TSharedPtr<SHorizontalBox> HeaderWidget = SNew(SHorizontalBox);

    HeaderProperties.Reset();
    bool bIsFeatureExtractorClassValid = false;
    // If the FeatureExtractor class already picked then we can show its inner properties on the header
    if (FeatureExtractorPropertyHandle.IsValid())
    {
        TSharedPtr<IPropertyHandle> IsEnabledPropertyHandle = FeatureExtractorPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UNVSceneFeatureExtractor, bIsEnabled));
        TSharedPtr<IPropertyHandle> DisplayNamePropertyHandle = FeatureExtractorPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UNVSceneFeatureExtractor, DisplayName));

        if (IsEnabledPropertyHandle.IsValid() && DisplayNamePropertyHandle.IsValid())
        {
            bIsFeatureExtractorClassValid = true;
            HeaderProperties.Add(IsEnabledPropertyHandle);
            HeaderProperties.Add(DisplayNamePropertyHandle);

            FText FeatureExtractorDescriptionText;

            const UObject* PropObjectValue = nullptr;
            FeatureExtractorPropertyHandle->GetValue(PropObjectValue);
            const UNVSceneFeatureExtractor* EditingObject = Cast<UNVSceneFeatureExtractor>(PropObjectValue);
            if (EditingObject)
            {
                FeatureExtractorDescriptionText = FText::FromString(EditingObject->Description);
            }
            DisplayNamePropertyHandle->SetToolTipText(FeatureExtractorDescriptionText);
            HeaderWidget->SetToolTipText(FeatureExtractorDescriptionText);
        }
    }

    if (!bIsFeatureExtractorClassValid)
    {
        HeaderProperties.Add(FeatureExtractorPropertyHandle);
    }

    for (auto CheckHeaderProperty : HeaderProperties)
    {
        if (CheckHeaderProperty.IsValid())
        {
            HeaderWidget->AddSlot()
            .AutoWidth()
            [
                CheckHeaderProperty->CreatePropertyValueWidget()
            ];
        }
    }

    HeaderRow
    .NameContent()
    [
        InPropertyHandle->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
    ]
    .ValueContent()
    .MaxDesiredWidth(0.0f) // don't constrain the combo button width
    [
        HeaderWidget.ToSharedRef()
    ];
}

void FNVFeatureExtractorSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    TSharedPtr<IPropertyHandle> FeatureExtractorPropertyHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNVFeatureExtractorSettings, FeatureExtractorRef));
    if (FeatureExtractorPropertyHandle.IsValid())
    {
        ChildBuilder.AddProperty(FeatureExtractorPropertyHandle.ToSharedRef());
    }
}

//======================================== FNVSceneCapturerViewpointSettingsCustomization ========================================
FNVSceneCapturerViewpointSettingsCustomization::FNVSceneCapturerViewpointSettingsCustomization()
{
}

TSharedRef<IPropertyTypeCustomization> FNVSceneCapturerViewpointSettingsCustomization::MakeInstance()
{
    return MakeShareable(new FNVSceneCapturerViewpointSettingsCustomization);
}

void FNVSceneCapturerViewpointSettingsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    HeaderProperties.Reset();
    TSharedPtr<IPropertyHandle> IsEnabledPropertyHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNVSceneCapturerViewpointSettings, bIsEnabled));
    TSharedPtr<IPropertyHandle> DisplayNamePropertyHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNVSceneCapturerViewpointSettings, DisplayName));

    HeaderProperties.Add(IsEnabledPropertyHandle);
    HeaderProperties.Add(DisplayNamePropertyHandle);

    TSharedPtr<SHorizontalBox> HeaderWidget = SNew(SHorizontalBox);
    for (auto CheckHeaderProperty : HeaderProperties)
    {
        if (CheckHeaderProperty.IsValid())
        {
            HeaderWidget->AddSlot()
            .AutoWidth()
            [
                CheckHeaderProperty->CreatePropertyValueWidget()
            ];
        }
    }

    HeaderRow
    .NameContent()
    [
        InPropertyHandle->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
    ]
    .ValueContent()
    .MaxDesiredWidth(0.0f) // don't constrain the combo button width
    [
        HeaderWidget.ToSharedRef()
    ];
}

void FNVSceneCapturerViewpointSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // This basically creates all the children as in default, we are not actually customizing the children.
    uint32 NumChildProps = 0;
    InPropertyHandle->GetNumChildren(NumChildProps);

    for (uint32 Idx = 0; Idx < NumChildProps; Idx++)
    {
        TSharedPtr<IPropertyHandle> PropHandle = InPropertyHandle->GetChildHandle(Idx);
        ChildBuilder.AddProperty(PropHandle.ToSharedRef());
    }
}

//======================================== FNVImageSizeCustomization ========================================
FNVImageSizeCustomization::FNVImageSizeCustomization()
{
    TArray<FNVNamedImageSizePreset> const& Presets = ANVSceneCapturerActor::GetImageSizePresets();

    int32 const NumPresets = Presets.Num();
    // first create preset combo list
    PresetComboList.Empty(NumPresets + 1);

    // first one is default one
    PresetComboList.Add(MakeShareable(new FString(TEXT("Custom..."))));

    // put all presets in the list
    for (FNVNamedImageSizePreset const& CheckPreset : Presets)
    {
        PresetComboList.Add(MakeShareable(new FString(CheckPreset.Name)));
    }
}

TSharedRef<IPropertyTypeCustomization> FNVImageSizeCustomization::MakeInstance()
{
    return MakeShareable(new FNVImageSizeCustomization);
}

void FNVImageSizeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    HeaderRow.
    NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MaxDesiredWidth(0.f)
    [
        SAssignNew(PresetComboBox, SComboBox< TSharedPtr<FString> >)
        .OptionsSource(&PresetComboList)
        .OnGenerateWidget(this, &FNVImageSizeCustomization::MakePresetComboWidget)
        .OnSelectionChanged(this, &FNVImageSizeCustomization::OnPresetChanged)
        .IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
        .ContentPadding(2)
        .Content()
        [
            SNew(STextBlock)
            .Text(this, &FNVImageSizeCustomization::GetPresetComboBoxContent)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .ToolTipText(this, &FNVImageSizeCustomization::GetPresetComboBoxContent)
        ]
    ];
}

void FNVImageSizeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Retrieve structure's child properties
    uint32 NumChildren;
    StructPropertyHandle->GetNumChildren(NumChildren);
    TMap<FName, TSharedPtr< IPropertyHandle > > PropertyHandles;
    for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
        TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
        const FName PropertyName = ChildHandle->GetProperty()->GetFName();
        PropertyHandles.Add(PropertyName, ChildHandle);
    }

    // Retrieve special case properties
    WidthHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FNVImageSize, Width));
    HeightHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FNVImageSize, Height));

    for (auto Iter(PropertyHandles.CreateConstIterator()); Iter; ++Iter)
    {
        IDetailPropertyRow& SettingsRow = ChildBuilder.AddProperty(Iter.Value().ToSharedRef());
    }
}

TSharedRef<SWidget> FNVImageSizeCustomization::MakePresetComboWidget(TSharedPtr<FString> InItem)
{
    return
        SNew(STextBlock)
        .Text(FText::FromString(*InItem))
        .Font(IDetailLayoutBuilder::GetDetailFont());
}

void FNVImageSizeCustomization::OnPresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    // if it's set from code, we did that on purpose
    if (SelectInfo != ESelectInfo::Direct)
    {
        FString const NewPresetName = *NewSelection.Get();

        // search presets for one that matches
        const TArray<FNVNamedImageSizePreset>& Presets = ANVSceneCapturerActor::GetImageSizePresets();
        const int32 NumPresets = Presets.Num();
        for (int32 PresetIdx = 0; PresetIdx < NumPresets; ++PresetIdx)
        {
            const FNVNamedImageSizePreset& CheckPreset = Presets[PresetIdx];
            if (CheckPreset.Name == NewPresetName)
            {
                const FScopedTransaction Transaction(LOCTEXT("ChangeImageSizePreset", "Change Image Size Preset"));

                // copy data from preset into properties
                // all SetValues except the last set to Interactive so we don't rerun construction scripts and invalidate subsequent property handles
                ensure(WidthHandle->SetValue(CheckPreset.ImageSize.Width, EPropertyValueSetFlags::InteractiveChange | EPropertyValueSetFlags::NotTransactable) == FPropertyAccess::Result::Success);
                ensure(HeightHandle->SetValue(CheckPreset.ImageSize.Height, EPropertyValueSetFlags::InteractiveChange | EPropertyValueSetFlags::NotTransactable) == FPropertyAccess::Result::Success);

                break;
            }
        }
        // if none of them found, do nothing
    }
}

FText FNVImageSizeCustomization::GetPresetComboBoxContent() const
{
    // just test one variable for multiple selection
    float Width;
    if (WidthHandle->GetValue(Width) == FPropertyAccess::Result::MultipleValues)
    {
        // multiple selection
        return LOCTEXT("MultipleValues", "Multiple Values");
    }

    return FText::FromString(*GetPresetString().Get());
}


TSharedPtr<FString> FNVImageSizeCustomization::GetPresetString() const
{
    int32 Width;
    WidthHandle->GetValue(Width);

    int32 Height;
    HeightHandle->GetValue(Height);

    // search presets for one that matches
    TArray<FNVNamedImageSizePreset> const& Presets = ANVSceneCapturerActor::GetImageSizePresets();
    int32 const NumPresets = Presets.Num();
    for (int32 PresetIdx = 0; PresetIdx < NumPresets; ++PresetIdx)
    {
        const FNVNamedImageSizePreset& CheckPreset = Presets[PresetIdx];
        if ((CheckPreset.ImageSize.Width == Width) && (CheckPreset.ImageSize.Height == Height))
        {
            // this is the one
            if (PresetComboList.IsValidIndex(PresetIdx + 1))
            {
                return PresetComboList[PresetIdx + 1];
            }
        }
    }

    return PresetComboList[0];
}
