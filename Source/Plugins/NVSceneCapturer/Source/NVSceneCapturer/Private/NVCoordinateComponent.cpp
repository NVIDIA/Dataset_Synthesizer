/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerModule.h"
#include "NVSceneCapturerUtils.h"
#include "NVCoordinateComponent.h"
#include "Engine.h"
#include "Components/ArrowComponent.h"
#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

UNVCoordinateComponent::UNVCoordinateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ArrowThickness = 0.1f;
    AxisSize = FVector(10.f, 10.f, 10.f);
}

void UNVCoordinateComponent::PostLoad()
{
    Super::PostLoad();
    UpdateArrowSize();
}

void UNVCoordinateComponent::BeginPlay()
{
    Super::BeginPlay();

    UpdateArrowSize();
    OnVisibilityChanged();
}

void UNVCoordinateComponent::OnVisibilityChanged()
{
    Super::OnVisibilityChanged();
}

#if WITH_EDITORONLY_DATA
void UNVCoordinateComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        const FName ChangedPropName = PropertyThatChanged->GetFName();
        if ((ChangedPropName == GET_MEMBER_NAME_CHECKED(UNVCoordinateComponent, ArrowThickness)) ||
                (ChangedPropName == GET_MEMBER_NAME_CHECKED(UNVCoordinateComponent, AxisSize)))
        {
            UpdateArrowSize();
        }
        Super::PostEditChangeProperty(PropertyChangedEvent);
    }
}
#endif // WITH_EDITORONLY_DATA

void UNVCoordinateComponent::SetSize(const FVector& NewAxisSize)
{
    AxisSize = NewAxisSize;
    UpdateArrowSize();
}

void UNVCoordinateComponent::UpdateArrowSize()
{
    // ToDo: implementation.
}