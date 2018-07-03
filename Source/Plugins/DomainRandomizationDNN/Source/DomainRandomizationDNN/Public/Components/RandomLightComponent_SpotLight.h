/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "DRUtils.h"
#include "RandomLightComponent.h"
#include "RandomLightComponent_SpotLight.generated.h"

/**
* Randomize the properties of the owner actor's light components
* NOTE: This is not a LightComponent, it modify other light components
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomLightComponent_SpotLight : public URandomLightComponent
{
    GENERATED_BODY()

public:
    URandomLightComponent_SpotLight();

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (InlineEditConditionToggle))
    bool bShouldModifyInnerConeAngle;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (EditCondition = "bShouldModifyInnerConeAngle"))
    FFloatInterval InnerConeAngleRange;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (InlineEditConditionToggle))
    bool bShouldModifyOuterConeAngle;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (EditCondition = "bShouldModifyOuterConeAngle"))
    FFloatInterval OuterConeAngleRange;

protected:
    void OnRandomization_Implementation() override;
};
