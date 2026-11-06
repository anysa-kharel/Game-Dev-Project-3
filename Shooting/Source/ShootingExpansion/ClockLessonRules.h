#pragma once
#include <algorithm>
#include <cmath>

// Engine-independent layout and progression rules, shared by generation and tests.
namespace ClockLessons
{
    inline constexpr double Gap=240,FarDockEnd=2500,OldDockCenter=1750,OldDockLength=1500;
    inline constexpr double PracticeX=-2250,PracticeTop=65,PracticeWidth=100;
    inline constexpr double BeamThickness=110,BeamHeight=40,DefaultClearance=150;
    inline bool Crouch(int Stage) { return Stage==2 || Stage==4; }
    inline bool Jump(int Stage) { return Stage==3 || Stage==4; }
    inline int Stage(int Section) { return Section>=1 && Section<=4 ? Section : 0; }
    struct Dock { double Start,End,Center,Length; };
    inline Dock Landing(double HandLength)
    {
        const double Start=HandLength+Gap;
        return {Start,FarDockEnd,(Start+FarDockEnd)*0.5,FarDockEnd-Start};
    }
    inline double Advance(double Age,double Delta,double Period,bool Started,int Lesson)
    {
        if (!Started) return Age;
        const double Next=Age+std::max(0.0,Delta);
        // A stable exit gives learners time to stand, position themselves, and jump.
        return Jump(Lesson) ? std::min(Next,std::max(12.0,Period)*0.5) : Next;
    }
    inline double Clearance(double Standing,double Crouched)
    {
        if (Standing<=0) return DefaultClearance;
        if (Standing<=45) return std::max(1.0,Standing*0.8);
        const double Low=Crouched>0 && Crouched<Standing ? Crouched : Standing*0.60;
        return (Standing+std::clamp(Low,Standing*0.20,Standing-1.0))*0.5;
    }
    inline bool ClearOfBeam(double X,double Y,double Radius)
    { return X>BeamThickness*0.5+Radius+24 || Y>250+Radius; }
    inline bool AtApproach(double X) { return X>-2800 && X<-1850; }
    inline bool OriginalDockPiece(double X,double Y,double Z,double Length)
    {
        return std::abs(X-OldDockCenter)<1 && std::abs(Y)<=600 && Z>=-36 && Z<=205 &&
            std::abs(Length-OldDockLength)<1;
    }
    enum class Cue { PracticeCrouch,PracticeJump,Board,Ride,Crouch,Stand,JumpExit };
    inline Cue Hint(int Lesson,double X,bool Started,bool Riding,bool PassedBeam,bool FarDock)
    {
        if (AtApproach(X) && Lesson==2) return Cue::PracticeCrouch;
        if (AtApproach(X) && Lesson==3) return Cue::PracticeJump;
        if (!Started || X<-1000) return Cue::Board;
        if (Crouch(Lesson) && Riding && !PassedBeam) return Cue::Crouch;
        if (Jump(Lesson) && FarDock) return Cue::JumpExit;
        if (Crouch(Lesson) && PassedBeam) return Cue::Stand;
        return Cue::Ride;
    }
}
