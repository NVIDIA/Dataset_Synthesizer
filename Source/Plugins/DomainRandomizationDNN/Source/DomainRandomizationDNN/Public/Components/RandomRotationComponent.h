/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "RandomRotationComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomRotationComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomRotationComponent();

protected: // Editor properties
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    FRandomRotationData RandomRotationData;

    // If true, the owner will be rotated around its original rotation
    // Otherwise the random rotation will be around the Zero rotation
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    bool bRelatedToOriginRotation;

protected:
    void BeginPlay() override;
    void OnRandomization_Implementation() override;

protected:
    UPROPERTY(Transient)
    FRotator OriginalRotation;
};
