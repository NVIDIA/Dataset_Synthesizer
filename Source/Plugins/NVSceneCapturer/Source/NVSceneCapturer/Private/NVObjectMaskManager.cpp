/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVObjectMaskManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

DEFINE_LOG_CATEGORY(LogNVObjectMaskManager);

//================================== UNVObjectMaskMananger ==================================
UNVObjectMaskMananger::UNVObjectMaskMananger()
{
    ActorMaskNameType = ENVActorMaskNameType::UseActorClassName;
    SegmentationIdAssignmentType = ENVIdAssignmentType::SpreadEvenly;
    bDebug = false;
}

void UNVObjectMaskMananger::Init(ENVActorMaskNameType NewMaskNameType, ENVIdAssignmentType NewIdAssignmentType)
{
	ActorMaskNameType = NewMaskNameType;
	SegmentationIdAssignmentType = NewIdAssignmentType;

	AllMaskNames.Reset();
	AllMaskActors.Reset();
}

FString UNVObjectMaskMananger::GetActorMaskName(ENVActorMaskNameType MaskNameType, const AActor* CheckActor)
{
    FString ActorMaskName = TEXT("");
    if (!CheckActor)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        switch (MaskNameType)
        {
            case ENVActorMaskNameType::UseActorTag:
            {
				const UNVCapturableActorTag* TagComponent = Cast<UNVCapturableActorTag>(CheckActor->GetComponentByClass(UNVCapturableActorTag::StaticClass()));
				if (TagComponent && TagComponent->IsValid())
				{
					ActorMaskName = TagComponent->Tag;
				}

				// TODO: Use the Actor's Tags array so we don't need to add a separated component to handle it
                //if (CheckActor->Tags.Num() > 0)
                //{
                //    ActorMaskName = CheckActor->Tags[0].ToString();
                //}

                break;
            }
            case ENVActorMaskNameType::UseActorMeshName:
            {
				TArray<UMeshComponent*> ActorMeshComps;
				CheckActor->GetComponents<UMeshComponent>(ActorMeshComps);
				for (auto CheckMeshComp : ActorMeshComps)
                {
                    if (CheckMeshComp && CheckMeshComp->IsVisible())
                    {
                        // Need to get the mesh which this component use
                        UStaticMeshComponent* CheckStaticMeshComp = Cast<UStaticMeshComponent>(CheckMeshComp);
                        if (CheckStaticMeshComp)
                        {
                            UStaticMesh* StaticMesh = CheckStaticMeshComp->GetStaticMesh();
                            if (StaticMesh)
                            {
                                ActorMaskName = StaticMesh->GetName();
                                // NOTE: Only use the first mesh for the name when the actor have multiple meshes
                                break;
                            }
                        }
                        else
                        {
                            USkeletalMeshComponent* CheckSkeletalMeshComp = Cast<USkeletalMeshComponent>(CheckMeshComp);
                            if (CheckSkeletalMeshComp)
                            {
                                USkeletalMesh* SkeletalMesh = CheckSkeletalMeshComp->SkeletalMesh;
                                if (SkeletalMesh)
                                {
                                    ActorMaskName = SkeletalMesh->GetName();
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            }
            case ENVActorMaskNameType::UseActorClassName:
            {
                ActorMaskName = CheckActor->GetClass()->GetName();
                break;
            }
            default:
			{
				ActorMaskName = CheckActor->GetName();
				break;
			}
		}
    }
    return ActorMaskName;
}

void UNVObjectMaskMananger::ApplyStencilMaskToActor(AActor* CheckActor, uint8 MaskId)
{
	ensure(CheckActor != nullptr);
	if (CheckActor == nullptr)
	{
		UE_LOG(LogNVObjectMaskManager, Error, TEXT("Invalid argument."));
	}
	else
	{
		// Update the actor's meshes to render custom depth or not as this component's data changed
		TArray<UMeshComponent*> ActorMeshComps;
		CheckActor->GetComponents< UMeshComponent>(ActorMeshComps);
		for (auto CheckMeshComp : ActorMeshComps)
		{
			if (CheckMeshComp)
			{
				CheckMeshComp->SetCustomDepthStencilValue((int32)MaskId);
				CheckMeshComp->SetRenderCustomDepth(true);
				//CheckMeshComp->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_255);
			}
		}
	}
}

void UNVObjectMaskMananger::ApplyVertexColorMaskToActor(AActor* CheckActor, uint32 MaskId)
{
	ensure(CheckActor != nullptr);
	if (!CheckActor)
	{
		UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
	}
	else
	{
		const FColor& MaskVertexColor = NVSceneCapturerUtils::ConvertInt32ToVertexColor(MaskId);
		NVSceneCapturerUtils::SetMeshVertexColor(CheckActor, MaskVertexColor);

#if WITH_EDITOR
		// Mark the actor as selected so it will show up on the vertex color view mode
		GSelectedActorAnnotation.Set(CheckActor);
#endif // WITH_EDITOR
	}
}

FString UNVObjectMaskMananger::GetActorMaskName(const AActor* CheckActor) const
{
    FString result = FString();
    ensure(CheckActor!=nullptr);
    if (CheckActor==nullptr)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        if (!CheckActor->IsHidden())
        {
            result = GetActorMaskName(ActorMaskNameType, CheckActor);
        }
    }
    return result;
}

bool UNVObjectMaskMananger::ShouldCheckActorMask(const AActor* CheckActor) const
{
    check(CheckActor);
    if (CheckActor && !CheckActor->IsHidden())
    {
		TArray<UMeshComponent*> ActorMeshComps;
		CheckActor->GetComponents< UMeshComponent>(ActorMeshComps);
		for (auto CheckMeshComp : ActorMeshComps)
        {
            if (CheckMeshComp && CheckMeshComp->IsVisible())
            {
                // TODO: May need to check if the mesh component are actually hook up with a mesh asset
                return true;
            }
        }
    }

    return false;
}

void UNVObjectMaskMananger::ScanActors(UWorld* World)
{
    AllMaskNames.Reset();
    AllMaskActors.Reset();

    ensure(World!=nullptr);
    if (!World)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("Invalid argument."));
    }
    else
    {
        // Scan all the actors in the world to find all the unique mask names
        for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
        {
            AActor* CheckActor = *ActorIt;
            if (ShouldCheckActorMask(CheckActor))
            {
                const FString ActorMaskName = GetActorMaskName(CheckActor);
                if (!ActorMaskName.IsEmpty())
                {
                    if (!AllMaskNames.Contains(ActorMaskName))
                    {
                        AllMaskNames.Add(ActorMaskName);
                    }

                    AllMaskActors.Add(CheckActor);
                }
            }
        }
        // Sort the mask names in alphabet order
        AllMaskNames.Sort([](const FString& A, const FString& B)
        {
            return (A < B);
        });
    }
}

//================================== UNVObjectMaskMananger_Stencil ==================================
UNVObjectMaskMananger_Stencil::UNVObjectMaskMananger_Stencil() : Super()
{
	ActorMaskNameType = ENVActorMaskNameType::UseActorInstanceName;
}

void UNVObjectMaskMananger_Stencil::ScanActors(UWorld* World)
{
    ensure(World!=nullptr);
    if (!World)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        Super::ScanActors(World);

        // TODO: Need to check whether the project enabled custom depth rendering or not

        // NOTE: Stencil buffer is only 8bits => only support 255 values (ignore the 0)
        static const uint32 MaxNumberOfMasks = MAX_uint8;

        const uint32 TotalMaskCount = (uint32)AllMaskNames.Num();
        if (TotalMaskCount > MaxNumberOfMasks)
        {
            UE_LOG(LogNVSceneCapturer, Error, TEXT("UNVObjectMaskMananger_Stencil - There are too many different masks. Some of the valid actors will not have mask - MaxNumberOfMasks: %d - TotalMaskCount : %d"), MaxNumberOfMasks, TotalMaskCount);

            if (bDebug)
            {
                UE_LOG(LogNVSceneCapturer, Warning, TEXT("UNVObjectMaskMananger_Stencil - All the mask name:"));
                for (uint32 i = 0; i < TotalMaskCount; i++)
                {
                    UE_LOG(LogNVSceneCapturer, Warning, TEXT("%s"), *AllMaskNames[i]);
                }
            }
        }

        MaskNameIdMap.Reset();

        const uint32 ValidMaskCount = FMath::Min(TotalMaskCount, MaxNumberOfMasks);

        // Assign the mask id for each valid map names
        for (uint32 i = 0; i < ValidMaskCount; i++)
        {
            uint8 NewMaskId = 0;
            if (SegmentationIdAssignmentType == ENVIdAssignmentType::Sequential)
            {
                NewMaskId = FMath::Min(i + 1, MaxNumberOfMasks);
            }
            else if (SegmentationIdAssignmentType == ENVIdAssignmentType::SpreadEvenly)
            {
                NewMaskId = (MaxNumberOfMasks / ValidMaskCount) * (i + 1);
            }
            MaskNameIdMap.Add(AllMaskNames[i], NewMaskId);
        }

        // Apply the mask to valid actors
        for (auto CheckActor : AllMaskActors)
        {
            if (CheckActor)
            {
                const uint8 ActorMaskId = GetMaskId(CheckActor);
                if (ActorMaskId > 0)
                {
                    ApplyStencilMaskToActor(CheckActor, ActorMaskId);
                }
            }
        }
    }
}

uint8 UNVObjectMaskMananger_Stencil::GetMaskId(const FString& MaskName) const
{
    return (!MaskName.IsEmpty() && MaskNameIdMap.Contains(MaskName))?
               MaskNameIdMap[MaskName]:
               0;
}

uint8 UNVObjectMaskMananger_Stencil::GetMaskId(const AActor* CheckActor) const
{
    uint8 result = 0;
    ensure(CheckActor!=nullptr);
    if (!CheckActor)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        const FString MaskName = GetActorMaskName(CheckActor);
        if (!MaskName.IsEmpty())
        {
            result = GetMaskId(MaskName);
        }
    }
    return result;
}

//================================== UNVObjectMaskMananger_VertexColor ==================================
UNVObjectMaskMananger_VertexColor::UNVObjectMaskMananger_VertexColor() : Super()
{
	ActorMaskNameType = ENVActorMaskNameType::UseActorMeshName;
}

uint32 UNVObjectMaskMananger_VertexColor::GetMaskId(const FString& MaskName) const
{
    if (!MaskName.IsEmpty() && MaskNameIdMap.Contains(MaskName))
    {
        return MaskNameIdMap[MaskName];
    }

    return 0;
}

uint32 UNVObjectMaskMananger_VertexColor::GetMaskId(const AActor* CheckActor) const
{
    uint32 result = 0;
    ensure(CheckActor!=nullptr);
    if (!CheckActor)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        const FString MaskName = GetActorMaskName(CheckActor);
        if (!MaskName.IsEmpty())
        {
            result= GetMaskId(MaskName);
        }
    }
    return result;
}

void UNVObjectMaskMananger_VertexColor::ScanActors(UWorld* World)
{
    ensure(World!=nullptr);
    if (!World)
    {
        UE_LOG(LogNVObjectMaskManager, Error, TEXT("invalid argument."));
    }
    else
    {
        Super::ScanActors(World);

        // TODO: Unify this function and UNVObjectMaskMananger_Stencil::ScanActors using template ?

        const uint32 TotalMaskCount = (uint32)AllMaskNames.Num();
        if (TotalMaskCount > NVSceneCapturerUtils::MaxVertexColorID)
        {
            UE_LOG(LogNVObjectMaskManager, Error, TEXT("UNVObjectMaskMananger_VertexColor - There are too many different masks. Some of the valid actors will not have mask - MaxNumberOfMasks: %d - TotalMaskCount : %d"), NVSceneCapturerUtils::MaxVertexColorID, TotalMaskCount);

            if (bDebug)
            {
                UE_LOG(LogNVObjectMaskManager, Warning, TEXT("UNVObjectMaskMananger_VertexColor - All the mask name:"));
                for (uint32 i = 0; i < TotalMaskCount; i++)
                {
                    UE_LOG(LogNVObjectMaskManager, Warning, TEXT("%s"), *AllMaskNames[i]);
                }
            }
        }

        MaskNameIdMap.Reset();

        const uint32 ValidMaskCount = FMath::Min(TotalMaskCount, NVSceneCapturerUtils::MaxVertexColorID);

        // Assign the mask id for each valid map names
        for (uint32 i = 0; i < ValidMaskCount; i++)
        {
            uint32 NewMaskId = 0;
            if (SegmentationIdAssignmentType == ENVIdAssignmentType::Sequential)
            {
                NewMaskId = FMath::Min(i + 1, NVSceneCapturerUtils::MaxVertexColorID);
            }
            else if (SegmentationIdAssignmentType == ENVIdAssignmentType::SpreadEvenly)
            {
                NewMaskId = (NVSceneCapturerUtils::MaxVertexColorID / ValidMaskCount) * (i + 1);
            }
            MaskNameIdMap.Add(AllMaskNames[i], NewMaskId);
        }

        // Apply the mask to valid actors
        for (auto CheckActor : AllMaskActors)
        {
            if (CheckActor)
            {
                const uint32 ActorMaskId = GetMaskId(CheckActor);
                if (ActorMaskId > 0)
                {
                    ApplyVertexColorMaskToActor(CheckActor, ActorMaskId);
                }
            }
        }
    }
}

//================================== FNVObjectSegmentation_Instance ==================================
FNVObjectSegmentation_Instance::FNVObjectSegmentation_Instance()
{
	SegmentationIdAssignmentType = ENVIdAssignmentType::SpreadEvenly;
	VertexColorMaskManager = nullptr;
}

uint32 FNVObjectSegmentation_Instance::GetInstanceId(const AActor* CheckActor) const
{
	ensure(VertexColorMaskManager != nullptr);
	if (VertexColorMaskManager)
	{
		return VertexColorMaskManager->GetMaskId(CheckActor);
	}

	return 0;
}

void FNVObjectSegmentation_Instance::Init(UObject* OwnerObject)
{
	check(OwnerObject != nullptr);
	check(VertexColorMaskManager == nullptr);
	VertexColorMaskManager = NewObject<UNVObjectMaskMananger_VertexColor>(OwnerObject, TEXT("NVObjectMaskMananger_VertexColor"));
	VertexColorMaskManager->Init(ENVActorMaskNameType::UseActorInstanceName, SegmentationIdAssignmentType);
}

void FNVObjectSegmentation_Instance::ScanActors(UWorld* World)
{
	check(VertexColorMaskManager != nullptr);
	VertexColorMaskManager->ScanActors(World);
}

//================================== FNVObjectSegmentation_Class ==================================
FNVObjectSegmentation_Class::FNVObjectSegmentation_Class()
{
	SegmentationIdAssignmentType = ENVIdAssignmentType::SpreadEvenly;
	StencilMaskManager = nullptr;
	ClassSegmentationType = ENVActorClassSegmentationType::UseActorMeshName;
}

uint8 FNVObjectSegmentation_Class::GetInstanceId(const AActor* CheckActor) const
{
	ensure(StencilMaskManager != nullptr);
	if (StencilMaskManager)
	{
		return StencilMaskManager->GetMaskId(CheckActor);
	}

	return 0;
}

void FNVObjectSegmentation_Class::Init(UObject* OwnerObject)
{
	check(OwnerObject != nullptr);
	check(StencilMaskManager == nullptr);
	StencilMaskManager = NewObject<UNVObjectMaskMananger_Stencil>(OwnerObject, TEXT("NVObjectMaskMananger_Stencil"));

	ENVActorMaskNameType ActorMaskNameType = ENVActorMaskNameType::UseActorMeshName;
	switch (ClassSegmentationType)
	{
	case ENVActorClassSegmentationType::UseActorTag:
		ActorMaskNameType = ENVActorMaskNameType::UseActorTag;
		break;
	case ENVActorClassSegmentationType::UseActorClassName:
		ActorMaskNameType = ENVActorMaskNameType::UseActorClassName;
		break;
	case ENVActorClassSegmentationType::UseActorMeshName:
	default:
		ActorMaskNameType = ENVActorMaskNameType::UseActorMeshName;
		break;
	}
	StencilMaskManager->Init(ActorMaskNameType, SegmentationIdAssignmentType);
}

void FNVObjectSegmentation_Class::ScanActors(UWorld* World)
{
	check(StencilMaskManager != nullptr);
	StencilMaskManager->ScanActors(World);
}
