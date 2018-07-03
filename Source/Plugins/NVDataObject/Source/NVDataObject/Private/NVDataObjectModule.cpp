/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVDataObjectModule.h"

IMPLEMENT_MODULE(INVDataObjectModule, NVDataObject)

//General Log
DEFINE_LOG_CATEGORY(LogNVDataObjectModule);

void INVDataObjectModule::StartupModule()
{
    UE_LOG(LogNVDataObjectModule, Warning, TEXT("Loaded NVDataObject Main"));
}

void INVDataObjectModule::ShutdownModule()
{
}

