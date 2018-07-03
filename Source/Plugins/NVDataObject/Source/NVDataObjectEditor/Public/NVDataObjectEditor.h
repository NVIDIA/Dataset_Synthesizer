/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"
#include "ComponentVisualizer.h"

//General Log
DECLARE_LOG_CATEGORY_EXTERN(LogNVDataObjectEditor, Log, All)

class FAssetTypeActions_Base;

class IModuleNVDataObjectEditor : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    void StartupModule();
    void ShutdownModule();

protected:
    void ScanDataObjects();

    TArray<TSharedRef<FAssetTypeActions_Base>> AssetTypeActions;
};