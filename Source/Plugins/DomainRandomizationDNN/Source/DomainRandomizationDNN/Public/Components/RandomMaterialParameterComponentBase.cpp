/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "Components/MeshComponent.h"
#include "Components/DecalComponent.h"
#include "RandomMaterialParameterComponentBase.h"

// Sets default values
URandomMaterialParameterComponentBase::URandomMaterialParameterComponentBase()
{
    AffectedComponentType = EAffectedMaterialOwnerComponentType::OnlyAffectMeshComponents;
    OwnerMeshComponents.Reset();
    OwnerDecalComponents.Reset();
}

void URandomMaterialParameterComponentBase::BeginPlay()
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
            if (CheckComp)
            {
                OwnerDecalComponents.Add(CheckComp);
            }
        }
    }

    Super::BeginPlay();
}

void URandomMaterialParameterComponentBase::OnRandomization_Implementation()
{
    const bool bAffectMeshComponents = (AffectedComponentType == EAffectedMaterialOwnerComponentType::OnlyAffectMeshComponents) ||
                                       (AffectedComponentType == EAffectedMaterialOwnerComponentType::AffectBothMeshAndDecalComponents);
    const bool bAffectDecalComponents = (AffectedComponentType == EAffectedMaterialOwnerComponentType::OnlyAffectDecalComponents) ||
                                        (AffectedComponentType == EAffectedMaterialOwnerComponentType::AffectBothMeshAndDecalComponents);

    if (bAffectMeshComponents && (OwnerMeshComponents.Num() > 0))
    {
        for (UMeshComponent* CheckMeshComp : OwnerMeshComponents)
        {
            if (CheckMeshComp)
            {
                UpdateMeshMaterial(CheckMeshComp);
            }
        }
    }

    if (bAffectDecalComponents && (OwnerDecalComponents.Num() > 0))
    {
        for (UDecalComponent* CheckDecalComp : OwnerDecalComponents)
        {
            if (CheckDecalComp)
            {
                UpdateDecalMaterial(CheckDecalComp);
            }
        }
    }
}

void URandomMaterialParameterComponentBase::UpdateMeshMaterial(class UMeshComponent* AffectedMeshComp)
{
    for (UMeshComponent* CheckMeshComp : OwnerMeshComponents)
    {
        if (CheckMeshComp)
        {
            const TArray<int32> AffectedMaterialIndexes = MaterialSelectionConfigData.GetAffectMaterialIndexes(CheckMeshComp);
            for (const int32 MaterialIndex : AffectedMaterialIndexes)
            {
                UMaterialInstanceDynamic* MeshMaterialInstance = CheckMeshComp->CreateDynamicMaterialInstance(MaterialIndex);
                if (MeshMaterialInstance)
                {
                    UpdateMaterial(MeshMaterialInstance);
                }
            }
        }
    }
}

void URandomMaterialParameterComponentBase::UpdateDecalMaterial(class UDecalComponent* AffectedDecalComp)
{
    for (UDecalComponent* CheckDecalComp : OwnerDecalComponents)
    {
        if (CheckDecalComp)
        {
            UMaterialInterface* CurrentDecalMaterial = CheckDecalComp->GetDecalMaterial();
            UMaterialInstanceDynamic* DecalMaterialInstance = Cast<UMaterialInstanceDynamic>(CurrentDecalMaterial);
            if (!DecalMaterialInstance)
            {
                DecalMaterialInstance = CheckDecalComp->CreateDynamicMaterialInstance();
            }

            if (DecalMaterialInstance)
            {
                UpdateMaterial(DecalMaterialInstance);
            }
        }
    }
}

#if WITH_EDITORONLY_DATA
void URandomMaterialParameterComponentBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();

        if (ChangedPropName == GET_MEMBER_NAME_CHECKED(URandomMaterialParameterComponentBase, MaterialSelectionConfigData))
        {
            MaterialSelectionConfigData.PostEditChangeProperty(PropertyChangedEvent);
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif //WITH_EDITORONLY_DATA
