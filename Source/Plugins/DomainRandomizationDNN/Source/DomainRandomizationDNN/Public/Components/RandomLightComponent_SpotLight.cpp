/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "Components/SpotLightComponent.h"
#include "RandomLightComponent_SpotLight.h"

// Sets default values
URandomLightComponent_SpotLight::URandomLightComponent_SpotLight() : Super()
{
    InnerConeAngleRange = FFloatInterval(10.f, 30.f);
    OuterConeAngleRange = FFloatInterval(30.f, 70.f);
}

void URandomLightComponent_SpotLight::OnRandomization_Implementation()
{
    Super::OnRandomization_Implementation();

    AActor* OwnerActor = GetOwner();
    if (OwnerActor)
    {
        TArray<ULightComponent*> RandLightCompList;
        OwnerActor->GetComponents(RandLightCompList);

        for (ULightComponent* LightComp : RandLightCompList)
        {
            USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComp);
            if (!SpotLightComp)
            {
                continue;
            }

            if (bShouldModifyInnerConeAngle)
            {
                float RandInnerConeAngle = FMath::RandRange(InnerConeAngleRange.Min, InnerConeAngleRange.Max);
                SpotLightComp->SetInnerConeAngle(RandInnerConeAngle);
            }

            if (bShouldModifyOuterConeAngle)
            {
                float RandOuterConeAngle = FMath::RandRange(OuterConeAngleRange.Min, OuterConeAngleRange.Max);
                SpotLightComp->SetOuterConeAngle(RandOuterConeAngle);
            }
        }
    }
}
