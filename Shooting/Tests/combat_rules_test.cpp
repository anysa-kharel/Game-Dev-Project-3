#include "../Source/ShootingExpansion/CombatRules.h"
#include "../Source/ShootingExpansion/ArenaLayout.h"
#include <cstdlib>
#include <iostream>
#include <string>

int Count=0;
void Check(bool Value,const std::string& Description)
{
    ++Count;
    if (!Value) { std::cerr<<"FAILED: "<<Description<<'\n'; std::exit(1); }
}
bool Near(double A,double B) { return std::abs(A-B)<0.000001; }
int main()
{
    using namespace CombatRules;
    Check(Near(ClockAngle(0),180) && Near(ClockAngle(3.99),180),"four-second near boarding window");
    Check(Near(ClockAngle(9),270),"hand carries player around halfway");
    Check(Near(ClockAngle(14),0) && Near(ClockAngle(17.99),0),"four-second far disembarking window");
    Check(Near(ClockAngle(23),90),"return route uses the other half of the clock");
    Check(Near(ClockAngle(28),180),"full clock cycle returns to boarding dock");
    Check(Near(ClockAngle(10,20),0),"custom period scales the entire clock cycle");
    Check(AtDock(359,true) && !AtDock(350,true),"far docking cue wraps around zero");
    Check(AtDock(180,false) && !AtDock(160,false),"near docking cue only at its dock");
    double Previous=ClockAngle(0);
    for (int I=1;I<=1400;++I)
    {
        const double A=ClockAngle(I*0.02);
        const double Difference=Wrap(A-Previous+180,360)-180;
        Check(Difference>=-0.000001 && Difference<=0.360001,"clock has no positional jump between phases");
        Previous=A;
    }

    Health H;
    Check(H.Alive() && Near(H.Value,100),"player begins with full health");
    Check(H.Damage(50) && Near(H.Value,50),"damage lowers health");
    H.Step(5.5); Check(Near(H.Value,50),"healing does not start before six quiet seconds");
    H.Step(1); Check(Near(H.Value,56),"crossing heal threshold counts only time after the threshold");
    H.Step(100); Check(Near(H.Value,100),"healing cannot exceed maximum");
    H.Damage(15); H.Step(5.9); Check(Near(H.Value,85),"another hit resets healing delay");
    H.Damage(1000); H.Step(100);
    Check(!H.Alive() && Near(H.Value,0),"death clamps to zero and cannot self-heal");
    Check(!H.Damage(10),"dead player cannot take damage repeatedly");
    H.Reset(); Check(H.Alive() && Near(H.Value,100) && Near(H.Quiet,0),"checkpoint resets health and delay");
    Check(!H.Damage(0) && !H.Damage(-10),"nonpositive damage cannot heal or reset quiet time");

    Attack A;
    A.Reset(0);
    Check(!A.Step(0.01,true,true,false) && A.Charging,"guard visibly charges before firing");
    Check(!A.Step(0.4,true,true,false),"partial charge cannot shoot");
    Check(!A.Step(0.5,false,true,false) && !A.Charging,"cover cancels a pending shot");
    Check(!A.Step(0.01,true,true,false) && A.Charging,"reacquiring player begins a fresh charge");
    Check(A.Step(0.8,true,true,false) && !A.Charging,"clear charged shot fires once");
    Check(!A.Step(0.5,true,true,false),"attack cooldown blocks immediate repeated shots");
    A.Reset(0); A.Step(0,true,true,false);
    Check(!A.Step(1,true,false,false),"out-of-range target cancels shot even with sight");
    A.Reset(0); A.Step(0,true,true,true);
    Check(A.Step(0.45,true,true,true),"scout attack has its own warning interval");
    A.Reset(0); A.Step(0,true,true,true);
    Check(!A.Step(0.5,false,true,true),"scout cannot finish attack through a wall");

    int Sections[2]={0,0};
    for (const auto& S : ArenaLayout::Enemies)
    {
        Check(S.Section>=1 && S.Section<=2,"valid enemy area");
        ++Sections[S.Section-1];
        for (const auto& C : ArenaLayout::Cover)
        {
            const bool XY=std::abs(S.X-C.X)<C.L*0.5+43 && std::abs(S.Y-C.Y)<C.W*0.5+43;
            const bool Z=S.Z+88>C.Z-C.H*0.5 && S.Z-88<C.Z+C.H*0.5;
            Check(!(XY && Z),"enemy spawn capsule does not intersect configured cover");
        }
    }
    Check(Sections[0]==6 && Sections[1]==6,"six robots in each combat area");
    Check(4200-1200<3200-43,"near dock overlaps the clock hand by more than capsule radius");
    Check(4200+1200>5200+43,"far dock overlaps the clock hand by more than capsule radius");
    Check(260>2*43+100,"hand width allows standing clearance");
    Check(5200-3200>2*650.0*600/980,"clock gap exceeds an ordinary sprint jump even with generous jump estimate");
    Check(std::atan2(240.0,1800)*180/3.14159265<40,"gallery ramps below standard walkable slope");
    std::cout<<Count<<" combat rule and layout checks passed.\n";
}
