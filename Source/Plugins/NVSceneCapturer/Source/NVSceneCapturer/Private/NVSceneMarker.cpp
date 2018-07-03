/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif
DEFINE_LOG_CATEGORY(LogNVSceneMaker);

UNVSceneMarkerInterface::UNVSceneMarkerInterface(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

const FNVSceneMarkerComponent& INVSceneMarkerInterface::GetSceneMarkerComponent() const
{
    return (const_cast<std::decay<decltype(*this)>::type*>(this))->GetSceneMarkerComponent();
}

bool INVSceneMarkerInterface::IsActive() const
{
    const FNVSceneMarkerComponent& SceneMarkerComp = GetSceneMarkerComponent();
    return SceneMarkerComp.bIsActive;
}

void INVSceneMarkerInterface::AddObserver(AActor* NewObserver)
{
    ensure(NewObserver != nullptr);
    if (NewObserver == nullptr)
    {
        UE_LOG(LogNVSceneMaker, Error, TEXT("invalid argument."));
    }
    else
    {
        FNVSceneMarkerComponent& SceneMarkerComp = GetSceneMarkerComponent();
        if (SceneMarkerComp.AddObserver(NewObserver))
        {
            OnObserverAdded(NewObserver);
        }
    }
}

void INVSceneMarkerInterface::RemoveAllObservers()
{
    FNVSceneMarkerComponent& SceneMarkerComp = GetSceneMarkerComponent();
    AActor* RemovedActor = SceneMarkerComp.RemoveObserver();
    while (RemovedActor)
    {
        OnObserverRemoved(RemovedActor);
        RemovedActor = SceneMarkerComp.RemoveObserver();
    }
}

void INVSceneMarkerInterface::OnObserverAdded(AActor* NewObserver)
{
    ensure(NewObserver!=nullptr);
    if (NewObserver == nullptr)
    {
        UE_LOG(LogNVSceneMaker, Error, TEXT("invalid argument."));
    }
    else
    {
        const AActor* SelfAsActor = Cast<AActor>(this);
        const FTransform& MarkerTransform = SelfAsActor->GetActorTransform();

        // NOTE: When add new observer, we want to teleport it to the new marker without checking the collision, only handle the collision later after
        const bool bSweep = false;
        const ETeleportType TeleportType = ETeleportType::TeleportPhysics;
        NewObserver->SetActorTransform(MarkerTransform, bSweep, nullptr, TeleportType);
    }
}

void INVSceneMarkerInterface::OnObserverRemoved(AActor* NewObserver)
{
    // Do nothing by default. To be implemented by child class
}

//============================================== FNVSceneMarkerComponent ==============================================
FNVSceneMarkerComponent::FNVSceneMarkerComponent()
{
    bIsActive = true;
    DisplayName = TEXT("");
    Description = TEXT("");
}

bool FNVSceneMarkerComponent::AddObserver(AActor* NewObserver)
{
    bool bResult = false;
    ensure(NewObserver != nullptr);
    if (NewObserver==nullptr)
    {
        UE_LOG(LogNVSceneMaker, Error, TEXT("Invalid argument."));
    }
    else
    {
        if (!Observers.Contains(NewObserver))
        {
            Observers.Add(NewObserver);
            bResult = true;
        }
    }
    return bResult;
}

AActor* FNVSceneMarkerComponent::RemoveObserver()
{
    AActor* result = nullptr;
    if (Observers.Num() > 0)
    {
        result = Observers.Pop();
    }
    return result;
}
