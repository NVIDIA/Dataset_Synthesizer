/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomMaterialParameterComponentBase.h"
#include "RandomMaterialParam_ColorComponent.generated.h"

/**
* RandomTextureComponent randomly change the color parameter of the materials in the owner's mesh
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMaterialParam_ColorComponent : public URandomMaterialParameterComponentBase
{
    GENERATED_BODY()

public:
    URandomMaterialParam_ColorComponent();

protected:
    void PostLoad() override;
    void UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy) override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

protected: // Editor properties
    // The name of the parameter which control the color change on the owner's material
    // DEPRECATED_FORGAME(4.16, "Use MaterialParameterNames instead")
    UPROPERTY()
    FName ColorParameterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    FRandomColorData ColorData;
};
