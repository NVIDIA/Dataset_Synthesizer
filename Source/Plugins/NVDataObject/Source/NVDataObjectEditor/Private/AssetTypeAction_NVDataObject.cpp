/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "AssetTypeAction_NVDataObject.h"
#include "Core.h"
#include "DesktopPlatformModule.h"
#include "IMainFrameModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "EditorFramework/AssetImportData.h"
#include "NVDataObjectEditorToolkit.h"

#define LOCTEXT_NAMESPACE "NVDataObjectEditor"
void FAssetTypeAction_NVDataObject::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
    auto DataObjects = GetTypedWeakObjectPtrs<UNVDataObjectAsset>(InObjects);

    MenuBuilder.AddMenuEntry(
        LOCTEXT("NVDataObject_ExportAsJSON", "Export as JSON"),
        LOCTEXT("NVDataObject_ExportAsJSONTooltip", "Export the Nvidia data object as json format."),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateSP(this, &FAssetTypeAction_NVDataObject::ExecuteExportAsJson, DataObjects),
            FCanExecuteAction()
        )
    );

    MenuBuilder.AddMenuEntry(
        LOCTEXT("NVDataObject_OpenSource", "Open source Json file (.json)"),
        LOCTEXT("NVDataObject_OpenSourceTooltip", "Opens the data table's source XLSX/XLSM file in an external editor."),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateSP(this, &FAssetTypeAction_NVDataObject::OpenSourceFiles, DataObjects),
            FCanExecuteAction::CreateSP(this, &FAssetTypeAction_NVDataObject::CanOpenSourceFiles, DataObjects)
        )
    );
}

void FAssetTypeAction_NVDataObject::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
    TArray<UNVDataObjectAsset*> DataObjectAssets;
    for (UObject* CheckObject : InObjects)
    {
        UNVDataObjectAsset* CheckDataObjectAsset = Cast<UNVDataObjectAsset>(CheckObject);
        if (CheckDataObjectAsset)
        {
            DataObjectAssets.Add(CheckDataObjectAsset);
        }
    }
    FNVDataObjectEditorToolkit::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, DataObjectAssets);
}

void FAssetTypeAction_NVDataObject::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
    FAssetTypeActions_Base::GetResolvedSourceFilePaths(TypeAssets, OutSourceFilePaths);
}

void FAssetTypeAction_NVDataObject::ExecuteExportAsJson(TArray< TWeakObjectPtr<UNVDataObjectAsset> > Objects)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

    void* ParentWindowWindowHandle = nullptr;

    IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
    const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
    if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
    {
        ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
    }

    int index = 0;
    for (auto DataObjectPtr : Objects)
    {
        UNVDataObjectAsset* DataObject = DataObjectPtr.Get();
        if (DataObject)
        {
            const FString TestFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("test_data_%d.json"), index));

            FString OutputFilePath = DataObject->AssetImportData->GetFirstFilename();

            if (OutputFilePath.IsEmpty())
            {
                const FString CurrentFilePath = DataObject->AssetImportData
                                                ? DataObject->AssetImportData->GetPathName()
                                                : FPaths::ProjectDir();
                const FString CurrentFilename = DataObject->AssetImportData
                                                ? DataObject->AssetImportData->GetFirstFilename()
                                                : FPaths::ProjectDir();
                const FText Title = FText::Format(LOCTEXT("DataObject_ExportAsJsonDialogTitle", "Export data object '{0}' as JSON..."), FText::FromString(*CurrentFilename));
                FString OutFolderPath;
                DesktopPlatform->OpenDirectoryDialog(
                    ParentWindowWindowHandle,
                    Title.ToString(),
                    (CurrentFilePath.IsEmpty()) ? TEXT("") : FPaths::GetPath(CurrentFilePath),
                    OutFolderPath);

                OutputFilePath = FPaths::Combine(OutFolderPath, DataObject->GetName());
            }

            UNVDataObjectAsset::SerializeToJsonFile(DataObject, OutputFilePath);
        }
        index++;
    }
}

bool FAssetTypeAction_NVDataObject::CanOpenSourceFiles(const TArray<TWeakObjectPtr<UNVDataObjectAsset>> DataObjects)
{
    return true;
}

void FAssetTypeAction_NVDataObject::OpenSourceFiles(const TArray<TWeakObjectPtr<UNVDataObjectAsset>> DataObjects)
{
    for (auto CheckDataObject : DataObjects)
    {
        if (CheckDataObject.IsValid())
        {
            const FString SourceFilePath = CheckDataObject->AssetImportData->GetFirstFilename();
            if (!SourceFilePath.IsEmpty() && (IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE))
            {
                FPlatformProcess::LaunchFileInDefaultExternalApplication(*SourceFilePath, NULL, ELaunchVerb::Open);
                break;
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE