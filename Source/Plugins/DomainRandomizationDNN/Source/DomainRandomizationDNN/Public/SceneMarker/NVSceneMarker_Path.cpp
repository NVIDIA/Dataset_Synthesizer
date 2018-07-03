/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "NVSceneMarker_Path.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

ANVSceneMarker_Path::ANVSceneMarker_Path(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ANVSceneMarker_Path::OnObserverAdded(AActor* NewObserver)
{
    INVSceneMarkerInterface::OnObserverAdded(NewObserver);
}