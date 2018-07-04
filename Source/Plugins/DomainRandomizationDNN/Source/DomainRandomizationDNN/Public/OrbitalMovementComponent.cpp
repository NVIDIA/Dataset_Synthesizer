/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "DRUtils.h"
#include "OrbitalMovementComponent.h"

UOrbitalMovementComponent::UOrbitalMovementComponent()
{
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.bCanEverTick = true;

    bShouldMove = false;
    bShouldChangeDistance = false;
    bOnlyChangeDistanceWhenPitchChanged = false;
    bRevertYawDirectionAfterEachRotation = false;
    bShouldWiggle = false;
    FocalTargetActor = nullptr;
    YawRotationRange = FFloatInterval(-30.f, 30.f);
    PitchRotationRange = FFloatInterval(30.f, 80.f);
    RotationSpeed = 10.f;
    PitchRotationSpeed = 0.f;
    bRandomizePitchAfterEachYawRotation = false;

    bTeleportRandomly = false;
    bCheckCollision = false;

    TargetDistanceRange = FFloatInterval(1000.f, 2000.f);
    DistanceChangeSpeed = 100.f;
    TargetDistanceChangeDuration = 1.f;
}

AActor* UOrbitalMovementComponent::GetFocalActor()
{
    return FocalTargetActor;
}

void UOrbitalMovementComponent::SetFocalActor(AActor* NewFocalActor)
{
    FocalTargetActor = NewFocalActor;

    // Reset the rotation
    RotationFromTarget.Yaw = YawRotationRange.Min;
    UpdateDistanceToTarget();
    if (bRandomizePitchAfterEachYawRotation)
    {
        RotationFromTarget.Pitch = FMath::RandRange(PitchRotationRange.Min, PitchRotationRange.Max);
    }
    else
    {
        RotationFromTarget.Pitch = PitchRotationRange.Min;
    }
}

void UOrbitalMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    // FIXME: May want to calculate the rotation from the current location of the exporter compare to the target
    RotationFromTarget = FRotator::ZeroRotator;
    RotationFromTarget.Pitch = PitchRotationRange.Min;
    RotationFromTarget.Yaw = YawRotationRange.Min;

    const FVector TargetLocation = FocalTargetActor ? FocalTargetActor->GetActorLocation() : FVector::ZeroVector;
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        DistanceToTarget = (OwnerActor->GetActorLocation() - TargetLocation).Size();
    }
    UpdateDistanceToTarget();

    if (PitchRotationSpeed <= 0.f)
    {
        PitchRotationSpeed = RotationSpeed;
    }
}

void UOrbitalMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* OwnerActor = GetOwner();
    if (!bShouldMove || !OwnerActor)
    {
        return;
    }

    if (bTeleportRandomly)
    {
        TeleportRandomly();
        return;
    }

    if (DistanceChangeCountdown > 0.f)
    {
        DistanceChangeCountdown -= DeltaTime;
    }

    if (DistanceChangeSpeed > 0.f)
    {
        const float TargetDistanceChanged = DistanceChangeSpeed * DeltaTime;
        if (CurrentDistanceToTarget < DistanceToTarget)
        {
            CurrentDistanceToTarget += TargetDistanceChanged;
            if (CurrentDistanceToTarget > DistanceToTarget)
            {
                CurrentDistanceToTarget = DistanceToTarget;
            }
        }
        else
        {
            CurrentDistanceToTarget -= TargetDistanceChanged;
            if (CurrentDistanceToTarget < DistanceToTarget)
            {
                CurrentDistanceToTarget = DistanceToTarget;
            }
        }
    }
    else
    {
        CurrentDistanceToTarget = DistanceToTarget;
    }

    // Update movement
    if (RotationFromTarget.Yaw != TargetYaw)
    {
        const float RotationAmount = RotationSpeed * DeltaTime;
        if (RotationFromTarget.Yaw < TargetYaw)
        {
            RotationFromTarget.Yaw += RotationAmount;
            if (RotationFromTarget.Yaw > TargetYaw)
            {
                RotationFromTarget.Yaw = TargetYaw;
            }
        }
        else
        {
            RotationFromTarget.Yaw -= RotationAmount;
            if (RotationFromTarget.Yaw < TargetYaw)
            {
                RotationFromTarget.Yaw = TargetYaw;
            }
        }

        if (!bOnlyChangeDistanceWhenPitchChanged)
        {
            UpdateDistanceToTarget();
        }
    }
    if (RotationFromTarget.Yaw == TargetYaw)
    {
        OnYawRotationCompleted(DeltaTime);
    }

    const FVector TargetLocation = FocalTargetActor ? FocalTargetActor->GetActorLocation() : FVector::ZeroVector;
    const FRotator TargetForwardRotation = FocalTargetActor ? FocalTargetActor->GetActorRotation() : FRotator::ZeroRotator;
    const FVector TargetToExporterDir = TargetForwardRotation.RotateVector(RotationFromTarget.Vector().GetSafeNormal());
    const FVector NewLocation = TargetLocation + TargetToExporterDir * CurrentDistanceToTarget;

    FRotator NewRotation = (-TargetToExporterDir).Rotation();
    if (bShouldWiggle)
    {
        NewRotation = WiggleRotationData.GetRandomRotationRelative(NewRotation);
    }

    OwnerActor->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UOrbitalMovementComponent::OnRandomization_Implementation()
{
    Super::OnRandomization_Implementation();
    if (bTeleportRandomly)
    {
        TeleportRandomly();
    }
}

void UOrbitalMovementComponent::UpdateDistanceToTarget()
{
    if (bShouldChangeDistance && (DistanceChangeCountdown <= 0.f))
    {
        DistanceToTarget = FMath::RandRange(TargetDistanceRange.Min, TargetDistanceRange.Max);
        DistanceChangeCountdown = TargetDistanceChangeDuration;
    }
}

void UOrbitalMovementComponent::OnYawRotationCompleted(float DeltaTime)
{
    if (bRevertYawDirectionAfterEachRotation)
    {
        TargetYaw = (TargetYaw <= YawRotationRange.Min) ? YawRotationRange.Max : YawRotationRange.Min;
    }
    else
    {
        RotationFromTarget.Yaw = YawRotationRange.Min;
        TargetYaw = YawRotationRange.Max;
    }

    if (bRandomizePitchAfterEachYawRotation)
    {
        RotationFromTarget.Pitch = FMath::RandRange(PitchRotationRange.Min, PitchRotationRange.Max);
    }
    else
    {
        const float PitchRotationAmount = PitchRotationSpeed * DeltaTime;
        RotationFromTarget.Pitch += PitchRotationAmount;
        // If we reach the top pitch then reset back from the bottom
        if (RotationFromTarget.Pitch > PitchRotationRange.Max)
        {
            RotationFromTarget.Pitch = PitchRotationRange.Min;
        }
    }

    UpdateDistanceToTarget();
}

void UOrbitalMovementComponent::TeleportRandomly()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        RotationFromTarget.Yaw = FMath::RandRange(YawRotationRange.Min, YawRotationRange.Max);
        RotationFromTarget.Pitch = FMath::RandRange(PitchRotationRange.Min, PitchRotationRange.Max);
        DistanceToTarget = FMath::RandRange(TargetDistanceRange.Min, TargetDistanceRange.Max);
        CurrentDistanceToTarget = DistanceToTarget;

        const FVector TargetLocation = FocalTargetActor ? FocalTargetActor->GetActorLocation() : FVector::ZeroVector;
        const FRotator TargetForwardRotation = FocalTargetActor ? FocalTargetActor->GetActorRotation() : FRotator::ZeroRotator;
        const FVector TargetToOwnerDir = TargetForwardRotation.RotateVector(RotationFromTarget.Vector().GetSafeNormal());
        const FVector NewLocation = TargetLocation + TargetToOwnerDir * CurrentDistanceToTarget;

        const bool bSweep = bCheckCollision;
        const ETeleportType TeleportType = bCheckCollision ? ETeleportType::None : ETeleportType::TeleportPhysics;
        OwnerActor->SetActorLocation(NewLocation, bSweep, nullptr, TeleportType);

        const FVector OwnerToTarget = TargetLocation - OwnerActor->GetActorLocation();
        FRotator NewRotation = OwnerToTarget.Rotation();
        if (bShouldWiggle)
        {
            NewRotation = WiggleRotationData.GetRandomRotationRelative(NewRotation);
        }

        OwnerActor->SetActorRotation(NewRotation, TeleportType);
    }
}
