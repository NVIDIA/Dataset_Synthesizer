/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator.h"
#include "SpatialLayoutGenerator_Grid2d.generated.h"

UCLASS(Blueprintable)
class DOMAINRANDOMIZATIONDNN_API USpatialLayoutGenerator_Grid2d : public USpatialLayoutGenerator
{
    GENERATED_BODY()

public:
    USpatialLayoutGenerator_Grid2d(const FObjectInitializer& ObjectInitializer);

    /** Get the transform (in world coordinate) for a group of actors to match this layout
     * LayoutTransform - transformation of the layout
     */
    virtual TArray<FTransform> GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const;

protected: // Editor properties
    /** The number of row in the grid */
    UPROPERTY(EditAnywhere, Category = GridLayout, meta = (UIMin = 1))
    uint32 GridRowCount;

    /** The number of column in the grid */
    UPROPERTY(EditAnywhere, Category = GridLayout, meta = (UIMin = 1))
    uint32 GridColumnCount;

    /** The size of each cell in the grid */
    UPROPERTY(EditAnywhere, Category = GridLayout)
    FVector2D GridCellSize;
};
