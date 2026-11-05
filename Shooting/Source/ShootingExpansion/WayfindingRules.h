#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Shared by the game, its HUD, and the standalone regression checks.
namespace Wayfinding
{
    inline constexpr float AmbientLift=0.35f;
    inline constexpr float SunIntensity=4.5f;
    inline constexpr float SkyIntensity=1.6f;
    inline constexpr float FlagHeight=700.0f;
    enum class Stage { ClearYard, FlagOne, BoardClock, FlagTwo, ClearFoundry, ReturnToYard, Exit };
    inline Stage NextStage(int Checkpoint,int YardLeft,int FoundryLeft,double X)
    {
        // Crossing early never hides the requirement to finish the yard.
        if (YardLeft>0) return X>3250 ? Stage::ReturnToYard : Stage::ClearYard;
        if (Checkpoint<1) return Stage::FlagOne;
        if (Checkpoint<2 || X<5200) return X<3250 ? Stage::BoardClock : Stage::FlagTwo;
        return FoundryLeft>0 ? Stage::ClearFoundry : Stage::Exit;
    }
    enum class FlagState { Future, Next, Saved, Locked, Open };
    inline FlagState StateFor(int Index,int SavedCheckpoint,int EnemiesLeft,bool IsExit)
    {
        if (IsExit) return EnemiesLeft>0 ? FlagState::Locked : FlagState::Open;
        if (Index<=SavedCheckpoint) return FlagState::Saved;
        return Index==SavedCheckpoint+1 ? FlagState::Next : FlagState::Future;
    }
    inline double BoardingOffset(double HandLength,bool Far,bool Docked)
    {
        // Wait on solid dock floor. When aligned, step past the floor edge onto the hand.
        return (Far ? 1 : -1)*(HandLength-(Docked ? 280 : 60));
    }
    struct Direction { double X,Y; bool TurnBack; };
    inline Direction Bearing(double Forward,double Right)
    {
        const double Length=std::hypot(Forward,Right);
        if (Length<0.001) return {0,-1,false};
        return {Right/Length,-Forward/Length,Forward<0};
    }
    struct Arrow { double X,Y,Yaw; };
    // Solid ground only; there is deliberately no breadcrumb trail over the gap.
    inline constexpr std::array<Arrow,18> Arrows={{
        {-1900,0,0},{-1200,0,0},{-600,0,0},{0,0,0},
        {650,-80,-15},{1100,-300,-10},{1480,-430,20},{2020,-160,15},
        {2400,0,0},{2980,0,0},{5500,0,0},{6040,0,0},
        {6640,0,0},{7150,-40,-50},{7500,-650,-10},{8140,-750,35},
        {8780,0,0},{9280,0,0}
    }};
}
