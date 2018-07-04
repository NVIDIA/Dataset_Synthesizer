/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "DomainRandomizationDNNModule.h"
#include "ModuleManager.h"

IMPLEMENT_GAME_MODULE(FDomainRandomizationDNNModule, DomainRandomizationDNN);

#define LOCTEXT_NAMESPACE "DomainRandomizationDNNModule"

void FDomainRandomizationDNNModule::StartupModule()
{
}

void FDomainRandomizationDNNModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
