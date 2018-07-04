/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "SpatialLayoutGenerator.h"
#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator_Circle.generated.h"

UCLASS(Blueprintable)
class DOMAINRANDOMIZATIONDNN_API USpatialLayoutGenerator_Circle : public USpatialLayoutGenerator
{
    GENERATED_BODY()

public:
    USpatialLayoutGenerator_Circle(const FObjectInitializer& ObjectInitializer);

    // Get the transform (in world coordinate) for a group of actors to match this layout
    // LayoutTransform - transformation of the layout
    virtual TArray<FTransform> GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const;

protected: // Editor properties
    // Radius of the circle
    UPROPERTY(EditAnywhere, Category = CircleLayout, meta = (UIMin = 1))
    float CircleRadius;

    // If true, all actors should face the center of the circle
    UPROPERTY(EditAnywhere, Category = CircleLayout, meta = (UIMin = 1))
    bool bActorFacingCenter;
};
