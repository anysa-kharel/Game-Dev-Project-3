#include "CAArenaBuilder.h"
#include "CAArenaActors.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "ArenaLayout.h"
#include "WayfindingRules.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

namespace CAConstruction
{
    void Tag(AActor* Actor,const TCHAR* Folder=TEXT("ClockArena/Environment"))
    {
        if (!Actor) return;
        Actor->Tags.AddUnique(TEXT("CAExpansion"));
#if WITH_EDITOR
        Actor->SetFolderPath(FName(Folder));
#endif
    }
}
ACAArenaBuilder::ACAArenaBuilder()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
void ACAArenaBuilder::Block(const FVector& Location,const FVector& Size,
    const FLinearColor& Color,bool Solid,float Glow,const FRotator& Rotation,const TCHAR* Shape)
{
    const FTransform Transform(Rotation,Location);
    ACAScenery* Actor=GetWorld()->SpawnActorDeferred<ACAScenery>(ACAScenery::StaticClass(),
        Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Actor) return;
    Actor->Dimensions=Size; Actor->Color=Color; Actor->bSolid=Solid;
    Actor->Glow=Glow; Actor->Shape=Shape;
    Actor->FinishSpawning(Transform);
    CAConstruction::Tag(Actor);
}
void ACAArenaBuilder::Sign(const FVector& Location,const FString& Label,float Size,
    const FLinearColor& Color,const FRotator& Rotation)
{
    ATextRenderActor* Actor=GetWorld()->SpawnActor<ATextRenderActor>(
        ATextRenderActor::StaticClass(),Location,Rotation);
    if (!Actor) return;
    UTextRenderComponent* Text=Actor->GetTextRender();
    Text->SetText(FText::FromString(Label)); Text->SetWorldSize(Size);
    Text->SetHorizontalAlignment(EHTA_Center); Text->SetVerticalAlignment(EVRTA_TextCenter);
    Text->SetTextRenderColor(Color.ToFColor(true));
    CAConstruction::Tag(Actor,TEXT("ClockArena/Signs"));
}
void ACAArenaBuilder::Floor(float X,float Length,float Width,float Height)
{
    Block(FVector(X,0,Height-35),FVector(Length,Width,70),CA::Steel());
    for (int32 Side : {-1,1})
        Block(FVector(X,Side*(Width*0.5f-16),Height+2),FVector(Length,12,4),CA::Cyan(),false,0.8f);
}
void ACAArenaBuilder::Rail(float X,float Length,float Y,float Height)
{
    Block(FVector(X,Y,Height+100),FVector(Length,26,200),CA::Navy());
    Block(FVector(X,Y,Height+204),FVector(Length,16,8),CA::Cyan(),false,0.8f);
}
void ACAArenaBuilder::Room(float X,float Length)
{
    Floor(X,Length);
    for (int32 Side : {-1,1})
    {
        Block(FVector(X,Side*1720,250),FVector(Length+80,40,500),CA::Navy());
        Block(FVector(X,Side*1695,490),FVector(Length,14,16),CA::Gold(),false,0.4f);
        for (float Column=X-Length*0.5f+300;Column<X+Length*0.5f;Column+=900)
        {
            Block(FVector(Column,Side*1655,360),FVector(85,85,720),CA::Steel());
            Block(FVector(Column,Side*1620,400),FVector(12,10,300),CA::Cyan(),false,1.3f);
        }
    }
    for (float Grid=X-Length*0.5f+400;Grid<X+Length*0.5f;Grid+=400)
        Block(FVector(Grid,0,1),FVector(3,3350,2),FLinearColor(0.19f,0.26f,0.3f),false);
    for (float Grid=-1200;Grid<=1200;Grid+=400)
        Block(FVector(X,Grid,1),FVector(Length,3,2),FLinearColor(0.19f,0.26f,0.3f),false);
}
void ACAArenaBuilder::Gallery(float Start,float End,float Y)
{
    const float Run=End-Start;
    const float Angle=FMath::RadiansToDegrees(FMath::Atan2(240.0f,Run));
    const float RampLength=FMath::Sqrt(Run*Run+240*240);
    const float CenterZ=120-20*FMath::Cos(FMath::DegreesToRadians(Angle));
    Block(FVector((Start+End)*0.5f,Y,CenterZ),FVector(RampLength,500,40),CA::Steel(),true,0,FRotator(Angle,0,0));
    Block(FVector(End+600,Y,215),FVector(1200,600,50),CA::Steel());
    Rail(End+600,1200,Y+(Y>0 ? 300 : -300),240);
    Block(FVector(End+600,Y,242),FVector(1150,18,4),CA::Cyan(),false,0.7f);
    for (float X : {End+100,End+1100})
        for (int32 Side : {-1,1})
            Block(FVector(X,Y+Side*250,100),FVector(45,45,200),CA::Navy());
}
void ACAArenaBuilder::Checkpoint(float X,int32 Index)
{
    const FTransform Transform(FRotator::ZeroRotator,FVector(X,0,200));
    ACAZone* Zone=GetWorld()->SpawnActorDeferred<ACAZone>(ACAZone::StaticClass(),Transform,this,
        nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Zone) return;
    Zone->CheckpointIndex=Index; Zone->FinishSpawning(Transform);
    CAConstruction::Tag(Zone,TEXT("ClockArena/Checkpoints"));
}
void ACAArenaBuilder::BuildArena()
{
    if (!GetWorld()) return;
    TArray<AActor*> Previous;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
        if (It->Tags.Contains(TEXT("CAExpansion"))) Previous.Add(*It);
    for (AActor* Actor : Previous) Actor->Destroy();
    bBuilt=false;

    // Loading yard: multiple cover lanes, two ramps and sheltered galleries.
    Room(0,4800);
    Block(FVector(-2435,0,280),FVector(70,3500,560),CA::Navy());
    for (int32 Side : {-1,1})
    {
        Gallery(-1000,1000,Side*1260);
        Block(FVector(2400,Side*1130,250),FVector(70,1140,500),CA::Navy());
    }
    Sign(FVector(-1800,0,500),TEXT("CLOCK ARENA / LOADING YARD"),54,CA::White());
    Sign(FVector(-1740,0,410),TEXT("SHOOT THE ROBOTS. USE THE CRATES AND UPPER ROUTES."),24,CA::Cyan());
    Sign(FVector(-750,-1250,130),TEXT("RAMP / HIGH GROUND"),26,CA::Gold());
    Floor(2800,800,1000);
    Rail(2800,800,-520); Rail(2800,800,520);
    Checkpoint(2760,1);
    Sign(FVector(2690,0,460),TEXT("CLOCK BRIDGE / BOARD THE GOLD HAND"),34,CA::Gold());
    Sign(FVector(2750,0,375),TEXT("WAIT AT 9. RIDE TO 3. STEP OFF AT THE FAR DOCK."),23,CA::Cyan());

    // An open dial keeps the gap visible: the gold hand is the walkable bridge.
    ACAClockBridge* Bridge=GetWorld()->SpawnActor<ACAClockBridge>(ACAClockBridge::StaticClass(),
        FVector(4200,0,0),FRotator::ZeroRotator);
    CAConstruction::Tag(Bridge,TEXT("ClockArena/ClockBridge"));
    for (int32 Tick=0;Tick<60;++Tick)
    {
        const float A=Tick*6.0f;
        const float R=FMath::DegreesToRadians(A);
        Block(FVector(4200+FMath::Cos(R)*1250,FMath::Sin(R)*1250,-83),
            FVector(Tick%5==0 ? 100 : 45,Tick%5==0 ? 18 : 9,8),
            Tick%5==0 ? CA::Gold() : CA::Cyan(),false,0.7f,FRotator(0,A,0));
    }
    const TCHAR* Numbers[]={TEXT("3"),TEXT("6"),TEXT("9"),TEXT("12")};
    for (int32 I=0;I<4;++I)
    {
        const float A=FMath::DegreesToRadians(I*90.0f);
        Sign(FVector(4200+FMath::Cos(A)*930,FMath::Sin(A)*930,-80),Numbers[I],100,
            CA::White(),FRotator(90,0,0));
    }
    Block(FVector(4200,190,-75),FVector(32,450,14),CA::White(),false,0.5f,FRotator(0,-30,0));
    Floor(5500,600,1000);
    Rail(5500,600,-520); Rail(5500,600,520);
    Block(FVector(5290,0,3),FVector(30,900,6),CA::Mint(),false,1);
    Sign(FVector(5450,0,440),TEXT("FAR DOCK / FOUNDRY AHEAD"),35,CA::Mint());
    Checkpoint(5680,2);

    // Foundry: a central machine splits the ground route, with elevated flanks.
    Room(7900,4200);
    for (int32 Side : {-1,1})
    {
        Gallery(6200,8000,Side*1260);
        Block(FVector(9500,Side*1130,250),FVector(70,1260,500),CA::Navy());
        Block(FVector(5910,Side*700,160),FVector(340,70,320),CA::Steel());
        Block(FVector(5910,Side*700,325),FVector(340,80,10),CA::Cyan(),false,1);
    }
    Sign(FVector(6060,0,520),TEXT("FOUNDRY / KEEP MOVING. USE COVER."),42,CA::White());
    Block(FVector(7750,0,535),FVector(500,500,70),CA::Gold());
    Block(FVector(7750,0,740),FVector(210,210,360),CA::Steel(),true,0,FRotator::ZeroRotator,TEXT("Cylinder"));
    Sign(FVector(7470,0,350),TEXT("CORE 07"),48,CA::Gold());
    Floor(9720,500,1000);
    ACAExitDoor* Door=GetWorld()->SpawnActor<ACAExitDoor>(ACAExitDoor::StaticClass(),
        FVector(9500,0,0),FRotator::ZeroRotator);
    CAConstruction::Tag(Door,TEXT("ClockArena/Extraction"));
    Sign(FVector(9460,0,470),TEXT("EXTRACTION / CLEAR ALL HOSTILES"),33,CA::Mint());
    const FTransform ExitTransform(FRotator::ZeroRotator,FVector(9780,0,200));
    ACAZone* Exit=GetWorld()->SpawnActorDeferred<ACAZone>(ACAZone::StaticClass(),ExitTransform,
        this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Exit)
    {
        Exit->Kind=ECAZoneKind::Exit; Exit->FinishSpawning(ExitTransform);
        CAConstruction::Tag(Exit,TEXT("ClockArena/Extraction"));
    }
    Block(FVector(10030,0,260),FVector(70,3500,520),CA::Navy());
    Sign(FVector(9990,0,240),TEXT("SAFE ZONE"),70,CA::Mint());

    for (const ArenaLayout::CoverBox& C : ArenaLayout::Cover)
    {
        Block(FVector(C.X,C.Y,C.Z),FVector(C.L,C.W,C.H),C.H<150 ? CA::Gold() : CA::Steel());
        Block(FVector(C.X-C.L*0.5-2,C.Y,C.Z+C.H*0.22),FVector(4,C.W-20,12),CA::Cyan(),false,0.8f);
        for (int32 Side : {-1,1})
            Block(FVector(C.X,C.Y+Side*(C.W*0.5-4),C.Z+C.H*0.5+3),
                FVector(C.L-15,8,6),CA::Gold(),false,0.5f);
    }
    for (const ArenaLayout::Spawn& S : ArenaLayout::Enemies)
    {
        const FTransform Transform(FRotator(0,180,0),FVector(S.X,S.Y,S.Z));
        ACAEnemy* Enemy=GetWorld()->SpawnActorDeferred<ACAEnemy>(ACAEnemy::StaticClass(),Transform,
            this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Enemy) continue;
        Enemy->Kind=static_cast<ECAEnemyKind>(S.Kind); Enemy->Section=S.Section;
        Enemy->MaxHits=S.Kind==1 ? 2 : 3; Enemy->FinishSpawning(Transform);
        CAConstruction::Tag(Enemy,TEXT("ClockArena/Enemies"));
    }

    // Industrial silhouettes and gantries make a contained arena feel larger.
    for (int32 Side : {-1,1})
    {
        for (int32 I=0;I<11;++I)
        {
            const float X=-2600+I*1150.0f;
            const float H=900+(I%3)*430;
            Block(FVector(X,Side*(2600+(I%2)*450),H*0.5f-450),FVector(500,650,H),
                CA::Navy(),false,0.15f);
            Block(FVector(X,Side*(2570+(I%2)*450),H-320),FVector(530,680,30),
                CA::Gold(),false,0.6f);
        }
    }
    Block(FVector(3700,0,-1700),FVector(22000,14000,80),CA::Navy(),false,0.2f);
    for (float X : {0.0f,1800.0f,6500.0f,9000.0f})
    {
        for (int32 Side : {-1,1})
            Block(FVector(X,Side*1770,660),FVector(100,100,1320),CA::Steel(),false);
        Block(FVector(X,0,1310),FVector(100,3650,100),CA::Steel(),false);
        Block(FVector(X,0,1250),FVector(20,3350,16),CA::Cyan(),false,1.5f);
    }
    APlayerStart* Start=GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),
        FVector(-2150,0,110),FRotator::ZeroRotator);
    CAConstruction::Tag(Start,TEXT("ClockArena/PlayerStart"));
    bBuilt=true;
    RefreshPresentation();
}

void ACAArenaBuilder::RefreshPresentation()
{
    if (!GetWorld()) return;
    // One sun: forward shading does not use two directional lights as key/fill.
    ADirectionalLight* Sun=nullptr;
    TArray<ADirectionalLight*> OldSuns;
    for (TActorIterator<ADirectionalLight> It(GetWorld());It;++It)
        if (It->Tags.Contains(TEXT("CAExpansion"))) OldSuns.Add(*It);
    if (OldSuns.Num()>0) Sun=OldSuns[0];
    else Sun=GetWorld()->SpawnActor<ADirectionalLight>();
    for (ADirectionalLight* Light : OldSuns) if (Light!=Sun) Light->Destroy();
    if (Sun)
    {
        if (UDirectionalLightComponent* Light=Sun->FindComponentByClass<UDirectionalLightComponent>())
        {
            Light->SetMobility(EComponentMobility::Movable);
            Sun->SetActorRotation(FRotator(-55,-35,0));
            Light->SetForwardShadingPriority(10);
            Light->SetAtmosphereSunLight(true);
            Light->SetIntensity(Wayfinding::SunIntensity);
            Light->SetLightColor(FLinearColor(1,0.98f,0.94f));
            Light->SetCastShadows(true);
        }
        CAConstruction::Tag(Sun,TEXT("ClockArena/Lighting"));
#if WITH_EDITOR
        Sun->SetActorLabel(TEXT("Daylight Sun / Priority 10"));
#endif
    }
    ASkyAtmosphere* Atmosphere=nullptr;
    for (TActorIterator<ASkyAtmosphere> It(GetWorld());It;++It)
        if (!Atmosphere) Atmosphere=*It;
    if (!Atmosphere) Atmosphere=GetWorld()->SpawnActor<ASkyAtmosphere>();
    CAConstruction::Tag(Atmosphere,TEXT("ClockArena/Lighting"));
    ASkyLight* Sky=nullptr;
    for (TActorIterator<ASkyLight> It(GetWorld());It;++It) if (!Sky) Sky=*It;
    if (!Sky) Sky=GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        if (USkyLightComponent* Light=Sky->FindComponentByClass<USkyLightComponent>())
        {
            Light->SetMobility(EComponentMobility::Movable);
            Light->SourceType=SLS_CapturedScene;
            Light->SetIntensity(Wayfinding::SkyIntensity);
            Light->SetLightColor(FLinearColor(0.94f,0.98f,1));
            // A light neutral lower hemisphere fills areas below the galleries.
            Light->bLowerHemisphereIsBlack=true;
            Light->LowerHemisphereColor=FLinearColor(0.35f,0.39f,0.43f);
            Light->SetCastShadows(false);
            Light->SetRealTimeCapture(true);
            Light->RecaptureSky();
        }
        CAConstruction::Tag(Sky,TEXT("ClockArena/Lighting"));
    }
    APostProcessVolume* Exposure=nullptr;
    for (TActorIterator<APostProcessVolume> It(GetWorld());It;++It)
        if (It->Tags.Contains(TEXT("CAVisibility"))) { Exposure=*It; break; }
    if (!Exposure) Exposure=GetWorld()->SpawnActor<APostProcessVolume>();
    if (Exposure)
    {
        Exposure->Tags.AddUnique(TEXT("CAVisibility"));
        CAConstruction::Tag(Exposure,TEXT("ClockArena/Lighting"));
        Exposure->bUnbound=true; Exposure->Priority=100; Exposure->BlendWeight=1;
        FPostProcessSettings& Settings=Exposure->Settings;
        Settings.bOverride_AutoExposureMinBrightness=true; Settings.AutoExposureMinBrightness=1;
        Settings.bOverride_AutoExposureMaxBrightness=true; Settings.AutoExposureMaxBrightness=1;
        Settings.bOverride_AutoExposureBias=true; Settings.AutoExposureBias=0;
        Settings.bOverride_AmbientOcclusionIntensity=true; Settings.AmbientOcclusionIntensity=0.1f;
        Settings.bOverride_VignetteIntensity=true; Settings.VignetteIntensity=0;
        Settings.bOverride_BloomIntensity=true; Settings.BloomIntensity=0.15f;
    }
    TArray<ACAScenery*> OldArrows;
    for (TActorIterator<ACAScenery> It(GetWorld());It;++It)
    {
        ACAScenery* Item=*It;
        if (!Item->Tags.Contains(TEXT("CAExpansion"))) continue;
        if (Item->Tags.Contains(TEXT("CARouteArrow"))) { OldArrows.Add(Item); continue; }
        // Replace only the original oversized black backdrop, not the user's walls.
        if (!Item->bSolid && Item->Dimensions.Equals(FVector(22000,100,6000),0.1f))
        { Item->Destroy(); continue; }
        if (Item->Color.Equals(FLinearColor(0.025f,0.05f,0.08f),0.001f)) Item->Color=CA::Navy();
        else if (Item->Color.Equals(FLinearColor(0.12f,0.18f,0.23f),0.001f)) Item->Color=CA::Steel();
        Item->OnConstruction(Item->GetActorTransform());
        if (!Item->bSolid) Item->Mesh->SetCastShadow(false);
    }
    // This owns only the new decorative arrows. Gameplay actors and map edits stay put.
    for (ACAScenery* Arrow : OldArrows) Arrow->Destroy();
    for (const auto& Arrow : Wayfinding::Arrows)
    {
        const FRotator Rotation(0,Arrow.Yaw,0);
        const FVector Origin(Arrow.X,Arrow.Y,6);
        auto Stripe=[&](const FVector& Offset,const FVector& Size,float Yaw)
        {
            const FTransform Transform(FRotator(0,Arrow.Yaw+Yaw,0),Origin+Rotation.RotateVector(Offset));
            ACAScenery* Part=GetWorld()->SpawnActorDeferred<ACAScenery>(ACAScenery::StaticClass(),Transform,
                this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!Part) return;
            Part->Dimensions=Size; Part->Color=CA::Gold(); Part->Glow=0.55f; Part->bSolid=false;
            Part->FinishSpawning(Transform);
            Part->Mesh->SetCastShadow(false);
            Part->Tags.AddUnique(TEXT("CARouteArrow"));
            CAConstruction::Tag(Part,TEXT("ClockArena/RouteArrows"));
        };
        Stripe(FVector(-25,0,0),FVector(90,20,3),0);
        Stripe(FVector(25,-22,0),FVector(70,20,3),40);
        Stripe(FVector(25,22,0),FVector(70,20,3),-40);
    }
    for (TActorIterator<ACAZone> It(GetWorld());It;++It) It->RefreshMarker();
    for (TActorIterator<ACAClockBridge> It(GetWorld());It;++It) It->OnConstruction(It->GetActorTransform());
}
