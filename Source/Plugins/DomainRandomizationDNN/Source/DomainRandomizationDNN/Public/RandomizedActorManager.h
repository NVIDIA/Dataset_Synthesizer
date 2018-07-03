/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "DomainRandomizationDNNPCH.h"
#include "RandomizedActorManager.generated.h"

UCLASS(Blueprintable)
class DOMAINRANDOMIZATIONDNN_API ARandomizedActorManager : public AActor
{
    GENERATED_BODY()

public:
    ARandomizedActorManager();

protected:
    virtual void BeginPlay() override;

protected:
    // List of actor classes to spawn and manage
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TSubclassOf<AActor>> ActorClassesToSpawn;

    // Total number of actors to spawn in the level
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumberOfActorsToSpawn;

    // The volume to choose the location where those managed actor can be in
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    AVolume* RandomLocationVolume;
};
