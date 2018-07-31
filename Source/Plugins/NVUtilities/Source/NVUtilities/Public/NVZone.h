/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NVZone.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class NVUTILITIES_API ANVZone : public AActor
{
    GENERATED_BODY()

public:
    ANVZone(const FObjectInitializer& ObjectInitializer);

    FVector GetRandomLocationInZone() const;

protected: // Editor properties
    // How wide is the zone (in the XY plane)
    // NOTE: If the min value > 0 then the zone have an empty area inside
    UPROPERTY(EditAnywhere, Category = Zone)
    FFloatInterval RadiusRange;

    // How tall is the zone (along Z axis)
    // NOTE: Min value is how far does the zone extends below the zone's center location (negative Z axis)
    // Max value is how far the zone extends above the zone's center location (positive Z axis)
    UPROPERTY(EditAnywhere, Category = Zone)
    FFloatInterval HeightRange;
};

UCLASS(Blueprintable)
class NVUTILITIES_API ANVNavigationZone : public ANVZone
{
    GENERATED_BODY()

public:
    ANVNavigationZone(const FObjectInitializer& ObjectInitializer);

protected: // Editor properties
    // If true, the navigating object need to look at the zone's center location
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Zone)
    bool bLookAtCenter;
};
