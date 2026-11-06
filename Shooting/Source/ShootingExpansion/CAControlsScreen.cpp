#include "CACombatHUD.h"
#include "CACombatGameMode.h"
#include "CAInterfaceRules.h"
#include "CAUtil.h"
#include "CanvasItem.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace TTGGMenu
{
    // Palette uses sRGB hex values, converted to Unreal's linear drawing colors.
    FLinearColor Color(uint8 R,uint8 G,uint8 B,float Alpha=1)
    { FLinearColor C=FLinearColor::FromSRGBColor(FColor(R,G,B));C.A=Alpha;return C; }
    const FLinearColor Ink=Color(12,13,14),PanelFill=Color(23,25,27,0.97f);
    const FLinearColor Gold=Color(211,164,59),BrightGold=Color(239,193,81);
    const FLinearColor White=Color(244,238,221),Muted=Color(192,185,170);
    const FLinearColor Edge=Color(91,74,41),KeyFace=Color(41,40,34);
    constexpr const TCHAR* Background=TEXT("ClockArena/UI/StartScreenBackground.png");
}

void ACACombatHUD::MenuText(const FString& Value,float X,float Y,float Height,
    const FLinearColor& Color,float MaxWidth,bool Bold,float WidthScale,bool Centered)
{
    if (!Canvas || !GEngine || Height<=0 || Value.IsEmpty()) return;
    // Large titles use the scalable font already owned by world-text defaults.
    // Its CDO reference keeps the font alive; no new asset or Slate text path is needed.
    UFont* Font=nullptr;
    if (Bold && Height>=28)
        if (const UTextRenderComponent* Defaults=GetDefault<UTextRenderComponent>()) Font=Defaults->Font.Get();
    if (!Font || Font->GetMaxCharHeight()<=0) Font=GEngine->GetMediumFont();
    if (!Font || Font->GetMaxCharHeight()<=0) Font=GEngine->GetSmallFont();
    if (!Font) return;
    const float PixelHeight=FMath::Max(1.0f,FMath::RoundToFloat(Height));
    float ScaleY=PixelHeight/FMath::Max(1.0f,static_cast<float>(Font->GetMaxCharHeight()));
    float MeasuredWidth=0,MeasuredHeight=0;
    GetTextSize(Value,MeasuredWidth,MeasuredHeight,Font,ScaleY);
    // Preserve normal letter proportions on controls and small instructions.
    const float Condense=Height>=28 ? FMath::Clamp(WidthScale,0.9f,1.0f) : 1.0f;
    float ScaleX=ScaleY*Condense;
    float DrawnWidth=MeasuredWidth*Condense;
    if (MaxWidth>0 && DrawnWidth>MaxWidth)
    {
        const float Fit=MaxWidth/DrawnWidth;
        ScaleX*=Fit;ScaleY*=Fit;DrawnWidth=MaxWidth;
    }
    const float Left=Centered ? X-DrawnWidth*0.5f : X;
    FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(Left),FMath::RoundToFloat(Y)),FText::FromString(Value),Font,Color);
    Item.Scale=FVector2D(ScaleX,ScaleY);
    // Fake bold via repeated matching outlines clogged the small glyphs.
    Item.bOutlined=false;Item.DisableShadow();
    Canvas->DrawItem(Item);
}

void ACACombatHUD::DrawControls()
{
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || W<=0 || H<=0) return;
    using namespace TTGGMenu;
    const auto Layout=CAInterface::Fit(W,H);const float U=Layout.Scale;

    if (!bBackdropChecked)
    {
        bBackdropChecked=true;
        const FString Path=FPaths::Combine(FPaths::ProjectContentDir(),Background);
        if (IFileManager::Get().FileExists(*Path))
            ControlsBackdrop=UKismetRenderingLibrary::ImportFileAsTexture2D(this,Path);
        if (!ControlsBackdrop)
            UE_LOG(LogTemp,Warning,TEXT("Tick Tock menu background is unavailable: %s. Copy the supplied PNG into Content/ClockArena/UI."),*Path);
    }
    DrawRect(Ink,0,0,W,H);
    if (ControlsBackdrop && ControlsBackdrop->GetSizeX()>0 && ControlsBackdrop->GetSizeY()>0)
    {
        // Cover the viewport without distorting the image at 4:3 or ultrawide ratios.
        const float ImageRatio=static_cast<float>(ControlsBackdrop->GetSizeX())/ControlsBackdrop->GetSizeY();
        const float ViewRatio=W/H;
        float SpanU=1,SpanV=1;
        if (ViewRatio<ImageRatio) SpanU=ViewRatio/ImageRatio;else SpanV=ImageRatio/ViewRatio;
        DrawTexture(ControlsBackdrop,0,0,W,H,(1-SpanU)*0.5f,(1-SpanV)*0.5f,SpanU,SpanV,FLinearColor::White);
    }
    DrawRect(FLinearColor(0,0,0,0.19f),0,0,W,H);
    // Dark edges protect the title, footer and controls even over muzzle flashes.
    for (int32 I=0;I<64;++I)
    {
        const float A=I/63.0f;
        const float X=FMath::FloorToFloat(W*I/64),NextX=FMath::FloorToFloat(W*(I+1)/64);
        const float Y=FMath::FloorToFloat(H*I/64),NextY=FMath::FloorToFloat(H*(I+1)/64);
        DrawRect(FLinearColor(0,0,0,0.35f*(1-A)),X,0,NextX-X,H);
        DrawRect(FLinearColor(0,0,0,0.32f*A),0,Y,W,NextY-Y);
    }

    auto Box=[&](double X,double Y,double Width,double Height,const FLinearColor& C)
    { const auto R=Layout.At(X,Y,Width,Height);DrawRect(C,R.X,R.Y,R.W,R.H); };
    auto Text=[&](const FString& T,double X,double Y,double Size,const FLinearColor& C,
        double Width=0,bool Bold=false,double Condense=1,bool Centered=false)
    {
        const auto R=Layout.At(X,Y,0,0);
        MenuText(T,R.X,R.Y,Size*U,C,Width*U,Bold,Condense,Centered);
    };
    auto Key=[&](const FString& T,double X,double Y,double Width)
    {
        Box(X,Y+3,Width,32,FLinearColor(0,0,0,0.8f));
        Box(X,Y,Width,32,KeyFace);Box(X,Y,Width,1,Gold);
        Text(T,X+Width*0.5,Y+8,16,White,Width-12,true,1,true);
    };
    auto Card=[&](double X,double Width)
    {
        Box(X,212,Width,330,TTGGMenu::PanelFill);Box(X,212,Width,2,Gold);
        Box(X,540,Width,1,Edge);Box(X,212,1,330,Edge);Box(X+Width-1,212,1,330,Edge);
    };

    Box(0,2,34,3,Gold);
    Text(TEXT("ROBOT WARFARE / CLOCK BRIDGES / SURVIVE"),48,0,15,Muted,730,true,1);
    Text(TEXT("TICK TOCK:"),-2,25,56,White,780,true,0.9);
    Text(TEXT("GUNS & GEARS"),-4,82,79,Gold,920,true,0.74);
    Text(TEXT("OUTGUN THE ROBOTS. MASTER THE CLOCK BRIDGES."),0,168,15,White,900,true,0.9);
    Text(TEXT("CONTROL BRIEFING / YOUR RUN BEGINS WHEN YOU CHOOSE START"),0,193,14,Muted,1160,true,1);

    Card(0,560);Card(584,576);
    Text(TEXT("01  MOVEMENT"),24,232,20,Gold,510,true,1);
    Text(TEXT("02  GUNFIRE & FIELD COMMANDS"),608,232,20,Gold,528,true,1);

    const TCHAR* MoveKeys[]={TEXT("W"),TEXT("A"),TEXT("S"),TEXT("D")};
    const TCHAR* MoveLabels[]={TEXT("FORWARD"),TEXT("LEFT"),TEXT("BACK"),TEXT("RIGHT")};
    for (int32 I=0;I<4;++I)
    {
        const float X=24+I*128;
        Key(MoveKeys[I],X+24,277,64);
        Text(MoveLabels[I],X+56,319,14,Muted,112,true,1,true);
    }
    const TCHAR* MovementKeys[]={TEXT("SPACE"),TEXT("SHIFT + W"),TEXT("C"),TEXT("MOUSE")};
    const TCHAR* MovementActions[]={TEXT("Jump"),TEXT("Sprint (Run Faster)"),TEXT("Crouch"),TEXT("Look around")};
    for (int32 I=0;I<4;++I)
    {
        const float Y=350+I*42;Key(MovementKeys[I],24,Y,118);
        Text(MovementActions[I],164,Y+7,20,White,372);
    }
    const TCHAR* CombatKeys[]={TEXT("LEFT MOUSE"),TEXT("RIGHT MOUSE"),TEXT("R"),TEXT("P"),TEXT("F1"),TEXT("BACKSPACE")};
    const TCHAR* CombatActions[]={TEXT("Shoot"),TEXT("Aim"),TEXT("Reload / refill magazine"),TEXT("Pause / resume"),TEXT("Open the field guide"),TEXT("Retry saved checkpoint")};
    for (int32 I=0;I<6;++I)
    {
        const float Y=277+I*42;Key(CombatKeys[I],608,Y,132);
        Text(CombatActions[I],764,Y+7,20,White,372);
    }

    Box(0,553,3,24,Gold);
    Text(TEXT("CLEAR ROBOT WAVES  /  FOLLOW FLAGS  /  RIDE CLOCK HANDS  /  REACH THE EXIT"),16,558,15,White,1144,true,1);
    const auto Start=Layout.Start();float MX=-1,MY=-1;
    const bool Hover=PlayerOwner && PlayerOwner->GetMousePosition(MX,MY) && Start.Contains(MX,MY);
    DrawRect(FLinearColor(0,0,0,0.65f),Start.X,Start.Y+4*U,Start.W,Start.H);
    DrawRect(Hover ? BrightGold : Gold,Start.X,Start.Y,Start.W,Start.H);
    const FString Caption=G->StartFlow.State==CAInterface::Phase::Armed ? TEXT("RELEASE TO START") : TEXT("START RUN  /  ENTER");
    MenuText(Caption,Start.X+Start.W*0.5f,Start.Y+18*U,23*U,Ink,Start.W-32*U,true,0.86f,true);
    AddHitBox(FVector2D(Start.X,Start.Y),FVector2D(Start.W,Start.H),TEXT("CA_StartRun"),true,100);
    Text(TEXT("CLICK START OR PRESS ENTER.  /  F1 OPENS THE CONTROLS GUIDE DURING PLAY."),580,667,14,Muted,1160,true,1,true);
}

void ACACombatHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if (BoxName==TEXT("CA_StartRun"))
        if (ACACombatGameMode* G=CA::Game(GetWorld())) G->RequestStart();
}
