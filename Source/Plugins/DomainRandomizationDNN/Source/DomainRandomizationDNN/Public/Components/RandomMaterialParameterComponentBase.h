/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "DRUtils.h"
#include "RandomMaterialParameterComponentBase.generated.h"

/**
* URandomMaterialParameterComponentBase randomly change the value of some parameters of the materials in the owner's mesh
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, Abstract, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMaterialParameterComponentBase : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomMaterialParameterComponentBase();

protected:
    virtual void BeginPlay() override;
    virtual void OnRandomization_Implementation() override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

    void UpdateMeshMaterial(class UMeshComponent* AffectedMeshComp);
    void UpdateDecalMaterial(class UDecalComponent* AffectedDecalComp);
    virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy)  PURE_VIRTUAL(URandomMaterialParameterComponentBase::UpdateMaterial,);

protected: // Editor properties
    // List of the parameters in the material that we want to modify
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    TArray<FName> MaterialParameterNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (ShowOnlyInnerProperties))
    FRandomMaterialSelection MaterialSelectionConfigData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    EAffectedMaterialOwnerComponentType AffectedComponentType;

protected:
    // List of the mesh components from the owner actor that we need to change their materials
    UPROPERTY(Transient)
    TArray<class UMeshComponent*> OwnerMeshComponents;

    UPROPERTY(Transient)
    TArray<class UDecalComponent*> OwnerDecalComponents;
};
