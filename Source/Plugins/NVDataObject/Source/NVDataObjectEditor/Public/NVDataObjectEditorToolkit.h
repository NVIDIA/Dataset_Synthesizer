/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "Toolkits/SimpleAssetEditor.h"

//General Log
DECLARE_LOG_CATEGORY_EXTERN(LogNVDataObjectEditorToolkit, Log, All)


// NOTE: This EditorToolkit is based on FSimpleAssetEditor
class FNVDataObjectEditorToolkit : public FAssetEditorToolkit
{
    /** App Identifier.*/
    static const FName DataObjectEditorAppIdentifier;

    /** The tab ids for all the tabs used */
    //static const FName MemberVariablesTabId;

    /** Property viewing widget */
    TSharedPtr<class IDetailsView> PropertyView;
    TSharedPtr<class FStructureDefaultValueView> DefaultValueView;
public:
    static TSharedRef<FNVDataObjectEditorToolkit> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UNVDataObjectAsset* ObjectToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects = FSimpleAssetEditor::FGetDetailsViewObjects());
    static TSharedRef<FNVDataObjectEditorToolkit> CreateEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<class UNVDataObjectAsset*>& ObjectsToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects = FSimpleAssetEditor::FGetDetailsViewObjects());

    void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, TArray<class UNVDataObjectAsset*> ObjectAssetToEdit, FSimpleAssetEditor::FGetDetailsViewObjects GetDetailsViewObjects = FSimpleAssetEditor::FGetDetailsViewObjects());

    /** Destructor */
    virtual ~FNVDataObjectEditorToolkit();

    /** IToolkit interface */
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FText GetToolkitName() const override;
    virtual FText GetToolkitToolTipText() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;

    virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

protected:
    void CustomizeToolbar();
    virtual void OpenSource_Execute();
    virtual bool CanOpenSource() const;
    void HandleAssetPostImport(class UFactory* InFactory, UObject* InObject);
    void HandleAssetReimport(UObject* InObject);

    /** Create the properties tab and its content */
    TSharedRef<SDockTab> SpawnPropertiesTab(const FSpawnTabArgs& Args);

    /** Used to show or hide certain properties */
    void SetPropertyVisibilityDelegate(FIsPropertyVisible InVisibilityDelegate);

protected:
    //TSharedRef<SDockTab> SpawnStructureTab(const FSpawnTabArgs& Args);
    /** The tab ids for all the tabs used */
    static const FName PropertiesTabId;

    /** Dockable tab for properties */
    TSharedPtr< SDockableTab > PropertiesTab;

    /** Details view */
    TSharedPtr< class IDetailsView > DetailsView;

    /** The objects open within this editor */
    TArray<class UNVDataObjectAsset*> EditingDataAssets;

    TArray<class UNVDataObject*> EditingDataObjects;
    TArray<class UObject*> EditingObjects;
};

class FNVDataObjectEditorCommonCommands : public TCommands< FNVDataObjectEditorCommonCommands >
{
public:
    FNVDataObjectEditorCommonCommands()
        : TCommands< FNVDataObjectEditorCommonCommands >(TEXT("NVDataObjectEditor"), NSLOCTEXT("Contexts", "NVDataObjectEditor", "NV Data Object Editor"), TEXT("EditorViewport"), FEditorStyle::GetStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr< FUICommandInfo > OpenSourceCommand;
    TSharedPtr< FUICommandInfo > ReimportCommand;
};

