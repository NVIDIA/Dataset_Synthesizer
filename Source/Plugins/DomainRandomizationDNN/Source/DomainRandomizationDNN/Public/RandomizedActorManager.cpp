/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomizedActorManager.h"
#include "RandomMovementComponent.h"

// Sets default values
ARandomizedActorManager::ARandomizedActorManager()
{
    NumberOfActorsToSpawn = 0;
    RandomLocationVolume = nullptr;
}

void ARandomizedActorManager::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();

    FBox LocationBox = RandomLocationVolume ? RandomLocationVolume->GetComponentsBoundingBox() : FBox(ForceInitToZero);
    for (int i = 0; i < NumberOfActorsToSpawn; i++)
    {
        TSubclassOf<AActor> ActorClass = ActorClassesToSpawn[FMath::Rand() % ActorClassesToSpawn.Num()];
        FVector SpawnLocation = FMath::RandPointInBox(LocationBox);
        // TODO: May need to pick random rotation for the actor too
        FRotator SpawnRotation = FRotator::ZeroRotator;
        FVector SpawnScale3D = FVector(1.f, 1.f, 1.f);

        FActorSpawnParameters SpawnParam;
        SpawnParam.Owner = this;

        AActor* NewActor = World->SpawnActor(ActorClass, &SpawnLocation, &SpawnRotation, SpawnParam);
        if (NewActor)
        {
            URandomMovementComponent* MovementComp = Cast<URandomMovementComponent>(NewActor->GetComponentByClass(URandomMovementComponent::StaticClass()));
            if (MovementComp)
            {
                MovementComp->SetRandomLocationVolume(RandomLocationVolume);
            }
        }
    }
}
