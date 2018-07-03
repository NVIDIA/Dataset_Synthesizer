/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomScaleComponent.h"

// Sets default values
URandomScaleComponent::URandomScaleComponent() : Super()
{
}

void URandomScaleComponent::BeginPlay()
{
    AActor* OwnerActor = GetOwner();

    Super::BeginPlay();
}

void URandomScaleComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if ( OwnerActor && RandomScaleData.ShouldRandomized())
    {
        FVector RandomScale3D = RandomScaleData.GetRandomScale3D();
        OwnerActor->SetActorScale3D(RandomScale3D);
    }
}
