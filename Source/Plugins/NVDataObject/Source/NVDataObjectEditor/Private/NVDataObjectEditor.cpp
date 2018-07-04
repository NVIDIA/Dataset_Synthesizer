/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVDataObjectEditor.h"
#include "NVDataObjectFactory.h"
#include "AssetTypeAction_NVDataObject.h"
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

IMPLEMENT_MODULE(IModuleNVDataObjectEditor, NVDataObjectEditor)

//General Log
DEFINE_LOG_CATEGORY(LogNVDataObjectEditor);

#define LOCTEXT_NAMESPACE "NVDataObjectEditor"
void IModuleNVDataObjectEditor::StartupModule()
{
    UE_LOG(LogNVDataObjectEditor, Warning, TEXT("Loaded NVDataObject Editor"));

    AssetTypeActions.Emplace(new FAssetTypeAction_NVDataObject());
    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    for (auto ActionRef : AssetTypeActions)
    {
        AssetTools.RegisterAssetTypeActions(ActionRef);
    }
    ScanDataObjects();
}

void IModuleNVDataObjectEditor::ShutdownModule()
{
    // NOTE: For some reason the AssetToolsModule already unloaded here => we can't use it anymore
    //IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    //for (auto ActionRef : AssetTypeActions)
    //{
    //  AssetTools.UnregisterAssetTypeActions(ActionRef);
    //}
    AssetTypeActions.Empty();
}

void IModuleNVDataObjectEditor::ScanDataObjects()
{
    const FString TempPackageRootPath = TEXT("/NVData/");
    const FString TempPackageContentPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("NVData"));
    const FString DataObjectPackagePath = TEXT("/NVData");
    FPackageName::RegisterMountPoint(TempPackageRootPath, TempPackageContentPath);

    const FString DataObjectFolderPath = TEXT("NVData");
    const FString FullDataFolderPath = FPaths::Combine(FPaths::ProjectDir(), DataObjectFolderPath) + TEXT("/");
    const FString SourceDataFolderPath = FullDataFolderPath;
    const FString SourceDataMountPoint = TEXT("/NVDataSource/");

    // Make sure the source data directory is monitored
    UEditorLoadingSavingSettings* EditorLoadingSavingSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
    if (EditorLoadingSavingSettings)
    {
        FAutoReimportDirectoryConfig AutoReimportDirConfig;
        AutoReimportDirConfig.SourceDirectory = SourceDataFolderPath;
        AutoReimportDirConfig.MountPoint = TempPackageRootPath;
        FAutoReimportWildcard ReimportWildcard;
        ReimportWildcard.Wildcard = TEXT("*.json");
        ReimportWildcard.bInclude = true;
        AutoReimportDirConfig.Wildcards.Add(ReimportWildcard);
        EditorLoadingSavingSettings->AutoReimportDirectorySettings.Add(AutoReimportDirConfig);
        EditorLoadingSavingSettings->bAutoCreateAssets = true;
    }

    if (!DataObjectFolderPath.IsEmpty())
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
        AssetRegistry.AddPath(DataObjectPackagePath);

        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

        FString DataFolderPath = FullDataFolderPath;
        IFileManager& FileManager = IFileManager::Get();

        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        TArray<FString> DirectoriesToSkip;
        DirectoriesToSkip.Reset();
        TArray<FString> DirectoriesToNotRecurse;
        DirectoriesToNotRecurse.Reset();
        FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, false);

        FileManager.IterateDirectory(*DataFolderPath, Visitor);

        TArray<UPackage*> PackageToSave;

        for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
        {
            const FString& FullFilename = TimestampIt.Key();
            const FString& FileName = FPaths::GetBaseFilename(FullFilename, true);
            FString RelativeFilename = FullFilename;
            FPaths::MakePathRelativeTo(RelativeFilename, *SourceDataFolderPath);
            const FString RelativePath = FPaths::GetPath(RelativeFilename);
            const FDateTime& FileTimestamp = TimestampIt.Value();

            const FString NewDataPackagePath = PackageTools::SanitizePackageName(FPaths::Combine(DataObjectPackagePath, RelativePath, FileName));

            UPackage* NewDataPackage = nullptr;
            FString ExistingFilename;
            const bool bPackageAlreadyExists = FPackageName::DoesPackageExist(NewDataPackagePath, NULL, &ExistingFilename);
            if (bPackageAlreadyExists)
            {
                NewDataPackage = LoadPackage(nullptr, *ExistingFilename, LOAD_None);
                // TODO: Check for the timestamp of the package and the source files
                // We don't need to reimport the data object if the package is newer than the source file
            }
            else
            {
                NewDataPackage = CreatePackage(nullptr, *NewDataPackagePath);
                NewDataPackage->SetPackageFlags(PKG_EditorOnly);
                NewDataPackage->SetFolderName(TEXT("DataObject"));
            }
            PackageToSave.Add(NewDataPackage);

            EObjectFlags ObjectFlags = RF_Public | RF_Standalone | RF_Transactional;
            UNVDataObjectAsset* NewDataObjectContainer = UNVDataObjectAsset::DeserializeFromJsonFile(FullFilename, NewDataPackage, *FileName, ObjectFlags);
            if (NewDataObjectContainer)
            {
                UAssetImportData* DataObjectImportData = NewDataObjectContainer->AssetImportData;
                if (DataObjectImportData)
                {
                    DataObjectImportData->SourceData.SourceFiles.Add(FullFilename);
                }

                FAssetRegistryModule::AssetCreated(NewDataObjectContainer);
            }
            NewDataPackage->MarkAsFullyLoaded();
        }
    }
}
