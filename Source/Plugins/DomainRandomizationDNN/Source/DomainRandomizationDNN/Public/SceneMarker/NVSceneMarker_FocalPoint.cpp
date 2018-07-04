/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "NVSceneMarker_FocalPoint.h"
#include "Components/StaticMeshComponent.h"
#include "OrbitalMovementComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

ANVSceneMarker_FocalPoint::ANVSceneMarker_FocalPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root"));
    BillboardComponent->SetMobility(EComponentMobility::Movable);
    RootComponent = BillboardComponent;

    SpringArm_ViewPointComponent = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("SpringArm_ViewPoint"));
    SpringArm_ViewPointComponent->SetupAttachment(RootComponent);
    SpringArm_ViewPointComponent->TargetArmLength = 100.0f; // The camera follows at this distance behind the projectile
    SpringArm_ViewPointComponent->bUsePawnControlRotation = false;
    SpringArm_ViewPointComponent->bEnableCameraLag = false;
    SpringArm_ViewPointComponent->bEnableCameraRotationLag = false;
    SpringArm_ViewPointComponent->bUseCameraLagSubstepping = false;

    bUseOrbitalMovement = true;
}

void ANVSceneMarker_FocalPoint::OnObserverAdded(AActor* NewObserver)
{
    INVSceneMarkerInterface::OnObserverAdded(NewObserver);

    if (NewObserver)
    {
        if (bUseOrbitalMovement)
        {
            TArray<UActorComponent*> OrbitalMovementComps = NewObserver->GetComponentsByClass(UOrbitalMovementComponent::StaticClass());
            for (auto& CheckComp : OrbitalMovementComps)
            {
                UOrbitalMovementComponent* CheckOrbitalMovementComp = Cast<UOrbitalMovementComponent>(CheckComp);
                if (CheckOrbitalMovementComp)
                {
                    CheckOrbitalMovementComp->SetFocalActor(this);
                    CheckOrbitalMovementComp->StartRandomizing();
                }
            }
        }
        else
        {
            NewObserver->AttachToComponent(SpringArm_ViewPointComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                                           USpringArmComponent::SocketName);
        }
    }
}

void ANVSceneMarker_FocalPoint::OnObserverRemoved(AActor* NewObserver)
{
    if (NewObserver)
    {
        if (bUseOrbitalMovement)
        {
            TArray<UActorComponent*> OrbitalMovementComps = NewObserver->GetComponentsByClass(UOrbitalMovementComponent::StaticClass());
            for (auto& CheckComp : OrbitalMovementComps)
            {
                UOrbitalMovementComponent* CheckOrbitalMovementComp = Cast<UOrbitalMovementComponent>(CheckComp);
                if (CheckOrbitalMovementComp)
                {
                    CheckOrbitalMovementComp->SetFocalActor(nullptr);
                    CheckOrbitalMovementComp->StopRandomizing();
                }
            }
        }
        else if (NewObserver->GetAttachParentActor() == this)
        {
            NewObserver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
    }
}
