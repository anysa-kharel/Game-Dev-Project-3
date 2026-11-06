#pragma once
#include <algorithm>

namespace CAInterface
{
    struct RGB { double R,G,B; };
    // Linear values decoded from the start-screen sRGB palette.
    inline constexpr RGB Ink{0.0036765073,0.0040247170,0.0043914420};
    inline constexpr RGB Card{0.0085681256,0.0097212173,0.0109600940};
    inline constexpr RGB White{0.9046611744,0.8549926081,0.7230551289};
    inline constexpr RGB Muted{0.5271151257,0.4851499401,0.4019777798};
    inline constexpr RGB Gold{0.6514056374,0.3712376805,0.0437350293};
    inline constexpr RGB BrightGold{0.8631572135,0.5332764040,0.0822827071};
    inline constexpr RGB Bronze{0.2086368701,0.1301364767,0.0356013149};
    inline constexpr RGB Edge{0.1046164841,0.0684781698,0.0221738848};
    inline constexpr RGB KeyFace{0.0221738848,0.0212190104,0.0159962934};
    inline constexpr RGB Cyan=Gold; // Compatibility for the existing interface checks.
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
