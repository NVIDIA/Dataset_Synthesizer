/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"

IMPLEMENT_MODULE(INVSceneCapturerModule, NVSceneCapturer)

//General Log
DEFINE_LOG_CATEGORY(LogNVSceneCapturer);

void INVSceneCapturerModule::StartupModule()
{
    UE_LOG(LogNVSceneCapturer, Warning, TEXT("Loaded NVSceneCapturer module"));
}

void INVSceneCapturerModule::ShutdownModule()
{
}

