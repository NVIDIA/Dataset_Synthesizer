/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "DRUtils.h"
#include "RandomMaterialComponent.generated.h"

/**
* RandomMaterialComponent randomly change the materials of the owner's mesh in specified material slot
*/
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMaterialComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomMaterialComponent();

protected:
    virtual void BeginPlay() override;
    virtual void OnRandomization_Implementation() override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

    bool HasMaterialToRandomize() const;

    class UMaterialInterface* GetNextMaterial();

protected: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (ShowOnlyInnerProperties))
    FRandomMaterialSelection MaterialSelectionConfigData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    bool bUseAllMaterialInDirectories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    EAffectedMaterialOwnerComponentType AffectedComponentType;

    // List of directories where we want to use the static meshes from
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bUseAllMaterialInDirectories", RelativeToGameContentDir))
    TArray<FDirectoryPath> MaterialDirectories;

    // List of the material that the owner actor will switch through
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "!bUseAllMaterialInDirectories"))
    TArray<UMaterialInterface*> MaterialList;

protected:
    // List of the mesh components from the owner actor that we need to change their materials
    UPROPERTY(Transient)
    TArray<class UMeshComponent*> OwnerMeshComponents;

    UPROPERTY(Transient)
    TArray<class UDecalComponent*> OwnerDecalComponents;

    UPROPERTY(Transient)
    FRandomAssetStreamer MaterialStreamer;
};
