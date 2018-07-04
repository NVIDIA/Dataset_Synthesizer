/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "DomainRandomizationDNNPCH.h"
#include "GroupActorManager.generated.h"

class USpatialLayoutGenerator;

USTRUCT(Blueprintable)
struct DOMAINRANDOMIZATIONDNN_API FNVActorTemplateConfig
{
    GENERATED_BODY()

    FNVActorTemplateConfig();

public:
    // The class to be used to spawn the new actor
    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> ActorClass;

    // The mesh to be used by the new actor
    UPROPERTY(EditAnywhere)
    class UStaticMesh* ActorOverrideMesh;
};

/**
* Manages array of actors to spawn with mesh and spatial randomization control.
*/
UCLASS(Blueprintable)
class DOMAINRANDOMIZATIONDNN_API AGroupActorManager : public AActor
{
    GENERATED_BODY()

public:
    AGroupActorManager(const FObjectInitializer& ObjectInitializer);

    void SpawnActors();
    void SpawnTemplateActors();

protected:
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;
    virtual void PostLoad() override;

    virtual void Tick(float DeltaTime) override;

    bool ShouldSpawnRepeatively() const;

    AActor* CreateActorFromTemplate(const FNVActorTemplateConfig& ActorTemplate, const FTransform& ActorTransform);

public: // Editor properties
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    bool bAutoActive;

    // The list of actor classes to spawn and manage
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    TArray<TSubclassOf<AActor>> ActorClassesToSpawn;

    // The list of meshes to used for managed actors
    // NOTE: If there are meshes in this list, the system will only use the first class in
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    TArray<class UStaticMesh*> OverrideActorMeshes;

    // How many instances of each actors do we need to spawn
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    FInt32Interval CountPerActor;

    // Total number of actors to spawn
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    FInt32Interval TotalNumberOfActorsToSpawn;

    // The layout for these managed actors
    UPROPERTY(EditAnywhere, Instanced, SimpleDisplay, Category = GroupActorManager)
    USpatialLayoutGenerator* LayoutGenerator;

    // The volume to choose the location where those managed actor can be in
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AVolume* RandomLocationVolume;

    // How long to wait until we spawn a new group of actors again
    // NOTE: If SpawnDuration <= 0 then we only spawn the actors once
    UPROPERTY(EditAnywhere, Category = GroupActorManager)
    float SpawnDuration;

protected: // Transient
    UPROPERTY(Transient)
    TArray<AActor*> ManagedActors;

    UPROPERTY(Transient)
    TArray<AActor*> TemplateActors;
    UPROPERTY(Transient)
    float CountdownUntilNextSpawn;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:
    // List of the proxy meshes used to show where actors should be on the level
    TArray<class UStaticMeshComponent*> ProxyMeshComponents;

    void UpdateProxyMeshes();
    void UpdateProxyMeshesVisibility();
    void DestroyManagedActors();

#endif // WITH_EDITORONLY_DATA
};
