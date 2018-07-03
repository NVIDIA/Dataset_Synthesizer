/* 
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

using System;
using System.IO;
using System.Collections.Generic;
using UnrealBuildTool;

public class DomainRandomizationDNN : ModuleRules
{
    public DomainRandomizationDNN(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "ShaderCore",
            }
        );

        // FIXME: This module shouldn't depend on the NVSceneCapturer, we should have a Core module which both of these modules share
        PrivateDependencyModuleNames.Add("NVSceneCapturer");

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("AssetRegistry");
        }

        PublicIncludePaths.AddRange(new string[] { "DomainRandomizationDNN/Public" });
    }
}
