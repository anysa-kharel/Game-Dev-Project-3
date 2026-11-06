#include "../Source/ShootingExpansion/ClockLessonRules.h"
#include "../Source/ShootingExpansion/CombatRules.h"
#include "../Source/ShootingExpansion/CampaignRules.h"
#include <cstdlib>
#include <iostream>

namespace
{
    int Checks=0;
    void Check(bool Value,const char* Why)
    {
        ++Checks;if (!Value) { std::cerr<<"FAIL: "<<Why<<'\n';std::exit(1); }
    }
    bool Close(double A,double B) { return std::abs(A-B)<1e-7; }
}
int main()
{
    using namespace ClockLessons;
    Check(Stage(0)==0 && Stage(5)==0 && Stage(-1)==0,"legacy and unrelated clocks keep their course disabled");
    for (int S=1;S<=4;++S)
    {
        Check(Stage(S)==S,"one stage per bridge");
        Check(Crouch(S)==(S==2 || S==4),"crouch is introduced then combined");
        Check(Jump(S)==(S==3 || S==4),"jump is introduced then combined");
        Check(Close(Advance(0,10,28,false,S),0),"no motion before boarding");
    }
    for (int S : {3,4}) for (double P : {24.0,26.0,28.0,32.0}) for (double DT : {1.0/30,1.0/120})
    {
        double Age=0;
        for (int Frame=0;Frame<static_cast<int>(120/DT);++Frame) Age=Advance(Age,DT,P,true,S);
        Check(Close(Age,P*0.5),"jump exit remains parked despite repeated ticks");
        Check(CombatRules::AtDock(CombatRules::ClockAngle(Age,P),true),"parked hand really points at far dock");
        Check(Close(Advance(Age,0,P,true,S),Age),"pause does not advance the parked hand");
        Age=0;
        Check(CombatRules::AtDock(CombatRules::ClockAngle(Age,P),false),"retry starts at near dock");
    }
    for (int S : {0,1,2})
        Check(Close(Advance(0,60,28,true,S),60),"legacy and basic clocks retain cyclic motion");

    // The uploaded template serializes Walk_Speed=600 and Crouch_Speed=400.
    // With Unreal's default 420 jump speed / 980 gravity, the same-height jump
    // has ample horizontal range when W is held before Space. This is a model,
    // not an engine collision/air-control playtest.
    const double Flight=2*420.0/980.0;
    const double Takeoff=1200-35,RequiredLanding=1200+Gap+55;
    Check(600*Flight>(RequiredLanding-Takeoff)*1.4,"normal running jump has range margin without sprint");
    Check(Gap>2*42+140,"gap is wider than the capsule diameter plus a walking margin");
    Check(PracticeTop>45 && PracticeTop<420.0*420/(2*980),"practice block requires a jump and is below jump apex");
    for (double Position : {-2300.0,-2250.0,-2200.0})
    {
        const double T=(Position-(-2490))/600;
        Check(420*T-490*T*T>PracticeTop,"jump from practice stripe clears the entire block");
    }
    const auto Dock=Landing(1200);
    Check(Close(Dock.Start,1440) && Close(Dock.End,2500),"jump dock is trimmed at the front and keeps its far connection");
    Check(Close(Dock.Center,1970) && Close(Dock.Length,1060),"dock dimensions describe the intended gap");
    Check(1700>Dock.Start+42 && 1700<Dock.End-42,"far checkpoint remains on broad safe floor");
    Check(ClockLessons::PracticeX+PracticeWidth/2<-1450-120,"practice is completed before the saved near-side flag");
    Check(Dock.Start>1200,"no floor is under the jump gap");

    const double Pieces[7][2]={{0,-35},{-484,2},{484,2},{-520,100},{520,100},{-520,204},{520,204}};
    for (const auto& Piece : Pieces)
    {
        Check(OriginalDockPiece(1750,Piece[0],Piece[1],1500),"all seven saved dock pieces are selected");
        double X=1750,Length=1500;bool Tagged=false;
        for (int Repeat=0;Repeat<5;++Repeat)
            if (Tagged || OriginalDockPiece(X,Piece[0],Piece[1],Length))
            { Tagged=true;X=Dock.Center;Length=Dock.Length; }
        Check(Close(X,Dock.Center) && Close(Length,Dock.Length),"refreshing the saved dock does not accumulate offsets");
        Check(Close(X-Length/2,Dock.Start),"floor stripes and rails cannot extend across the gap");
    }
    Check(!OriginalDockPiece(1700,0,-35,1500),"unrelated custom floor is preserved");
    Check(!OriginalDockPiece(1750,800,0,1500),"distant scenery is preserved");
    Check(!OriginalDockPiece(1750,0,500,1500),"high scenery is preserved");
    Check(!OriginalDockPiece(1750,0,0,1200),"different sized props are preserved");

    Check(Close(Clearance(0,0),150),"editor preview has usable default clearance");
    Check(Clearance(30,0)>0 && Clearance(30,0)<30,"small scales do not produce invalid clamp bounds");
    Check(Clearance(176,160)>160 && Clearance(176,160)<176,"a shallow custom crouch still fits beneath the beam");
    Check(Clearance(176,176)<176,"first crouch frame uses a safe provisional clearance");
    for (double Standing : {160.0,176.0,192.0,200.0})
        for (double Fraction : {0.5,0.6,0.7})
        {
            const double Crouched=Standing*Fraction,H=Clearance(Standing,Crouched);
            Check(H>Crouched+10,"crouched capsule has clearance");
            Check(H<Standing-10,"standing capsule cannot pass the gantry");
            Check(H>90,"hand rails do not strike the gantry");
        }
    Check(!ClearOfBeam(90,-600,42),"instructions keep the rider low while capsule overlaps the beam");
    Check(ClearOfBeam(125,-600,42),"stand prompt waits until the whole capsule clears the beam");
    Check(Hint(2,-2250,false,false,false,false)==Cue::PracticeCrouch,"safe crouch lesson precedes boarding");
    Check(Hint(3,-2250,false,false,false,false)==Cue::PracticeJump,"safe jump lesson precedes boarding");
    Check(!AtApproach(-4000),"practice prompt does not replace combat-room navigation");
    Check(Hint(2,-500,true,true,false,false)==Cue::Crouch,"moving rider gets crouch instructions");
    Check(Hint(4,200,true,true,true,false)==Cue::Stand,"combined lesson asks player to stand after clearance");
    Check(Hint(4,1165,true,true,true,true)==Cue::JumpExit,"combined lesson ends with jump instructions");
    Check(Hint(1,-500,true,true,false,false)==Cue::Ride,"first crossing teaches riding only");
    Check(Campaign::EnemyCount==60 && Campaign::Checkpoints==8,"campaign encounter and checkpoint counts are unchanged");
    std::cout<<"Clock lesson checks passed: "<<Checks<<'\n';
}
