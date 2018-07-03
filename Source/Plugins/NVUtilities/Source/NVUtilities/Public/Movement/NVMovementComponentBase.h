/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NVMovementComponentBase.generated.h"

/**
 *
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class NVUTILITIES_API UNVMovementComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    UNVMovementComponentBase();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void UpdateSpeed(float DeltaTime);
    virtual void UpdateLocation(float DeltaTime);
    virtual void UpdateRotation(float DeltaTime);
    float GetTravelDistanceInDuration(float DeltaTime);

protected: // Editor properties
    // The min and max movement speed (world units per seconds).
    // NOTE: If the movement doesn't have acceleration then just use the max speed
    UPROPERTY(EditAnywhere)
    FFloatInterval SpeedRange;

    // How fast can the movement speed change (world units per seconds)
    // If SpeedAcceleration < 0 then this movement doesn't use acceleration and instantly move with the maximum speed
    UPROPERTY(EditAnywhere)
    float SpeedAcceleration;

    // If true, rotate the actor to face where it's moving
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFaceMovingDirection;

    // How fast (degree per seconds) the actor rotate when it need to change direction
    UPROPERTY(EditAnywhere)
    float RotationSpeed;

protected:
    UPROPERTY(Transient)
    FVector MoveDirection;

    UPROPERTY(Transient)
    float CurrentSpeed;

    UPROPERTY(Transient)
    float TargetSpeed;
};
