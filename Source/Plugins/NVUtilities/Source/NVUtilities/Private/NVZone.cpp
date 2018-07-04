/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#include "NVUtilitiesModule.h"
#include "NVZone.h"

ANVZone::ANVZone(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    RadiusRange = FFloatInterval(0.f, 100.f);
    HeightRange = FFloatInterval(0, 100.f);
}

FVector ANVZone::GetRandomLocationInZone() const
{
    FVector RandomLocation;
    // Pick X, Y in the circle in XY plane
    RandomLocation.X = FMath::RandRange(RadiusRange.Min, RadiusRange.Max);
    RandomLocation.Y = FMath::RandRange(RadiusRange.Min, RadiusRange.Max);
    // Pick Z along the Z axis
    RandomLocation.Z = FMath::RandRange(HeightRange.Min, HeightRange.Max);

    const FTransform& ZoneTransform = GetActorTransform();

    RandomLocation = ZoneTransform.TransformPosition(RandomLocation);
    return RandomLocation;
}

//=========================================== ANVNavigationZone ===========================================
ANVNavigationZone::ANVNavigationZone(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bLookAtCenter = false;
}
