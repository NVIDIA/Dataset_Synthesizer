/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NVNavigationPath.generated.h"

class ANVWaypoint;

// The enum represent how to navigate a path
UENUM()
enum class ENVPathNavigationType: uint8
{
    // Follow point to point in the WaypointList
    Sequential = 0,
    // Randomly pick the next waypoint after the current one
    Random,
    NVPathNavigationType_MAX UMETA(Hidden)
};

/**
 *
 */
UCLASS(Blueprintable)
class NVUTILITIES_API ANVNavigationPath : public AActor
{
    GENERATED_BODY()

public:
    ANVNavigationPath(const FObjectInitializer& ObjectInitializer);

    ANVWaypoint* GetRandomWaypoint() const;

public: // Editor properties
    // List of waypoints in the path
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Zone)
    TArray<ANVWaypoint*> WaypointList;

    // If true, the last waypoint in the list will be connected to the first one
    // Otherwise the movement stop at the last waypoint
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Zone)
    bool bClosedPath;
};
