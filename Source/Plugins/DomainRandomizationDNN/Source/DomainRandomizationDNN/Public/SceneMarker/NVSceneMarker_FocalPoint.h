/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneMarker.h"
#include "NVSceneMarker_FocalPoint.generated.h"

/**
 * ANVSceneMarker_FocalPoint - The scene marker actors are placed in the map and control how the capturer move around
 */
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config=Engine, HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API ANVSceneMarker_FocalPoint : public AActor, public INVSceneMarkerInterface
{
    GENERATED_BODY()

public:
    ANVSceneMarker_FocalPoint(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void OnObserverAdded(AActor* NewObserver) override;
    virtual void OnObserverRemoved(AActor* NewObserver) override;

protected: // Editor properties
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class UBillboardComponent* BillboardComponent;

    // The spring arm component control where to attach the viewpoint
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class USpringArmComponent* SpringArm_ViewPointComponent;

    UPROPERTY(EditAnywhere)
    FNVSceneMarkerComponent SceneMarker;
    NV_DECLARE_SCENE_MARKER_INTERFACE(SceneMarker);

    // If true, the observer's orbital movement will be activated and it will focus on this focal point
    // If false, the observer's position will be attached to a socket on SpringArm_ViewPointComponent with no random movement
    UPROPERTY(EditAnywhere)
    bool bUseOrbitalMovement;
};
