/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator_Grid2d.h"

// Sets default values
USpatialLayoutGenerator_Grid2d::USpatialLayoutGenerator_Grid2d(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    GridRowCount = 1;
    GridColumnCount = 1;
    GridCellSize = FVector2D(10.f, 10.f);
}

TArray<FTransform> USpatialLayoutGenerator_Grid2d::GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const
{
    FVector2D FullGridSize;
    FullGridSize.X = FMath::Max(1.f, GridCellSize.X) * GridColumnCount;
    FullGridSize.Y = FMath::Max(1.f, GridCellSize.Y) * GridRowCount;

    const FVector& GridCenterLoc = LayoutTransform.GetLocation();
    const FRotator& LayoutRotation = LayoutTransform.GetRotation().Rotator();
    const FVector& LayoutScale = LayoutTransform.GetScale3D();

    FVector GridTopLeft = GridCenterLoc;
    GridTopLeft.X = GridCenterLoc.X - FullGridSize.X * 0.5f;
    GridTopLeft.Y = GridCenterLoc.Y - FullGridSize.Y * 0.5f;

    TArray<FTransform> ActorTransforms;
    ActorTransforms.Reserve(TotalNumberOfActors);

    for (uint32 i = 0; i < TotalNumberOfActors; i++)
    {
        uint32 RowIndex = i / GridColumnCount;
        uint32 ColIndex = i % GridColumnCount;

        FVector NewLocation;
        NewLocation.X = GridCellSize.X * (ColIndex + 0.5f) + GridTopLeft.X;
        NewLocation.Y = GridCellSize.Y * (RowIndex + 0.5f) + GridTopLeft.Y;
        NewLocation.Z = GridCenterLoc.Z;

        FTransform NewTransform(LayoutRotation, NewLocation, LayoutScale);
        ActorTransforms.Add(NewTransform);
    }

    return ActorTransforms;
}
