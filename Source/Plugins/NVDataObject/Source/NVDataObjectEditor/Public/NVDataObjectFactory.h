/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "AssetTypeActions_Base.h"
#include "Factories/Factory.h"
#include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#include "NVDataObjectFactory.generated.h"

class UNVDataObjectAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogNVDataObjectFactory, Log, All)

UCLASS(hidecategories = Object)
class  UNVDataObjectFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()

    UNVDataObjectFactory(const FObjectInitializer& ObjectInitializer);
public:
    UPROPERTY(EditAnywhere, Category = DataAsset)
    TSubclassOf<UNVDataObjectAsset> NVDataObjectClass;

    // UFactory interface
    virtual bool ConfigureProperties() override;

    virtual FText GetDisplayName() const override;
    virtual bool DoesSupportClass(UClass* Class) override;
    virtual bool FactoryCanImport(const FString& Filename) override;
    virtual IImportSettingsParser* GetImportSettingsParser() override
    {
        return nullptr;
    }

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
    // End of UFactory interface

    //~ Begin FReimportHandler Interface
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override;
    //~ End FReimportHandler Interface
};
