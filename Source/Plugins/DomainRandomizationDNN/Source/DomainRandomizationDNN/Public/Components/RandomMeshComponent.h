/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "DRUtils.h"
#include "RandomMeshComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomMeshComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomMeshComponent();

protected:
    virtual void BeginPlay() override;
    virtual void OnRandomization_Implementation() override;
    bool HasMeshToRandomize() const;

protected: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    bool bUseAllMeshInDirectories;

    // List of directories where we want to use the static meshes from
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bUseAllMeshInDirectories", RelativeToGameContentDir))
    TArray<FDirectoryPath> MeshDirectories;

    // List of the StaticMesh that the actor will change through
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "!bUseAllMeshInDirectories"))
    TArray<UStaticMesh*> StaticMeshList;

protected: // Transient properties
    UPROPERTY(Transient)
    FRandomAssetStreamer MeshStreamer;
};
