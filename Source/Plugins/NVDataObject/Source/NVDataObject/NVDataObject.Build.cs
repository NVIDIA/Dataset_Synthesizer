/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

using UnrealBuildTool;
using System.IO;
 
public class NVDataObject : ModuleRules
{
	public NVDataObject(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] { "NVDataObject/Private" });
 
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "JsonUtilities", "InputCore"});
        PrivateDependencyModuleNames.AddRange(new string[] { });

        PrivatePCHHeaderFile = "Public/NVDataObjectModule.h";

        bUseUnity = true;
    }
}