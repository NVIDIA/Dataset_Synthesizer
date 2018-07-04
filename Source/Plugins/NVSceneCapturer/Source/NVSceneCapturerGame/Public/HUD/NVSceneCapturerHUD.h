/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NVSceneCapturerUtils.h"
#include "NVSceneCapturerHUD.generated.h"

class UCanvasRenderTarget2D;
class UFont;
class UNVSceneCapturerHUD_Overlay;

/**
 *
 */
UCLASS(Blueprintable)
class NVSCENECAPTURERGAME_API ANVSceneCapturerHUD : public AHUD
{
    GENERATED_BODY()

public:
    ANVSceneCapturerHUD(const FObjectInitializer& ObjectInitializer);

	UNVSceneCapturerHUD_Overlay* GetHUDOverlay() const
	{
		return HUDOverlay;
	}
	void ToggleOverlay();

	bool GetShowExportActorDebug() const
	{
		return bShowExportActorDebug;
	}
	void SetShowExportActorDebug(bool bShowDebug);
	void ToggleShowExportActorDebug();

public: // Override
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

protected: // Editor properties:
    // The HUD widget class we want to use for the HUD overlay
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD")
    TSubclassOf<UNVSceneCapturerHUD_Overlay> HUDOverlayClass;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    UFont* DebugFont;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    FLinearColor DebugLineColor;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    FLinearColor DebugTextColor;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    FLinearColor DebugTextBackgroundColor;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    float DebugLineThickness;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    float DebugVertexRadius;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    float DebugLineThickness3d;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    float DebugVertexRadius3d;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    FLinearColor DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::CuboidVertexType_MAX];

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    bool bDrawDebugCuboidIn3d;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    float CuboidDirectionDebugLength;

    UPROPERTY(EditDefaultsOnly, Category = Debug)
    FRotator CuboidDirectionRotation_HACK;

    UPROPERTY(EditAnywhere, Category = Debug)
    bool bShowExportActorDebug;

    UPROPERTY(EditAnywhere, Category = Debug)
    bool bShowDebugCapturerPath;

    UPROPERTY(EditDefaultsOnly, Category = Debug, meta = (EditCondition = "bShowDebugCapturerPath"))
    FLinearColor DebugCapturerPathColor;

    UPROPERTY(EditDefaultsOnly, Category = Debug, meta = (EditCondition = "bShowDebugCapturerPath"))
    float DebugCapturerPathLifeTime;

    UPROPERTY(EditDefaultsOnly, Category = Debug, meta = (EditCondition = "bShowDebugCapturerPath"))
    float DebugCapturerDirectionLength;

protected:
    void DrawDebugInfo();
    void DrawDebugInfo_ExportedActor(const AActor* TargetActor);
    void DrawDebugInfo_HumanPose(const AActor* TargetActor);

    void DrawLineBetweenSocket(const class USkeletalMeshComponent* SkeletalComp, const FName& SocketName1, const FName& SocketName2);
    void DrawCuboid3d(const FNVCuboidData& Cuboid, const FLinearColor& LineColor);
    void DrawCuboid2d(const FNVCuboidData& Cuboid, const FLinearColor& LineColor, float LineThickness = 1.f);
    void DrawProjectedLine(const FVector& Begin, const FVector& End, const FLinearColor& LineColor, float LineWidth = 1.f);
    void DrawDebugPoint2d(const FVector2D& ScreenLoc, const FLinearColor& PointColor = FLinearColor::Black, float Radius = 1.f);

protected: // Transient properties
    UPROPERTY(Transient)
    UNVSceneCapturerHUD_Overlay* HUDOverlay;

    UPROPERTY(Transient)
    FVector LastCapturerLocation;
};
