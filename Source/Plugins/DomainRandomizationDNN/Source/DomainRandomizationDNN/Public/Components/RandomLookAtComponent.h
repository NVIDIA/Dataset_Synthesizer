/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "RandomLookAtComponent.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomLookAtComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomLookAtComponent();

protected: // Editor properties
    // List of the focal actor to choose from
    UPROPERTY(EditAnywhere, Category = "Randomization")
    TArray<AActor*> FocalTargetActors;

    // How fast (degree per seconds) the actor rotate when it need to change direction
    UPROPERTY(EditAnywhere, Category = "Randomization")
    float RotationSpeed;

protected:
    virtual void BeginPlay() override;
    virtual void OnRandomization_Implementation() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    UPROPERTY(Transient)
    AActor* CurrentFocalTarget;
};
