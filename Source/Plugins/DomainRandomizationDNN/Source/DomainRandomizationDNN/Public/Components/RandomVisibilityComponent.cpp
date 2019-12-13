/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomVisibilityComponent.h"

// Sets default values
URandomVisibilityComponent::URandomVisibilityComponent()
{
}

void URandomVisibilityComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        bool bNewHidden = !OwnerActor->IsHidden();

        OwnerActor->SetActorHiddenInGame(bNewHidden);
        OwnerActor->SetActorEnableCollision(!bNewHidden);
    }
}
