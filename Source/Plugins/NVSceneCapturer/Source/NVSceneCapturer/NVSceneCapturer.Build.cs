/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

using UnrealBuildTool;
using System.IO;
 
public class NVSceneCapturer : ModuleRules
{
	public NVSceneCapturer(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] { "NVSceneCapturer/Private" });
 
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "JsonUtilities", "InputCore", "RHI", "RenderCore"});
        PublicDependencyModuleNames.AddRange(new string[] { "MovieSceneCapture", "ImageWrapper" });

        PrivateDependencyModuleNames.AddRange(new string[] { "zlib", "UElibPNG" } );

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("UnrealEd");
        }

        PrivatePCHHeaderFile = "Public/NVSceneCapturerModule.h";

        bUseUnity = true;
    }
}