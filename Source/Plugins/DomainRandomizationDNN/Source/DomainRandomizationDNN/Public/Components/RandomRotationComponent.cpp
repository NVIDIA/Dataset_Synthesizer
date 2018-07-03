/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomRotationComponent.h"

// Sets default values
URandomRotationComponent::URandomRotationComponent()
{
    bRelatedToOriginRotation = false;
}

void URandomRotationComponent::BeginPlay()
{
    AActor* OwnerActor = GetOwner();
    OriginalRotation = OwnerActor ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;

    Super::BeginPlay();
}

void URandomRotationComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && RandomRotationData.ShouldRandomized())
    {
        FRotator RandomRotation = bRelatedToOriginRotation ? RandomRotationData.GetRandomRotationRelative(OriginalRotation) : RandomRotationData.GetRandomRotation();
        OwnerActor->SetActorRotation(RandomRotation);
    }
}
