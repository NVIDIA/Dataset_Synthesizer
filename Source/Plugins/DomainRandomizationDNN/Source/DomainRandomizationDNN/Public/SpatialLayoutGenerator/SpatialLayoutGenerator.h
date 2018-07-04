/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "DomainRandomizationDNNPCH.h"
#include "SpatialLayoutGenerator.generated.h"

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DOMAINRANDOMIZATIONDNN_API USpatialLayoutGenerator : public UObject
{
    GENERATED_BODY()

public:
    USpatialLayoutGenerator(const FObjectInitializer& ObjectInitializer);

    // Get the locations (in world coordinate) for a group of actors to match this layout
    // LayoutLocation - center location of the layout
    virtual TArray<FVector> GetLocationForActors(const FVector& LayoutLocation, uint32 TotalNumberOfActors) const;

    // Get the transform (in world coordinate) for a group of actors to match this layout
    // LayoutTransform - transformation of the layout
    virtual TArray<FTransform> GetTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors) const;

    // Default transform for actor in a layout
    static TArray<FTransform> GetDefaultTransformForActors(const FTransform& LayoutTransform, uint32 TotalNumberOfActors);
};
