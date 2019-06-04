/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVDataObjectEditorToolkit.h"
#include "NVDataObjectEditor.h"
#include "NVDataObjectFactory.h"
#include "NVDataObject.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "DetailCustomizations.h"
#include "Editor.h"
#include "UnrealEd.h"
#include "LevelEditor.h"
#include "LevelEditorActions.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "IPlacementModeModule.h"
#include "PackageTools.h"

DEFINE_LOG_CATEGORY(LogNVDataObjectEditorToolkit);

#define LOCTEXT_NAMESPACE "NVDataObjectEditor"
const FName FNVDataObjectEditorToolkit::DataObjectEditorAppIdentifier(TEXT("DataObjectEditorApp"));
const FName FNVDataObjectEditorToolkit::PropertiesTabId(TEXT("GenericEditor_Properties"));

TSharedRef<FNVDataObjectEditorToolkit> FNVDataObjectEditorToolkit::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UNVDataObjectAsset* ObjectToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects /*= FSimpleAssetEditor::FGetDetailsViewObjects()*/)
{
    TSharedRef< FNVDataObjectEditorToolkit > NewEditor(new FNVDataObjectEditorToolkit());

    TArray<UNVDataObjectAsset*> ObjectsToEdit;
    ObjectsToEdit.Add(ObjectToEdit);
    NewEditor->InitEditor(Mode, InitToolkitHost, ObjectsToEdit, GetDetailsViewObjects);

    return NewEditor;
}

TSharedRef<FNVDataObjectEditorToolkit> FNVDataObjectEditorToolkit::CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UNVDataObjectAsset*>& ObjectsToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects /*= FSimpleAssetEditor::FGetDetailsViewObjects()*/)
{
    TSharedRef< FNVDataObjectEditorToolkit > NewEditor(new FNVDataObjectEditorToolkit());
    NewEditor->InitEditor(Mode, InitToolkitHost, ObjectsToEdit, GetDetailsViewObjects);
    return NewEditor;
}

void FNVDataObjectEditorToolkit::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, TArray<UNVDataObjectAsset*> NVObjectAssetsToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects /*= FSimpleAssetEditor::FGetDetailsViewObjects()*/)
{
    EditingDataAssets = NVObjectAssetsToEdit;

    EditingDataObjects.Reset();
    EditingObjects.Reset();
    TArray<class UObject*> EditingAssets;
    for (UNVDataObjectAsset* NVObjectAsset : NVObjectAssetsToEdit)
    {
        if (NVObjectAsset)
        {
            EditingDataObjects.Add(NVObjectAsset->DataObject);
            EditingObjects.Add(NVObjectAsset->DataObject);
            EditingAssets.Add(NVObjectAsset);
        }
    }

    const bool bIsUpdatable = false;
    const bool bAllowFavorites = true;
    const bool bIsLockable = false;

    GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.AddRaw(this, &FNVDataObjectEditorToolkit::HandleAssetPostImport);
    GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddRaw(this, &FNVDataObjectEditorToolkit::HandleAssetReimport);

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    const FDetailsViewArgs DetailsViewArgs(bIsUpdatable, bIsLockable, true, FDetailsViewArgs::ObjectsUseNameArea, false);
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_SimpleAssetEditor_Layout_v3")
            ->AddArea
            (
                FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
                ->Split
                (
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.1f)
                    ->SetHideTabWell(true)
                    ->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
                )
                ->Split
                (
                    FTabManager::NewSplitter()
                    ->Split
                    (
                        FTabManager::NewStack()
                        ->AddTab(PropertiesTabId, ETabState::OpenedTab)
                    )
                )
            );

    const bool bCreateDefaultStandaloneMenu = true;
    const bool bCreateDefaultToolbar = true;
    FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, DataObjectEditorAppIdentifier,
                                         StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingAssets, true);

    // @todo toolkit world centric editing
    // Setup our tool's layout
    /*if( IsWorldCentricAssetEditor() && !PropertiesTab.IsValid() )
    {
    const FString TabInitializationPayload(TEXT(""));       // NOTE: Payload not currently used for asset properties
    SpawnToolkitTab(GetToolbarTabId(), FString(), EToolkitTabSpot::ToolBar);
    PropertiesTab = SpawnToolkitTab( PropertiesTabId, TabInitializationPayload, EToolkitTabSpot::Details );
    }*/

    // Get the list of objects to edit the details of
    //const TArray<UObject*> ObjectsToEditInDetailsView = (GetDetailsViewObjects.IsBound()) ? GetDetailsViewObjects.Execute(EditingDataObjects) : EditingDataObjects;

    // Ensure all objects are transactable for undo/redo in the details panel
    for (UObject* ObjectToEditInDetailsView : EditingObjects)
    {
        ObjectToEditInDetailsView->SetFlags(RF_Transactional);
    }

    if (DetailsView.IsValid())
    {
        // Make sure details window is pointing to our object
        DetailsView->SetObjects(EditingObjects);
    }

    CustomizeToolbar();
}

FNVDataObjectEditorToolkit::~FNVDataObjectEditorToolkit()
{
    GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.RemoveAll(this);
    GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.RemoveAll(this);

    DetailsView.Reset();
    PropertiesTab.Reset();
}

FName FNVDataObjectEditorToolkit::GetToolkitFName() const
{
    return FName("NVDataObjectEditor");
}

FText FNVDataObjectEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("AppLabel", "Data Object Editor");
}

FText FNVDataObjectEditorToolkit::GetToolkitName() const
{
    const TArray<UObject*>& EditingObjs = GetEditingObjects();

    check(EditingObjs.Num() > 0);

    FFormatNamedArguments Args;
    Args.Add(TEXT("ToolkitName"), GetBaseToolkitName());

    if (EditingObjs.Num() == 1)
    {
        const UObject* EditingObject = EditingObjs[0];

        const bool bDirtyState = EditingObject->GetOutermost()->IsDirty();

        Args.Add(TEXT("ObjectName"), FText::FromString(EditingObject->GetName()));
        Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
        return FText::Format(LOCTEXT("ToolkitTitle", "{ObjectName}{DirtyState} - {ToolkitName}"), Args);
    }
    else
    {
        bool bDirtyState = false;
        UClass* SharedBaseClass = nullptr;
        for (int32 x = 0; x < EditingObjs.Num(); ++x)
        {
            UObject* Obj = EditingObjs[x];
            check(Obj);

            UClass* ObjClass = Cast<UClass>(Obj);
            if (ObjClass == nullptr)
            {
                ObjClass = Obj->GetClass();
            }
            check(ObjClass);

            // Initialize with the class of the first object we encounter.
            if (SharedBaseClass == nullptr)
            {
                SharedBaseClass = ObjClass;
            }

            // If we've encountered an object that's not a subclass of the current best baseclass,
            // climb up a step in the class hierarchy.
            while (!ObjClass->IsChildOf(SharedBaseClass))
            {
                SharedBaseClass = SharedBaseClass->GetSuperClass();
            }

            // If any of the objects are dirty, flag the label
            bDirtyState |= Obj->GetOutermost()->IsDirty();
        }

        check(SharedBaseClass);

        Args.Add(TEXT("NumberOfObjects"), EditingObjs.Num());
        Args.Add(TEXT("ClassName"), FText::FromString(SharedBaseClass->GetName()));
        Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
        return FText::Format(LOCTEXT("ToolkitTitle_EditingMultiple", "{NumberOfObjects} {ClassName}{DirtyState} - {ToolkitName}"), Args);
    }
}

FText FNVDataObjectEditorToolkit::GetToolkitToolTipText() const
{
    const TArray<UObject*>& EditingObjs = GetEditingObjects();

    check(EditingObjs.Num() > 0);

    FFormatNamedArguments Args;
    Args.Add(TEXT("ToolkitName"), GetBaseToolkitName());

    if (EditingObjs.Num() == 1)
    {
        const UObject* EditingObject = EditingObjs[0];
        return FAssetEditorToolkit::GetToolTipTextForObject(EditingObject);
    }
    else
    {
        UClass* SharedBaseClass = NULL;
        for (int32 x = 0; x < EditingObjs.Num(); ++x)
        {
            UObject* Obj = EditingObjs[x];
            check(Obj);

            UClass* ObjClass = Cast<UClass>(Obj);
            if (ObjClass == nullptr)
            {
                ObjClass = Obj->GetClass();
            }
            check(ObjClass);

            // Initialize with the class of the first object we encounter.
            if (SharedBaseClass == nullptr)
            {
                SharedBaseClass = ObjClass;
            }

            // If we've encountered an object that's not a subclass of the current best baseclass,
            // climb up a step in the class hierarchy.
            while (!ObjClass->IsChildOf(SharedBaseClass))
            {
                SharedBaseClass = SharedBaseClass->GetSuperClass();
            }
        }

        check(SharedBaseClass);

        Args.Add(TEXT("NumberOfObjects"), EditingObjs.Num());
        Args.Add(TEXT("ClassName"), FText::FromString(SharedBaseClass->GetName()));
        return FText::Format(LOCTEXT("ToolkitTitle_EditingMultipleToolTip", "{NumberOfObjects} {ClassName} - {ToolkitName}"), Args);
    }
}

FString FNVDataObjectEditorToolkit::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("UDStructWorldCentricTabPrefix", "NV Data Object ").ToString();
}

FLinearColor FNVDataObjectEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.0f, 0.0f, 1.0f, 0.5f);
}

void FNVDataObjectEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& NewTabManager)
{
    WorkspaceMenuCategory = NewTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_GenericAssetEditor", "Asset Editor"));

    FAssetEditorToolkit::RegisterTabSpawners(NewTabManager);

    NewTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FNVDataObjectEditorToolkit::SpawnPropertiesTab))
    .SetDisplayName(LOCTEXT("PropertiesTab", "Details"))
    .SetGroup(WorkspaceMenuCategory.ToSharedRef())
    .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FNVDataObjectEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& OldTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(OldTabManager);

    TabManager->UnregisterTabSpawner(PropertiesTabId);
}

TSharedRef<SDockTab> FNVDataObjectEditorToolkit::SpawnPropertiesTab(const FSpawnTabArgs& Args)
{
    check(Args.GetTabId() == PropertiesTabId);

    return SNew(SDockTab)
           .Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
           .Label(LOCTEXT("GenericDetailsTitle", "Details"))
           .TabColorScale(GetTabColorScale())
           [
               DetailsView.ToSharedRef()
           ];
}

void FNVDataObjectEditorToolkit::CustomizeToolbar()
{
    FNVDataObjectEditorCommonCommands::Register();

    ToolkitCommands->MapAction(
        FNVDataObjectEditorCommonCommands::Get().OpenSourceCommand,
        FExecuteAction::CreateSP(this, &FNVDataObjectEditorToolkit::OpenSource_Execute),
        FCanExecuteAction::CreateSP(this, &FNVDataObjectEditorToolkit::CanOpenSource));

    ToolkitCommands->MapAction(
        FNVDataObjectEditorCommonCommands::Get().ReimportCommand,
        FExecuteAction::CreateSP(this, &FNVDataObjectEditorToolkit::Reimport_Execute),
        FCanExecuteAction::CreateSP(this, &FNVDataObjectEditorToolkit::CanReimport));

    struct Local
    {
        static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
        {
            const FNVDataObjectEditorCommonCommands& NVDataObjectCommands = FNVDataObjectEditorCommonCommands::Get();

            ToolbarBuilder.BeginSection("Command");
            {
                ToolbarBuilder.AddToolBarButton(NVDataObjectCommands.OpenSourceCommand);
                ToolbarBuilder.AddToolBarButton(NVDataObjectCommands.ReimportCommand);
            }
            ToolbarBuilder.EndSection();
        }
    };

    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
    );
    AddToolbarExtender(ToolbarExtender);

    RegenerateMenusAndToolbars();
}

void FNVDataObjectEditorToolkit::OpenSource_Execute()
{
    if (EditingDataAssets.Num() == 0)
    {
        return;
    }

    for (auto DataAsset : EditingDataAssets)
    {
        if (DataAsset)
        {
            UAssetImportData* ObjectImportData = DataAsset->AssetImportData;
            // TODO: Make this into a function and use it with AssetTypeAction_NVDataObject
            if (ObjectImportData)
            {
                TArray<FString> ImportPaths;
                ObjectImportData->ExtractFilenames(ImportPaths);
                for (const FString& ImportPath : ImportPaths)
                {
                    if (!ImportPath.IsEmpty() && (IFileManager::Get().FileSize(*ImportPath) != INDEX_NONE))
                    {
                        FPlatformProcess::LaunchFileInDefaultExternalApplication(*ImportPath, NULL, ELaunchVerb::Open);
                        break;
                    }
                }
            }
        }
    }
}

bool FNVDataObjectEditorToolkit::CanOpenSource() const
{
    return true;
}

void FNVDataObjectEditorToolkit::HandleAssetPostImport(class UFactory* InFactory, UObject* InObject)
{
    ensure(InObject!=nullptr);
    if (InObject == nullptr)
    {
        UE_LOG(LogNVDataObjectEditorToolkit, Error, TEXT("invalid argument."));
    }
    else
    {
        if (EditingDataAssets.Contains(InObject))
        {
            // The details panel likely needs to be refreshed if an asset was imported again
            DetailsView->SetObjects(EditingObjects);
        }
    }
}

void FNVDataObjectEditorToolkit::HandleAssetReimport(UObject* InObject)
{
    ensure(InObject!=nullptr);
    if (InObject == nullptr)
    {
        UE_LOG(LogNVDataObjectEditorToolkit, Error, TEXT("invalid argument."));
    }
    else
    {
        if (EditingDataAssets.Contains(InObject))
        {
            EditingObjects.Reset();
            for (UNVDataObjectAsset* NVObjectAsset : EditingDataAssets)
            {
                if (NVObjectAsset)
                {
                    EditingObjects.Add(NVObjectAsset->DataObject);
                }
            }

            // The details panel likely needs to be refreshed if an asset was imported again
            DetailsView->SetObjects(EditingObjects);
        }
    }
}

void FNVDataObjectEditorToolkit::SetPropertyVisibilityDelegate(FIsPropertyVisible InVisibilityDelegate)
{
    DetailsView->SetIsPropertyVisibleDelegate(InVisibilityDelegate);
}

void FNVDataObjectEditorCommonCommands::RegisterCommands()
{
    UI_COMMAND(OpenSourceCommand, "Open Source", "Open source data file", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::O));
    UI_COMMAND(ReimportCommand, "Reimport", "Reimport from the source data file", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::R));
}

#undef LOCTEXT_NAMESPACE