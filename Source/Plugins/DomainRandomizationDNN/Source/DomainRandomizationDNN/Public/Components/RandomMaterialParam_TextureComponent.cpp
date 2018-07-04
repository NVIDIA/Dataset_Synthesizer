/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "DRUtils.h"
#include "RandomMaterialParam_TextureComponent.h"

// Sets default values
URandomMaterialParam_TextureComponent::URandomMaterialParam_TextureComponent()
{
    TextureParameterName = TEXT("Texture");

    bUseAllTextureInAFolder = false;

    FDirectoryPath DefaultBackgroundDir;
    DefaultBackgroundDir.Path = TEXT("Backgrounds");
    TextureDirectories.Reset();
    TextureDirectories.Add(DefaultBackgroundDir);
}

void URandomMaterialParam_TextureComponent::PostLoad()
{
    Super::PostLoad();

    // Maintain old data which still use the deprecated property TextureParameterName
    if ((MaterialParameterNames.Num() == 0) && !TextureParameterName.IsNone())
    {
        MaterialParameterNames.Add(TextureParameterName);
    }
}

void URandomMaterialParam_TextureComponent::BeginPlay()
{
    if (bUseAllTextureInAFolder)
    {
        TextureStreamer.Init(TextureDirectories, UTexture2D::StaticClass());
    }

    Super::BeginPlay();
}

void URandomMaterialParam_TextureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool URandomMaterialParam_TextureComponent::HasAssetToRandomize() const
{
    return bUseAllTextureInAFolder?
        TextureStreamer.HasAssets() :
        (TextureList.Num() > 0);
}

void URandomMaterialParam_TextureComponent::UpdateMaterial(UMaterialInstanceDynamic* MaterialToMofidy)
{
    if (MaterialToMofidy && HasAssetToRandomize())
    {
        for (const FName& ParamName : MaterialParameterNames)
        {
            UTexture* OldTextureParamValue = nullptr;
            // Only need to request random texture and apply it if the material actually have the desired parameter
            bool bHaveTextureParam = MaterialToMofidy->GetTextureParameterValue(ParamName, OldTextureParamValue);
            if (bHaveTextureParam)
            {
                UTexture* RandomTexture = nullptr;
                if (bUseAllTextureInAFolder)
                {
                    RandomTexture = TextureStreamer.GetNextAsset<UTexture2D>();
                }
                else
                {
                    // TODO: Add option to use the same texture for all the parameters or not
                    RandomTexture = TextureList[FMath::Rand() % TextureList.Num()];
                }

                if (RandomTexture)
                {
                    MaterialToMofidy->SetTextureParameterValue(ParamName, RandomTexture);
                }
            }
        }
    }
}