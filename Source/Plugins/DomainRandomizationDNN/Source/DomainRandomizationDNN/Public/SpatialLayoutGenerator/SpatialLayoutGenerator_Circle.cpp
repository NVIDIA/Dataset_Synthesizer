/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator_Circle.h"

// Sets default values
USpatialLayoutGenerator_Circle::USpatialLayoutGenerator_Circle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    CircleRadius = 100.f;
    bActorFacingCenter = true;
}

TArray<FTransform> USpatialLayoutGenerator_Circle::GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const
{
    TArray<FTransform> ActorTransforms;
    if (TotalNumberOfActors > 0)
    {
        ActorTransforms.Reserve(TotalNumberOfActors);

        const FVector& CicleCenterLoc = LayoutTransform.GetLocation();
        const FRotator& LayoutRotation = LayoutTransform.GetRotation().Rotator();
        const FVector& LayoutScale = LayoutTransform.GetScale3D();

        const float PieAngle = 2 * PI / TotalNumberOfActors;

        for (uint32 i = 0; i < TotalNumberOfActors; i++)
        {
            const float ActorAngle = PieAngle * i;
            FVector NewLocation;
            NewLocation.X = CicleCenterLoc.X + CircleRadius * FMath::Cos(ActorAngle);
            NewLocation.Y = CicleCenterLoc.Y + CircleRadius * FMath::Sin(ActorAngle);
            NewLocation.Z = CicleCenterLoc.Z;

            FRotator NewRotation = LayoutRotation;
            if (bActorFacingCenter)
            {
                NewRotation.Yaw = -FMath::RadiansToDegrees(ActorAngle);
            }

            // TODO: Add randomization to each transform
            FTransform NewTransform(NewRotation, NewLocation, LayoutScale);
            ActorTransforms.Add(NewTransform);
        }
    }
    return ActorTransforms;
}
