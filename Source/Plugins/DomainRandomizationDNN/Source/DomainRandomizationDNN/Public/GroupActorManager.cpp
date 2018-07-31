/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator/SpatialLayoutGenerator.h"
#include "RandomMovementComponent.h"
#include "NVAnnotatedActor.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneManager.h"
#include "NVObjectMaskManager.h"
#include "GroupActorManager.h"

// Sets default values
AGroupActorManager::AGroupActorManager(const FObjectInitializer& ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;

    LayoutGenerator = nullptr;
    bAutoActive = true;

    ActorClassesToSpawn.Reset();

    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));

    SpawnDuration = 0.f;
    CountPerActor = FInt32Interval(1, 1);
    TotalNumberOfActorsToSpawn = FInt32Interval(0, 0);
}

void AGroupActorManager::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoActive)
    {
        SpawnActors();
    }

#if WITH_EDITORONLY_DATA
    UpdateProxyMeshesVisibility();
#endif // WITH_EDITORONLY_DATA
}

void AGroupActorManager::SpawnActors()
{
    DestroyManagedActors();

    TArray<FNVActorTemplateConfig> ActorTemplates;
    ActorTemplates.Reset();

    const bool bSpawnTotalNumberOfActors = (TotalNumberOfActorsToSpawn.Max > 0);

    const bool bUseMesh = (OverrideActorMeshes.Num() > 0);
    if (bUseMesh)
    {
        TSubclassOf<AActor> ActorClassTemplate = nullptr;
        for (int i = 0; i < ActorClassesToSpawn.Num(); i++)
        {
            if (ActorClassesToSpawn[i])
            {
                ActorClassTemplate = ActorClassesToSpawn[i];
                break;
            }
        }

        TArray<UStaticMesh*> SpawnMeshes;
        if (bSpawnTotalNumberOfActors)
        {
            const int MeshCount = OverrideActorMeshes.Num();
            SpawnMeshes.Reset();
            int TotalNumberOfActors = FMath::RandRange(TotalNumberOfActorsToSpawn.Min, TotalNumberOfActorsToSpawn.Max);
            for (int i = 0; i < TotalNumberOfActors; i++)
            {
                // Pick a random mesh from the list
                UStaticMesh* CheckMesh = OverrideActorMeshes[FMath::Rand() % MeshCount];
                // TODO: Make sure the mesh is valid
                SpawnMeshes.Add(CheckMesh);
            }
        }
        else
        {
            SpawnMeshes = OverrideActorMeshes;
        }

        // Pick up the meshes randomly
        for (int i = 0; i < SpawnMeshes.Num(); i++)
        {
            UStaticMesh* CheckMesh = SpawnMeshes[i];
            if (CheckMesh)
            {
                const int32 NumberInstanceOfActor = FMath::Max(FMath::RandRange(CountPerActor.Min, CountPerActor.Max), 0);
                for (int j = 0; j < NumberInstanceOfActor; j++)
                {
                    FNVActorTemplateConfig NewActorTemplate;
                    NewActorTemplate.ActorClass = ActorClassTemplate;
                    NewActorTemplate.ActorOverrideMesh = CheckMesh;
                    ActorTemplates.Add(NewActorTemplate);
                }
            }
        }
    }
    else
    {
        if (bSpawnTotalNumberOfActors)
        {
            const int NumberOfActorClasses = ActorClassesToSpawn.Num();
            int TotalNumberOfActors = FMath::RandRange(TotalNumberOfActorsToSpawn.Min, TotalNumberOfActorsToSpawn.Max);
            for (int i = 0; i < TotalNumberOfActors; i++)
            {
                // Pick a random class from the list
                FNVActorTemplateConfig NewActorTemplate;
                NewActorTemplate.ActorClass = ActorClassesToSpawn[FMath::Rand() % NumberOfActorClasses];
                NewActorTemplate.ActorOverrideMesh = nullptr;
                ActorTemplates.Add(NewActorTemplate);
            }
        }
        else
        {
            TArray<TSubclassOf<AActor>> ActorClasses;
            for (int i = 0; i < ActorClassesToSpawn.Num(); i++)
            {
                TSubclassOf<AActor> ActorClass = ActorClassesToSpawn[i];
                if (ActorClass)
                {
                    const int32 NumberInstanceOfActor = FMath::Max(FMath::RandRange(CountPerActor.Min, CountPerActor.Max), 0);
                    for (int j = 0; j < NumberInstanceOfActor; j++)
                    {
                        FNVActorTemplateConfig NewActorTemplate;
                        NewActorTemplate.ActorClass = ActorClass;
                        NewActorTemplate.ActorOverrideMesh = nullptr;
                        ActorTemplates.Add(NewActorTemplate);
                    }
                }
            }
        }
    }
    const uint32 NumberOfActorsToSpawn = (uint32)ActorTemplates.Num();
    // Randomly rearrange the templates in the list
    for (uint32 i = NumberOfActorsToSpawn / 2; i < NumberOfActorsToSpawn; i++)
    {
        if (i > 0)
        {
            uint32 j = FMath::RandRange(0, i - 1);
            ActorTemplates.Swap(i, j);
        }
    }

    const FTransform& LayoutTransform = GetActorTransform();
    const TArray<FTransform>& ActorTransformList = LayoutGenerator ? LayoutGenerator->GetTransformForActors(LayoutTransform, NumberOfActorsToSpawn)
            : USpatialLayoutGenerator::GetDefaultTransformForActors(LayoutTransform, NumberOfActorsToSpawn);

    for (uint32 i = 0; i < NumberOfActorsToSpawn; i++)
    {
        const FNVActorTemplateConfig& ActorTemplate = ActorTemplates[i];
        const FTransform& ActorTransform = ActorTransformList[i];

        AActor* NewActor = CreateActorFromTemplate(ActorTemplate, ActorTransform);
        if (NewActor)
        {
            ManagedActors.Add(NewActor);
        }
    }
}

void AGroupActorManager::SpawnTemplateActors()
{
    for (AActor* CheckActor : TemplateActors)
    {
        if (CheckActor)
        {
            CheckActor->Destroy();
        }
    }
    TemplateActors.Reset();

    const bool bUseMesh = (OverrideActorMeshes.Num() > 0);
    if (bUseMesh)
    {
        TSubclassOf<AActor> ActorClassTemplate = nullptr;
        for (int i = 0; i < ActorClassesToSpawn.Num(); i++)
        {
            if (ActorClassesToSpawn[i])
            {
                ActorClassTemplate = ActorClassesToSpawn[i];
                break;
            }
        }

        FTransform TemplateActorTranform;
        TemplateActorTranform.SetLocation(FVector::ZeroVector);

        for (UStaticMesh* OverrideMesh : OverrideActorMeshes)
        {
            FNVActorTemplateConfig ActorTemplate;
            ActorTemplate.ActorClass = ActorClassTemplate;
            ActorTemplate.ActorOverrideMesh = OverrideMesh;

            AActor* NewActor = CreateActorFromTemplate(ActorTemplate, TemplateActorTranform);
            if (NewActor)
            {
                // Hide the actor and disable its collision since it's just for template
                NewActor->DisableComponentsSimulatePhysics();
                NewActor->SetActorEnableCollision(false);
                NewActor->SetActorHiddenInGame(true);
                TemplateActors.Add(NewActor);
            }
        }
    }
    else
    {
        // TODO
    }
}

AActor* AGroupActorManager::CreateActorFromTemplate(const FNVActorTemplateConfig& ActorTemplate, const FTransform& ActorTransform)
{
    AActor* NewActor = nullptr;

    UWorld* World = GetWorld();
    ensure(World);
    if (World)
    {
        FActorSpawnParameters SpawnParam;
        SpawnParam.Owner = this;
        SpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        const TSubclassOf<AActor> ActorClass = ActorTemplate.ActorClass;
        // TODO: Retry if failed to spawn actor?
        NewActor = World->SpawnActor<AActor>(ActorClass, ActorTransform, SpawnParam);

        if (NewActor)
        {
            // TODO: Pass shared config data to the new actor
            if (ActorTemplate.ActorOverrideMesh)
            {
                ANVAnnotatedActor* AnnotatedActor = Cast<ANVAnnotatedActor>(NewActor);
                if (AnnotatedActor)
                {
                    AnnotatedActor->SetStaticMesh(ActorTemplate.ActorOverrideMesh);

                    ANVSceneManager* SceneManager = ANVSceneManager::GetANVSceneManagerPtr();
                    if (SceneManager)
                    {
                        const uint32 ClassSegmentationId = SceneManager->ObjectClassSegmentation.GetInstanceId(AnnotatedActor);
                        AnnotatedActor->SetClassSegmentationId(ClassSegmentationId);
                    }
                }
                else
                {
                    UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(NewActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
                    if (StaticMeshComp)
                    {
                        StaticMeshComp->SetStaticMesh(ActorTemplate.ActorOverrideMesh);
                    }
                }
            }

            if (RandomLocationVolume)
            {
                URandomMovementComponent* MovementComp = Cast<URandomMovementComponent>(NewActor->GetComponentByClass(URandomMovementComponent::StaticClass()));
                if (MovementComp)
                {
                    MovementComp->SetRandomLocationVolume(RandomLocationVolume, true);
                }
            }
        }
    }

    return NewActor;
}

bool AGroupActorManager::ShouldSpawnRepeatively() const
{
    return SpawnDuration > 0.f;
}

void AGroupActorManager::BeginDestroy()
{
#if WITH_EDITORONLY_DATA
    for (auto CheckMesh : ProxyMeshComponents)
    {
        if (CheckMesh)
        {
            CheckMesh->DestroyComponent();
        }
    }
    ProxyMeshComponents.Reset();
#endif //WITH_EDITORONLY_DATA

    Super::BeginDestroy();
}


void AGroupActorManager::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITORONLY_DATA
    UpdateProxyMeshes();
#endif //WITH_EDITORONLY_DATA}
}

void AGroupActorManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ShouldSpawnRepeatively())
    {
        if (CountdownUntilNextSpawn > 0.f)
        {
            CountdownUntilNextSpawn -= DeltaTime;
        }
        if (CountdownUntilNextSpawn <= 0.f)
        {
            SpawnActors();
            CountdownUntilNextSpawn = SpawnDuration;
        }
    }
}



void AGroupActorManager::DestroyManagedActors()
{
    for (auto CheckActor : ManagedActors)
    {
        if (CheckActor)
        {
            CheckActor->SetActorHiddenInGame(true);
            CheckActor->Destroy();
        }
    }
    ManagedActors.Reset();
}
#if WITH_EDITORONLY_DATA
void AGroupActorManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();

        if ((ChangedPropName == GET_MEMBER_NAME_CHECKED(AGroupActorManager, ActorClassesToSpawn))
                || (ChangedPropName == GET_MEMBER_NAME_CHECKED(AGroupActorManager, TotalNumberOfActorsToSpawn))
                || (ChangedPropName == GET_MEMBER_NAME_CHECKED(AGroupActorManager, LayoutGenerator)))
        {
            UpdateProxyMeshes();
        }

        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}

void AGroupActorManager::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
    AGroupActorManager* This = CastChecked<AGroupActorManager>(InThis);
    for (auto CheckMeshComp : This->ProxyMeshComponents)
    {
        Collector.AddReferencedObject(CheckMeshComp);
    }

    Super::AddReferencedObjects(InThis, Collector);
}

void AGroupActorManager::UpdateProxyMeshes()
{
    UWorld* World = GetWorld();
    bool bShouldHideProxyMesh = World && World->IsGameWorld();

    int CurrentNumberOfProxyMeshs = ProxyMeshComponents.Num();
    if (CurrentNumberOfProxyMeshs < TotalNumberOfActorsToSpawn.Max)
    {
        const int NewProxyMeshToAdd = TotalNumberOfActorsToSpawn.Max - CurrentNumberOfProxyMeshs;
        for (int i = 0; i < NewProxyMeshToAdd; i++)
        {
            UStaticMeshComponent* NewProxyMeshComponent = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transactional | RF_TextExportTransient);
            NewProxyMeshComponent->SetupAttachment(GetRootComponent());
            NewProxyMeshComponent->bIsEditorOnly = true;
            NewProxyMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            NewProxyMeshComponent->bHiddenInGame = true;
            NewProxyMeshComponent->bVisible = !bShouldHideProxyMesh;
            NewProxyMeshComponent->CastShadow = false;
            NewProxyMeshComponent->PostPhysicsComponentTick.bCanEverTick = false;
            NewProxyMeshComponent->CreationMethod = EComponentCreationMethod::Instance;
            NewProxyMeshComponent->RegisterComponentWithWorld(World);

            ProxyMeshComponents.Add(NewProxyMeshComponent);
        }
    }
    else if (CurrentNumberOfProxyMeshs > TotalNumberOfActorsToSpawn.Max)
    {
        for (int i = CurrentNumberOfProxyMeshs - 1; i >= TotalNumberOfActorsToSpawn.Max; i--)
        {
            auto CheckMeshComp = ProxyMeshComponents[i];
            if (CheckMeshComp)
            {
                CheckMeshComp->DestroyComponent();
            }
        }
        ProxyMeshComponents.SetNum(TotalNumberOfActorsToSpawn.Max);
    }

    const FTransform& LayoutTransform = GetActorTransform();
    const TArray<FTransform>& ActorTransformList = LayoutGenerator ? LayoutGenerator->GetTransformForActors(LayoutTransform, TotalNumberOfActorsToSpawn.Max)
            : USpatialLayoutGenerator::GetDefaultTransformForActors(LayoutTransform, TotalNumberOfActorsToSpawn.Max);

    AActor* DefaultActor = nullptr;
    if (ActorClassesToSpawn.Num() > 0)
    {
        TSubclassOf<AActor> ActorClass = ActorClassesToSpawn[0];
        if (ActorClass)
        {
            DefaultActor = ActorClass->GetDefaultObject<AActor>();
        }
    }

    // Get default mesh for the proxy meshes
    UStaticMesh* ProxyMesh = nullptr;
    if (DefaultActor)
    {
        UStaticMeshComponent* ChildStaticMeshComp = Cast<UStaticMeshComponent>(DefaultActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (ChildStaticMeshComp)
        {
            ProxyMesh = ChildStaticMeshComp->GetStaticMesh();
        }
    }

    if (ProxyMesh)
    {
        for (int i = 0; i < ProxyMeshComponents.Num(); i++)
        {
            auto CheckMesh = ProxyMeshComponents[i];
            if (CheckMesh)
            {
                const FTransform& MeshTransform = ActorTransformList[i];
                CheckMesh->SetStaticMesh(ProxyMesh);
                CheckMesh->SetWorldTransform(MeshTransform);
            }
        }
    }
}

void AGroupActorManager::UpdateProxyMeshesVisibility()
{
    UWorld* World = GetWorld();
    bool bShouldHideProxyMesh = World && World->IsGameWorld();

    for (int i = 0; i < ProxyMeshComponents.Num(); i++)
    {
        auto CheckMesh = ProxyMeshComponents[i];
        if (CheckMesh)
        {
            CheckMesh->bHiddenInGame = bShouldHideProxyMesh;
            CheckMesh->bVisible = !bShouldHideProxyMesh;
        }
    }
}

#endif // WITH_EDITORONLY_DATA

FNVActorTemplateConfig::FNVActorTemplateConfig()
{
    ActorClass = nullptr;
    ActorOverrideMesh = nullptr;
}
