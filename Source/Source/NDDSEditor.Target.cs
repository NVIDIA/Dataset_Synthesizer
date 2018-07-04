/*

* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0

* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)

*/

using UnrealBuildTool;
using System.Collections.Generic;

public class NDDSEditorTarget : TargetRules
{
	public NDDSEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "NDDS" } );
	}
}
