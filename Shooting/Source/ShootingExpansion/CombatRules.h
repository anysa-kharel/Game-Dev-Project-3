#pragma once
#include <algorithm>
#include <cmath>

namespace CombatRules
{
    inline double Wrap(double Value,double Period)
    {
        const double R=std::fmod(Value,Period);
        return R<0 ? R+Period : R;
    }
    // Four-second boarding windows, ten seconds between the two docks.
    inline double ClockAngle(double Seconds,double Period=28)
    {
        const double T=Wrap(Seconds*28/std::max(12.0,Period),28);
        if (T<4) return 180;
        if (T<14) return 180+(T-4)*18;
        if (T<18) return 0;
        return (T-18)*18;
    }
    inline bool AtDock(double Angle,bool Far)
    {
        const double Difference=std::abs(Wrap(Angle-(Far ? 0 : 180)+180,360)-180);
        return Difference<8;
    }
    struct Health
    {
        double Value=100;
        double Quiet=0;
        bool Alive() const { return Value>0; }
        bool Damage(double Amount)
        {
            if (!Alive() || Amount<=0) return false;
            Value=std::max(0.0,Value-Amount); Quiet=0;
            return true;
        }
        void Step(double Delta)
        {
            if (!Alive() || Delta<=0) return;
            const double Before=Quiet;
            Quiet+=Delta;
            const double HealingTime=std::max(0.0,Quiet-6)-std::max(0.0,Before-6);
            Value=std::min(100.0,Value+HealingTime*12);
        }
        void Reset() { Value=100; Quiet=0; }
    };
    struct Attack
    {
        double Cooldown=1.5;
        double Charge=0;
        bool Charging=false;
        // Losing sight cancels a charge. A shot never completes through cover.
        bool Step(double Delta,bool HasSight,bool InRange,bool Melee)
        {
            Delta=std::max(0.0,Delta);
            Cooldown=std::max(0.0,Cooldown-Delta);
            if (!HasSight || !InRange)
            {
                Charging=false; Charge=0;
                return false;
            }
            if (Cooldown>0) return false;
            if (!Charging) { Charging=true; Charge=Melee ? 0.45 : 0.8; return false; }
            Charge-=Delta;
            if (Charge>0) return false;
            Charging=false;
            Cooldown=Melee ? 1.3 : 1.7;
            return true;
        }
        void Reset(double Initial=1.5) { Cooldown=Initial; Charge=0; Charging=false; }
    };
}
