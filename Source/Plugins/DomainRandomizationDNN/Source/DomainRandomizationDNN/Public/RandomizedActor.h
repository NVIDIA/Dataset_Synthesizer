/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "RandomizedActor.generated.h"

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API ARandomizedActor : public AActor
{
    GENERATED_BODY()

public:
    ARandomizedActor(const class FObjectInitializer& ObjectInitializer);

    void OnRandomization();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UStaticMeshComponent* StaticMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomMeshComponent* RandomMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomMaterialComponent* RandomMaterialComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomMaterialParam_ColorComponent* RandomMaterialParam_ColorComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomVisibilityComponent* RandomVisibilityComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomMovementComponent* RandomMovementComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class URandomRotationComponent* RandomRotationComp;

    UPROPERTY(EditAnywhere, Instanced, Category="Randomization")
    TArray<class URandomDataObject*> RandomDataObjects;
};
