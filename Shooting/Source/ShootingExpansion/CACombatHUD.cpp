#include "CACombatHUD.h"
#include "CACombatGameMode.h"
#include "CAArenaActors.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void ACACombatHUD::Label(const FString& Value,float X,float Y,float Size,const FLinearColor& Color)
{
    DrawText(Value,Color,X,Y,GEngine->GetMediumFont(),Size/16*S,false);
}
void ACACombatHUD::Center(const FString& Value,float Y,float Size,const FLinearColor& Color)
{
    float TW=0,TH=0;
    GetTextSize(Value,TW,TH,GEngine->GetMediumFont(),Size/16*S);
    Label(Value,(W-TW)*0.5f,Y,Size,Color);
}
void ACACombatHUD::Panel(float X,float Y,float Width,float Height,const FLinearColor& Accent)
{
    DrawRect(FLinearColor(0.01f,0.025f,0.04f,0.92f),X,Y,Width,Height);
    DrawRect(Accent,X,Y,3*S,Height);
}
void ACACombatHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn) return;
    W=Canvas->SizeX; H=Canvas->SizeY;
    S=FMath::Clamp(FMath::Min(W/1600.0f,H/900.0f),0.5f,2.0f);
    Panel(28*S,28*S,830*S,92*S,CA::Cyan());
    Label(TEXT("CLOCK ARENA / FPS TEMPLATE EXPANSION"),46*S,40*S,13,CA::Cyan());
    Label(G->Objective(),46*S,70*S,22,CA::White());
    Label(FString::Printf(TEXT("HOSTILES %d / %d"),G->RemainingEnemies(),G->TotalEnemies),
        W-230*S,32*S,20,CA::Gold());
    const int32 T=static_cast<int32>(G->RunSeconds);
    Label(FString::Printf(TEXT("%02d:%02d / CHECKPOINT %d"),T/60,T%60,G->CheckpointIndex),
        W-260*S,68*S,14,CA::White());

    Panel(28*S,H-125*S,330*S,94*S,CA::Mint());
    const float Health=static_cast<float>(G->PlayerHealth.Value);
    Label(FString::Printf(TEXT("HEALTH / %.0f"),Health),46*S,H-113*S,22,
        Health<30 ? CA::Red() : CA::White());
    DrawRect(CA::Navy(),46*S,H-73*S,290*S,12*S);
    DrawRect(Health<30 ? CA::Red() : CA::Mint(),46*S,H-73*S,290*S*Health/100,12*S);
    Label(G->PlayerHealth.Quiet>=6 && Health<100 ? TEXT("RECOVERING IN COVER")
        : TEXT("Hide for 6 seconds to recover health"),46*S,H-49*S,11,CA::Cyan());
    Label(TEXT("P / PAUSE    F1 / HELP    BACKSPACE / RETRY"),W*0.37f,H-38*S,12,CA::White());

    const float CX=W*0.5f,CY=H*0.5f;
    DrawLine(CX-8*S,CY,CX-3*S,CY,CA::White(),1.2f*S);
    DrawLine(CX+3*S,CY,CX+8*S,CY,CA::White(),1.2f*S);
    DrawLine(CX,CY-8*S,CX,CY-3*S,CA::White(),1.2f*S);
    DrawLine(CX,CY+3*S,CX,CY+8*S,CA::White(),1.2f*S);
    if (G->HitMarker>0)
        for (int32 X : {-1,1}) for (int32 Y : {-1,1})
            DrawLine(CX+X*12*S,CY+Y*12*S,CX+X*20*S,CY+Y*20*S,CA::Gold(),2*S);

    FVector2D Point;
    if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(G->Waypoint(),Point,true))
    {
        Point.X=FMath::Clamp<double>(Point.X,60.0*S,W-190.0*S);
        Point.Y=FMath::Clamp<double>(Point.Y,155.0*S,H-200.0*S);
        DrawRect(CA::Cyan(),Point.X-4*S,Point.Y-4*S,8*S,8*S);
        Label(FString::Printf(TEXT("NEXT / %.0f m"),FVector::Dist(G->Waypoint(),G->PlayerPawn->GetActorLocation())/100),
            Point.X+14*S,Point.Y-8*S,12,CA::Cyan());
    }
    for (const ACAEnemy* Enemy : G->Enemies)
    {
        if (!IsValid(Enemy) || Enemy->bDead || !Enemy->bAlert ||
            FVector::DistSquared(Enemy->GetActorLocation(),G->PlayerPawn->GetActorLocation())>FMath::Square(2200.0)) continue;
        FVector2D Screen;
        if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(Enemy->GetActorLocation()+FVector(0,0,132),Screen,true))
            for (int32 I=0;I<Enemy->MaxHits;++I)
                DrawRect(I<Enemy->HitsLeft ? CA::Red() : CA::Navy(),
                    Screen.X+(I-Enemy->MaxHits*0.5f)*16*S,Screen.Y,12*S,6*S);
    }
    if (G->MessageTime>0)
    {
        DrawRect(FLinearColor(0.01f,0.025f,0.04f,0.92f),W*0.06f,H-197*S,W*0.88f,42*S);
        Center(G->Message,H-185*S,13,CA::White());
    }
    if (G->Clock && G->PlayerPawn->GetActorLocation().X>2450 && G->PlayerPawn->GetActorLocation().X<5700)
    {
        const bool Near=CombatRules::AtDock(G->Clock->Angle(),false);
        const bool Far=CombatRules::AtDock(G->Clock->Angle(),true);
        Center(Near ? TEXT("CLOCK HAND / BOARD AT THE 9 DOCK") : Far
            ? TEXT("CLOCK HAND / STEP OFF AT THE 3 DOCK") : TEXT("CLOCK HAND IN TRANSIT / STAY ABOARD"),
            H-240*S,16,CA::Gold());
    }
    if (G->DamageFlash>0)
    {
        const FLinearColor Color(0.95f,0.04f,0.02f,G->DamageFlash*1.5f);
        DrawRect(Color,0,0,W,10*S); DrawRect(Color,0,H-10*S,W,10*S);
        DrawRect(Color,0,0,10*S,H); DrawRect(Color,W-10*S,0,10*S,H);
    }
    if (G->bHelp)
    {
        const float X=W-525*S,Y=160*S;
        Panel(X,Y,490*S,320*S,CA::Gold());
        Label(TEXT("FIELD GUIDE"),X+18*S,Y+18*S,23,CA::Gold());
        const TCHAR* Lines[]={
            TEXT("Your template's shooting controls are unchanged."),
            TEXT("WASD move / MOUSE look / SPACE jump"),
            TEXT("LEFT CLICK shoot / RIGHT CLICK aim / R reload"),
            TEXT("SHIFT sprint / crouch behind low cover"),
            TEXT("RED guards shoot. GOLD scouts rush you."),
            TEXT("Their eyes turn gold before an attack."),
            TEXT("Crates and walls stop incoming projectiles."),
            TEXT("Hide for 6 seconds to begin healing."),
            TEXT("Board the GOLD hand at 9. Ride to the 3 dock."),
            TEXT("Clear both areas, then reach extraction."),
            TEXT("P pause / BACKSPACE retry / F1 close guide")
        };
        float YLine=Y+65*S;
        for (const TCHAR* Line : Lines) { Label(Line,X+18*S,YLine,12,CA::White()); YLine+=22*S; }
    }
    if (!G->PlayerHealth.Alive() || G->bPaused || G->bWon)
    {
        DrawRect(FLinearColor(0.005f,0.015f,0.025f,0.94f),0,0,W,H);
        Center(G->bWon ? TEXT("AREA SECURED") : G->bPaused ? TEXT("PAUSED")
            : TEXT("RETURNING TO CHECKPOINT"),H*0.31f,48,G->bWon ? CA::Mint() : CA::Gold());
        if (G->bWon)
        {
            Center(FString::Printf(TEXT("%d ROBOTS CLEARED / %02d:%02d / %d RETRIES"),
                G->Defeated,T/60,T%60,G->Retries),H*0.45f,22,CA::White());
            Center(TEXT("ENTER / PLAY AGAIN"),H*0.57f,22,CA::Cyan());
        }
        else if (G->bPaused)
        {
            Center(TEXT("P OR ENTER / RESUME"),H*0.47f,24,CA::Cyan());
            Center(TEXT("F1 toggles the field guide during play."),H*0.57f,16,CA::White());
        }
        else Center(TEXT("Defeated robots stay defeated. Your health and magazine will be restored."),
            H*0.48f,18,CA::White());
    }
}
