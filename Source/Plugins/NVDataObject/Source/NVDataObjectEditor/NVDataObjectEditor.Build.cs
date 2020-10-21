/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

using UnrealBuildTool;
using System.IO;
 
public class NVDataObjectEditor : ModuleRules
{
	public NVDataObjectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "JsonUtilities", "InputCore", "RHI", "UnrealEd", "DesktopPlatform", "MainFrame" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "EditorStyle", "AssetTools", "AssetRegistry", "PlacementMode", "LevelEditor", "PropertyEditor" });
        PrivateDependencyModuleNames.AddRange(new string[] { "NVDataObject" });

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseUnity = true;
    }
}