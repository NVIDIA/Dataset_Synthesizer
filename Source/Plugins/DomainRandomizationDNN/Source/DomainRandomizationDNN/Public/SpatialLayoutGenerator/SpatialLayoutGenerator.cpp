/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator.h"

// Sets default values
USpatialLayoutGenerator::USpatialLayoutGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

TArray<FVector> USpatialLayoutGenerator::GetLocationForActors(const FVector& LayoutLocation, uint32 TotalNumberOfActors) const
{
    TArray<FVector> ActorLocations;
    ActorLocations.Reserve(TotalNumberOfActors);
    for (uint32 i = 0; i < TotalNumberOfActors; i++)
    {
        ActorLocations.Add(LayoutLocation);
    }
    return ActorLocations;
}

TArray<FTransform> USpatialLayoutGenerator::GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const
{
    return GetDefaultTransformForActors(LayoutTransform, TotalNumberOfActors);
}

TArray<FTransform> USpatialLayoutGenerator::GetDefaultTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors)
{
    TArray<FTransform> ActorTransforms;
    if (TotalNumberOfActors > 0)
    {
        ActorTransforms.Reserve(TotalNumberOfActors);
        for (uint32 i = 0; i < TotalNumberOfActors; i++)
        {
            // TODO: May be offset these location along a specific axis
            ActorTransforms.Add(LayoutTransform);
        }
    }
    return ActorTransforms;
}
