#pragma once
#include <algorithm>
#include <cmath>

namespace PresentationAudio
{
    inline constexpr double MenuLevel=0.20,GameLevel=0.055,DuckFactor=0.16;
    struct Mix
    {
        double Menu=0,Game=0,FocusLeft=0;
        void Focus(double Hold=0.8) { FocusLeft=std::max(FocusLeft,std::clamp(Hold,0.0,3.0)); }
        void Step(double DT,bool Briefing,bool Paused,bool Quiet,bool Muted,double MenuVolume,double GameVolume)
        {
            DT=std::max(0.0,DT);
            const bool Focused=FocusLeft>0;
            FocusLeft=std::max(0.0,FocusLeft-DT);
            if (Muted) { Menu=Game=0;return; }
            const double MenuTarget=Briefing ? std::clamp(MenuVolume,0.0,1.0) : 0;
            double GameTarget=Briefing ? 0 : std::clamp(GameVolume,0.0,1.0);
            if (Paused || Quiet) GameTarget*=0.35;
            if (Focused) GameTarget*=DuckFactor;
            const auto Ease=[DT](double Current,double Target,double Rate)
            { return Target+(Current-Target)*std::exp(-Rate*DT); };
            Menu=Ease(Menu,MenuTarget,Focused && MenuTarget<Menu ? 35.0 : 3.0);
            Game=Ease(Game,GameTarget,GameTarget<Game ? 35.0 : 2.5);
        }
    };
    struct GateCue
    {
        bool Played=false;
        bool Opened() { if (Played) return false;Played=true;return true; }
    };
}
