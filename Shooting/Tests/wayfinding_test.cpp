#include "../Source/ShootingExpansion/WayfindingRules.h"
#include "../Source/ShootingExpansion/ArenaLayout.h"
#include <cstdlib>
#include <iostream>
#include <string>

int Count=0;
void Check(bool Value,const std::string& Why)
{
    ++Count;
    if (!Value) { std::cerr<<"FAILED: "<<Why<<'\n'; std::exit(1); }
}
int main()
{
    using namespace Wayfinding;
    Check(NextStage(0,6,6,-2150)==Stage::ClearYard,"initial guidance teaches combat before flag one");
    Check(NextStage(1,1,6,2850)==Stage::ClearYard,"touching flag one cannot hide an unfinished yard");
    Check(NextStage(2,1,0,9000)==Stage::ReturnToYard,"last yard robot is still required after crossing early");
    Check(NextStage(2,1,0,4200)==Stage::ReturnToYard,"return guidance persists while riding back");
    Check(NextStage(2,1,0,3000)==Stage::ClearYard,"return guidance changes back to the yard robot after landing");
    Check(NextStage(0,0,6,2200)==Stage::FlagOne,"yard clear points to the checkpoint flag");
    Check(NextStage(1,0,6,3000)==Stage::BoardClock,"saved near checkpoint points to boarding");
    Check(NextStage(1,0,6,3280)==Stage::FlagTwo,"fully boarding the hand points to the far flag");
    Check(NextStage(1,0,6,5500)==Stage::FlagTwo,"landing requires walking through flag two");
    Check(NextStage(2,0,6,5800)==Stage::ClearFoundry,"far checkpoint starts foundry guidance");
    Check(NextStage(2,0,0,9000)==Stage::Exit,"all hostiles cleared points to extraction");
    Check(NextStage(2,0,0,2800)==Stage::BoardClock,"all-clear player on wrong bank is guided back across");
    Check(StateFor(1,0,12,false)==FlagState::Next,"first flag is the next checkpoint");
    Check(StateFor(2,0,12,false)==FlagState::Future,"second flag is visible before reaching it");
    Check(StateFor(1,1,6,false)==FlagState::Saved,"checkpoint one gains a saved state");
    Check(StateFor(2,1,6,false)==FlagState::Next,"checkpoint two becomes next");
    Check(StateFor(2,2,1,false)==FlagState::Saved,"far flag stays saved while combat remains");
    Check(StateFor(0,2,1,true)==FlagState::Locked,"one remaining robot keeps the exit locked");
    Check(StateFor(0,2,0,true)==FlagState::Open,"exit becomes ready only after the last robot");
    for (int Flag=1;Flag<=2;++Flag)
        for (int Left=0;Left<=12;++Left)
            Check(StateFor(Flag,2,Left,false)==FlagState::Saved,"combat cannot undo an activated checkpoint");
    const double NearWait=4200+BoardingOffset(1200,false,false);
    const double NearBoard=4200+BoardingOffset(1200,false,true);
    const double FarWait=4200+BoardingOffset(1200,true,false);
    const double FarBoard=4200+BoardingOffset(1200,true,true);
    Check(NearWait+43<3200 && NearWait-43>2400,"near wait cue is safely on the dock");
    Check(FarWait-43>5200 && FarWait+43<5800,"far wait cue is safely on the dock");
    Check(NearBoard-43>3200 && NearBoard+43<4200,"near boarding cue steps completely beyond the floor onto the hand");
    Check(FarBoard+43<5200 && FarBoard-43>4200,"return boarding cue steps completely beyond the far floor");
    Check(NextStage(1,0,6,NearBoard)==Stage::FlagTwo,"boarding target advances the guidance stage");
    Check(Bearing(10,0).Y==-1 && !Bearing(10,0).TurnBack,"front target points up");
    Check(Bearing(0,10).X==1 && Bearing(0,-10).X==-1,"left and right targets point to the correct edges");
    Check(Bearing(-10,0).Y==1 && Bearing(-10,0).TurnBack,"behind target remains visible with a turn-back cue");
    Check(std::isfinite(Bearing(0,0).Y),"standing at a target does not divide by zero");
    for (int Degrees=-180;Degrees<=180;++Degrees)
    {
        const double A=Degrees*3.141592653589793/180;
        const auto D=Bearing(std::cos(A),std::sin(A));
        Check(std::abs(D.X*D.X+D.Y*D.Y-1)<0.00001,"edge cue remains normalized through a full turn");
    }
    for (const auto& Arrow : Arrows)
    {
        Check(Arrow.X<3200 || Arrow.X>5200,"floor arrows never encourage walking over the clock gap");
        const bool Narrow=(Arrow.X>2400 && Arrow.X<3200)||(Arrow.X>5200 && Arrow.X<5800);
        Check(std::abs(Arrow.Y)+75<(Narrow ? 500 : 1700),"arrow footprint fits the walkable path");
        for (const auto& C : ArenaLayout::Cover)
            Check(!(std::abs(Arrow.X-C.X)<C.L*0.5+75 && std::abs(Arrow.Y-C.Y)<C.W*0.5+75),
                "floor arrow is visible outside configured cover footprints");
    }
    double HighestCover=0;
    for (const auto& C : ArenaLayout::Cover) HighestCover=std::max(HighestCover,C.Z+C.H*0.5);
    Check(FlagHeight-190>HighestCover,"flag banner sits above ordinary configured cover");
    std::cout<<Count<<" route, flag, boarding, direction and breadcrumb checks passed.\n";
}
