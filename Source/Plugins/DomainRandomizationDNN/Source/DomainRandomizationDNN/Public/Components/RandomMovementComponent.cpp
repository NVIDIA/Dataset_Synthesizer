/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomMovementComponent.h"

// Sets default values
URandomMovementComponent::URandomMovementComponent()
{
    bIsMoving = false;

    bShouldTeleport = true;
    bCheckCollision = false;
    RandomSpeedRange = FFloatInterval(100.f, 200.f);

    bRelatedToOriginLocation = true;
    bUseObjectAxesInsteadOfWorldAxes = true;
}

void URandomMovementComponent::SetRandomLocationVolume(AVolume* NewVolume, bool bForceUseVolume /*= false*/)
{
    RandomLocationVolume = NewVolume;
    // If is forced to use the volume then we can't use the relative origin location type anymore
    if (bForceUseVolume && RandomLocationVolume)
    {
        bRelatedToOriginLocation = false;
    }
}

void URandomMovementComponent::BeginPlay()
{
    AActor* OwnerActor = GetOwner();
    OriginalTransform = OwnerActor ? OwnerActor->GetTransform() : FTransform();

    Super::BeginPlay();
}

void URandomMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* OwnerActor = GetOwner();
    if (OwnerActor && bIsMoving)
    {
        FVector currentLocation = OwnerActor->GetActorLocation();
        FVector movingDirection = TargetLocation - currentLocation;
        FVector movingDirNorm = movingDirection.GetSafeNormal();
        float targetDistance = movingDirection.Size();

        float maxMoveDistance = CurrentSpeed * DeltaTime;
        float moveDistance = FMath::Min(maxMoveDistance, targetDistance);

        FVector newLocation = currentLocation + movingDirNorm * moveDistance;
        // FIXME: Handle the collision on the way
        OwnerActor->SetActorLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);

        // Finished moving if we got to the destination
        if (maxMoveDistance >= targetDistance)
        {
            bIsMoving = false;
            OnFinishedRandomization();
        }
    }
}

void URandomMovementComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || (!RandomLocationData.ShouldRandomized() && !RandomLocationVolume))
    {
        return;
    }

    if (bRelatedToOriginLocation)
    {
        if (bUseObjectAxesInsteadOfWorldAxes)
        {
            TargetLocation = RandomLocationData.GetRandomLocationInLocalSpace(OriginalTransform);
        }
        else
        {
            TargetLocation = RandomLocationData.GetRandomLocationRelative(OriginalTransform.GetLocation());
        }
    }
    else
    {
        TargetLocation = RandomLocationVolume ? FMath::RandPointInBox(RandomLocationVolume->GetComponentsBoundingBox()) : OwnerActor->GetActorLocation();
    }

    CurrentSpeed = FMath::RandRange(RandomSpeedRange.Min, RandomSpeedRange.Max);

    if (bShouldTeleport)
    {
        const bool bSweep = bCheckCollision;
        OwnerActor->SetActorLocation(TargetLocation, bSweep, nullptr, ETeleportType::TeleportPhysics);
        bIsMoving = false;
    }
    else
    {
        bIsMoving = true;
    }
}

void URandomMovementComponent::OnFinishedRandomization()
{
    if (!bIsMoving)
    {
        Super::OnFinishedRandomization();
    }
}
