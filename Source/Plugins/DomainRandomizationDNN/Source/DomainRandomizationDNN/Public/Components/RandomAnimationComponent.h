/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "RandomComponentBase.h"
#include "Animation/AnimInstance.h"
#include "RandomAnimationComponent.generated.h"

UENUM(BlueprintType)
enum class ENVHalfBodyType : uint8
{
    LeftSide = 0,
    RightSide = 1,
    NVHalfBodyType_MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ENVHalfBodyBoneType : uint8
{
    Heel = 0,
    Knee,
    Pelvis,
    Shoulder,
    Elbow,
    Wrist,
    Ear,
    Eye,
    Nose,
    NVHalfBodyBoneType_MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FNVHalfBodyBoneData
{
    GENERATED_BODY()

public: // Editor properties
    UPROPERTY(EditAnywhere)
    FName BoneNames[(uint8)ENVHalfBodyBoneType::NVHalfBodyBoneType_MAX];
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FNVHumanSkeletalBoneData
{
    GENERATED_BODY()

public: // Editor properties
    UPROPERTY(EditAnywhere)
    FNVHalfBodyBoneData SideBoneData[(uint8)ENVHalfBodyType::NVHalfBodyType_MAX];

public:
    FName GetBoneName(ENVHalfBodyType BodySide, ENVHalfBodyBoneType BoneType) const
    {
        return SideBoneData[(uint8)BodySide].BoneNames[(uint8)BoneType];
    }
};

/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), meta = (BlueprintSpawnableComponent))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class DOMAINRANDOMIZATIONDNN_API URandomAnimationComponent : public URandomComponentBase
{
    GENERATED_BODY()

public:
    URandomAnimationComponent();
    virtual void BeginPlay() override;

public: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    bool bUseAllAnimationsInAFolder;

    // The path to the folder where to find the animation sequence
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bUseAllTextureInAFolder"))
    FString AnimationFolderPath;

    UPROPERTY(EditAnywhere, Category = Randomization, meta = (EditCondition = "!bUseAllTextureInAFolder"))
    TArray<UAnimSequence*> RandomAnimList;

    UPROPERTY(EditAnywhere, Category = Randomization)
    float TimeWaitAfterEachAnimation;

    UPROPERTY(EditAnywhere, Category = Randomization)
    FNVHumanSkeletalBoneData HumanSkeletalBoneData;

protected:
    virtual void OnRandomization_Implementation() override;
    virtual void OnFinishedRandomization() override;

protected:
    UPROPERTY(Transient)
    UAnimSequence* CurrentAnimation;

    UPROPERTY(Transient)
    TArray<FSoftObjectPath> FolderAnimSequenceReferences;
};
