/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NVWaypoint.generated.h"

/**
 *
 */
UCLASS(Blueprintable, ClassGroup = (NVIDIA))
class NVUTILITIES_API ANVWaypoint : public AActor
{
    GENERATED_BODY()

public:
    ANVWaypoint(const FObjectInitializer& ObjectInitializer);
};
