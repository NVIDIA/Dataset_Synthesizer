/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "NVSceneCapturerUtils.h"
#include "Components/PrimitiveComponent.h"
#include "NVCoordinateComponent.generated.h"

/**
 * The new actor which get annotated and have its info captured and exported
 */
/// @cond DOXYGEN_SUPPRESSED_CODE
UCLASS(Blueprintable, ClassGroup = (NVIDIA), Config = Engine, HideCategories = (Replication, Tick, Tags, Input))
/// @endcond DOXYGEN_SUPPRESSED_CODE
class NVSCENECAPTURER_API UNVCoordinateComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UNVCoordinateComponent(const FObjectInitializer& ObjectInitializer);

    void SetSize(const FVector& NewAxisSize);

protected:
    virtual void PostLoad() override;
    virtual void BeginPlay() override;
    virtual void OnVisibilityChanged() override;
#if WITH_EDITORONLY_DATA
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif // WITH_EDITORONLY_DATA

    void UpdateArrowSize();

public: // Editor properties
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ArrowThickness;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector AxisSize;
};
