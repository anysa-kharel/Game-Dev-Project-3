#pragma once
#include "CoreMinimal.h"
#include "CAInterfaceRules.h"

// Shared presentation palette. Enemy role colors remain in CAUtil / CAEnemy.
namespace CATheme
{
    inline FLinearColor Color(CAInterface::RGB Value,float Alpha=1)
    { return FLinearColor(Value.R,Value.G,Value.B,Alpha); }
    inline FLinearColor Ink(float Alpha=1) { return Color(CAInterface::Ink,Alpha); }
    inline FLinearColor PanelFill(float Alpha=0.97f) { return Color(CAInterface::Card,Alpha); }
    inline FLinearColor Gold(float Alpha=1) { return Color(CAInterface::Gold,Alpha); }
    inline FLinearColor Highlight(float Alpha=1) { return Color(CAInterface::BrightGold,Alpha); }
    inline FLinearColor White(float Alpha=1) { return Color(CAInterface::White,Alpha); }
    inline FLinearColor Muted(float Alpha=1) { return Color(CAInterface::Muted,Alpha); }
    inline FLinearColor Bronze(float Alpha=1) { return Color(CAInterface::Bronze,Alpha); }
    inline FLinearColor Edge() { return Color(CAInterface::Edge); }
    inline FLinearColor KeyFace() { return Color(CAInterface::KeyFace); }
}
