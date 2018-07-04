/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "DomainRandomizationDNNPCH.h"
#include "NVSceneManager.h"
#include "DRSceneManager.generated.h"

/**
 * The new actor which get annotated and have its info captured and exported
 */
 /// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config=Engine, HideCategories = (Replication, Tick, Tags, Input, Actor, Rendering))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API ADRSceneManager : public ANVSceneManager
{
    GENERATED_BODY()

public:
    ADRSceneManager(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void PostLoad() override;
    virtual void BeginPlay() override;
    virtual void UpdateSettingsFromCommandLine() override;
    virtual void SetupSceneInternal() override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITORONLY_DATA

protected: // Editor properties
    // Reference to the actor that manage the training actors
    UPROPERTY(EditInstanceOnly)
    class AGroupActorManager* GroupActorManager;

    UPROPERTY(EditInstanceOnly)
    class AGroupActorManager* NoiseActorManager;

protected: // Transient properties
    UPROPERTY(Transient)
    bool bIsReady;
};
