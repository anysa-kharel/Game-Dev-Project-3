#include "../Source/ShootingExpansion/GantryCollisionRules.h"
#include "../Source/ShootingExpansion/ClockLessonRules.h"
#include <cstdlib>
#include <iostream>

namespace
{
    int Checks=0;
    void Check(bool Value,const char* Why)
    { ++Checks;if (!Value) { std::cerr<<"FAIL: "<<Why<<'\n';std::exit(1); } }
    bool Close(double A,double B) { return std::abs(A-B)<1e-6; }
}
int main()
{
    using namespace GantryCollision;
    const Beam Obstacle{{0,-600,170},{55,800,20}};
    const Capsule Standing{{-300,-900,90},42,88};
    const Capsule Crouched{{-300,-900,52},42,50};
    Check(Close(Entry(Standing.Center,{300,-900,90},Standing,Obstacle),1.0/3),"standing capsule collides before entering the beam");
    Check(!std::isfinite(Entry(Crouched.Center,{300,-900,52},Crouched,Obstacle)),"actual crouched capsule fits below the beam");
    const auto Growing=ForCrouchState(Crouched,88,false);
    Check(Close(Growing.Center.Z-Growing.HalfHeight,Crouched.Center.Z-Crouched.HalfHeight),"uncrouch prediction preserves the feet");
    Check(Close(Growing.HalfHeight,88),"uncrouch intent guards the standing height immediately");
    Check(std::isfinite(Entry(Growing.Center,{300,-900,Growing.Center.Z},Growing,Obstacle)),"releasing crouch early cannot exploit a still-small animation capsule");
    Check(Close(ForCrouchState(Crouched,88,true).HalfHeight,50),"crouch intent retains the actual capsule dimensions");
    Check(std::isfinite(Entry({-300,-900,90},{300,-900,90},{{-300,-900,90},42,78},Obstacle)),
        "starting the crouch animation cannot bypass collision while still too tall");
    Check(!std::isfinite(Entry({-300,500,90},{300,500,90},Standing,Obstacle)),"passages outside the beam are free");
    Check(!std::isfinite(Entry({-300,-900,300},{300,-900,300},{{-300,-900,300},42,80},Obstacle)),"a capsule entirely above the beam is not inside it");
    Check(!std::isfinite(Entry(Standing.Center,{-500,-900,90},Standing,Obstacle)),"movement away from the obstacle stays free");

    for (int Stage : {2,4})
    {
        const double Period=Stage==2 ? 24 : 26;
        for (double DT : {1.0/120,1.0/60,1.0/30,0.2,1.0})
            for (double Radius : {150.0,350.0,750.0,1150.0})
            {
                double Age=0;bool Blocked=false;
                for (int Frame=0;Frame<static_cast<int>(Period*2/DT)+2;++Frame)
                {
                    const Point Position=Rotate({-Radius,0,90},CombatRules::ClockAngle(Age,Period)-180);
                    const auto Result=Constrain(Age,ClockLessons::Advance(Age,DT,Period,true,Stage),Period,
                        {Position,42,88},Obstacle);
                    Age=Result.Age;
                    if (Result.Blocked) { Blocked=true;break; }
                }
                Check(Blocked,"a standing rider is stopped on both crouch clocks at every tested radius/frame rate");
                Point Position=Rotate({-Radius,0,90},CombatRules::ClockAngle(Age,Period)-180);
                Check(Position.X<-99,"clock stops on the entry side with a capsule clearance margin");
                const double StoppedAge=Age;
                for (int Repeat=0;Repeat<20;++Repeat)
                {
                    Position=Rotate({-Radius,0,90},CombatRules::ClockAngle(Age,Period)-180);
                    const auto Result=Constrain(Age,Age+DT,Period,{Position,42,88},Obstacle);
                    Check(Result.Blocked,"standing does not silently resume the hand");Age=Result.Age;
                }
                Check(Age-StoppedAge<0.001,"repeated ticks do not creep through the beam");
                bool CrouchBlocked=false;
                for (int Frame=0;Frame<static_cast<int>(Period/DT)+2;++Frame)
                {
                    Position=Rotate({-Radius,0,52},CombatRules::ClockAngle(Age,Period)-180);
                    const auto Result=Constrain(Age,ClockLessons::Advance(Age,DT,Period,true,Stage),Period,
                        {Position,42,50},Obstacle);
                    CrouchBlocked=CrouchBlocked || Result.Blocked;Age=Result.Age;
                    if (CombatRules::AtDock(CombatRules::ClockAngle(Age,Period),true)) break;
                }
                Check(!CrouchBlocked,"shrinking the real capsule releases the stop");
                Check(CombatRules::AtDock(CombatRules::ClockAngle(Age,Period),true),"crouching rider reaches the far dock");
            }
    }
    const auto Hitch=Constrain(0,13,26,{{-1000,0,90},42,88},Obstacle);
    Check(Hitch.Blocked && Hitch.Age<13,"a frame hitch cannot tunnel past the gantry");
    const auto HugeHitch=Constrain(0,26000,26,{{-1000,0,90},42,88},Obstacle);
    Check(HugeHitch.Blocked,"whole revolutions cannot skip the standing collision test");
    const auto LowHitch=Constrain(0,13,26,{{-1000,0,52},42,50},Obstacle);
    Check(!LowHitch.Blocked && Close(LowHitch.Age,13),"crouched hitch reaches the intended exit");
    const auto Return=Constrain(12,24,24,{{1000,0,90},42,88},Obstacle);
    Check(!Return.Blocked && Close(Return.Age,24),"return arc follows the clear side rather than a straight chord through the beam");
    const auto Wait=Constrain(8,8,26,{{-1000,0,90},42,88},Obstacle);
    Check(!Wait.Blocked && Close(Wait.Age,8),"pause is motionless");
    const double UnderAge=9*26.0/28;
    const auto EarlyStand=Constrain(UnderAge,UnderAge+0.1,26,{{0,-900,90},42,88},Obstacle);
    Check(EarlyStand.Blocked && Close(EarlyStand.Age,UnderAge),"uncrouching inside the beam cannot advance the hand");
    const auto DuckAgain=Constrain(UnderAge,UnderAge+0.1,26,{{0,-900,52},42,50},Obstacle);
    Check(!DuckAgain.Blocked && DuckAgain.Age>UnderAge,"crouching again clears an initial overlap stop");
    const auto OnRail=Constrain(0,12,24,{{-1000,0,180},42,88},Obstacle);
    Check(OnRail.Blocked,"standing on a rotating hand rail is also too tall");
    const auto LowOnRail=Constrain(0,12,24,{{-1000,0,142},42,50},Obstacle);
    Check(LowOnRail.Blocked,"a rider must step down from the raised rail to fit below the gantry");
    std::cout<<"Gantry collision checks passed: "<<Checks<<'\n';
}
