/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVDataObjectFactory.h"
#include "NVDataObject.h"
#include "EditorFramework/AssetImportData.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Factories/Factory.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Kismet2/KismetEditorUtilities.h"


DEFINE_LOG_CATEGORY(LogNVDataObjectFactory);

#define LOCTEXT_NAMESPACE "NVDataObjectFactory"
UNVDataObjectFactory::UNVDataObjectFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ImportPriority = DefaultImportPriority;

    //bCreateNew = true;
    // NOTE: We only import the text data file, we don't create a new text data file in the editor
    bCreateNew = false;

    bText = true;
    bEditAfterNew = true;
    bEditorImport = true;
    SupportedClass = UNVDataObjectAsset::StaticClass();

    Formats.Add(TEXT("json;Json"));

    // TODO
    //AutomatedImportData
}

// NOTE: is defined privately inside EditorFactories.cpp => we can't include it so just duplicate it here
class FAssetClassParentFilter : public IClassViewerFilter
{
public:
    FAssetClassParentFilter()
        : DisallowedClassFlags(CLASS_None), bDisallowBlueprintBase(false)
    {}

    /** All children of these classes will be included unless filtered out by another setting. */
    TSet< const UClass* > AllowedChildrenOfClasses;

    /** Disallowed class flags. */
    EClassFlags DisallowedClassFlags;

    /** Disallow blueprint base classes. */
    bool bDisallowBlueprintBase;

    virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
    {
        bool bAllowed = !InClass->HasAnyClassFlags(DisallowedClassFlags)
                        && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

        if (bAllowed && bDisallowBlueprintBase)
        {
            if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
            {
                return false;
            }
        }

        return bAllowed;
    }

    virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
    {
        if (bDisallowBlueprintBase)
        {
            return false;
        }

        return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
               && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
    }
};

bool UNVDataObjectFactory::ConfigureProperties()
{
    NVDataObjectClass = nullptr;

    // Load the classviewer module to display a class picker
    FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

    // Fill in options
    FClassViewerInitializationOptions Options;
    Options.Mode = EClassViewerMode::ClassPicker;

    TSharedPtr<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
    Options.ClassFilter = Filter;

    Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
    // Pick a child class of UNVDataObject
    UClass* ParentClass = UNVDataObjectAsset::StaticClass();
    Filter->AllowedChildrenOfClasses.Add(ParentClass);

    const FText TitleText = LOCTEXT("CreateDataAssetOptions", "Pick Data Asset Class");
    UClass* ChosenClass = nullptr;
    const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, ParentClass);

    if (bPressedOk)
    {
        NVDataObjectClass = ChosenClass;
    }

    return bPressedOk;
}

FText UNVDataObjectFactory::GetDisplayName() const
{
    return LOCTEXT("NVDataObjectFactoryDescription", "Import NVDataObject From Text Format");
}

bool UNVDataObjectFactory::DoesSupportClass(UClass* Class)
{
    return (Class && Class->IsChildOf(SupportedClass));
}

bool UNVDataObjectFactory::FactoryCanImport(const FString& Filename)
{
    return !Filename.IsEmpty();
}

UObject* UNVDataObjectFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    if (NVDataObjectClass != nullptr)
    {
        return NewObject<UNVDataObjectAsset>(InParent, NVDataObjectClass, Name, Flags | RF_Transactional);
    }
    else
    {
        // if we have no data asset class, use the passed-in class instead
        check(Class->IsChildOf(UNVDataObjectAsset::StaticClass()));
        return NewObject<UNVDataObjectAsset>(InParent, Class, Name, Flags);
    }
}

UObject* UNVDataObjectFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
    UNVDataObjectAsset* ImportedDataObj = nullptr;
    check(InClass);
    check(InParent);
    if ((InClass == nullptr) || (InParent == nullptr))
    {
        UE_LOG(LogNVDataObjectFactory, Error, TEXT("invalid argument."));
    }
    else
    {
        FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

        // Get the string from the input buffer
        FString TextDataString;
        const int32 NumChars = (BufferEnd - Buffer);
        TArray<TCHAR>& StringChars = TextDataString.GetCharArray();
        StringChars.AddUninitialized(NumChars + 1);
        FMemory::Memcpy(StringChars.GetData(), Buffer, NumChars * sizeof(TCHAR));
        StringChars.Last() = 0;

        ImportedDataObj = UNVDataObjectAsset::DeserializeFromJsonString(TextDataString, InParent, InName, Flags);
    }
    return ImportedDataObj;
}

bool UNVDataObjectFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UNVDataObjectAsset* DataObjectAsset = Cast<UNVDataObjectAsset>(Obj);
    if (DataObjectAsset)
    {
        DataObjectAsset->AssetImportData->ExtractFilenames(OutFilenames);
        return true;
    }
    return false;
}

void UNVDataObjectFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    check(Obj);
    if (Obj == nullptr)
    {
        UE_LOG(LogNVDataObjectFactory, Error, TEXT("invalid argument."));
    }
    else
    {
        UNVDataObjectAsset* DataObjectAsset = Cast<UNVDataObjectAsset>(Obj);
        if (DataObjectAsset && ensure(NewReimportPaths.Num() == 1))
        {
            DataObjectAsset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
        }
    }
}

EReimportResult::Type UNVDataObjectFactory::Reimport(UObject* Obj)
{
    auto Result = EReimportResult::Failed;

    check(Obj);
    if (Obj == nullptr)
    {
        UE_LOG(LogNVDataObjectFactory, Error, TEXT("invalid argument."));
    }
    else
    {
        if (UNVDataObjectAsset* DataObjectAsset = Cast<UNVDataObjectAsset>(Obj))
        {
            if (DataObjectAsset->DataObject)
            {
                //DataObjectAsset->DataObject;
                DataObjectAsset->DataObject = nullptr;
            }
            const FString SourceFilePath = DataObjectAsset->AssetImportData->GetFirstFilename();
            const FString SourceFileName = FPaths::GetBaseFilename(SourceFilePath, true);
            EObjectFlags ObjectFlags = RF_Public | RF_Standalone | RF_Transactional;
            UNVDataObject* ImportedDataObject = UNVDataObject::DeserializeFromJsonFile(SourceFilePath, DataObjectAsset, *SourceFileName, ObjectFlags);
            if (ImportedDataObject)
            {
                DataObjectAsset->DataObject = ImportedDataObject;
                Result = EReimportResult::Succeeded;
            }
        }
    }
    return Result;
}

int32 UNVDataObjectFactory::GetPriority() const
{
    return ImportPriority;
}

#undef LOCTEXT_NAMESPACE