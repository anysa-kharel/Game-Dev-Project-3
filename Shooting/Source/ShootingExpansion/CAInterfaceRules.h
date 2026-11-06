#pragma once
#include <algorithm>

namespace CAInterface
{
    struct RGB { double R,G,B; };
    inline constexpr RGB Ink{0.006,0.010,0.018},Card{0.018,0.032,0.052};
    inline constexpr RGB White{0.96,0.98,1.0},Muted{0.60,0.71,0.84};
    inline constexpr RGB Cyan{0.12,0.80,1.0},Gold{1.0,0.68,0.15};
    inline double Luminance(RGB C) { return 0.2126*C.R+0.7152*C.G+0.0722*C.B; }
    inline double Contrast(RGB A,RGB B)
    { return (std::max(Luminance(A),Luminance(B))+0.05)/(std::min(Luminance(A),Luminance(B))+0.05); }
    enum class Phase { Instructions,Armed,Playing };
    struct StartGate
    {
        Phase State=Phase::Instructions;
        bool Waiting() const { return State!=Phase::Playing; }
        bool Running() const { return State==Phase::Playing; }
        void Request() { if (State==Phase::Instructions) State=Phase::Armed; }
        bool Step(bool MouseHeld,bool EnterHeld)
        {
            if (State!=Phase::Armed || MouseHeld || EnterHeld) return false;
            State=Phase::Playing;return true;
        }
    };
    struct Rect
    {
        double X,Y,W,H;
        bool Contains(double PX,double PY) const { return PX>=X && PX<=X+W && PY>=Y && PY<=Y+H; }
    };
    struct Layout
    {
        double Scale,X,Y;
        Rect At(double PX,double PY,double W,double H) const
        { return {X+PX*Scale,Y+PY*Scale,W*Scale,H*Scale}; }
        Rect Start() const { return At(375,592,410,58); }
    };
    inline Layout Fit(double W,double H)
    {
        const double S=std::max(0.1,std::min({(W-48)/1160,(H-32)/690,1.6}));
        return {S,(W-1160*S)*0.5,(H-690*S)*0.5};
    }
}
