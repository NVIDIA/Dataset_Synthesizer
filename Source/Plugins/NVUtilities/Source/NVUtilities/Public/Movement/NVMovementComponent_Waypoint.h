/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#pragma once

#include "CoreMinimal.h"
#include "NVMovementComponentBase.h"
#include "NVNavigationPath.h"
#include "NVMovementComponent_Waypoint.generated.h"

class ANVWaypoint;

/**
 *
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class NVUTILITIES_API UNVMovementComponent_Waypoint : public UNVMovementComponentBase
{
    GENERATED_BODY()

public:
    UNVMovementComponent_Waypoint();

protected:
    virtual void BeginPlay() override;
    virtual void UpdateLocation(float DeltaTime) override;

    void UpdateMoveDirection();
    float GetDistanceToDestination() const;
    void OnWaypointReached();

protected: // Editor properties
    // Reference to the path that the owner actor need to follow
    UPROPERTY(EditInstanceOnly)
    ANVNavigationPath* PathToFollow;

    UPROPERTY(EditInstanceOnly)
    ENVPathNavigationType NavigationType;

    // When the actor get in this distance of a waypoint, it's considered as reach it
    UPROPERTY(EditAnywhere)
    float CloseEnoughDistance;

    // How long to stay at each waypoint
    UPROPERTY(EditAnywhere)
    float WaypointStopDuration;

    // If true, instead of moving between waypoint, just teleport instantly between them
    UPROPERTY(EditAnywhere)
    bool bShouldTeleport;

protected: // Transient properties
    UPROPERTY(Transient)
    ANVWaypoint* CurrentDestinationWaypoint;

    UPROPERTY(Transient)
    int32 CurrentWaypointIndex;

    UPROPERTY(Transient)
    float WaitingCountdown;
};
