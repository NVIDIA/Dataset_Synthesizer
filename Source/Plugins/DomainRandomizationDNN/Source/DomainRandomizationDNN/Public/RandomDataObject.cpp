/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomDataObject.h"

URandomDataObject::URandomDataObject()
{
    bShouldRandomize = true;
}

bool URandomDataObject::ShouldRandomize() const
{
    return bShouldRandomize;
}

void URandomDataObject::OnRandomization()
{
    // TODO
}

URandomMovementDataObject::URandomMovementDataObject() : Super()
{
    bRelatedToOriginLocation = true;
    bShouldTeleport = true;
}

void URandomMovementDataObject::OnRandomization()
{
    UObject* OuterObj = GetOuter();
    AActor* OwnerActor = Cast<AActor>(OuterObj);
    if (!OwnerActor || (!RandomLocationData.ShouldRandomized() && !RandomLocationVolume))
    {
        return;
    }

    FVector TargetLocation = FVector::ZeroVector;
    if (bRelatedToOriginLocation)
    {
        if (bUseObjectAxesInsteadOfWorldAxes)
        {
            FTransform OriginalTransform = FTransform::Identity;
            TargetLocation = RandomLocationData.GetRandomLocationInLocalSpace(OriginalTransform);
        }
        // FIXME
        //if (bUseObjectAxesInsteadOfWorldAxes)
        //{
        //  TargetLocation = RandomLocationData.GetRandomLocationInLocalSpace(OriginalTransform);
        //}
        //else
        //{
        //  TargetLocation = RandomLocationData.GetRandomLocationRelative(OriginalTransform.GetLocation());
        //}
    }
    else
    {
        TargetLocation = FMath::RandPointInBox(RandomLocationVolume->GetComponentsBoundingBox());
    }

    if (bShouldTeleport)
    {
        OwnerActor->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

URandomRotationDataObject::URandomRotationDataObject() : Super()
{
    bRelatedToOriginRotation = true;
}

void URandomRotationDataObject::OnRandomization()
{
    UObject* OuterObj = GetOuter();
    AActor* OwnerActor = Cast<AActor>(OuterObj);
    if (!OwnerActor || (!RandomRotationData.ShouldRandomized()))
    {
        return;
    }
    FRotator OriginalRotation = FRotator::ZeroRotator;
    FRotator RandomRotation = bRelatedToOriginRotation ? RandomRotationData.GetRandomRotationRelative(OriginalRotation) : RandomRotationData.GetRandomRotation();
    OwnerActor->SetActorRotation(RandomRotation);
}
