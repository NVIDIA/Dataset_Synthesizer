/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneMarker.h"
#include "NVSceneMarker_Path.generated.h"

/**
 * ANVSceneMarker_Path - The scene marker actors are placed in the map and control how the capturer move around
 */
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config=Engine, HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API ANVSceneMarker_Path : public AActor, public INVSceneMarkerInterface
{
    GENERATED_BODY()

public:
    ANVSceneMarker_Path(const FObjectInitializer& ObjectInitializer);

protected: // Editor properties
    UPROPERTY(EditAnywhere)
    FNVSceneMarkerComponent SceneMarker;
    NV_DECLARE_SCENE_MARKER_INTERFACE(SceneMarker);

protected:
    virtual void OnObserverAdded(AActor* NewObserver) override;
};
