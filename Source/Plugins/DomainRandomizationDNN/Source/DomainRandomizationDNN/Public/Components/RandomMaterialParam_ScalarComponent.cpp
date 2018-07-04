/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "RandomMaterialParam_ScalarComponent.h"

// Sets default values
URandomMaterialParam_ScalarComponent::URandomMaterialParam_ScalarComponent()
{
    ValueRange.Min = 0.f;
    ValueRange.Max = 1.f;
}

void URandomMaterialParam_ScalarComponent::UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy)
{
    if (MaterialToMofidy)
    {
        for (const FName& ParamName : MaterialParameterNames)
        {
            // TODO: Add option to use the same value for all the parameters or not
            float RandValue = FMath::RandRange(ValueRange.Min, ValueRange.Max);
            MaterialToMofidy->SetScalarParameterValue(ParamName, RandValue);
        }
    }
}
