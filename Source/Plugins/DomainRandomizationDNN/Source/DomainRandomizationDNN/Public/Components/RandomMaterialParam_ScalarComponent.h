/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomMaterialParameterComponentBase.h"
#include "RandomMaterialParam_ScalarComponent.generated.h"

/**
* RandomTextureComponent randomly change the color parameter of the materials in the owner's mesh
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMaterialParam_ScalarComponent : public URandomMaterialParameterComponentBase
{
    GENERATED_BODY()

public:
    URandomMaterialParam_ScalarComponent();

protected:
    void UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy) override;

protected: // Editor properties
    // Range of the scalar value to randomize
    UPROPERTY(EditAnywhere, Category = Randomization)
    FFloatInterval ValueRange;
};
