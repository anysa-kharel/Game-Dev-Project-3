#include "../Source/ShootingExpansion/CampaignRules.h"
#include "../Source/ShootingExpansion/WayfindingRules.h"
#include "../Source/ShootingExpansion/CombatRules.h"
#include <cstdlib>
#include <iostream>
#include <string>

int Checks=0;
void Check(bool Value,const std::string& Why)
{
    ++Checks;
    if (!Value) { std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1); }
}
int main()
{
    using namespace Campaign;
    int Counts[5][2]={},Types[7]={};
    for (const auto& P : Spawns)
    {
        Check(P.Section>=1 && P.Section<=5 && P.Wave>=0 && P.Wave<=1,"valid sector and wave");
        Check(P.Kind>=0 && P.Kind<7,"valid enemy role");
        ++Counts[P.Section-1][P.Wave];++Types[P.Kind];
        const double LocalX=P.X-(P.Section-1)*Spacing;
        const double Radius=P.Kind==6 ? 70 : 43,Half=P.Kind==6 ? 130 : 88;
        Check(std::abs(LocalX)+Radius<HalfRoom && std::abs(P.Y)+Radius<1700,"enemy starts inside its sector");
        for (const auto& C : CoverLayout)
        {
            const bool XY=std::abs(LocalX-C.X)<C.L*0.5+Radius && std::abs(P.Y-C.Y)<C.W*0.5+Radius;
            const bool Z=P.Z+Half>C.Z-C.H*0.5 && P.Z-Half<C.Z+C.H*0.5;
            Check(!(XY && Z),"spawn capsule clears configured cover");
        }
        if (P.Section==3)
            Check(!(std::abs(LocalX-300)<250+Radius && std::abs(P.Y)<225+Radius),"gearworks machine clears enemy spawns");
        if (P.Z>300)
            Check(LocalX>=600 && LocalX<=1800 && std::abs(P.Y)==1260,"raised enemies have gallery floor");
    }
    for (int S=0;S<5;++S) for (int W=0;W<2;++W) Check(Counts[S][W]==6,"two six-enemy squads in every sector");
    for (int K=0;K<7;++K) Check(Types[K]>0,"all seven roles appear");
    Check(Types[6]==1 && Cast[4][9]==6,"one final-wave Warden");
    Check(Cast[0][0]==0 && Cast[1][1]==3 && Cast[2][0]==4 && Cast[3][0]==5,"new tactics introduced progressively");
    Check(ExitX-StartX>55000,"the playable route is longer than 550 meters");
    for (int CP=0;CP<8;++CP)
    {
        Check(!CanSave(CP,CP+2,0),"cannot skip a checkpoint");
        Check(CanSave(CP,CP+1,0),"next checkpoint works after clearing its sector");
        if (CP%2==0) Check(!CanSave(CP,CP+1,1),"near bridge flag waits for both squads to be cleared");
        Check(SectionForCheckpoint(CP)==CP/2+1,"checkpoint routes to the right sector");
    }
    Check(!CanSave(-1,0,0) && !CanSave(8,9,0),"invalid checkpoint bounds rejected");
    Check(SectionForCheckpoint(8)==5 && SectionForCheckpoint(99)==5,"final sector stays in range");
    for (int Bridge=0;Bridge<4;++Bridge)
    {
        const double Center=ClockX(Bridge),NearEdge=Center-1000,FarEdge=Center+1000;
        Check(FlagX(Bridge*2+1)>NearEdge-1800 && FlagX(Bridge*2+1)<NearEdge,"near flag lies on its dock");
        Check(FlagX(Bridge*2+2)>FarEdge && FlagX(Bridge*2+2)<FarEdge+1500,"far flag lies on its dock");
        const double NearBoard=Center+Wayfinding::BoardingOffset(1200,false,true);
        Check(NearBoard-43>NearEdge && NearBoard+43<Center,"board target is fully on the hand beyond the dock");
        Check(NextGoal(Bridge*2+1,0,NearBoard)==Goal::FarFlag,"boarding advances guidance to the far flag");
        Check(NextGoal(Bridge*2+1,0,NearEdge-200)==Goal::Board,"standing on dock keeps boarding guidance");
        Check((Bridge+1)*Spacing-HalfRoom==FarEdge+1500,"far corridor meets the next sector without a gap");
        Check(Center-1200<NearEdge-43 && Center+1200>FarEdge+43,"hand overlaps both docks by capsule clearance");
    }
    Check(NextGoal(0,12,StartX)==Goal::Fight,"begin with the first squad");
    Check(NextGoal(0,0,2000)==Goal::NearFlag,"cleared sector points at next flag");
    Check(NextGoal(8,1,50000)==Goal::Fight && NextGoal(8,0,53000)==Goal::Exit,"exit waits for final squad including Warden");
    Check(!Boarded(-1140,0,110,260),"clock stays parked while player is on dock floor");
    Check(Boarded(-920,0,110,260),"clock starts after player is safely on hand");
    Check(!Boarded(-920,400,110,260) && !Boarded(-920,0,-400,260),"off-hand or falling player cannot start clock");
    Check(ShieldBlocks(0.9,2,false) && !ShieldBlocks(-0.9,2,false),"shield blocks front and rewards flanking");
    Check(!ShieldBlocks(0.9,2,true) && !ShieldBlocks(0.9,0,false),"charging and broken shields expose body");
    Check(BurstCount(0,3,3)==1 && BurstCount(2,3,3)==2,"guard and sentry attack differently");
    Check(BurstCount(6,12,12)==3 && BurstCount(6,6,12)==5,"Warden escalates at half health");
    for (const auto& P : Profiles)
    {
        Charge C;C.Cooldown=0;
        Check(!C.Step(0,true,P) && C.Charging,"every role visibly winds up");
        Check(!C.Step(P.Windup/2,true,P),"partial windup cannot attack");
        Check(!C.Step(P.Windup,false,P) && !C.Charging,"cover or leaving range cancels attack");
        C.Step(0,true,P);Check(C.Step(P.Windup,true,P),"full uninterrupted windup attacks once");
        Check(!C.Step(0.1,true,P),"cooldown prevents immediate repeat attack");
    }
    for (int S=0;S<5;++S) for (const auto& A : Wayfinding::Arrows)
    {
        if (A.X>3000) continue;
        const double Y=S==2 && A.X>=-600 && A.X<=650 ? -740 : A.Y;
        for (const auto& C : CoverLayout)
            Check(!(std::abs(A.X-C.X)<C.L*0.5+75 && std::abs(Y-C.Y)<C.W*0.5+75),"new route arrows clear cover");
        if (S==2) Check(!(std::abs(A.X-300)<325 && std::abs(Y)<300),"arrows lead around the gearworks machine");
    }
    std::cout<<Checks<<" campaign progression, encounter, combat and layout checks passed.\n";
}
