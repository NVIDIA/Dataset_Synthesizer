/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#include "NVUtilitiesModule.h"
#include "NVMovementComponentBase.h"

UNVMovementComponentBase::UNVMovementComponentBase() : Super()
{
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    bAutoRegister = true;

    SpeedRange = FFloatInterval(100.f, 1000.f);
    SpeedAcceleration = -1.f;
    MoveDirection = FVector::ZeroVector;

    RotationSpeed = 90.f;

    CurrentSpeed = 0.f;
    TargetSpeed = 0.f;
}

void UNVMovementComponentBase::BeginPlay()
{
    Super::BeginPlay();

    // NOTE: May need to support which component affected by this movement component
    // Right now we just always move the owner actor

    // Mark the owner movable
    if (AActor* OwnerActor = GetOwner())
    {
        if (!OwnerActor->IsRootComponentMovable())
        {
            USceneComponent* OwnerRootComp = OwnerActor->GetRootComponent();
            if (OwnerRootComp)
            {
                OwnerRootComp->SetMobility(EComponentMobility::Movable);
            }
        }
    }

    TargetSpeed = SpeedRange.Max;
}

void UNVMovementComponentBase::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateSpeed(DeltaTime);
    UpdateLocation(DeltaTime);
    UpdateRotation(DeltaTime);
}

void UNVMovementComponentBase::UpdateSpeed(float DeltaTime)
{
    float NewSpeed = FMath::Min(CurrentSpeed + SpeedAcceleration * DeltaTime, TargetSpeed);

    CurrentSpeed = NewSpeed;
}

void UNVMovementComponentBase::UpdateLocation(float DeltaTime)
{
    const float MoveDistance = GetTravelDistanceInDuration(DeltaTime);
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && (MoveDistance > 0.f) && (!MoveDirection.IsZero()))
    {
        const FVector CurrentLocation = OwnerActor->GetActorLocation();
        const FVector NewLocation = CurrentLocation + MoveDirection * MoveDistance;
        // NOTE: Just teleport the actor to the new location
        OwnerActor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
    }
}

void UNVMovementComponentBase::UpdateRotation(float DeltaTime)
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && bFaceMovingDirection)
    {
        const FRotator CurrentRotation = OwnerActor->GetActorRotation();
        const FRotator TargetRotation = MoveDirection.ToOrientationRotator();
        const FVector CurrentDirection = CurrentRotation.Vector();
        const FVector TargetDirection = MoveDirection;

        if (!(CurrentDirection - TargetDirection).IsNearlyZero())
        {
            const FVector NewDirection = FMath::VInterpNormalRotationTo(CurrentDirection, TargetDirection, DeltaTime, RotationSpeed);
            const FRotator NewRotation = NewDirection.ToOrientationRotator();

            OwnerActor->SetActorRotation(NewRotation);
        }
    }
}

float UNVMovementComponentBase::GetTravelDistanceInDuration(float DeltaTime)
{
    // Just support linear movement for now
    const float MoveDistance = CurrentSpeed * DeltaTime;
    return MoveDistance;
}
