#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace Campaign
{
    inline constexpr int Sectors=5,Checkpoints=8,EnemyCount=60;
    inline constexpr double Spacing=12500,HalfRoom=3600,ClockOffset=6400;
    inline constexpr double StartX=-3100,ExitX=54400;
    inline constexpr std::array<const char*,5> Names={{"ARRIVAL YARD","COOLING GARDENS","GEARWORKS","SIGNAL ROOFS","CLOCK CITADEL"}};
    struct Profile { int Hits; double Speed,Range,Windup,Cooldown; };
    inline constexpr std::array<Profile,7> Profiles={{
        {3,170,2400,0.8,1.7},{2,270,175,0.45,1.3},{3,0,2900,1.0,2.3},
        {2,290,2200,0.65,1.8},{4,130,2300,1.1,2.0},{2,235,430,1.8,2.0},
        {12,110,3100,1.2,2.0}
    }};
    struct Cover { double X,Y,Z,L,W,H; };
    inline constexpr std::array<Cover,12> CoverLayout={{
        {-2550,350,115,320,220,230},{-1800,-250,65,300,200,130},
        {-1050,-1100,115,260,200,230},{-700,500,115,320,200,230},
        {100,-450,65,320,200,130},{650,650,115,260,220,230},
        {1250,-800,115,260,200,230},{1750,200,115,340,240,230},
        {2450,-500,65,280,220,130},{2750,1000,115,280,240,230},
        {1250,1480,330,250,100,180},{1350,-1480,330,250,100,180}
    }};
    struct Spawn { double X,Y,Z; int Kind,Section,Wave; };
    inline constexpr std::array<std::array<int,12>,5> Cast={{
        {{0,1,0,0,1,2, 0,1,0,2,0,1}},
        {{0,3,1,3,0,2, 3,1,0,3,2,0}},
        {{4,0,3,4,1,2, 4,1,3,0,2,4}},
        {{5,3,0,5,1,2, 5,4,3,0,2,5}},
        {{4,5,3,0,1,2, 4,5,3,6,2,0}}
    }};
    inline constexpr std::array<std::array<double,3>,12> LocalSpawns={{
        {{-2200,-850,105}},{{-1650,800,105}},{{-750,-250,105}},
        {{150,950,105}},{{650,-700,105}},{{1000,100,105}},
        {{-1100,950,105}},{{200,-950,105}},{{1400,-350,105}},
        {{2300,600,105}},{{1500,1260,345}},{{1600,-1260,345}}
    }};
    inline constexpr std::array<Spawn,EnemyCount> Spawns=[] {
        std::array<Spawn,EnemyCount> Out{};
        for (int S=0;S<Sectors;++S) for (int N=0;N<12;++N)
        {
            const auto& P=LocalSpawns[N]; const int K=Cast[S][N];
            Out[S*12+N]={S*Spacing+P[0],P[1],K==6 ? 135.0 : P[2],K,S+1,N/6};
        }
        return Out;
    }();
    inline int SectionForCheckpoint(int Saved) { return std::clamp(Saved/2+1,1,Sectors); }
    inline double FlagX(int Index)
    {
        const int Bridge=(Index-1)/2;
        return Bridge*Spacing+(Index%2 ? 4950 : 8100);
    }
    inline double ClockX(int Index) { return Index*Spacing+ClockOffset; }
    inline bool CanSave(int Saved,int Requested,int SectorLeft)
    {
        return Saved>=0 && Requested>=1 && Requested==Saved+1 && Requested<=Checkpoints && (Requested%2==0 || SectorLeft==0);
    }
    inline bool Boarded(double X,double Y,double Z,double Width)
    { return X>-950 && X<-100 && std::abs(Y)<Width*0.5+40 && Z>=-30 && Z<260; }
    inline int BurstCount(int Kind,int Hits,int Maximum)
    { return Kind==2 ? 2 : Kind==6 ? (Hits<=Maximum/2 ? 5 : 3) : 1; }
    enum class Goal { Fight, NearFlag, Board, FarFlag, Exit };
    inline Goal NextGoal(int Saved,int Remaining,double X)
    {
        if (Remaining>0) return Goal::Fight;
        if (Saved>=Checkpoints) return Goal::Exit;
        if (Saved%2==0) return Goal::NearFlag;
        return X<ClockX(Saved/2)-950 ? Goal::Board : Goal::FarFlag;
    }
    struct Charge
    {
        double Cooldown=1.5,Remaining=0; bool Charging=false;
        bool Step(double Delta,bool Allowed,const Profile& P)
        {
            Delta=std::max(0.0,Delta); Cooldown=std::max(0.0,Cooldown-Delta);
            if (!Allowed) { Charging=false;Remaining=0;return false; }
            if (Cooldown>0) return false;
            if (!Charging) { Charging=true;Remaining=P.Windup;return false; }
            Remaining-=Delta;
            if (Remaining>0) return false;
            Charging=false;Cooldown=P.Cooldown;return true;
        }
        void Reset() { Cooldown=1.8;Remaining=0;Charging=false; }
    };
    inline bool ShieldBlocks(double FacingDot,int ShieldHits,bool Charging)
    { return ShieldHits>0 && !Charging && FacingDot>0.45; }
}
