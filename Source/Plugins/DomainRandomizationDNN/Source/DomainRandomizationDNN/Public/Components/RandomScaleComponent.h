/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "RandomScaleComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomScaleComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomScaleComponent();

protected: // Editor properties
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    FRandomScale3DData RandomScaleData;

protected:
    void BeginPlay() override;
    void OnRandomization_Implementation() override;
};
