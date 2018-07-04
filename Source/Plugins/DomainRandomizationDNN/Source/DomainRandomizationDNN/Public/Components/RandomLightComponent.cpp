/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomLightComponent.h"

// Sets default values
URandomLightComponent::URandomLightComponent()
{
    IntensityRange = FFloatInterval(1000.f, 5000.f);
}

void URandomLightComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        TArray<ULightComponent*> RandLightCompList;
        OwnerActor->GetComponents(RandLightCompList);

        for (ULightComponent* LightComp : RandLightCompList)
        {
            if (!LightComp)
            {
                continue;
            }

            if (bShouldModifyIntensity)
            {
                float RandIntensity = FMath::RandRange(IntensityRange.Min, IntensityRange.Max);
                LightComp->SetIntensity(RandIntensity);
            }

            if (bShouldModifyColor)
            {
                FLinearColor RandomColor = ColorData.GetRandomColor();
                LightComp->SetLightColor(RandomColor);
            }
        }
    }
}

#if WITH_EDITORONLY_DATA
void URandomLightComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();

        if (ChangedPropName == GET_MEMBER_NAME_CHECKED(URandomLightComponent, ColorData))
        {
            ColorData.PostEditChangeProperty(PropertyChangedEvent);
        }

        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}
#endif //WITH_EDITORONLY_DATA