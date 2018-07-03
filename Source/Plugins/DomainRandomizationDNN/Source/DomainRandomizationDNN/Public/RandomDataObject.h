/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomDataObject.generated.h"

//====================== URandomDataObject ======================
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, Abstract, ClassGroup = (NVIDIA))
class DOMAINRANDOMIZATIONDNN_API URandomDataObject : public UObject
{
    GENERATED_BODY()

public:
    URandomDataObject();

    bool ShouldRandomize() const;
    virtual void OnRandomization();

protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    bool bShouldRandomize;
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, ClassGroup = (NVIDIA))
class DOMAINRANDOMIZATIONDNN_API URandomMovementDataObject : public URandomDataObject
{
    GENERATED_BODY()

public:
    URandomMovementDataObject();

    virtual void OnRandomization() override;

protected:
    // If true, the owner will be moving around its original location
    // Otherwise the random location will be selected in the RandomLocationVolume
    UPROPERTY(EditAnywhere, Category = Randomization)
    bool bRelatedToOriginLocation;

    UPROPERTY(EditAnywhere, meta = (EditCondition = "bRelatedToOriginLocation"), Category = Randomization)
    FRandomLocationData RandomLocationData;

    // If true, the location will be picked along the object's axes instead of the world axes
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bRelatedToOriginLocation"), Category = Randomization)
    bool bUseObjectAxesInsteadOfWorldAxes;

    // The volume to choose a random location from
    UPROPERTY(EditAnywhere, meta = (EditCondition = "!bRelatedToOriginLocation"), Category = Randomization)
    AVolume* RandomLocationVolume;

    // If true, the actor will instantly teleport whenever location changed
    // otherwise the object will move toward the new location with speed in the RandomSpeedRange
    UPROPERTY(EditAnywhere, Category = Randomization)
    bool bShouldTeleport;

    UPROPERTY(EditAnywhere, meta = (EditCondition = "!bShouldTeleport"), Category = Randomization)
    FFloatInterval RandomSpeedRange;
};

UCLASS(ClassGroup = (NVIDIA))
class DOMAINRANDOMIZATIONDNN_API URandomRotationDataObject : public URandomDataObject
{
    GENERATED_BODY()

public:
    URandomRotationDataObject();

    virtual void OnRandomization() override;

protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    FRandomRotationData RandomRotationData;

    // If true, the owner will be rotated around its original rotation
    // Otherwise the random rotation will be around the Zero rotation
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    bool bRelatedToOriginRotation;

    // TODO: Add support for rotation speed for linear rotation instead of instantly change
};
