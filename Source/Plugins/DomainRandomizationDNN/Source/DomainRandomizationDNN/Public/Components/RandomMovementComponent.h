/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "RandomMovementComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMovementComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomMovementComponent();

    AVolume* GetRandomLocationVolume() const
    {
        return RandomLocationVolume;
    }
    void SetRandomLocationVolume(AVolume* NewVolume, bool bForceUseVolume = false);

protected: // Editor properties

    // If true, the owner will be moving around its original location
    // Otherwise the random location will be selected in the RandomLocationVolume
    UPROPERTY(EditAnywhere)
    bool bRelatedToOriginLocation;

    UPROPERTY(EditAnywhere, meta = (EditCondition = "bRelatedToOriginLocation"))
    FRandomLocationData RandomLocationData;

    // If true, the location will be picked along the object's axes instead of the world axes
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bRelatedToOriginLocation"))
    bool bUseObjectAxesInsteadOfWorldAxes;

    // The volume to choose a random location from
    UPROPERTY(EditAnywhere, meta = (EditCondition = "!bRelatedToOriginLocation"))
    AVolume* RandomLocationVolume;

    // If true, the actor will instantly teleport whenever location changed
    // otherwise the object will move toward the new location with speed in the RandomSpeedRange
    UPROPERTY(EditAnywhere)
    bool bShouldTeleport;

    // If true, the actor will not be able to move if its way is blocked
    // Otherwise the actor can go through other object or teleport overlap with the other
    UPROPERTY(EditAnywhere)
    bool bCheckCollision;

    UPROPERTY(EditAnywhere, meta=(EditCondition="!bShouldTeleport"))
    FFloatInterval RandomSpeedRange;

protected:
    void BeginPlay() override;
    void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void OnRandomization_Implementation() override;
    void OnFinishedRandomization() override;

protected:
    bool bIsMoving;
    FVector TargetLocation;
    float CurrentSpeed;

    UPROPERTY(Transient)
    FTransform OriginalTransform;
};
