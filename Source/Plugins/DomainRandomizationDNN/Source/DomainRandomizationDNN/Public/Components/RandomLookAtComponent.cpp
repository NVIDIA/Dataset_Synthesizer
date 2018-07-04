/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomLookAtComponent.h"

// Sets default values
URandomLookAtComponent::URandomLookAtComponent()
{
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.bCanEverTick = true;
    CurrentFocalTarget = nullptr;
    RotationSpeed = 90.f;
}

void URandomLookAtComponent::BeginPlay()
{
    Super::BeginPlay();
}

void URandomLookAtComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || (FocalTargetActors.Num() <= 0))
    {
        return;
    }

    AActor* NewFocalTarget = FocalTargetActors[FMath::Rand() % FocalTargetActors.Num()];
    if (NewFocalTarget)
    {
        CurrentFocalTarget = NewFocalTarget;
    }
}

void URandomLookAtComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* OwnerActor = GetOwner();
    if (OwnerActor && CurrentFocalTarget)
    {
        const FVector& OwnerLocation = OwnerActor->GetActorLocation();
        const FVector& TargetLocation = CurrentFocalTarget->GetActorLocation();
        const FVector& TargetDirection = TargetLocation - OwnerLocation;

        const FRotator CurrentRotation = OwnerActor->GetActorRotation();
        const FVector CurrentDirection = CurrentRotation.Vector();

        if (!(CurrentDirection - TargetDirection).IsNearlyZero())
        {
            const FVector NewDirection = FMath::VInterpNormalRotationTo(CurrentDirection, TargetDirection, DeltaTime, RotationSpeed);
            const FRotator NewRotation = NewDirection.ToOrientationRotator();

            OwnerActor->SetActorRotation(NewRotation);
        }
    }
}
