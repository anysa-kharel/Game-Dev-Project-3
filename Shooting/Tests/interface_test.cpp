#include "../Source/ShootingExpansion/CAInterfaceRules.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    int Checks=0;
    void Check(bool Value,const std::string& Why)
    {
        ++Checks;
        if (!Value) { std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1); }
    }
}
int main()
{
    using namespace CAInterface;
    StartGate Idle;
    for (int Frame=0;Frame<600;++Frame)
    {
        Check(!Idle.Step(Frame%2==0,Frame%3==0),"reading controls never starts a run without confirmation");
        Check(Idle.Waiting() && !Idle.Running(),"the run remains held while the player reads");
    }
    for (int Key=0;Key<3;++Key)
    {
        StartGate Gate;
        Gate.Request();Gate.Request();
        Check(Gate.Waiting(),"duplicate confirmations do not skip the release guard");
        for (int Frame=0;Frame<30;++Frame)
            Check(!Gate.Step(Key!=1,Key!=0),"held start keys cannot leak a mouse press into gameplay");
        Check(Gate.Step(false,false),"releasing confirmation starts the run");
        Check(Gate.Running() && !Gate.Waiting(),"the run is active after confirmation");
        Gate.Request();
        Check(!Gate.Step(false,false) && Gate.Running(),"start fires once and cannot reset during gameplay");
    }
    StartGate ClickReleasedBeforeTick;
    ClickReleasedBeforeTick.Request();
    Check(ClickReleasedBeforeTick.Step(false,false),"a short click between frames still starts the run");
    Check(Contrast(White,Ink)>12 && Contrast(White,Card)>10,"white body text contrasts strongly with both panels");
    Check(Contrast(Muted,Ink)>7 && Contrast(Muted,Card)>7,"secondary instructions remain readable");
    Check(Contrast(Gold,Ink)>7 && Contrast(Cyan,Ink)>7,"the button and accents contrast with the dark surface");
    const double Screens[][2]={{640,480},{800,600},{1024,768},{1280,720},{1280,800},
        {1366,768},{1512,982},{1600,900},{1920,1080},{2560,1440},{3440,1440},{3840,2160}};
    for (const auto& View : Screens)
    {
        const auto L=Fit(View[0],View[1]);
        const auto Menu=L.At(0,0,1160,690),Start=L.Start();
        const auto Left=L.At(0,126,560,360),Right=L.At(584,126,576,360);
        Check(Menu.X>=0 && Menu.Y>=0 && Menu.X+Menu.W<=View[0] && Menu.Y+Menu.H<=View[1],"the complete briefing fits the viewport");
        Check(Left.X+Left.W<Right.X,"movement and combat columns stay separate");
        Check(Start.Y>Left.Y+Left.H && Start.Y+Start.H<L.At(0,666,0,0).Y,"the button clears instructions and footer");
        Check(Start.Contains(Start.X+Start.W/2,Start.Y+Start.H/2),"the visual button center accepts a click");
        Check(!Start.Contains(Start.X-1,Start.Y+Start.H/2) && !Start.Contains(Start.X+Start.W+1,Start.Y),"clicks outside the button are ignored");
    }
    std::cout<<Checks<<" start-flow, contrast and viewport checks passed.\n";
}
