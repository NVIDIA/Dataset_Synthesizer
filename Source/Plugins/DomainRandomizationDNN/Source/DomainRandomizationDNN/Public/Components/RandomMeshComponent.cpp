/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomMaterialComponent.h"
#include "RandomMeshComponent.h"
#include "DRUtils.h"

// Sets default values
URandomMeshComponent::URandomMeshComponent()
{
    bUseAllMeshInDirectories = false;
    MeshDirectories.Reset();
    StaticMeshList.Reset();
}

void URandomMeshComponent::BeginPlay()
{
    if (bUseAllMeshInDirectories)
    {
        // NOTE: Only support static meshes for now
        MeshStreamer.Init(MeshDirectories, UStaticMesh::StaticClass());
    }

    AActor* OwnerActor = GetOwner();
    UStaticMeshComponent* OwnerStaticMeshComp = Cast<UStaticMeshComponent>(OwnerActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
    if (OwnerStaticMeshComp)
    {
        StaticMeshList.AddUnique(OwnerStaticMeshComp->GetStaticMesh());
    }

    Super::BeginPlay();
}

void URandomMeshComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && HasMeshToRandomize())
    {
        UStaticMeshComponent* OwnerStaticMeshComp = Cast<UStaticMeshComponent>(OwnerActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
        if (OwnerStaticMeshComp)
        {
            // Choose a random mesh
            UStaticMesh* NewMesh = nullptr;
            if (bUseAllMeshInDirectories)
            {
                NewMesh = MeshStreamer.GetNextAsset<UStaticMesh>();
            }
            else
            {
                NewMesh = StaticMeshList[FMath::Rand() % StaticMeshList.Num()];
            }

            if (NewMesh && NewMesh != OwnerStaticMeshComp->GetStaticMesh())
            {
                OwnerStaticMeshComp->SetStaticMesh(NewMesh);

                // Reset the overrided materials
                int32 TotalNumberOfMaterials = OwnerStaticMeshComp->GetNumMaterials();
                for (int32 i = 0; i < TotalNumberOfMaterials; i++)
                {
                    OwnerStaticMeshComp->SetMaterial(i, nullptr);
                }

                // Make sure to update other components that want to change the mesh's materials
                TArray<URandomMaterialComponent*> RandomMatCompList;
                OwnerActor->GetComponents(RandomMatCompList);
                for (auto RandMatComp : RandomMatCompList)
                {
                    if (RandMatComp && RandMatComp->ShouldRandomize())
                    {
                        RandMatComp->Randomize();
                    }
                }
            }
            // TODO: Support randomized skeletal meshes
        }
    }
}

bool URandomMeshComponent::HasMeshToRandomize() const
{
    return bUseAllMeshInDirectories?
        MeshStreamer.HasAssets() :
        (StaticMeshList.Num() > 0);
}
