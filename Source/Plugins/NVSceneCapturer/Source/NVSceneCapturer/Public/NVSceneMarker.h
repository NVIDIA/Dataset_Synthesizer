/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVSceneMarker.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVSceneMaker, Log, All)
///
/// Base interface for object that can be placed in the map as marker for other actors
///
UINTERFACE()
class NVSCENECAPTURER_API UNVSceneMarkerInterface : public UInterface
{
    GENERATED_UINTERFACE_BODY()
};

struct FNVSceneMarkerComponent;

///
/// Base interface for object that can be placed in the map as marker for other actors
///
class NVSCENECAPTURER_API INVSceneMarkerInterface
{
    GENERATED_IINTERFACE_BODY()

public:
    virtual FNVSceneMarkerComponent& GetSceneMarkerComponent() = 0;
    virtual const FNVSceneMarkerComponent& GetSceneMarkerComponent() const;

    bool IsActive() const;
    void AddObserver(AActor* NewObserver);
    void RemoveAllObservers();

protected:
    virtual void OnObserverAdded(AActor* NewObserver);
    virtual void OnObserverRemoved(AActor* NewObserver);
};

#define NV_DECLARE_SCENE_MARKER_INTERFACE(ComponentName) \
    virtual FNVSceneMarkerComponent& GetSceneMarkerComponent() final { return this->ComponentName; } \
    FNVSceneMarkerComponent const& GetSceneMarkerComponent() const { return this->ComponentName; }

//============================================== FNVSceneMarkerComponent ==============================================
///
/// Anchor for point of interest in map.
///
USTRUCT(BlueprintType)
struct NVSCENECAPTURER_API FNVSceneMarkerComponent
{
    GENERATED_BODY()

public:
    FNVSceneMarkerComponent();

    bool AddObserver(AActor* NewObserver);
    AActor* RemoveObserver();

public: // Editor properties
    // ToDo: move to protected.
    /// Scene markers can be manually deactivated via toggling the IsActive attribute to allow the user to control which subset of scene markers gets captured and exported.
    UPROPERTY(EditAnywhere)
    bool bIsActive;
protected: // Editor properties
    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere)
    FString Description;

protected: // Transient properties
    UPROPERTY(Transient)
    TArray<AActor*> Observers;
};
