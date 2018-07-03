/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerHUD.h"
#include "NVSceneCapturerGameModeBase.h"
#include "NVSceneCapturerPlayerController.h"
#include "HUD/NVSceneCapturerHUD_Overlay.h"
#include "EngineMinimal.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/HUD.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "DrawDebugHelpers.h"
#include "NVSceneCapturerActor.h"
#include "NVSceneCapturerUtils.h"

ANVSceneCapturerHUD::ANVSceneCapturerHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DebugLineColor = FLinearColor::Red;
    DebugTextColor = FLinearColor::Black;
    DebugTextBackgroundColor = FLinearColor::White;
    DebugLineThickness = 2.f;
    DebugVertexRadius = 5.f;
    DebugLineThickness3d = 5.f;
    DebugVertexRadius3d = 10.f;
    bShowExportActorDebug = false;
    bDrawDebugCuboidIn3d = false;

    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::FrontTopLeft] = FLinearColor::Red;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::FrontTopRight] = FLinearColor::Red;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::FrontBottomLeft] = FLinearColor::Red;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::FrontBottomRight] = FLinearColor::Red;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::RearTopLeft] = FLinearColor::Blue;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::RearTopRight] = FLinearColor::Blue;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::RearBottomLeft] = FLinearColor::Blue;
    DebugCuboidVertexColor[(uint8)ENVCuboidVertexType::RearBottomRight] = FLinearColor::Blue;

    bShowDebugCapturerPath = false;
    DebugCapturerPathLifeTime = 2.f;
    DebugCapturerDirectionLength = 20.f;
    DebugCapturerPathColor = FLinearColor::Green;
}

void ANVSceneCapturerHUD::BeginPlay()
{
    Super::BeginPlay();

    if (HUDOverlayClass && !HUDOverlay)
    {
        UWorld* World = GetWorld();

        // Assign the outer to the game instance if it exists, otherwise use the world
        HUDOverlay = CreateWidget<UNVSceneCapturerHUD_Overlay>(World, HUDOverlayClass);
        if (HUDOverlay)
        {
            if (PlayerOwner)
            {
                HUDOverlay->SetPlayerContext(FLocalPlayerContext(PlayerOwner));
            }
            HUDOverlay->Initialize();
            HUDOverlay->AddToViewport();
        }
    }

    if (!DebugFont)
    {
        DebugFont = GEngine->GetMediumFont();
    }
}

void ANVSceneCapturerHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HUDOverlay)
    {
        HUDOverlay->RemoveFromParent();
        HUDOverlay = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ANVSceneCapturerHUD::DrawHUD()
{
    Super::DrawHUD();

    ANVSceneCapturerPlayerController* PC = Cast<ANVSceneCapturerPlayerController>(PlayerOwner);
    if (PC)
    {
        if (HUDOverlay)
        {
            HUDOverlay->Update();
        }
        DrawDebugInfo();
    }
}

void ANVSceneCapturerHUD::ToggleOverlay()
{
    if (HUDOverlay)
    {
        const ESlateVisibility CurrentOverlayVisibility = HUDOverlay->GetVisibility();
        const ESlateVisibility NewOverlayVisibility = (CurrentOverlayVisibility == ESlateVisibility::Hidden) ?
                ESlateVisibility::Visible : ESlateVisibility::Hidden;
        HUDOverlay->SetVisibility(NewOverlayVisibility);
    }
}

void ANVSceneCapturerHUD::SetShowExportActorDebug(bool bShowDebug)
{
    bShowExportActorDebug = bShowDebug;
}

void ANVSceneCapturerHUD::ToggleShowExportActorDebug()
{
    bShowExportActorDebug = !bShowExportActorDebug;
}

void ANVSceneCapturerHUD::DrawDebugInfo()
{
    // FIXME: Should let each of the training actors request the debug canvas
    UWorld* World = GetWorld();
    if (World)
    {
        for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
        {
            AActor* CheckActor = *ActorIt;
            if (!CheckActor)
            {
                continue;
            }

            if (bShowExportActorDebug)
            {
                DrawDebugInfo_ExportedActor(CheckActor);
            }
        }

        if (bShowDebugCapturerPath)
        {
            ANVSceneCapturerGameModeBase* GameMode = Cast<ANVSceneCapturerGameModeBase>(World->GetAuthGameMode());
            if (GameMode)
            {
                const ANVSceneCapturerActor* TargetCapturer = nullptr;
                const TArray<ANVSceneCapturerActor*>& CapturerList = GameMode->GetSceneCapturerList();
                for (const ANVSceneCapturerActor* CheckCapturer : CapturerList)
                {
                    if (CheckCapturer)
                    {
                        TargetCapturer = CheckCapturer;
                        break;
                    }
                }

                if (TargetCapturer)
                {
                    const FVector& CurrentCapturerLocation = TargetCapturer->GetActorLocation();
                    const FVector& CurrentCapturerDirection = TargetCapturer->GetActorRotation().Vector();

                    DrawDebugLine(World, LastCapturerLocation, CurrentCapturerLocation, DebugCapturerPathColor.ToFColor(true), true, DebugCapturerPathLifeTime, 0, DebugLineThickness3d);
                    DrawDebugLine(World, CurrentCapturerLocation, CurrentCapturerLocation + CurrentCapturerDirection * DebugCapturerDirectionLength,
                                  DebugCapturerPathColor.ToFColor(true), true, DebugCapturerPathLifeTime, 0, DebugLineThickness3d);
                    //World->LineBatcher->DrawLine(LastCapturerLocation, CurrentCapturerLocation, DebugCapturerPathColor, SDPG_World, DebugLineThickness3d, DebugCapturerPathLifeTime);
                    //World->LineBatcher->DrawLine(CurrentCapturerLocation, CurrentCapturerLocation + CurrentCapturerDirection * DebugCapturerDirectionLength, DebugCapturerPathColor, SDPG_World, DebugLineThickness3d, DebugCapturerPathLifeTime);

                    LastCapturerLocation = CurrentCapturerLocation;
                }
            }
        }
    }
}

void ANVSceneCapturerHUD::DrawDebugInfo_ExportedActor(const AActor* TargetActor)
{
    UNVCapturableActorTag* Tag = TargetActor ? Cast<UNVCapturableActorTag>(TargetActor->GetComponentByClass(UNVCapturableActorTag::StaticClass())) : nullptr;
    if (Tag)
    {
        const FNVCuboidData ActorCuboid = NVSceneCapturerUtils::GetActorCuboid_OOBB_Simple(TargetActor);

        if (bDrawDebugCuboidIn3d)
        {
            DrawCuboid3d(ActorCuboid, DebugLineColor);
        }
        else
        {
            DrawCuboid2d(ActorCuboid, DebugLineColor, DebugLineThickness);
        }

        // TODO: Draw the 2d bounding box
        UWorld* World = GetWorld();
        ensure(World);
        if (World)
        {
            const FString WorldName = World->GetName();

            // Show sockets
            if (Tag->bExportAllMeshSocketInfo)
            {
                UMeshComponent* TargetMesh = NVSceneCapturerUtils::GetFirstValidMeshComponent(TargetActor);
                if (TargetMesh)
                {
                    TArray<FName> MeshSockets = TargetMesh->GetAllSocketNames();
                    for (const FName& SocketName : MeshSockets)
                    {
                        const FVector& Socket_WorldLoc = TargetMesh->GetSocketLocation(SocketName);
                        const FVector& Socket_ScreenLoc = Project(Socket_WorldLoc);
                        DrawDebugPoint2d(FVector2D(Socket_ScreenLoc), DebugTextColor, DebugVertexRadius);
                    }
                }
            }
            else if (Tag->SocketNameToExportList.Num() > 0)
            {
                UMeshComponent* TargetMesh = NVSceneCapturerUtils::GetFirstValidMeshComponent(TargetActor);
                if (TargetMesh)
                {
                    for (const FName& SocketName : Tag->SocketNameToExportList)
                    {
                        const FVector& Socket_WorldLoc = TargetMesh->GetSocketLocation(SocketName);
                        const FVector& Socket_ScreenLoc = Project(Socket_WorldLoc);
                        DrawDebugPoint2d(FVector2D(Socket_ScreenLoc), DebugTextColor, DebugVertexRadius);
                    }
                }
            }
        }
    }
}

void ANVSceneCapturerHUD::DrawLineBetweenSocket(const class USkeletalMeshComponent* SkeletalMeshComp, const FName& SocketName1, const FName& SocketName2)
{
    const FVector& SocketLoc1 = SkeletalMeshComp->GetSocketLocation(SocketName1);
    const FVector& SocketLoc2 = SkeletalMeshComp->GetSocketLocation(SocketName2);

    const FVector& Socket_ScreenLoc1 = this->Project(SocketLoc1);
    const FVector& Socket_ScreenLoc2 = this->Project(SocketLoc2);

    // Draw a line from the current bone to the next in the sequence
    this->DrawLine(Socket_ScreenLoc1.X, Socket_ScreenLoc1.Y, Socket_ScreenLoc2.X, Socket_ScreenLoc2.Y, DebugLineColor, DebugLineThickness);
}

void ANVSceneCapturerHUD::DrawCuboid3d(const FNVCuboidData& Cuboid, const FLinearColor& LineColor)
{
#define DRAW_3D_LINE_BETWEEN_VERTEX(BeginVertex, EndVertex) \
    Draw3DLine(Cuboid.GetVertex(BeginVertex), Cuboid.GetVertex(EndVertex), LineColor.ToFColor(false));

    // Draw the front face
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopLeft, ENVCuboidVertexType::FrontTopRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopRight, ENVCuboidVertexType::FrontBottomRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomRight, ENVCuboidVertexType::FrontBottomLeft);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomLeft, ENVCuboidVertexType::FrontTopLeft);
    // Draw the rear face
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearTopLeft, ENVCuboidVertexType::RearTopRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearTopRight, ENVCuboidVertexType::RearBottomRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearBottomRight, ENVCuboidVertexType::RearBottomLeft);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearBottomLeft, ENVCuboidVertexType::RearTopLeft);
    // Connect 2 faces
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopLeft, ENVCuboidVertexType::RearTopLeft);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopRight, ENVCuboidVertexType::RearTopRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomRight, ENVCuboidVertexType::RearBottomRight);
    DRAW_3D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomLeft, ENVCuboidVertexType::RearBottomLeft);

    UWorld* World = GetWorld();
    for (int i = 0; i < FNVCuboidData::TotalVertexesCount; i++)
    {
        const FColor& VertexColor = DebugCuboidVertexColor[i].ToFColor(false);
        const FVector& VertexLoc = Cuboid.Vertexes[i];
        DrawDebugPoint(World, VertexLoc, DebugVertexRadius3d, VertexColor, false, 0.1);
    }

    const float DebugLineLifeTime = 0.05f;

    const FVector& CuboidCenter = Cuboid.GetCenter();
    const FVector& CuboidDirection = Cuboid.GetDirection().GetSafeNormal();
    const FVector& CuboidDirEndPoint = CuboidCenter + CuboidDirection * CuboidDirectionDebugLength;
    DrawDebugPoint(World, CuboidCenter, DebugVertexRadius3d, LineColor.ToFColor(false), false, DebugLineLifeTime);
    //Draw3DLine(CuboidCenter, CuboidDirEndPoint, LineColor.ToFColor(false));
    DrawDebugLine(World, CuboidCenter, CuboidDirEndPoint, LineColor.ToFColor(false), false, DebugLineLifeTime, 0, DebugLineThickness3d);
}

void ANVSceneCapturerHUD::DrawCuboid2d(const FNVCuboidData& Cuboid, const FLinearColor& LineColor, float LineThickness)
{
#define DRAW_2D_LINE_BETWEEN_VERTEX(BeginVertex, EndVertex) \
    { \
        const FVector2D& BeginVertex_ScreenLoc = FVector2D(Project(Cuboid.GetVertex(BeginVertex))); \
        const FVector2D& EndVertex_ScreenLoc = FVector2D(Project(Cuboid.GetVertex(EndVertex))); \
        DrawLine(BeginVertex_ScreenLoc.X, BeginVertex_ScreenLoc.Y, EndVertex_ScreenLoc.X, EndVertex_ScreenLoc.Y, LineColor, LineThickness); \
    }

    // Draw the front face
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopLeft, ENVCuboidVertexType::FrontTopRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopRight, ENVCuboidVertexType::FrontBottomRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomRight, ENVCuboidVertexType::FrontBottomLeft);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomLeft, ENVCuboidVertexType::FrontTopLeft);
    // Draw the rear face
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearTopLeft, ENVCuboidVertexType::RearTopRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearTopRight, ENVCuboidVertexType::RearBottomRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearBottomRight, ENVCuboidVertexType::RearBottomLeft);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::RearBottomLeft, ENVCuboidVertexType::RearTopLeft);
    // Connect 2 faces
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopLeft, ENVCuboidVertexType::RearTopLeft);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontTopRight, ENVCuboidVertexType::RearTopRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomRight, ENVCuboidVertexType::RearBottomRight);
    DRAW_2D_LINE_BETWEEN_VERTEX(ENVCuboidVertexType::FrontBottomLeft, ENVCuboidVertexType::RearBottomLeft);

    for (int i = 0; i < FNVCuboidData::TotalVertexesCount; i++)
    {
        const FLinearColor& VertexColor = DebugCuboidVertexColor[i];
        const FVector2D& Vertex_ScreenLoc = FVector2D(Project(Cuboid.Vertexes[i]));
        DrawDebugPoint2d(Vertex_ScreenLoc, VertexColor, DebugVertexRadius);
    }

    const FVector& CuboidCenter = Cuboid.GetCenter();
    DrawDebugPoint2d(FVector2D(Project(CuboidCenter)), DebugLineColor, DebugVertexRadius);
    // HACK: Since most of the model have wrong forward direction (Y instead of X), we need to rotate them here
    const FVector& CuboidDirection = CuboidDirectionRotation_HACK.RotateVector(Cuboid.GetDirection().GetSafeNormal());
    const FVector& CuboidDirEndPoint = CuboidCenter + CuboidDirection * CuboidDirectionDebugLength;
    DrawProjectedLine(CuboidCenter, CuboidDirEndPoint, DebugLineColor, DebugLineThickness);
}

void ANVSceneCapturerHUD::DrawProjectedLine(const FVector& Begin, const FVector& End, const FLinearColor& LineColor, float LineWidth)
{
    const FVector& Begin_ScreenLoc = this->Project(Begin);
    const FVector& End_ScreenLoc = this->Project(End);

    // Draw a line from the current bone to the next in the sequence
    this->DrawLine(Begin_ScreenLoc.X, Begin_ScreenLoc.Y, End_ScreenLoc.X, End_ScreenLoc.Y, DebugLineColor, DebugLineThickness);
}

void ANVSceneCapturerHUD::DrawDebugPoint2d(const FVector2D& ScreenLoc, const FLinearColor& PointColor /*= FLinearColor::Black*/, float Radius /*= 1.f*/)
{
    // Draw a circle around the debug point
    DrawRect(PointColor, ScreenLoc.X - Radius, ScreenLoc.Y - Radius, 2 * Radius, 2 * Radius);
}
