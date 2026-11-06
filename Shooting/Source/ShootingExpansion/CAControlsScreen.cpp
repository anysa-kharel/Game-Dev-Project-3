#include "CACombatHUD.h"
#include "CACombatGameMode.h"
#include "CAInterfaceRules.h"
#include "CAUtil.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

namespace CAControls
{
    FLinearColor Color(CAInterface::RGB C) { return FLinearColor(C.R,C.G,C.B,1); }
}
void ACACombatHUD::MenuText(const FString& Value,float X,float Y,float Height,const FLinearColor& Color,float MaxWidth)
{
    UFont* Font=GEngine->GetMediumFont();
    float Scale=Height/FMath::Max(1.0f,static_cast<float>(Font->GetMaxCharHeight()));
    float TW=0,TH=0;GetTextSize(Value,TW,TH,Font,Scale);
    if (MaxWidth>0 && TW>MaxWidth) Scale*=MaxWidth/TW;
    FCanvasTextItem Item(FVector2D(X,Y),FText::FromString(Value),Font,Color);
    Item.Scale=FVector2D(Scale);Canvas->DrawItem(Item);
}
void ACACombatHUD::DrawControls()
{
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G) return;
    const auto Layout=CAInterface::Fit(W,H);const float U=Layout.Scale;
    const auto Ink=CAControls::Color(CAInterface::Ink),Card=CAControls::Color(CAInterface::Card);
    const auto White=CAControls::Color(CAInterface::White),Muted=CAControls::Color(CAInterface::Muted);
    const auto Cyan=CAControls::Color(CAInterface::Cyan),Gold=CAControls::Color(CAInterface::Gold);
    DrawRect(Ink,0,0,W,H);
    auto Box=[&](double X,double Y,double Width,double Height,FLinearColor C)
    { const auto R=Layout.At(X,Y,Width,Height);DrawRect(C,R.X,R.Y,R.W,R.H); };
    auto Text=[&](const FString& T,double X,double Y,double Size,FLinearColor C,double Width=0)
    { const auto R=Layout.At(X,Y,0,0);MenuText(T,R.X,R.Y,Size*U,C,Width*U); };
    auto Key=[&](const FString& T,double X,double Y,double Width)
    {
        Box(X,Y+3,Width,36,FLinearColor(0,0,0,1));Box(X,Y,Width,36,FLinearColor(0.045f,0.08f,0.12f,1));
        Box(X,Y,Width,2,Cyan);
        UFont* Font=GEngine->GetMediumFont();float TW=0,TH=0;
        GetTextSize(T,TW,TH,Font,15*U/FMath::Max(1.0f,static_cast<float>(Font->GetMaxCharHeight())));
        Text(T,X+(Width-TW/U)*0.5,Y+10,15,White,Width-8);
    };
    Text(TEXT("CLOCK ARENA / CITADEL RUN"),0,0,14,Cyan,1160);
    Text(TEXT("YOUR CONTROLS"),0,28,42,White,1160);
    Text(TEXT("Take a moment to get ready. The run starts when you choose START."),0,86,18,Muted,1160);
    Box(0,126,560,360,Card);Box(584,126,576,360,Card);
    Box(0,126,560,3,Cyan);Box(584,126,576,3,Gold);
    Text(TEXT("01 / MOVEMENT"),24,148,19,White,510);
    Text(TEXT("02 / COMBAT & SHORTCUTS"),608,148,19,White,528);
    const TCHAR* MoveKeys[]={TEXT("W"),TEXT("A"),TEXT("S"),TEXT("D")};
    const TCHAR* MoveLabels[]={TEXT("FORWARD"),TEXT("LEFT"),TEXT("BACK"),TEXT("RIGHT")};
    for (int32 I=0;I<4;++I)
    {
        const float X=24+I*128;Key(MoveKeys[I],X+24,192,64);
        float TW=0,TH=0;
        GetTextSize(MoveLabels[I],TW,TH,GEngine->GetMediumFont(),13*U/FMath::Max(1.0f,static_cast<float>(GEngine->GetMediumFont()->GetMaxCharHeight())));
        Text(MoveLabels[I],X+(112-TW/U)*0.5,242,13,Muted,112);
    }
    const TCHAR* MovementKeys[]={TEXT("SPACE"),TEXT("SHIFT"),TEXT("C"),TEXT("MOUSE")};
    const TCHAR* MovementActions[]={TEXT("Jump"),TEXT("Sprint"),TEXT("Crouch"),TEXT("Look around")};
    for (int32 I=0;I<4;++I)
    {
        const float Y=280+I*46;Key(MovementKeys[I],24,Y,104);
        Text(MovementActions[I],152,Y+9,18,White,382);
    }
    const TCHAR* CombatKeys[]={TEXT("LEFT MOUSE"),TEXT("RIGHT MOUSE"),TEXT("R"),TEXT("P"),TEXT("F1"),TEXT("BACKSPACE")};
    const TCHAR* CombatActions[]={TEXT("Shoot"),TEXT("Aim"),TEXT("Reload / refill magazine"),TEXT("Pause / resume"),TEXT("Open the field guide"),TEXT("Retry saved checkpoint")};
    for (int32 I=0;I<6;++I)
    {
        const float Y=192+I*46;Key(CombatKeys[I],608,Y,132);
        Text(CombatActions[I],764,Y+9,18,White,372);
    }
    Box(0,506,1160,70,Card);
    Text(TEXT("YOUR MISSION"),24,518,12,Cyan,1112);
    Text(TEXT("Clear waves  /  Follow flags  /  Ride clock hands  /  Reach the green exit"),24,540,18,White,1112);
    const auto Start=Layout.Start();float MX=-1,MY=-1;
    const bool Hover=PlayerOwner && PlayerOwner->GetMousePosition(MX,MY) && Start.Contains(MX,MY);
    DrawRect(Hover ? FLinearColor(1.0f,0.82f,0.30f,1) : Gold,Start.X,Start.Y,Start.W,Start.H);
    const FString Caption=G->StartFlow.State==CAInterface::Phase::Armed ? TEXT("RELEASE TO START") : TEXT("START RUN  /  ENTER");
    float TW=0,TH=0;GetTextSize(Caption,TW,TH,GEngine->GetMediumFont(),20*U/FMath::Max(1.0f,static_cast<float>(GEngine->GetMediumFont()->GetMaxCharHeight())));
    MenuText(Caption,Start.X+(Start.W-TW)*0.5f,Start.Y+19*U,20*U,Ink,Start.W-32*U);
    AddHitBox(FVector2D(Start.X,Start.Y),FVector2D(Start.W,Start.H),TEXT("CA_StartRun"),true,100);
    Text(TEXT("Click START or press ENTER. Controls stay available in the F1 field guide."),0,666,14,Muted,1160);
}
void ACACombatHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if (BoxName==TEXT("CA_StartRun"))
        if (ACACombatGameMode* G=CA::Game(GetWorld())) G->RequestStart();
}
