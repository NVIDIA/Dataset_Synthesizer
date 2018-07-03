/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "DRSceneManager.h"
#include "GroupActorManager.h"

#include "NVSceneMarker.h"
#include "NVSceneCapturerActor.h"
#include "OrbitalMovementComponent.h"
#include "RandomMovementComponent.h"

#include "Engine.h"

ADRSceneManager::ADRSceneManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;

    bIsActive = true;
}

void ADRSceneManager::PostLoad()
{
    Super::PostLoad();

    if (GroupActorManager)
    {
        GroupActorManager->bAutoActive = false;
    }
}

void ADRSceneManager::BeginPlay()
{
    Super::BeginPlay();
}

void ADRSceneManager::UpdateSettingsFromCommandLine()
{
    Super::UpdateSettingsFromCommandLine();

    if (!GroupActorManager)
    {
        return;
    }

    const auto CommandLine = FCommandLine::Get();

    int32 CountPerActorOverride = 0;
    if (FParse::Value(CommandLine, TEXT("-CountPerActor="), CountPerActorOverride))
    {
        GroupActorManager->CountPerActor.Min = GroupActorManager->CountPerActor.Max = CountPerActorOverride;
    }

    int32 TotalNumberOfActorsToSpawn_MinOverride = 0;
    if (FParse::Value(CommandLine, TEXT("-TotalNumberOfActorsToSpawn_Min="), TotalNumberOfActorsToSpawn_MinOverride))
    {
        GroupActorManager->TotalNumberOfActorsToSpawn.Min = TotalNumberOfActorsToSpawn_MinOverride;
    }
    int32 TotalNumberOfActorsToSpawn_MaxOverride = 0;
    if (FParse::Value(CommandLine, TEXT("-TotalNumberOfActorsToSpawn_Max="), TotalNumberOfActorsToSpawn_MaxOverride))
    {
        GroupActorManager->TotalNumberOfActorsToSpawn.Max = TotalNumberOfActorsToSpawn_MaxOverride;
    }

    FString TrainingActorClassesOverride = TEXT("");
    if (FParse::Value(CommandLine, TEXT("-TrainingActorClasses="), TrainingActorClassesOverride))
    {
        FString RemainderStr;

        GroupActorManager->ActorClassesToSpawn.Reset();

        TArray<FString> ActorClassNames;
        TrainingActorClassesOverride.ParseIntoArray(ActorClassNames, TEXT(","));
        for (const FString& ActorClassName : ActorClassNames)
        {
            FString StrippedActorClassName = ActorClassName;
            ConstructorHelpers::StripObjectClass(StrippedActorClassName);
            UClass* ActorClass = FindObject<UClass>(ANY_PACKAGE, *StrippedActorClassName);
            if (!ActorClass)
            {
                ActorClass = LoadObject<UClass>(NULL, *ActorClassName);
            }
            if (ActorClass)
            {
                GroupActorManager->ActorClassesToSpawn.Add(ActorClass);
            }
        }
    }

    FString TrainingActorMeshesOverride = TEXT("");
    if (FParse::Value(CommandLine, TEXT("-TrainingActorMeshes="), TrainingActorMeshesOverride))
    {
        FString MeshPathName;
        FString RemainderStr;

        GroupActorManager->OverrideActorMeshes.Reset();

        TArray<FString> MeshNames;
        TrainingActorMeshesOverride.ParseIntoArray(MeshNames, TEXT(","));
        for (const FString& CheckMeshName : MeshNames)
        {
            FString StrippepMeshName = CheckMeshName;
            ConstructorHelpers::StripObjectClass(StrippepMeshName);
            UStaticMesh* MeshRef = FindObject<UStaticMesh>(ANY_PACKAGE, *StrippepMeshName);
            if (!MeshRef)
            {
                MeshRef = LoadObject<UStaticMesh>(NULL, *CheckMeshName);
            }
            if (MeshRef)
            {
                GroupActorManager->OverrideActorMeshes.AddUnique(MeshRef);
            }
        }

        GroupActorManager->SpawnTemplateActors();
    }

    // TODO: Let the NoiseActorManger parse the command arguments itself?
    if (NoiseActorManager)
    {
        int32 NoiseObjectCountOverride = 0;
        if (FParse::Value(CommandLine, TEXT("-NoiseObjectCount="), NoiseObjectCountOverride))
        {
            NoiseActorManager->TotalNumberOfActorsToSpawn.Min = NoiseActorManager->TotalNumberOfActorsToSpawn.Max = NoiseObjectCountOverride;
        }
    }
}

void ADRSceneManager::SetupSceneInternal()
{
    Super::SetupSceneInternal();

    if (CurrentSceneMarker)
    {
        const FTransform& MarkerTransform = CurrentSceneMarker->GetActorTransform();
        if (GroupActorManager)
        {
            GroupActorManager->SetActorTransform(MarkerTransform);
        }
    }

    if (GroupActorManager)
    {
        GroupActorManager->SpawnActors();
    }
}

#if WITH_EDITORONLY_DATA
void ADRSceneManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        // TODO
        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}
#endif // WITH_EDITORONLY_DATA