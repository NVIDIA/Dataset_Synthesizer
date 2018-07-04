/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"

// Sets default values
URandomComponentBase::URandomComponentBase()
{
    // TODO (TT): Should only tick if the componnent is actively do randomization
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.bCanEverTick = true;

    bShouldRandomize = true;
    RandomizationDurationRange = FFloatRange(1.f, 3.f);
    RandomizationDurationInterval = FFloatInterval(0.1f, 0.5f);

    bAutoActivate = true;
    bAutoRegister = true;

    CountdownUntilNextRandomization = -1.f;

    bOnlyRandomizeOnce = false;
    bAlreadyRandomized = false;
}

bool URandomComponentBase::ShouldRandomize() const
{
    return bShouldRandomize && (!bOnlyRandomizeOnce || !bAlreadyRandomized);
}

void URandomComponentBase::StartRandomizing()
{
    bShouldRandomize = true;
    UpdateRandomization();
}

void URandomComponentBase::StopRandomizing()
{
    bShouldRandomize = false;
}

void URandomComponentBase::Randomize()
{
    if (ShouldRandomize())
    {
        OnRandomization();
        bAlreadyRandomized = true;
    }
}

void URandomComponentBase::PostLoad()
{
    Super::PostLoad();

    // Backward compatible for RandomizationDurationInterval
    if ((RandomizationDurationInterval.Min == -1.f) && (RandomizationDurationInterval.Max == -1.f))
    {
        RandomizationDurationInterval.Min = RandomizationDurationRange.GetLowerBoundValue();
        RandomizationDurationInterval.Max = RandomizationDurationRange.GetUpperBoundValue();
    }
}

void URandomComponentBase::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CountdownUntilNextRandomization >= 0.f)
    {
        CountdownUntilNextRandomization -= DeltaTime;
        if (CountdownUntilNextRandomization <= 0.f)
        {
            UpdateRandomization();
        }
    }
}

void URandomComponentBase::BeginPlay()
{
    Super::BeginPlay();

    UpdateRandomization();
    // NOTE: Since the order of the randomizing components matter, just don't mark the component to be already randomized from BeginPlay and wait after its 1rst time randomizing
    bAlreadyRandomized = false;
}

void URandomComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CountdownUntilNextRandomization = -1.f;

    Super::EndPlay(EndPlayReason);
}

void URandomComponentBase::OnRandomization_Implementation()
{
}

void URandomComponentBase::OnFinishedRandomization()
{
    const float MinDuration = RandomizationDurationInterval.Min;
    const float MaxDuration = RandomizationDurationInterval.Max;
    if (MaxDuration >= 0.f)
    {
        CountdownUntilNextRandomization = FMath::RandRange(MinDuration, MaxDuration);
    }
}

void URandomComponentBase::UpdateRandomization()
{
    if (ShouldRandomize())
    {
        OnRandomization();

        OnFinishedRandomization();

        bAlreadyRandomized = true;
    }
}
