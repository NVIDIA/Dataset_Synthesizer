/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NVSceneCapturerGameModeBase.generated.h"

class ANVSceneCapturerActor;
class ANVSceneManager;

UCLASS(Blueprintable)
class NVSCENECAPTURERGAME_API ANVSceneCapturerGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    ANVSceneCapturerGameModeBase();

	UFUNCTION(BlueprintCallable, Category = "Capturer")
    const TArray<ANVSceneCapturerActor*>& GetSceneCapturerList() const
    {
        return SceneExporterList;
    }
    UFUNCTION(BlueprintCallable, Category = "Capturer")
    ANVSceneCapturerActor* GetFirstActiveCapturer() const;

protected:
    void BeginPlay() override;

protected:
	/// Default class of the SceneManager to create automatically if the scene doesn't have one
	UPROPERTY(EditDefaultsOnly, Category = "Capturer")
	TSubclassOf<ANVSceneManager> DefaultSceneManagerClass;

    UPROPERTY(Transient)
    TArray<ANVSceneCapturerActor*> SceneExporterList;
};
