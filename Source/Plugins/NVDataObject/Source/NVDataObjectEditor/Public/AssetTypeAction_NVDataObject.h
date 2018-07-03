/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "AssetTypeActions_Base.h"
#include "NVDataObject.h"

class FAssetTypeAction_NVDataObject : public FAssetTypeActions_Base
{
public:
    // IAssetTypeActions Implementation
    virtual FText GetName() const override
    {
        return NSLOCTEXT("AssetTypeActions", "AssetTypeAction_NVDataObject", "Nvidia Data Object");
    }
    virtual FColor GetTypeColor() const override
    {
        return FColor(80, 255, 80);
    }
    virtual UClass* GetSupportedClass() const override
    {
        return UNVDataObjectAsset::StaticClass();
    }
    virtual uint32 GetCategories() override
    {
        return EAssetTypeCategories::Misc;
    }

    virtual bool HasActions(const TArray<UObject*>& InObjects) const override
    {
        return true;
    }
    virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
    virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
    // End IAssetTypeActions Implementation

protected:
    /** Handler for when Json is selected */
    void ExecuteExportAsJson(TArray< TWeakObjectPtr<UNVDataObjectAsset> > Objects);

    /** Handler for opening the source file for this asset */
    bool CanOpenSourceFiles(const TArray<TWeakObjectPtr<UNVDataObjectAsset>> DataObjects);
    void OpenSourceFiles(const TArray<TWeakObjectPtr<UNVDataObjectAsset>> DataObjects);
};
