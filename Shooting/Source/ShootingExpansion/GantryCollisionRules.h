#pragma once
#include "CombatRules.h"
#include <algorithm>
#include <cmath>
#include <limits>

// Conservative capsule bounds swept along the actual clock arc. This also
// protects based movement paths that do not sweep a character against its base.
namespace GantryCollision
{
    struct Point { double X,Y,Z; };
    struct Capsule { Point Center;double Radius,HalfHeight; };
    struct Beam { Point Center,HalfSize; };
    struct Result { double Age;bool Blocked; };
    inline constexpr double Skin=3;
    inline Capsule ForCrouchState(Capsule Actual,double StandingHalfHeight,bool Crouching)
    {
        if (!Crouching && StandingHalfHeight>Actual.HalfHeight)
        {
            // Preserve the feet while guarding an uncrouch animation that has
            // not finished growing the real capsule yet.
            Actual.Center.Z+=StandingHalfHeight-Actual.HalfHeight;
            Actual.HalfHeight=StandingHalfHeight;
        }
        return Actual;
    }
    inline Point Rotate(Point P,double Degrees)
    {
        const double R=Degrees*3.14159265358979323846/180;
        return {P.X*std::cos(R)-P.Y*std::sin(R),P.X*std::sin(R)+P.Y*std::cos(R),P.Z};
    }
    inline double Entry(Point From,Point To,const Capsule& Body,const Beam& Obstacle)
    {
        const double A[]={From.X,From.Y,From.Z},B[]={To.X,To.Y,To.Z};
        const double C[]={Obstacle.Center.X,Obstacle.Center.Y,Obstacle.Center.Z};
        const double E[]={Obstacle.HalfSize.X+Body.Radius+Skin,
            Obstacle.HalfSize.Y+Body.Radius+Skin,Obstacle.HalfSize.Z+Body.HalfHeight+Skin};
        double First=0,Last=1;
        for (int Axis=0;Axis<3;++Axis)
        {
            const double Delta=B[Axis]-A[Axis],Low=C[Axis]-E[Axis],High=C[Axis]+E[Axis];
            if (std::abs(Delta)<1e-10)
            {
                if (A[Axis]<Low || A[Axis]>High) return std::numeric_limits<double>::infinity();
                continue;
            }
            const double T1=(Low-A[Axis])/Delta,T2=(High-A[Axis])/Delta;
            First=std::max(First,std::min(T1,T2));Last=std::min(Last,std::max(T1,T2));
            if (First>Last) return std::numeric_limits<double>::infinity();
        }
        return First;
    }
    inline Result Constrain(double Age,double ProposedAge,double Period,const Capsule& Body,const Beam& Obstacle)
    {
        if (ProposedAge<=Age) return {Age,false};
        const double Cycle=std::max(12.0,Period);
        // Bound extreme hitches to one revolution; at most 2 degrees per sample.
        const double End=std::min(ProposedAge,Age+Cycle);
        const int Steps=std::max(1,static_cast<int>(std::ceil((End-Age)/(Cycle/252))));
        const double StartAngle=CombatRules::ClockAngle(Age,Period);
        double PreviousAge=Age;Point Previous=Body.Center;
        for (int Step=1;Step<=Steps;++Step)
        {
            const double SampleAge=Age+(End-Age)*Step/Steps;
            const Point Next=Rotate(Body.Center,CombatRules::ClockAngle(SampleAge,Period)-StartAngle);
            const double Contact=Entry(Previous,Next,Body,Obstacle);
            if (Contact<=1)
            {
                const double Interval=SampleAge-PreviousAge;
                const double Safe=std::max(0.0,Contact-0.002/Interval);
                return {PreviousAge+Interval*Safe,true};
            }
            PreviousAge=SampleAge;Previous=Next;
        }
        return {End,false};
    }
}
