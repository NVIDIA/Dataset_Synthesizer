/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "NVSceneCapturerUIUtils.h"
#include "EngineMinimal.h"
#include "Rendering/DrawElements.h"

namespace NVSceneCapturerUIUtils
{
    void DrawLine(FPaintContext& Context, const FVector2D& StartLoc, const FVector2D& EndLoc, const FLinearColor& LineColor, float LineThickness /*= 0.f*/)
    {
        //if (bIncreaseMaxLayer)
        //  Context.MaxLayer++;

        const bool bAntiAlias = true;
        TArray<FVector2D> Points;
        Points.Add(StartLoc);
        Points.Add(EndLoc);

        //FSlateDrawElement::MakeLines(
        //  Context.OutDrawElements,
        //  Context.MaxLayer,
        //  Context.AllottedGeometry.ToPaintGeometry(),
        //  Points,
        //  Context.MyCullingRect,
        //  ESlateDrawEffect::None,
        //  LineColor,
        //  bAntiAlias,
        //  LineThickness);
    }

    void DrawRect(FPaintContext& Context, const FVector2D& TopLeftLoc, const FVector2D& RectSize, const FSlateBrush& RectBrush, const FLinearColor& RectColor)
    {
        //if (bIncreaseMaxLayer)
        //  Context.MaxLayer++;

        const float RectScale = 1.f;
        FPaintGeometry PaintGeometry = Context.AllottedGeometry.ToPaintGeometry(RectSize,
                                       FSlateLayoutTransform(RectScale, TransformPoint(RectScale, TopLeftLoc)));

        //FSlateDrawElement::MakeBox(
        //  Context.OutDrawElements,
        //  Context.MaxLayer,
        //  PaintGeometry,
        //  &RectBrush,
        //  Context.MyCullingRect,
        //  ESlateDrawEffect::None,
        //  RectColor);
    }

    void DrawBoxAroundPoint(FPaintContext& Context, const FVector2D& CenterPoint, float Radius, const FSlateBrush& RectBrush, const FLinearColor& RectColor)
    {
        const FVector2D RectTopLeft(CenterPoint.X - Radius, CenterPoint.Y - Radius);
        const FVector2D RectSize(2.f * Radius, 2.f * Radius);

        DrawRect(Context, RectTopLeft, RectSize, RectBrush, RectColor);
    }

    FString ConvertTimeSecondsToString(float TimeSeconds)
    {
        FString TimeStr = TEXT("...");
        if (TimeSeconds > 0.f)
        {
            //RemainTimeStr = UKismetStringLibrary::TimeSecondsToString(EstRemainTime);
            int32 MinuteCount = FMath::FloorToInt(TimeSeconds / 60.f);
            int32 HourCount = MinuteCount / 60;
            int32 DayCount = HourCount / 24.f;

            float SecondsCount = TimeSeconds - MinuteCount * 60.f;
            if (HourCount > 0)
            {
                MinuteCount -= HourCount * 60;
            }
            if (DayCount > 0)
            {
                HourCount -= DayCount * 24;
            }

            TimeStr = TEXT("");
            if (DayCount > 0)
            {
                TimeStr += FString::Printf(TEXT("%dd - "), DayCount);
            }
            if (HourCount > 0)
            {
                TimeStr += FString::Printf(TEXT("%dh - "), HourCount);
            }
            if (MinuteCount > 0)
            {
                TimeStr += FString::Printf(TEXT("%dm - "), MinuteCount);
            }
            TimeStr += FString::Printf(TEXT("%.1fs"), SecondsCount);
        }
        return TimeStr;
    }

}