/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "GameFramework/Volume.h"
#include "NVSceneMarker.h"
#include "NVSceneMarker_Volume.generated.h"

/**
 * ANVSceneMarker_Volume - The scene marker actors are placed in the map and control how the capturer move around
 */
 /// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API ANVSceneMarker_Volume : public AVolume, public INVSceneMarkerInterface
{
    GENERATED_BODY()

public:
    ANVSceneMarker_Volume(const FObjectInitializer& ObjectInitializer);

protected: // Editor properties
    UPROPERTY(EditAnywhere)
    FNVSceneMarkerComponent SceneMarker;
    NV_DECLARE_SCENE_MARKER_INTERFACE(SceneMarker);

protected:
    virtual void OnObserverAdded(AActor* NewObserver) override;
    virtual void OnObserverRemoved(AActor* NewObserver) override;
};
