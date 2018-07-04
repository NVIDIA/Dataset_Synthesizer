/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "GameFramework/Actor.h"
#include "NVObjectMaskManager.generated.h"

/// This enum describe how to get the mask name out of an actor
UENUM(BlueprintType)
enum class ENVActorMaskNameType : uint8
{
	/// Use the actor's instance name for its mask name. Each actors in the scene will have a unique name
	/// so this option will cause all the actors to  have its own unique mask.
	/// NOTE: Should only use this option for VertexColor mask since the total number of masks can be huge
	UseActorInstanceName = 0,

	/// Use the actor's mesh name for its mask name, actor with no visible mesh will be ignored
	/// NOTE: Since each actor can have multiple mesh components, we only use the first valid mesh's name for the mask
	UseActorMeshName,

    /// Use the actor's Tags for its mask name, actor with no tags will be ignored
    /// NOTE: Since each actor can have multiple tags, we only use the first one in the Tags list for the mask
    UseActorTag,

	/// Use the actor's class (either C++ or blueprint) name for its mask name. All the actor instances of the same class/blueprint will have the same mask
    UseActorClassName,

    // TODO: Add support for UNVCapturableActorTag?

    /// @endcond DOXYGEN_SUPPRESSED_CODE
    NVObjectClassMaskType_MAX UMETA(Hidden)
    /// @endcond DOXYGEN_SUPPRESSED_CODE
};

/// This enum describe how to assign an id for a mask
/// The reason for the "spread evenly" is for easy visualization.
/// IDs are translated to color linearly (grayscale 8 bits for stencil mask,
/// RGBA8 for vertex color mask).When the number of objects is small,
/// the mask images all look black and thus are hard to distinguish.
/// In this case "spread evenly" is useful to emphasize the color difference for
/// visibility.However, for all other scenarios, such as DL training,
/// the color doesn't matter and a sequentially allocating IDs may be easier to debug.
/// It also helps in cases where on is manually assigning additional IDs --
/// one need only increase the max ID.
UENUM(BlueprintType)
enum class ENVIdAssignmentType : uint8
{
    /// The id will be given sequentially to each masks
    Sequential = 0,

    /// The id will be spread evenly between mask
    /// The gap between id = MaxMaskValue / NumberOfMasks
    SpreadEvenly,

	/// @endcond DOXYGEN_SUPPRESSED_CODE
	NVActorMaskIdType_MAX UMETA(Hidden)
	/// @endcond DOXYGEN_SUPPRESSED_CODE
};

DECLARE_LOG_CATEGORY_EXTERN(LogNVObjectMaskManager, Log, All)

/// Mask base class: scan actors in the scene, assign them an ID based on mask type
UCLASS(NotBlueprintable, Abstract, DefaultToInstanced, editinlinenew, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVObjectMaskMananger : public UObject
{
    GENERATED_BODY()

public:
    UNVObjectMaskMananger();

    virtual void ScanActors(UWorld* World);

	void Init(ENVActorMaskNameType NewMaskNameType, ENVIdAssignmentType NewIdAssignmentType);

protected:
	static FString GetActorMaskName(ENVActorMaskNameType MaskNameType, const AActor* CheckActor);
	static void ApplyStencilMaskToActor(AActor* CheckActor, uint8 MaskId);
	static void ApplyVertexColorMaskToActor(AActor* CheckActor, uint32 MaskId);

    FString GetActorMaskName(const AActor* CheckActor) const;
    bool ShouldCheckActorMask(const AActor* CheckActor) const;

protected: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ActorMask)
    ENVActorMaskNameType ActorMaskNameType;

	/// How the segmentation id get generated for actors in the scene
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ActorMask)
    ENVIdAssignmentType SegmentationIdAssignmentType;

    /// Turn on this flag to print out debug information (e.g: list of mask name ...) when this object run
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = ActorMask)
    bool bDebug;

protected: // Transient
    UPROPERTY(Transient)
    TArray<FString> AllMaskNames;

    UPROPERTY(Transient)
    TArray<AActor*> AllMaskActors;
};

/// UNVObjectMaskMananger_Stencil scan actors in the scene, assign them an ID using StencilMask
/// NOTE: MaskId 0 mean the actor is ignored
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVObjectMaskMananger_Stencil : public UNVObjectMaskMananger
{
    GENERATED_BODY()

public:
    UNVObjectMaskMananger_Stencil();

    void ScanActors(UWorld* World) override;

    uint8 GetMaskId(const FString& MaskName) const;
    uint8 GetMaskId(const AActor* CheckActor) const;
protected:

protected: // Transient
    UPROPERTY(Transient)
    TMap<FString, uint8> MaskNameIdMap;
};

/// UNVObjectMaskMananger_VertexColor scan actors in the scene, assign them an ID using VertexColor (32bits)
/// NOTE: MaskId 0 mean the actor is ignored
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew, ClassGroup = (NVIDIA))
class NVSCENECAPTURER_API UNVObjectMaskMananger_VertexColor: public UNVObjectMaskMananger
{
	GENERATED_BODY()
	
public:
    UNVObjectMaskMananger_VertexColor();

    void ScanActors(UWorld* World) override;

    uint32 GetMaskId(const FString& MaskName) const;
    uint32 GetMaskId(const AActor* CheckActor) const;

protected: // Transient
    UPROPERTY(Transient)
    TMap<FString, uint32> MaskNameIdMap;

    static const uint32 MaxVertexColorID;
};

USTRUCT(Blueprintable)
struct NVSCENECAPTURER_API FNVObjectSegmentation_Instance
{
	GENERATED_BODY()

public:
	FNVObjectSegmentation_Instance();

	uint32 GetInstanceId(const AActor* CheckActor) const;
	void Init(UObject* OwnerObject);
	void ScanActors(UWorld* World);

protected:
// Editor properties

	UPROPERTY(EditAnywhere, Category = "Segmentation")
	ENVIdAssignmentType SegmentationIdAssignmentType;

// Transient properties

	UPROPERTY(Transient)
	UNVObjectMaskMananger_VertexColor* VertexColorMaskManager;
};

/// This enum describe how to get the class type out of an actor to use for segmentation
UENUM(BlueprintType)
enum class ENVActorClassSegmentationType : uint8
{
	/// Use the actor's mesh name for its class type name, actor with no visible mesh will be ignored
	/// NOTE: Since each actor can have multiple mesh components, we only use the first valid mesh's name for the mask
	UseActorMeshName = 0,

	/// Use the actor's Tags for its mask name, actor with no tags will be ignored
	/// NOTE: Since each actor can have multiple tags, we only use the first one in the Tags list for the mask
	UseActorTag,

	/// Use the actor's class (either C++ or blueprint) name for its class name. All the actor instances of the same class/blueprint will have the same mask
	UseActorClassName,

	/// @endcond DOXYGEN_SUPPRESSED_CODE
	ENVActorClassSegmentationType_MAX UMETA(Hidden)
	/// @endcond DOXYGEN_SUPPRESSED_CODE
};

USTRUCT(Blueprintable)
struct NVSCENECAPTURER_API FNVObjectSegmentation_Class
{
	GENERATED_BODY()

public:
	FNVObjectSegmentation_Class();

	uint8 GetInstanceId(const AActor* CheckActor) const;
	void Init(UObject* OwnerObject);
	void ScanActors(UWorld* World);

protected:
// Editor properties
	/// How to get the class type out of actors in the scene to use for segmentation
	UPROPERTY(EditAnywhere, Category = "Segmentation")
	ENVActorClassSegmentationType ClassSegmentationType;

	UPROPERTY(EditAnywhere, Category = "Segmentation")
	ENVIdAssignmentType SegmentationIdAssignmentType;

// Transient properties
	UPROPERTY(Transient)
	UNVObjectMaskMananger_Stencil* StencilMaskManager;
};