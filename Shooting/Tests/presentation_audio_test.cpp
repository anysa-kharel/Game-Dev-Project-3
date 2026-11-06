#include "../Source/ShootingExpansion/AudioPresentationRules.h"
#include "../Source/ShootingExpansion/CAInterfaceRules.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    int Checks=0;
    void Check(bool Value,const std::string& Why)
    { ++Checks;if (!Value) { std::cerr<<"FAILED: "<<Why<<'\n';std::exit(1); } }
    void Advance(PresentationAudio::Mix& M,int Frames,bool Menu,bool Pause=false,bool Quiet=false,bool Mute=false)
    { for(int I=0;I<Frames;++I) M.Step(1.0/60,Menu,Pause,Quiet,Mute,.20,.055); }
}
int main()
{
    using namespace PresentationAudio;
    Mix M;
    Advance(M,600,true);
    Check(std::abs(M.Menu-.20)<1e-6 && M.Game==0,"only menu music is audible during the briefing");
    const double BeforeMenu=M.Menu;
    Advance(M,1,false);
    Check(M.Menu<BeforeMenu && M.Menu>0 && M.Game>0,"starting begins a crossfade without a volume jump");
    Advance(M,600,false);
    Check(M.Menu<1e-6 && std::abs(M.Game-.055)<1e-6,"gameplay stays at the quiet configured level");
    M.Focus();Advance(M,6,false);
    Check(M.Game<.011,"combat ducks music quickly");
    for(int Shot=0;Shot<80;++Shot)
    {
        M.Focus();Advance(M,6,false);
        Check(M.Game<=.011 && M.Game>=0,"repeated shots keep music low without gain stacking");
    }
    Advance(M,60,false);
    Check(M.Game>.055*DuckFactor && M.Game<.055,"music returns gradually after the last cue");
    Advance(M,600,false,true);
    Check(std::abs(M.Game-.055*.35)<1e-6,"pause holds a softer music level");
    Advance(M,600,false,false,true);
    Check(std::abs(M.Game-.055*.35)<1e-6,"death and victory screens remain quiet");
    Advance(M,1,false,false,false,true);
    Check(M.Menu==0 && M.Game==0,"music mute is immediate");
    Advance(M,1,false);
    Check(M.Game>0 && M.Game<.01,"unmuting fades up gently");
    Mix MenuMute;Advance(MenuMute,60,true);Advance(MenuMute,1,true,false,false,true);
    Check(MenuMute.Menu==0 && MenuMute.Game==0,"mute also works on the controls screen");
    M.Focus(100);Check(M.FocusLeft==3,"focus hold is bounded");
    M.Step(1,false,false,false,false,-3,20);
    Check(M.Game>=0 && M.Game<=1 && M.Menu>=0 && M.Menu<=1,"invalid configured gains are clamped");
    M.Step(-1,false,false,false,false,.2,.055);Check(M.FocusLeft>=0,"negative delta cannot reverse the timer");
    for(int Index=0;Index<5;++Index)
    {
        GateCue Gate;int Plays=0;
        for(int Frame=0;Frame<900;++Frame) if(Gate.Opened()) ++Plays;
        Check(Plays==1,"each gate produces exactly one opening cue");
    }
    const auto Encode=[](double V){return static_cast<int>(std::round(255*(V<=.0031308 ? 12.92*V : 1.055*std::pow(V,1.0/2.4)-.055)));};
    using namespace CAInterface;
    Check(Encode(Gold.R)==211 && Encode(Gold.G)==164 && Encode(Gold.B)==59,"shared gold exactly matches #D3A43B");
    Check(Encode(Ink.R)==12 && Encode(Ink.G)==13 && Encode(Ink.B)==14,"shared ink exactly matches #0C0D0E");
    Check(Contrast(White,Ink)>12 && Contrast(Gold,Card)>5,"presentation text has strong dark-panel contrast");
    std::cout<<Checks<<" music-envelope, gate-cue and palette checks passed.\n";
}
