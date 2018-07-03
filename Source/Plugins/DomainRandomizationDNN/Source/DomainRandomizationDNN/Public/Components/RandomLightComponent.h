/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "DRUtils.h"
#include "RandomLightComponent.generated.h"

/**
* Randomize the properties of the owner actor's light components
* NOTE: This is not a LightComponent, it modify other light components
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomLightComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomLightComponent();

protected: // Editor properties
    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (InlineEditConditionToggle))
    bool bShouldModifyIntensity;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (EditCondition = "bShouldModifyIntensity"))
    FFloatInterval IntensityRange;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (InlineEditConditionToggle))
    bool bShouldModifyColor;

    UPROPERTY(EditAnywhere, Category = "Randomization", meta = (EditCondition = "bShouldModifyColor"))
    FRandomColorData ColorData;

protected:
#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

    void OnRandomization_Implementation() override;
};
