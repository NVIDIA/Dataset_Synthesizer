/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomMaterialParameterComponentBase.h"
#include "DRUtils.h"
#include "RandomMaterialParam_TextureComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMaterialParam_TextureComponent : public URandomMaterialParameterComponentBase
{
    GENERATED_BODY()

public:
    URandomMaterialParam_TextureComponent();

protected:
    virtual void PostLoad() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy) override;

    bool HasAssetToRandomize() const;

protected: // Editor properties
    // DEPRECATED_FORGAME(4.16, "Use MaterialParameterNames instead")
    UPROPERTY()
    FName TextureParameterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    bool bUseAllTextureInAFolder;

    // Path to the directory where we want to use textures from
    UPROPERTY()
    FDirectoryPath TextureDirectory;

    // List of directories where we want to use the static meshes from
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bUseAllTextureInAFolder", RelativeToGameContentDir))
    TArray<FDirectoryPath> TextureDirectories;

    // List of the texture that the owner mesh's material will change through
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "!bUseAllTextureInAFolder"))
    TArray<UTexture*> TextureList;

protected:
    UPROPERTY(Transient)
    FRandomAssetStreamer TextureStreamer;
};
