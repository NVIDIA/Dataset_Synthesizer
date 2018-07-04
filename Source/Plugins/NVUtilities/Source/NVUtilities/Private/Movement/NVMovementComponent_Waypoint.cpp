/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#include "NVUtilitiesModule.h"
#include "NVMovementComponent_Waypoint.h"
#include "NVWaypoint.h"
#include "NVNavigationPath.h"

UNVMovementComponent_Waypoint::UNVMovementComponent_Waypoint() : Super()
{
    CloseEnoughDistance = 1.f;
    WaypointStopDuration = 0.f;
    PathToFollow = nullptr;
    CurrentWaypointIndex = -1;
    NavigationType = ENVPathNavigationType::Sequential;
}

void UNVMovementComponent_Waypoint::BeginPlay()
{
    Super::BeginPlay();

    // Must be at least 1 unit, otherwise we can't get to exact location
    CloseEnoughDistance = FMath::Max(CloseEnoughDistance, 1.f);

    OnWaypointReached();
    UpdateMoveDirection();

    WaitingCountdown = 0.f;
}

void UNVMovementComponent_Waypoint::UpdateLocation(float DeltaTime)
{
    if (WaitingCountdown > 0.f)
    {
        WaitingCountdown -= DeltaTime;
    }
    else if (CurrentDestinationWaypoint)
    {
        const float DestDistance = GetDistanceToDestination();
        if (DestDistance <= CloseEnoughDistance)
        {
            OnWaypointReached();
        }
        else
        {
            UpdateMoveDirection();
            TargetSpeed = SpeedRange.Max;
            const float MoveDistance = FMath::Min(GetTravelDistanceInDuration(DeltaTime), DestDistance);

            if ((MoveDistance > 0.f) && (!MoveDirection.IsZero()))
            {
                AActor* OwnerActor = GetOwner();
                const FVector CurrentLocation = OwnerActor->GetActorLocation();
                const FVector NewLocation = CurrentLocation + MoveDirection * MoveDistance;
                // NOTE: Just teleport the actor to the new location
                OwnerActor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
            }
        }
    }
}

void UNVMovementComponent_Waypoint::UpdateMoveDirection()
{
    MoveDirection = FVector::ZeroVector;
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && CurrentDestinationWaypoint)
    {
        if (bShouldTeleport)
        {
            const FTransform& DestinationTransform = CurrentDestinationWaypoint->GetActorTransform();
            OwnerActor->SetActorTransform(DestinationTransform, false, nullptr, ETeleportType::None);

            MoveDirection = FVector::ZeroVector;
        }
        else
        {
            const FVector& CurrentLocation = OwnerActor->GetActorLocation();
            const FVector& DestLocation = CurrentDestinationWaypoint->GetActorLocation();

            MoveDirection = (DestLocation - CurrentLocation).GetSafeNormal();
        }
    }
}

float UNVMovementComponent_Waypoint::GetDistanceToDestination() const
{
    float Distance = 0.f;
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && CurrentDestinationWaypoint)
    {
        const FVector CurrentLocation = OwnerActor->GetActorLocation();
        const FVector DestLocation = CurrentDestinationWaypoint->GetActorLocation();

        Distance = (DestLocation - CurrentLocation).Size();
    }

    return Distance;
}

void UNVMovementComponent_Waypoint::OnWaypointReached()
{
    if (PathToFollow)
    {
        // TODO: Let the ANVNavigationPath handle the navigation type
        if (NavigationType == ENVPathNavigationType::Sequential)
        {
            CurrentDestinationWaypoint = nullptr;
            CurrentWaypointIndex++;
            int32 TotalWaypointCount = PathToFollow->WaypointList.Num();
            if ((CurrentWaypointIndex >= TotalWaypointCount) && PathToFollow->bClosedPath)
            {
                CurrentWaypointIndex = 0;
            }

            if (CurrentWaypointIndex < TotalWaypointCount)
            {
                CurrentDestinationWaypoint = PathToFollow->WaypointList[CurrentWaypointIndex];
            }
        }
        else
        {
            ANVWaypoint* NextWaypoint = PathToFollow->GetRandomWaypoint();
            while (NextWaypoint == CurrentDestinationWaypoint)
            {
                NextWaypoint = PathToFollow->GetRandomWaypoint();
            }
            CurrentDestinationWaypoint = NextWaypoint;
        }

    }
    else
    {
        CurrentWaypointIndex = -1;
    }

    if (CurrentDestinationWaypoint)
    {
        UpdateMoveDirection();
    }

    WaitingCountdown = WaypointStopDuration;

    TargetSpeed = 0.f;
}
