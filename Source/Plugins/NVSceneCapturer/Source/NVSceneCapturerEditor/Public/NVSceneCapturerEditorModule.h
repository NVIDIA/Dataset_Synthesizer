/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"
#include "ComponentVisualizer.h"

//General Log
DECLARE_LOG_CATEGORY_EXTERN(LogNVSceneCapturerEditor, Log, All)

class IModuleNVSceneCapturerEditor : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    void StartupModule();
    void ShutdownModule();

protected:
    /** Array of component class names we have registered, so we know what to unregister afterwards */
    TArray<FName> RegisteredComponentClassNames;
    void RegisterComponentVisualizers();
    void RegisterComponentVisualizer(UClass* ComponentClass, TSharedPtr<FComponentVisualizer> Visualizer);
    bool bRegisteredComponentVisualizers;

    void RegisterPlacementModes();
    void RegisterToolbarWidgets();
    void CreateToolbarWidgets(FToolBarBuilder& ToolbarBuilder);

    void RegisterContentMountPoint();
};