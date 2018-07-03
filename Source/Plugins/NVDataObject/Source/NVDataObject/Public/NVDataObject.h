/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVDataObject.generated.h"

class UNVDataObject;

DECLARE_LOG_CATEGORY_EXTERN(LogNVDataObject, Log, All)

// UNVDataObject contain data which define how we want to create an object in the game
UCLASS()
class NVDATAOBJECT_API UNVDataObjectAsset : public UObject
{
    GENERATED_BODY()

public:
    UNVDataObjectAsset(const FObjectInitializer& ObjectInitializer);

    static bool SerializeToJsonFile(const UNVDataObjectAsset* DataObject, const FString& OutputFilePath);
    static UNVDataObjectAsset* DeserializeFromJsonFile(const FString& SourceFilePath, UObject* InParent, FName InName, EObjectFlags Flags);
    static UNVDataObjectAsset* DeserializeFromJsonString(const FString& SourceJsonString, UObject* InParent, FName InName, EObjectFlags Flags);
    static UObject* CreateObjectFromDefinitionData(UNVDataObjectAsset* DefinitionDataObj);

    virtual void Serialize(FArchive& Ar) override;

    virtual bool IsAsset() const override
    {
        return true;
    }

    virtual void PostSaveRoot(bool bCleanupIsRequired) override;

public: // Editor properties
    UPROPERTY(EditAnywhere, SimpleDisplay)
    UNVDataObject* DataObject;

#if WITH_EDITORONLY_DATA
public:
    UPROPERTY(VisibleAnywhere, Instanced, AdvancedDisplay, Category = Config)
    class UAssetImportData* AssetImportData;

    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
    virtual void PostInitProperties() override;
#endif //WITH_EDITORONLY_DATA
};

UCLASS(DefaultToInstanced, editinlinenew)
class NVDATAOBJECT_API UNVDataObject : public UObject
{
    GENERATED_BODY()

public:
    UNVDataObject(const FObjectInitializer& ObjectInitializer);

    static bool SerializeToJsonFile(const UNVDataObject* DataObject, const FString& OutputFilePath);
    static UNVDataObject* DeserializeFromJsonFile(const FString& SourceFilePath, UObject* InParent, FName InName, EObjectFlags Flags);
    static UNVDataObject* DeserializeFromJsonString(const FString& SourceJsonString, UObject* InParent, FName InName, EObjectFlags Flags);
    static UObject* CreateObjectFromDefinitionData(UNVDataObjectAsset* DefinitionDataObj);

    virtual void Serialize(FArchive& Ar) override;

protected: // Editor properties
    // The class of the logic object
    UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = Config)
    TSubclassOf<UObject> LogicHandlerClassType;
};

UINTERFACE()
class NVDATAOBJECT_API UNVDataObjectOwner : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

class NVDATAOBJECT_API INVDataObjectOwner
{
    GENERATED_IINTERFACE_BODY()

public:
    virtual void SetDataObject(UNVDataObjectAsset* NewDataObject) = 0;
    virtual const UNVDataObjectAsset* GetDataObject() const = 0;
};
