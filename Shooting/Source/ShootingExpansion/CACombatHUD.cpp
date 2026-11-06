#include "CACombatHUD.h"
#include "CACombatGameMode.h"
#include "CAArenaActors.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "CanvasItem.h"
#include "CAInterfaceRules.h"
#include "CATheme.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"

void ACACombatHUD::Label(const FString& Value,float X,float Y,float Size,const FLinearColor& Color)
{
    MenuText(Value,X,Y,Size*S,Color,0,Size>=28,1,false);
}
void ACACombatHUD::Center(const FString& Value,float Y,float Size,const FLinearColor& Color)
{
    MenuText(Value,W*0.5f,Y,Size*S,Color,W-60*S,Size>=28,1,true);
}
void ACACombatHUD::Panel(float X,float Y,float Width,float Height,const FLinearColor& Accent)
{
    DrawRect(CATheme::PanelFill(),X,Y,Width,Height);
    DrawRect(Accent,X,Y,2*S,Height);
}
void ACACombatHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn) return;
    W=Canvas->SizeX; H=Canvas->SizeY;
    S=FMath::Clamp(FMath::Min(W/1600.0f,H/900.0f),0.5f,2.0f);
    if (G->StartFlow.Waiting()) { DrawControls();return; }
    Panel(28*S,28*S,830*S,92*S,CATheme::Gold());
    Label(TEXT("TICK TOCK: GUNS & GEARS"),46*S,40*S,13,CATheme::Gold());
    Label(G->Objective(),46*S,70*S,22,CATheme::White());
    Label(FString::Printf(TEXT("ELIMINATED %d / %d"),G->Defeated,G->TotalEnemies),
        W-230*S,32*S,20,CATheme::Gold());
    const int32 T=static_cast<int32>(G->RunSeconds);
    Label(FString::Printf(TEXT("%02d:%02d / CHECKPOINT %d"),T/60,T%60,G->CheckpointIndex),
        W-260*S,68*S,14,CATheme::White());

    if (G->bCampaign)
    {
        Panel(28*S,132*S,830*S,68*S,CATheme::Gold());
        Label(TEXT("CLEAR SQUAD > CHECKPOINT > CLOCK BRIDGE > NEXT SECTOR"),46*S,141*S,12,CATheme::Gold());
        Label(FString::Printf(TEXT("SECTOR %d / 5    WAVE %d / 2    FLAGS %d / 8    ROBOTS HERE %d"),
            G->ActiveSection(),G->ActiveWave()+1,G->CheckpointIndex,G->RemainingInSection(G->ActiveSection())),
            46*S,169*S,13,CATheme::White());
    }
    else
    {
    Panel(28*S,132*S,830*S,68*S,CATheme::Gold());
    Label(TEXT("ROUTE / GOLD ARROWS LEAD BETWEEN FLAGS"),46*S,141*S,12,CATheme::Gold());
    Label(G->CheckpointIndex>=1 ? TEXT("FLAG 1: SAVED") : TEXT("FLAG 1: CHECKPOINT"),
        46*S,169*S,13,G->CheckpointIndex>=1 ? CATheme::Highlight() : CATheme::Gold());
    Label(TEXT("> CLOCK HAND >"),260*S,169*S,13,CATheme::Gold());
    Label(G->CheckpointIndex>=2 ? TEXT("FLAG 2: SAVED") : TEXT("FLAG 2: CHECKPOINT"),
        437*S,169*S,13,G->CheckpointIndex>=2 ? CATheme::Highlight() : CATheme::Gold());
    Label(G->RemainingEnemies()==0 ? TEXT("> EXIT: OPEN") : TEXT("> EXIT: LOCKED"),
        668*S,169*S,13,G->RemainingEnemies()==0 ? CATheme::Highlight() : CATheme::Gold());

    }

    Panel(W-358*S,H-125*S,330*S,94*S,CATheme::Highlight());
    const float Health=static_cast<float>(G->PlayerHealth.Value);
    Label(FString::Printf(TEXT("HEALTH / %.0f%s"),Health,Health<30 ? TEXT(" / LOW") : TEXT("")),W-340*S,H-113*S,22,
        Health<30 ? CATheme::Highlight() : CATheme::White());
    DrawRect(CATheme::Ink(),W-340*S,H-73*S,290*S,12*S);
    DrawRect(Health<30 ? CATheme::Highlight() : CATheme::Gold(),W-340*S,H-73*S,290*S*Health/100,12*S);
    Label(G->PlayerHealth.Quiet>=6 && Health<100 ? TEXT("RECOVERING IN COVER")
        : TEXT("Hide for 6 seconds to recover health"),W-340*S,H-49*S,11,CATheme::Gold());
    Label(TEXT("P PAUSE   F1 GUIDE   M MUSIC   BACKSPACE RETRY"),W*0.37f,H-38*S,12,CATheme::White());

    const float CX=W*0.5f,CY=H*0.5f;
    DrawLine(CX-8*S,CY,CX-3*S,CY,CATheme::White(),1.2f*S);
    DrawLine(CX+3*S,CY,CX+8*S,CY,CATheme::White(),1.2f*S);
    DrawLine(CX,CY-8*S,CX,CY-3*S,CATheme::White(),1.2f*S);
    DrawLine(CX,CY+3*S,CX,CY+8*S,CATheme::White(),1.2f*S);
    if (G->HitMarker>0)
        for (int32 X : {-1,1}) for (int32 Y : {-1,1})
            DrawLine(CX+X*12*S,CY+Y*12*S,CX+X*20*S,CY+Y*20*S,CATheme::Gold(),2*S);

    if (PlayerOwner)
    {
        const FVector Target=G->Waypoint();
        FVector2D Point;
        const float Left=55*S,Right=W-55*S,Top=285*S,Bottom=H-260*S;
        const bool Projected=PlayerOwner->ProjectWorldLocationToScreen(Target,Point,true);
        const bool InView=Projected && Point.X>=Left && Point.X<=Right && Point.Y>=Top && Point.Y<=Bottom;
        bool TurnBack=false;
        const FLinearColor Color=G->GuidanceStage()==Wayfinding::Stage::Exit ? CATheme::Highlight() : CATheme::Gold();
        if (!InView)
        {
            FVector View; FRotator Rotation;
            PlayerOwner->GetPlayerViewPoint(View,Rotation);
            const FVector Delta=Target-View;
            const FRotationMatrix Axes(FRotator(0,Rotation.Yaw,0));
            const auto Direction=Wayfinding::Bearing(
                FVector::DotProduct(Delta,Axes.GetUnitAxis(EAxis::X)),
                FVector::DotProduct(Delta,Axes.GetUnitAxis(EAxis::Y)));
            TurnBack=Direction.TurnBack;
            const FVector2D CenterPoint((Left+Right)*0.5f,(Top+Bottom)*0.5f);
            const double HalfW=(Right-Left)*0.5,HalfH=(Bottom-Top)*0.5;
            const double Scale=FMath::Min(HalfW/FMath::Max(0.001,std::abs(Direction.X)),
                HalfH/FMath::Max(0.001,std::abs(Direction.Y)));
            Point=CenterPoint+FVector2D(Direction.X,Direction.Y)*Scale;
            const FVector2D D(Direction.X,Direction.Y),Perp(-Direction.Y,Direction.X);
            const FVector2D A=Point-D*15*S+Perp*7*S,B=Point-D*15*S-Perp*7*S;
            DrawLine(A.X,A.Y,Point.X,Point.Y,Color,3*S);
            DrawLine(B.X,B.Y,Point.X,Point.Y,Color,3*S);
        }
        else
        {
            DrawLine(Point.X,Point.Y-8*S,Point.X+8*S,Point.Y,Color,2*S);
            DrawLine(Point.X+8*S,Point.Y,Point.X,Point.Y+8*S,Color,2*S);
            DrawLine(Point.X,Point.Y+8*S,Point.X-8*S,Point.Y,Color,2*S);
            DrawLine(Point.X-8*S,Point.Y,Point.X,Point.Y-8*S,Color,2*S);
        }
        const float LabelX=FMath::Clamp<float>(Point.X-125*S,35*S,W-300*S);
        const FString Caption=(TurnBack ? TEXT("TURN BACK / ") : TEXT("NEXT / "))+G->WaypointLabel();
        float TextW=0,TextH=0;
        GetTextSize(Caption,TextW,TextH,GEngine->GetMediumFont(),12.0f/16*S);
        const float CaptionSize=FMath::Min(12.0f,12.0f*(248*S)/FMath::Max(TextW,1.0f));
        // Keep the card above the marker so it does not cover a targeted robot.
        Panel(LabelX,Point.Y-63*S,270*S,48*S,Color);
        Label(Caption,LabelX+10*S,Point.Y-57*S,CaptionSize,Color);
        Label(FString::Printf(TEXT("%.0f m"),FVector::Dist(Target,G->PlayerPawn->GetActorLocation())/100),
            LabelX+10*S,Point.Y-35*S,11,CATheme::White());
    }
    for (const ACAEnemy* Enemy : G->Enemies)
    {
        if (!IsValid(Enemy) || Enemy->bDead || !Enemy->bAlert ||
            FVector::DistSquared(Enemy->GetActorLocation(),G->PlayerPawn->GetActorLocation())>FMath::Square(2200.0)) continue;
        FVector2D Screen;
        if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(Enemy->GetActorLocation()+FVector(0,0,Enemy->Kind==ECAEnemyKind::Warden ? 190 : 132),Screen,true))
        {
            const FString Type=Enemy->TypeName();
            float TW=0,TH=0; GetTextSize(Type,TW,TH,GEngine->GetMediumFont(),11.0f/16*S);
            DrawRect(CATheme::Ink(0.94f),Screen.X-TW*0.5f-6*S,Screen.Y-24*S,TW+12*S,20*S);
            Label(Type,Screen.X-TW*0.5f,Screen.Y-21*S,11,Enemy->TypeColor());
            for (int32 I=0;I<Enemy->MaxHits;++I)
                DrawRect(I<Enemy->HitsLeft ? Enemy->TypeColor() : CATheme::Ink(),
                    Screen.X+(I-Enemy->MaxHits*0.5f)*16*S,Screen.Y,12*S,6*S);
        }
    }
    if (G->MessageTime>0)
    {
        DrawRect(CATheme::PanelFill(),W*0.06f,H-197*S,W*0.88f,42*S);
        Center(G->Message,H-185*S,13,CATheme::White());
    }
    if (G->Clock && G->bCampaign && G->Clock->SectionBefore==G->ActiveSection() &&
        G->CheckpointIndex<G->Clock->SectionBefore*2 && G->RemainingInSection(G->ActiveSection())==0 &&
        FMath::Abs(G->PlayerPawn->GetActorLocation().X-G->Clock->GetActorLocation().X)<2800)
    {
        Panel(28*S,H-265*S,W-56*S,55*S,CATheme::Gold());
        Center(G->Clock->LessonHint(),H-247*S,16,CATheme::White());
    }
    else if (!G->bCampaign && G->Clock && FMath::Abs(G->PlayerPawn->GetActorLocation().X-G->Clock->GetActorLocation().X)<G->Clock->HandLength+800)
    {
        const bool Near=CombatRules::AtDock(G->Clock->Angle(),false);
        const bool Far=CombatRules::AtDock(G->Clock->Angle(),true);
        const bool Returning=!G->bCampaign && G->GuidanceStage()==Wayfinding::Stage::ReturnToYard;
        Center(Near ? (Returning ? TEXT("STEP OFF AT 9 / FINISH THE YARD") : TEXT("BOARD THE GOLD HAND AT 9 / RIDE TO THE NEXT FLAG")) : Far
            ? (Returning ? TEXT("BOARD AT 3 / RIDE BACK TO THE YARD") : TEXT("STEP OFF AT 3 / WALK THROUGH THE NEXT FLAG"))
            : TEXT("CLOCK HAND IN TRANSIT / STAY ABOARD"),
            H-240*S,16,CATheme::Gold());
    }
    if (G->DamageFlash>0)
    {
        const FLinearColor Color=CATheme::Gold(G->DamageFlash*1.5f);
        DrawRect(Color,0,0,W,10*S); DrawRect(Color,0,H-10*S,W,10*S);
        DrawRect(Color,0,0,10*S,H); DrawRect(Color,W-10*S,0,10*S,H);
    }
    if (G->bHelp)
    {
        const float X=W-525*S,Y=160*S;
        Panel(X,Y,490*S,380*S,CATheme::Gold());
        Label(TEXT("FIELD GUIDE"),X+18*S,Y+18*S,23,CATheme::Gold());
        const TCHAR* Lines[]={
            TEXT("WASD move / MOUSE look / SPACE jump"),
            TEXT("LEFT CLICK shoot / RIGHT CLICK aim / R reload"),
            TEXT("SHIFT + W sprint / C crouch / M music"),
            TEXT("RED guards fire one bolt. GOLD rushers close in."),
            TEXT("CYAN sentries fire two bolts. Use the galleries."),
            TEXT("VIOLET skirmishers strafe. Track their movement."),
            TEXT("BLUE shields: flank or fire when they lower."),
            TEXT("ORANGE bombers: leave the warning circle."),
            TEXT("The crowned WARDEN fires a wide bolt fan."),
            TEXT("Clear waves / Follow flags / Reach the GOLD EXIT."),
            TEXT("SAVED flags restore health, ammo, and progress."),
            TEXT("Clocks: 1 RIDE / 2 CROUCH / 3 JUMP / 4 COMBINE."),
            TEXT("For gaps: keep W held, then SPACE at the gold tip."),
            TEXT("P pause / BACKSPACE retry / F1 close guide")
        };
        float YLine=Y+65*S;
        for (const TCHAR* Line : Lines) { Label(Line,X+18*S,YLine,12,CATheme::White()); YLine+=22*S; }
    }
    if (!G->PlayerHealth.Alive() || G->bPaused || G->bWon)
    {
        DrawRect(CATheme::Ink(0.97f),0,0,W,H);
        Center(G->bWon ? TEXT("AREA SECURED") : G->bPaused ? TEXT("PAUSED")
            : TEXT("RETURNING TO CHECKPOINT"),H*0.31f,48,G->bWon ? CATheme::Highlight() : CATheme::Gold());
        if (G->bWon)
        {
            Center(FString::Printf(TEXT("%d ROBOTS CLEARED / %02d:%02d / %d RETRIES"),
                G->Defeated,T/60,T%60,G->Retries),H*0.45f,22,CATheme::White());
            Center(TEXT("ENTER / PLAY AGAIN"),H*0.57f,22,CATheme::Gold());
        }
        else if (G->bPaused)
        {
            Center(TEXT("P OR ENTER / RESUME"),H*0.47f,24,CATheme::Gold());
            Center(TEXT("F1 toggles the field guide during play."),H*0.57f,16,CATheme::White());
        }
        else Center(TEXT("Defeated robots stay defeated. Your health and magazine will be restored."),
            H*0.48f,18,CATheme::White());
    }
}
