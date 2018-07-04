/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "OrbitalMovementComponent.generated.h"

/**
* RandomTextureComponent randomly change the color parameter of the materials in the owner's mesh
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent), HideCategories = (Replication, ComponentReplication, Cooking, Events, ComponentTick, Actor, Input, Rendering, Collision, PhysX, Activation, Sockets))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API UOrbitalMovementComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    UOrbitalMovementComponent();

    AActor* GetFocalActor();
    void SetFocalActor(AActor* NewFocalActor);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void OnRandomization_Implementation() override;

    void UpdateDistanceToTarget();
    void OnYawRotationCompleted(float DeltaTime);

    void TeleportRandomly();

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = "Movement")
    bool bShouldMove;

    // If true, the owner will teleport instead of moving between random locations on the constrained sphere
    UPROPERTY(EditAnywhere, Category = "Movement")
    bool bTeleportRandomly;

    // If true, the actor will not be able to move if its way is blocked
    // Otherwise the actor can go through other object or teleport overlap with the other
    UPROPERTY(EditAnywhere)
    bool bCheckCollision;

    // The actor which is the center of the orbit movement
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    AActor* FocalTargetActor;

    // How fast (angle per second) should the owner rotate
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    float RotationSpeed;

    // How fast (angle per second) should the owner change the pitch
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    float PitchRotationSpeed;

    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    FFloatInterval YawRotationRange;

    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    FFloatInterval PitchRotationRange;

    // If true, after each full yaw rotation, the yaw direction change from min to max to max to min and vice-versa
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    bool bRevertYawDirectionAfterEachRotation;

    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldMove))
    bool bRandomizePitchAfterEachYawRotation;

    // If true, the owner will change its distance (selected randomly in TargetDistanceRange) to the target when orbiting
    UPROPERTY(EditAnywhere, Category = "Movement")
    bool bShouldChangeDistance;

    // How far does the owner need to stay from the target
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldChangeDistance))
    FFloatInterval TargetDistanceRange;

    // How fast (unit per second) does the owner change its distance from the target
    // NOTE: This speed is independent from the rotation speed
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldChangeDistance))
    float DistanceChangeSpeed;

    // How long (seconds) to wait before the owner change its distance to the target
    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldChangeDistance))
    float TargetDistanceChangeDuration;

    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldChangeDistance))
    bool bOnlyChangeDistanceWhenPitchChanged;

    // If true, the owner also randomly rotated when moving
    UPROPERTY(EditAnywhere, Category = "Movement")
    bool bShouldWiggle;

    UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = bShouldWiggle))
    FRandomRotationData WiggleRotationData;

protected: // Transient properties
    UPROPERTY(Transient)
    FRotator RotationFromTarget;
    UPROPERTY(Transient)
    float CurrentDistanceToTarget;
    UPROPERTY(Transient)
    float DistanceToTarget;
    UPROPERTY(Transient)
    float DistanceChangeCountdown;
    UPROPERTY(Transient)
    float TargetYaw;
};
