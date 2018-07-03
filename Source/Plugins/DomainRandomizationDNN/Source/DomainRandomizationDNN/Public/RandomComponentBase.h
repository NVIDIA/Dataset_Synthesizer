/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "RandomComponentBase.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories = (Replication, ComponentReplication, Cooking, Events, ComponentTick, Actor, Input, Rendering, Collision, PhysX, Activation, Sockets, Tags))
class DOMAINRANDOMIZATIONDNN_API URandomComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    URandomComponentBase();

    bool ShouldRandomize() const;
    void Randomize();
    void StartRandomizing();
    void StopRandomizing();

protected:
    virtual void PostLoad() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintNativeEvent, Category = Randomization)
    void OnRandomization();
    virtual void OnRandomization_Implementation();

    virtual void OnFinishedRandomization();

protected: // Editor properties
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Randomization)
    bool bShouldRandomize;

    // How long (in seconds) does the component wait between randomizations
    // NOTE: If the UpperBound of RandomizationDuration is <= 0 then this component doesnt update at all
    // If LowerBound different from UpperBound then the component will choose a random duration to wait inside that range in between randomization update
    // DEPRECATED_FORGAME(4.16, "Use RandomizationDurationInterval instead")
    UPROPERTY()
    FFloatRange RandomizationDurationRange;

    // How long (in seconds) does the component wait between randomizations
    // NOTE: If the Max value of RandomizationDurationInterval is < 0 then this component doesnt update at all
    // If the Max value of RandomizationDurationInterval is = 0 then this component update every frame
    // If the Min value different from the Max value then the component will choose a random duration to wait inside that range in between randomization update
    UPROPERTY(EditAnywhere, Category = Randomization, meta = (EditCondition = bShouldRandomize))
    FFloatInterval RandomizationDurationInterval;

    // If true, this component only randomize once in the begining
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    bool bOnlyRandomizeOnce;

protected: // Transient properties
    UPROPERTY(Transient)
    float CountdownUntilNextRandomization;
    UPROPERTY(Transient)
    bool bAlreadyRandomized;

private:
    void UpdateRandomization();
};
