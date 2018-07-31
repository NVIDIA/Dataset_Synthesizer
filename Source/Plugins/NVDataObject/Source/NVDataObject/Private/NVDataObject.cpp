/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVDataObjectModule.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Components/ChildActorComponent.h"
#include "EditorFramework/AssetImportData.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "NVDataObject.h"

DEFINE_LOG_CATEGORY(LogNVDataObject);

// Sets default values
UNVDataObjectAsset::UNVDataObjectAsset(const FObjectInitializer& ObjectInitializer)
{
}

bool UNVDataObjectAsset::SerializeToJsonFile(const UNVDataObjectAsset* DataObjectContainer, const FString& OutputFilePath)
{
    bool bResult = false;
    if (DataObjectContainer && DataObjectContainer->DataObject)
    {
        bResult = UNVDataObject::SerializeToJsonFile(DataObjectContainer->DataObject, OutputFilePath);
    }

    return bResult;
}

UNVDataObjectAsset* UNVDataObjectAsset::DeserializeFromJsonFile(const FString& SourceFilePath, UObject* InParent, FName InName, EObjectFlags Flags)
{
    UNVDataObjectAsset* result = nullptr;

    ensure(InParent!=nullptr);
    if (InParent == nullptr)
    {
        UE_LOG(LogNVDataObject, Error, TEXT("invalid argument."));
    }
    else
    {
        FString JsonString = TEXT("");
        if (FFileHelper::LoadFileToString(JsonString, *SourceFilePath))
        {
            result= DeserializeFromJsonString(JsonString, InParent, InName, Flags);
        }
    }

    return result;
}

UNVDataObjectAsset* UNVDataObjectAsset::DeserializeFromJsonString(const FString& SourceJsonString, UObject* InParent, FName InName, EObjectFlags Flags)
{
    UNVDataObjectAsset* result = nullptr;

    ensure(InParent!=nullptr);
    if (InParent == nullptr)
    {
        UE_LOG(LogNVDataObject, Error, TEXT("invalid argument."));
    }
    else
    {
        const FName ContainerName = InName;

        result = NewObject<UNVDataObjectAsset>(InParent, UNVDataObjectAsset::StaticClass(), ContainerName, Flags, nullptr);
        if (result)
        {
            UNVDataObject* ImportedDataObject = UNVDataObject::DeserializeFromJsonString(SourceJsonString, result, InName, Flags);
            result->DataObject = ImportedDataObject;
        }
    }
    return result;
}

UObject* UNVDataObjectAsset::CreateObjectFromDefinitionData(UNVDataObjectAsset* DefinitionDataObj)
{
    return nullptr;
}

void UNVDataObjectAsset::Serialize(FArchive& Ar)
{
    if (Ar.IsLoading())
    {
        SetFlags(RF_Transactional);
    }

#if WITH_EDITORONLY_DATA
    if (Ar.IsSaving())
    {
        if (DataObject && AssetImportData)
        {
            // If the data object is transactional => it's just reimported
            //if (DataObject->HasAnyFlags(RF_Transactional))
            //{
            //  DataObject->ClearFlags(RF_Transactional);
            //}
            UPackage* Package = GetOutermost();
            if (!Package || !Package->IsDirty())
            {
                bool bTest = true;
            }
            else if (AssetImportData->SourceData.SourceFiles.Num() > 0)
            {
                const FString SourceFilePath = AssetImportData->SourceData.SourceFiles[0].RelativeFilename;
                UNVDataObject::SerializeToJsonFile(DataObject, SourceFilePath);
            }
        }
    }
#endif // WITH_EDITORONLY_DATA

    Super::Serialize(Ar);
}

void UNVDataObjectAsset::PostSaveRoot(bool bCleanupIsRequired)
{
    Super::PostSaveRoot(bCleanupIsRequired);
}

#if WITH_EDITORONLY_DATA
void UNVDataObjectAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
    if (AssetImportData)
    {
        OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }

    Super::GetAssetRegistryTags(OutTags);
}

void UNVDataObjectAsset::PostInitProperties()
{
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
    }

    Super::PostInitProperties();
}
#endif // WITH_EDITORONLY_DATA

//========================================= UNVDataObjectOwner =========================================
UNVDataObjectOwner::UNVDataObjectOwner(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

//========================================= UNVDataObject =========================================
UNVDataObject::UNVDataObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    LogicHandlerClassType = nullptr;
}

bool UNVDataObject::SerializeToJsonFile(const UNVDataObject* DataObject, const FString& OutputFilePath)
{
    bool bResult = false;
    if (DataObject)
    {
        FString OutputString;
        int64 CheckFlags = CPF_Edit; // 0
        int64 SkipFlags = 0;

        TSharedRef<FJsonObject> JSonObject = MakeShareable(new FJsonObject);
        if (FJsonObjectConverter::UStructToJsonObject(DataObject->GetClass(), DataObject, JSonObject, CheckFlags, SkipFlags, 0))
        {
            static const FString ClassNameFieldString = TEXT("_class");
            const FString ClassName = UObjectProperty::GetExportPath(DataObject->GetClass(), nullptr, nullptr, 0);
            JSonObject->SetStringField(ClassNameFieldString, ClassName);

            FString JsonObjStr = TEXT("");
            TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&JsonObjStr, 0);

            FJsonSerializer::Serialize(JSonObject, JsonWriter, true);

            if (!FFileHelper::SaveStringToFile(JsonObjStr, *OutputFilePath))
            {
                // TODO: Warning
                return bResult;
            }
        }

        bResult = true;
    }

    return bResult;
}

UNVDataObject* UNVDataObject::DeserializeFromJsonFile(const FString& SourceFilePath, UObject* InParent, FName InName, EObjectFlags Flags)
{
    UNVDataObject* result = nullptr;
    ensure(InParent!=nullptr);
    if (InParent == nullptr)
    {
        UE_LOG(LogNVDataObject, Error, TEXT("invalid argument."));
    }
    else
    {
        FString JsonString = TEXT("");
        if (FFileHelper::LoadFileToString(JsonString, *SourceFilePath))
        {
            result= DeserializeFromJsonString(JsonString, InParent, InName, Flags);
        }
    }
    return result;
}

UNVDataObject* UNVDataObject::DeserializeFromJsonString(const FString& SourceJsonString, UObject* InParent, FName InName, EObjectFlags Flags)
{
    UNVDataObject* ImportedDataObj = nullptr;
    ensure(InParent!=nullptr);
    if (InParent == nullptr)
    {
        UE_LOG(LogNVDataObject, Error, TEXT("invalid argument."));
    }
    else
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(SourceJsonString);
        if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
        {
            return nullptr;
        }

        // TODO: Group all these fields into a namespace
        static const FString ClassNameFieldString = TEXT("_class");
        const FString& ObjectClassName = JsonObject->GetStringField(ClassNameFieldString);
        UClass* FindClassType = UStruct::StaticClass();

        UObject* LoadedClassObj = StaticLoadObject(FindClassType, nullptr, *ObjectClassName, nullptr, LOAD_None, nullptr, true);
        UStruct* ObjectStructType = Cast<UStruct>(LoadedClassObj);
        UClass* ObjectClassType = Cast<UClass>(LoadedClassObj);

        if (ObjectClassType)
        {
            EObjectFlags ObjectFlags = Flags;
            ImportedDataObj = NewObject<UNVDataObject>(InParent, ObjectClassType, InName, ObjectFlags, nullptr);

            if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), ObjectClassType, ImportedDataObj, 0, 0))
            {
                bool bError = true;
            }
        }
    }
    return ImportedDataObj;
}

UObject* UNVDataObject::CreateObjectFromDefinitionData(UNVDataObjectAsset* DefinitionDataObj)
{
    // TODO
    return nullptr;
}

void UNVDataObject::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    if (Ar.IsLoading())
    {
        SetFlags(RF_Transactional);
    }
}
