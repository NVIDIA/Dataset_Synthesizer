/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneCapturerEditorModule.h"
#include "Engine.h"
#include "Widgets/SWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

class FDetailWidgetRow;
class IPropertyHandle;
class IDetailLayoutBuilder;

class FNVSceneCapturerDetails : public IDetailCustomization
{
public:
    FNVSceneCapturerDetails();

    /** Makes a new instance of this detail layout class for a specific detail view requesting it */
    static TSharedRef<IDetailCustomization> MakeInstance();

    /** IDetailCustomization interface */
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    void GenerateViewpointInfo(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

    class UNVSceneCapturerViewpointComponent* GetViewpointComponent(TSharedRef<IPropertyHandle> InPropertyHandle) const;
    ECheckBoxState IsViewpointEnabled(TSharedRef<IPropertyHandle> InPropertyHandle) const;
    void HandleViewpointEnabledStateChanged(ECheckBoxState NewValue, TSharedRef<IPropertyHandle> InPropertyHandle);
    FText GetViewpointName(TSharedRef<IPropertyHandle> InPropertyHandle) const;
    void OnSelectViewpointComponentClicked(TSharedRef<IPropertyHandle> InPropertyHandle);
};

class FNVFeatureExtractorSettingsCustomization : public IPropertyTypeCustomization
{
public:
    FNVFeatureExtractorSettingsCustomization();

    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /** IPropertyTypeCustomization instance */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
    // List of properties that we don't want to show in the detail child widgets part
    TArray<TSharedPtr<IPropertyHandle>> IgnoreDetailProperties;

    // List of the properties to show on the header of the struct
    TArray<TSharedPtr<IPropertyHandle>> HeaderProperties;
};

class FNVSceneCapturerViewpointSettingsCustomization : public IPropertyTypeCustomization
{
public:
    FNVSceneCapturerViewpointSettingsCustomization();

    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /** IPropertyTypeCustomization instance */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
    // List of the properties to show on the header of the struct
    TArray<TSharedPtr<IPropertyHandle>> HeaderProperties;
};

class FNVImageSizeCustomization : public IPropertyTypeCustomization
{
public:
    FNVImageSizeCustomization();

    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /** IPropertyTypeCustomization instance */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
    TSharedPtr<IPropertyHandle> WidthHandle;
    TSharedPtr<IPropertyHandle> HeightHandle;

    TSharedPtr<class SComboBox< TSharedPtr<FString> > > PresetComboBox;
    TArray< TSharedPtr< FString > >                     PresetComboList;

    void OnPresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    TSharedRef<SWidget> MakePresetComboWidget(TSharedPtr<FString> InItem);

    FText GetPresetComboBoxContent() const;
    TSharedPtr<FString> GetPresetString() const;
};
