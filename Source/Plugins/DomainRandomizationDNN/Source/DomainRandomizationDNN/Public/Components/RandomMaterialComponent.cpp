/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "Components/MeshComponent.h"
#include "Components/DecalComponent.h"
#include "RandomMaterialParameterComponentBase.h"
#include "RandomMaterialComponent.h"

// Sets default values
URandomMaterialComponent::URandomMaterialComponent()
{
    AffectedComponentType = EAffectedMaterialOwnerComponentType::OnlyAffectMeshComponents;
    OwnerMeshComponents.Reset();
    OwnerDecalComponents.Reset();
}

void URandomMaterialComponent::BeginPlay()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        OwnerMeshComponents = DRUtils::GetValidChildMeshComponents(OwnerActor);

		TArray<UDecalComponent*> ChildDecalComps;
		OwnerActor->GetComponents<UDecalComponent>(ChildDecalComps);
        OwnerDecalComponents.Reset();
        for (auto* CheckComp : ChildDecalComps)
        {
            UDecalComponent* CheckDecalComp = Cast<UDecalComponent>(CheckComp);
            if (CheckComp)
            {
                OwnerDecalComponents.Add(CheckComp);
            }
        }
    }

    if (bUseAllMaterialInDirectories)
    {
        MaterialStreamer.Init(MaterialDirectories, UMaterialInterface::StaticClass());
    }

    // NOTE: Only randomize once if there's only 1 material to choose from
    if (bUseAllMaterialInDirectories)
    {
        if (MaterialStreamer.GetAssetsCount() <= 1)
        {
            bOnlyRandomizeOnce = true;
        }
    }
    else if (MaterialList.Num() <= 1)
    {
        bOnlyRandomizeOnce = true;
    }

    Super::BeginPlay();
}

void URandomMaterialComponent::OnRandomization_Implementation()
{
    if (!HasMaterialToRandomize())
    {
        return;
    }

    const bool bAffectMeshComponents = (AffectedComponentType == EAffectedMaterialOwnerComponentType::OnlyAffectMeshComponents) ||
                                       (AffectedComponentType == EAffectedMaterialOwnerComponentType::AffectBothMeshAndDecalComponents);
    const bool bAffectDecalComponents = (AffectedComponentType == EAffectedMaterialOwnerComponentType::OnlyAffectDecalComponents) ||
                                        (AffectedComponentType == EAffectedMaterialOwnerComponentType::AffectBothMeshAndDecalComponents);

    bool bActorMaterialChanged = false;

    // Update the owner's mesh components list if it's not initialized
    if (OwnerMeshComponents.Num() == 0)
    {
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            OwnerMeshComponents = DRUtils::GetValidChildMeshComponents(OwnerActor);
        }
    }

    if (bAffectMeshComponents && (OwnerMeshComponents.Num() > 0))
    {
        // TODO: Add option to use the same material to all the material slots or not
        for (UMeshComponent* CheckMeshComp : OwnerMeshComponents)
        {
            if (CheckMeshComp)
            {
                UMaterialInterface* NewMaterial = GetNextMaterial();
                if (NewMaterial)
                {
                    const TArray<int32> AffectedMaterialIndexes = MaterialSelectionConfigData.GetAffectMaterialIndexes(CheckMeshComp);
                    for (const int32 MaterialIndex : AffectedMaterialIndexes)
                    {
                        CheckMeshComp->SetMaterial(MaterialIndex, NewMaterial);
                    }
                    bActorMaterialChanged = true;
                }
            }
        }
    }

    if (bAffectDecalComponents && (OwnerDecalComponents.Num() > 0))
    {
        for (auto CheckDecalComp : OwnerDecalComponents)
        {
            if (CheckDecalComp)
            {
                UMaterialInterface* NewMaterial = GetNextMaterial();
                if (NewMaterial)
                {
                    CheckDecalComp->SetDecalMaterial(NewMaterial);
                    bActorMaterialChanged = true;
                }
            }
        }
    }

    if (bActorMaterialChanged)
    {
        // Make sure to update other components that want to change the material's parameteres like color, texture ...
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            TArray<URandomMaterialParameterComponentBase*> RandMatParamCompList;
            OwnerActor->GetComponents(RandMatParamCompList);
            for (auto RandMatParamComp : RandMatParamCompList)
            {
                if (RandMatParamComp && RandMatParamComp->ShouldRandomize())
                {
                    RandMatParamComp->Randomize();
                }
            }
        }
    }
}

#if WITH_EDITORONLY_DATA
void URandomMaterialComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();

        if (ChangedPropName == GET_MEMBER_NAME_CHECKED(URandomMaterialComponent, MaterialSelectionConfigData))
        {
            MaterialSelectionConfigData.PostEditChangeProperty(PropertyChangedEvent);
        }

        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}
#endif //WITH_EDITORONLY_DATA
bool URandomMaterialComponent::HasMaterialToRandomize() const
{
#if WITH_EDITORONLY_DATA
    return bUseAllMaterialInDirectories ? MaterialStreamer.HasAssets() : (MaterialList.Num() > 0);
#else
    return false;
#endif //WITH_EDITORONLY_DATA
}


class UMaterialInterface* URandomMaterialComponent::GetNextMaterial()
{
#if WITH_EDITORONLY_DATA
    // Choose a random material in the list
    UMaterialInterface* NewMaterial = nullptr;
    if (bUseAllMaterialInDirectories)
    {
        NewMaterial = MaterialStreamer.GetNextAsset<UMaterialInterface>();
    }
    else if (MaterialList.Num() > 0)
    {
        NewMaterial = MaterialList[FMath::Rand() % MaterialList.Num()];
    }
    return NewMaterial;
#else
    return nullptr;
#endif //WITH_EDITORONLY_DATA
}