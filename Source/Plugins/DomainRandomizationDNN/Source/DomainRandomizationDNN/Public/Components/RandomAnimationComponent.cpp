/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomAnimationComponent.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistryModule.h"

// Sets default values
URandomAnimationComponent::URandomAnimationComponent()
{
    CountdownUntilNextRandomization = 0.f;
    TimeWaitAfterEachAnimation = 0.f;
    CurrentAnimation = nullptr;
}

void URandomAnimationComponent::BeginPlay()
{
    if (bUseAllAnimationsInAFolder && bShouldRandomize)
    {
        FolderAnimSequenceReferences.Reset();

        // FIXME: The AssetRegistryModule only work for Editor build
#if WITH_EDITORONLY_DATA
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetList;

        if (AssetRegistryModule.Get().GetAssetsByPath(*AnimationFolderPath, AssetList, true))
        {
            FStreamableManager AssetStreamer;

            for (const FAssetData& AssetData : AssetList)
            {
                UClass* AssetClass = AssetData.GetClass();
                if (AssetClass && AssetClass->IsChildOf<UAnimSequence>())
                {
                    const FSoftObjectPath& StrAssetRef = AssetData.ToSoftObjectPath();
                    FolderAnimSequenceReferences.Add(StrAssetRef);
                    // TODO: Should only async loading a small batch instead of all the assets
                    AssetStreamer.RequestAsyncLoad(StrAssetRef);
                }
            }
        }
#endif // WITH_EDITORONLY_DATA
    }

    Super::BeginPlay();
}

void URandomAnimationComponent::OnRandomization_Implementation()
{
    AActor* OwnerActor = GetOwner();
    int32 AnimCount = bUseAllAnimationsInAFolder ? FolderAnimSequenceReferences.Num() : RandomAnimList.Num();
    if (!OwnerActor || (AnimCount == 0))
    {
        return;
    }

    USkeletalMeshComponent* OwnerSkeletalMeshComp = Cast<USkeletalMeshComponent>(OwnerActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
    if (!OwnerSkeletalMeshComp)
    {
        return;
    }

    // Select a random animation to play next
    // NOTE: Try not to pick the same animation to play in a row
    UAnimSequence* RandAnim = CurrentAnimation;
    while (RandAnim == CurrentAnimation && RandomAnimList.Num() > 1)
    {
        RandAnim = RandomAnimList[FMath::Rand() % RandomAnimList.Num()];
        if (bUseAllAnimationsInAFolder)
        {
            int32 RandIndex = FMath::Rand() % AnimCount;
            FSoftObjectPath& RandomAsset = FolderAnimSequenceReferences[RandIndex];
            RandAnim = Cast<UAnimSequence>(RandomAsset.ResolveObject());
        }
        else
        {
            RandAnim = RandomAnimList[FMath::Rand() % AnimCount];
        }
    }

    if (RandAnim)
    {
        OwnerSkeletalMeshComp->PlayAnimation(RandAnim, false);
        CurrentAnimation = RandAnim;
    }
}

void URandomAnimationComponent::OnFinishedRandomization()
{
    if (CurrentAnimation)
    {
        const float AnimPlayLength = CurrentAnimation->GetPlayLength();
        static const float MinAnimTime = 1.f;
        CountdownUntilNextRandomization = FMath::Max(AnimPlayLength, MinAnimTime) + TimeWaitAfterEachAnimation;
    }
}
