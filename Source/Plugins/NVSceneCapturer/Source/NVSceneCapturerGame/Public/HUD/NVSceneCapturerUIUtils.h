/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#define DEFINE_UI_WIDGET(Type, Name) Type* Name = Cast<Type>(GetWidgetFromName(TEXT(#Name)));
#define FIND_UI_WIDGET(Type, Name) Name = Cast<Type>(GetWidgetFromName(TEXT(#Name)));

namespace NVSceneCapturerUIUtils
{
    extern void DrawLine(FPaintContext& Context, const FVector2D& StartLoc, const FVector2D& EndLoc, const FLinearColor& LineColor, float LineThickness = 0.f);
    extern void DrawRect(FPaintContext& Context, const FVector2D& TopLeftLoc, const FVector2D& RectSize, const FSlateBrush& RectBrush, const FLinearColor& RectColor);
    extern void DrawBoxAroundPoint(FPaintContext& Context, const FVector2D& CenterPoint, float Radius, const FSlateBrush& RectBrush, const FLinearColor& RectColor);
    extern FString ConvertTimeSecondsToString(float TimeSeconds);
};
