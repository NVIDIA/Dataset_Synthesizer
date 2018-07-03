/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerEditorModule.h"
#include "NVSceneCapturerCustomization.h"
#include "NVSceneCapturerViewpointComponent.h"
#include "NVSceneCapturerActor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "DetailCustomizations.h"
#include "Editor.h"
#include "UnrealEd.h"
#include "LevelEditor.h"
#include "LevelEditorActions.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IPlacementModeModule.h"

IMPLEMENT_MODULE(IModuleNVSceneCapturerEditor, NVSceneCapturerEditor)

//General Log
DEFINE_LOG_CATEGORY(LogNVSceneCapturerEditor);

#define LOCTEXT_NAMESPACE "NVSceneCapturerEditor"
void IModuleNVSceneCapturerEditor::StartupModule()
{
    bRegisteredComponentVisualizers = false;

    UE_LOG(LogNVSceneCapturerEditor, Log, TEXT("Loaded NVSceneCapturer Editor"));

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout("NVFeatureExtractorSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNVFeatureExtractorSettingsCustomization::MakeInstance));
    PropertyModule.RegisterCustomPropertyTypeLayout("NVSceneCapturerViewpointSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNVSceneCapturerViewpointSettingsCustomization::MakeInstance));
    PropertyModule.RegisterCustomPropertyTypeLayout("NVImageSize", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNVImageSizeCustomization::MakeInstance));

    RegisterComponentVisualizers();
    RegisterPlacementModes();
    RegisterToolbarWidgets();
}

void IModuleNVSceneCapturerEditor::ShutdownModule()
{
    if (GUnrealEd != NULL)
    {
        // Iterate over all class names we registered for
        for (FName ClassName : RegisteredComponentClassNames)
        {
            GUnrealEd->UnregisterComponentVisualizer(ClassName);
        }
    }
}

void IModuleNVSceneCapturerEditor::RegisterComponentVisualizers()
{
    //For some reason this crashes in commandlets, we wouldn't see the visualizer anyway
    if (bRegisteredComponentVisualizers || IsRunningCommandlet())
    {
        return;
    }

    bRegisteredComponentVisualizers = true;
}

void IModuleNVSceneCapturerEditor::RegisterComponentVisualizer(UClass* ComponentClass, TSharedPtr<FComponentVisualizer> Visualizer)
{
    ensure(ComponentClass!=nullptr);
    if (ComponentClass && GUnrealEd && Visualizer.IsValid())
    {
        GUnrealEd->RegisterComponentVisualizer(ComponentClass->GetFName(), Visualizer);
        RegisteredComponentClassNames.Add(ComponentClass->GetFName());
        Visualizer->OnRegister();
    }
}

void IModuleNVSceneCapturerEditor::RegisterPlacementModes()
{
    // Add new placement modes
    IPlacementModeModule& PlacementModeModule = IPlacementModeModule::Get();

    const FName CategoryName = FName(TEXT("NVIDIA Types"));

    const FPlacementCategoryInfo* NVPlacementCategory = PlacementModeModule.GetRegisteredPlacementCategory(CategoryName);
    // Register the NVidia class types if it's not registerd yet
    if (!NVPlacementCategory)
    {
        PlacementModeModule.RegisterPlacementCategory(
            FPlacementCategoryInfo(
                //NSLOCTEXT("PlacementMode", "RecentlyPlaced", "Recently Placed"),
                FText::FromString(TEXT("NVIDIA Types")),
                CategoryName,
                TEXT("NvidiaClasses"),
                TNumericLimits<int32>::Lowest(),
                true
            )
        );
    }

    if (GConfig)
    {
        const FString& NvidiaSection = TEXT("/Script/Engine.Nvidia");
        const FString& NvidiaPlacementTypesKey = TEXT("NvidiaPlacementTypes");
        TArray<FString> PlacementTypes;
        GConfig->GetArray(
            *NvidiaSection,
            *NvidiaPlacementTypesKey,
            PlacementTypes,
            //GEditorIni
            GEngineIni
        );

        // Load the asset registry module
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked< FAssetRegistryModule >(FName("AssetRegistry"));
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        static const FName NativeParentClassTag(TEXT("NativeParentClass"));
        const FString CapturerActorClassName = FString::Printf(TEXT("%s'%s'"), *UClass::StaticClass()->GetName(), *ANVSceneCapturerActor::StaticClass()->GetPathName());
        TArray<FAssetData> AssetDatas;
        AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetDatas);
        for (FAssetData AssetData : AssetDatas)
        {
            const FString& AssetClassType = AssetData.GetTagValueRef<FString>(NativeParentClassTag);
            if (AssetClassType == CapturerActorClassName)
            {
                UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
                if (Blueprint != nullptr)
                {
                    PlacementModeModule.RegisterPlaceableItem(CategoryName,
                            MakeShareable(new FPlaceableItem(nullptr, AssetData)));
                }
            }
        }

        for (const FString& PlacementTypeName : PlacementTypes)
        {
            UClass* PlacementClassType = LoadClass<AActor>(nullptr, *PlacementTypeName, nullptr, LOAD_None, nullptr);
            if (PlacementClassType)
            {
                PlacementModeModule.RegisterPlaceableItem(CategoryName, MakeShareable(new FPlaceableItem(nullptr, FAssetData(PlacementClassType))));
            }
        }
    }
}

void IModuleNVSceneCapturerEditor::RegisterToolbarWidgets()
{
    TSharedPtr<FExtender> NVSettingsExtender = MakeShareable(new FExtender);

    TSharedRef<FUICommandList> NVSettingsCommandList(new FUICommandList());

    NVSettingsExtender->AddToolBarExtension(TEXT("Settings"),
                                            EExtensionHook::After,
                                            NVSettingsCommandList,
                                            FToolBarExtensionDelegate::CreateRaw(this, &IModuleNVSceneCapturerEditor::CreateToolbarWidgets));

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<FExtender> Extenders = LevelEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders();
    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(NVSettingsExtender);
}

void IModuleNVSceneCapturerEditor::CreateToolbarWidgets(FToolBarBuilder& ToolbarBuilder)
{
    //ToolbarBuilder.BeginSection("Settings");
    ToolbarBuilder.BeginSection(FName(TEXT("NVDLSettings")));
    {
        /**
        * Adds a tool bar button
        *
        * @param    UIAction    Actions to execute on this menu item.
        * @param    InLabel     Label to show in the menu entry
        * @param    InToolTip   Tool tip used when hovering over the menu entry
        * @param    InIcon      The icon to use
        * @param    UserInterfaceActionType Type of interface action
        * @param    InTutorialHighlightName Name to identify this widget and highlight during tutorials
        */
        //void AddToolBarButton(const FUIAction& InAction, FName InExtensionHook = NAME_None, const TAttribute<FText>& InLabelOverride = TAttribute<FText>(), const TAttribute<FText>& InToolTipOverride = TAttribute<FText>(), const TAttribute<FSlateIcon>& InIconOverride = TAttribute<FSlateIcon>(), const EUserInterfaceActionType::Type UserInterfaceActionType = EUserInterfaceActionType::Button, FName InTutorialHighlightName = NAME_None);

        ToolbarBuilder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([=] { FGlobalTabmanager::Get()->InvokeTab(FTabId("WidgetReflector")); }),
                FCanExecuteAction()
            ),
            NAME_None,
            LOCTEXT("NVDLSettings_Label", "NVDL Settings"),
            LOCTEXT("NVDLSettings_Tooltip", "Nvidia Deep learning toolkit settings"),
            FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.GameSettings"),
            EUserInterfaceActionType::ToggleButton);
    }
    ToolbarBuilder.EndSection();
}

void IModuleNVSceneCapturerEditor::RegisterContentMountPoint()
{
    const FString MountContentPath = TEXT("/Nvidia Scene Capturer/");
    const FString ModuleContentPath = FPaths::ProjectPluginsDir() / TEXT("NVSceneCapturer/Content/");
    FPackageName::RegisterMountPoint(MountContentPath, ModuleContentPath);
}

#undef LOCTEXT_NAMESPACE