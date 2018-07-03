/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomMovementComponent.h"
#include "NVSceneMarker_Volume.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

ANVSceneMarker_Volume::ANVSceneMarker_Volume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bColored = true;
    BrushColor.R = 100;
    BrushColor.G = 255;
    BrushColor.B = 100;
    BrushColor.A = 255;
}

void ANVSceneMarker_Volume::OnObserverAdded(AActor* NewObserver)
{
    INVSceneMarkerInterface::OnObserverAdded(NewObserver);

    if (NewObserver)
    {
        TArray<UActorComponent*> RandomMovementComps = NewObserver->GetComponentsByClass(URandomMovementComponent::StaticClass());
        for (auto& CheckComp : RandomMovementComps)
        {
            URandomMovementComponent* CheckRandomMovementComp = Cast<URandomMovementComponent>(CheckComp);
            if (CheckRandomMovementComp)
            {
                CheckRandomMovementComp->SetRandomLocationVolume(this, true);
                CheckRandomMovementComp->StartRandomizing();
            }
        }
    }
}

void ANVSceneMarker_Volume::OnObserverRemoved(AActor* NewObserver)
{
    if (NewObserver)
    {
        TArray<UActorComponent*> RandomMovementComps = NewObserver->GetComponentsByClass(URandomMovementComponent::StaticClass());
        for (auto& CheckComp : RandomMovementComps)
        {
            URandomMovementComponent* CheckRandomMovementComp = Cast<URandomMovementComponent>(CheckComp);
            if (CheckRandomMovementComp && (CheckRandomMovementComp->GetRandomLocationVolume() == this))
            {
                CheckRandomMovementComp->SetRandomLocationVolume(nullptr, false);
                CheckRandomMovementComp->StopRandomizing();
            }
        }
    }
}
