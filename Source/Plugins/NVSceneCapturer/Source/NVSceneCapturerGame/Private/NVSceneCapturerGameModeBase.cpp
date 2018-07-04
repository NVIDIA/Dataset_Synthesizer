/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerGameModeBase.h"
#include "NVSceneCapturerGameModule.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneManager.h"

ANVSceneCapturerGameModeBase::ANVSceneCapturerGameModeBase() : Super()
{
	DefaultSceneManagerClass = ANVSceneManager::StaticClass();
}

void ANVSceneCapturerGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    SceneExporterList.Reset();
    UWorld* World = GetWorld();
    if (World)
    {
		const auto ItSceneMgr = TActorIterator<ANVSceneManager>(World);
		if (!ItSceneMgr)
		{
			if (DefaultSceneManagerClass)
			{
				UE_LOG(LogNVSceneCapturerGame, Warning, TEXT("Missing Capturer Scene Manager, creating a default one with type: %s."), *DefaultSceneManagerClass.Get()->GetName());
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.bNoFail = true;
				World->SpawnActor<ANVSceneManager>(DefaultSceneManagerClass, SpawnParams);
			}
			else
			{
				UE_LOG(LogNVSceneCapturerGame, Error, TEXT("Missing Capturer Scene Manager and the game mode does not specify a default class, the capturer will not work. (make sure you either add a Scene Manager to the level or set a default manager class in the game mode."));
			}
		}

        for (TActorIterator<ANVSceneCapturerActor> It(World); It; ++It)
        {
            ANVSceneCapturerActor* CheckExporter = *It;
            if (CheckExporter)
            {
                SceneExporterList.Add(CheckExporter);
            }
        }
    }
}

ANVSceneCapturerActor* ANVSceneCapturerGameModeBase::GetFirstActiveCapturer() const
{
    for (ANVSceneCapturerActor* CheckCapturer : SceneExporterList)
    {
        if (CheckCapturer && (CheckCapturer->GetCurrentState() != ENVSceneCapturerState::NotActive))
        {
            return CheckCapturer;
        }
    }

    return nullptr;
}
