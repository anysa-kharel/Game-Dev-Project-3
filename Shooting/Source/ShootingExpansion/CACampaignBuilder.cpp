#include "CAArenaBuilder.h"
#include "CAArenaActors.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "CampaignRules.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

namespace CACampaignConstruction
{
    void Mark(AActor* Actor,const TCHAR* Folder)
    {
        if (!Actor) return;
        Actor->Tags.AddUnique(TEXT("CAExpansion"));
#if WITH_EDITOR
        Actor->SetFolderPath(FName(Folder));
#endif
    }
}
void ACAArenaBuilder::BuildCampaign()
{
    if (!GetWorld()) return;
    TArray<AActor*> Previous;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
        if (It->Tags.Contains(TEXT("CAExpansion"))) Previous.Add(*It);
    for (AActor* Actor : Previous) Actor->Destroy();
    bBuilt=false; bCampaignLayout=true;
    const FLinearColor Accents[]={CA::Cyan(),CA::Mint(),CA::Gold(),FLinearColor(0.65f,0.3f,1),CA::Gold()};
    const TCHAR* Tips[]={TEXT("RED GUARDS SHOOT / GOLD SCOUTS RUSH / USE COVER"),
        TEXT("VIOLET SKIRMISHERS STRAFE / TRACK THEM BETWEEN COVER"),
        TEXT("BLUE SHIELDS / FLANK THEM OR SHOOT WHILE THEY CHARGE"),
        TEXT("ORANGE BOMBERS / BACK AWAY FROM THE FLASHING CORE"),
        TEXT("THE WARDEN FIRES A FAN OF BOLTS / DODGE SIDEWAYS")};
    for (int32 S=0;S<Campaign::Sectors;++S)
    {
        const float X=S*Campaign::Spacing;
        const FLinearColor Accent=Accents[S];
        Room(X,Campaign::HalfRoom*2);
        if (S==0) Block(FVector(X-3640,0,250),FVector(80,3500,500),CA::Steel());
        for (int32 Side : {-1,1}) Gallery(X-1400,X+600,Side*1260);
        Sign(FVector(X-2400,0,520),FString::Printf(TEXT("%02d / %s"),S+1,UTF8_TO_TCHAR(Campaign::Names[S])),52,Accent);
        Sign(FVector(X-2350,0,390),Tips[S],25,CA::White());
        for (const auto& C : Campaign::CoverLayout)
        {
            Block(FVector(X+C.X,C.Y,C.Z),FVector(C.L,C.W,C.H),C.H<150 ? Accent : CA::Steel());
            Block(FVector(X+C.X-C.L*0.5-3,C.Y,C.Z+C.H*0.5-16),FVector(5,C.W-18,20),Accent,false,0.7f);
        }
        // Side routes have different silhouettes and landmarks in every sector.
        for (int32 Side : {-1,1})
            for (int32 N=0;N<4;++N)
            {
                const float PX=X-2400+N*1600;
                if (S==1)
                {
                    Block(FVector(PX,Side*1610,110),FVector(230,170,220),CA::White());
                    Block(FVector(PX,Side*1610,290),FVector(240,170,240),CA::Mint(),false,0.1f,FRotator::ZeroRotator,TEXT("Sphere"));
                }
                else if (S==3)
                {
                    Block(FVector(PX,Side*1610,600),FVector(30,30,1200),CA::Steel(),false,0,FRotator::ZeroRotator,TEXT("Cylinder"));
                    Block(FVector(PX,Side*1610,1200),FVector(65),Accent,false,1.2f,FRotator::ZeroRotator,TEXT("Sphere"));
                }
                else
                {
                    Block(FVector(PX,Side*1630,500),FVector(100,80,1000),CA::Steel(),false);
                    Block(FVector(PX,Side*1575,500),FVector(25,15,600),Accent,false,0.7f);
                }
            }
        if (S==2)
        {
            Block(FVector(X+300,0,250),FVector(500,450,500),CA::Steel());
            Block(FVector(X+300,0,550),FVector(650,600,100),CA::Gold());
            Block(FVector(X+300,0,790),FVector(230,230,400),CA::Steel(),true,0,FRotator::ZeroRotator,TEXT("Cylinder"));
            Sign(FVector(X-50,0,650),TEXT("GEAR CORE / USE BOTH FLANKS"),32,CA::Gold());
        }
        if (S==4)
            for (int32 Side : {-1,1})
                Block(FVector(X+3000,Side*1500,700),FVector(180,200,1400),CA::Gold(),false);
        // Outer silhouettes give each room depth without enclosing the daylight sky.
        for (int32 Side : {-1,1})
            for (int32 N=0;N<4;++N)
                Block(FVector(X-3000+N*2200,Side*2800,350),FVector(500,600,1100+N*120),CA::Navy(),false,0.05f);

        for (int32 Side : {-1,1})
            Block(FVector(X+3620,Side*1120,250),FVector(80,1200,500),CA::Steel());
        if (S==Campaign::Sectors-1) continue;
        Floor(X+4500,1800,1000);
        Rail(X+4500,1800,-520); Rail(X+4500,1800,520);
        Checkpoint(X+4950,S*2+1);
        ACAExitDoor* Gate=GetWorld()->SpawnActor<ACAExitDoor>(ACAExitDoor::StaticClass(),FVector(X+3660,0,0),FRotator::ZeroRotator);
        if (Gate) { Gate->RequiredSection=S+1; Gate->OnConstruction(Gate->GetActorTransform()); }
        CACampaignConstruction::Mark(Gate,TEXT("ClockCampaign/SectorGates"));
        Sign(FVector(X+3550,0,550),TEXT("CLEAR BOTH WAVES / CLOCK BRIDGE AHEAD"),30,Accent);
        const FTransform BT(FRotator::ZeroRotator,FVector(X+Campaign::ClockOffset,0,0));
        ACAClockBridge* Bridge=GetWorld()->SpawnActorDeferred<ACAClockBridge>(ACAClockBridge::StaticClass(),BT,this,nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Bridge)
        {
            Bridge->SectionBefore=S+1; Bridge->Period=S==1 ? 24 : S==2 ? 32 : S==3 ? 26 : 28;
            Bridge->HandWidth=S==2 ? 300 : 260;
            Bridge->FinishSpawning(BT);
        }
        CACampaignConstruction::Mark(Bridge,TEXT("ClockCampaign/ClockBridges"));
        for (int32 Tick=0;Tick<60;++Tick)
        {
            const float A=Tick*6.0f,R=FMath::DegreesToRadians(A);
            Block(FVector(X+Campaign::ClockOffset+FMath::Cos(R)*1250,FMath::Sin(R)*1250,-83),
                FVector(Tick%5==0 ? 100 : 45,Tick%5==0 ? 18 : 9,8),Accent,false,0.7f,FRotator(0,A,0));
        }
        const TCHAR* Numbers[]={TEXT("3"),TEXT("6"),TEXT("9"),TEXT("12")};
        for (int32 N=0;N<4;++N)
        {
            const float A=FMath::DegreesToRadians(N*90.0f);
            Sign(FVector(X+Campaign::ClockOffset+FMath::Cos(A)*930,FMath::Sin(A)*930,-80),Numbers[N],100,CA::White(),FRotator(90,0,0));
        }
        Floor(X+8150,1500,1000);
        Rail(X+8150,1500,-520); Rail(X+8150,1500,520);
        Checkpoint(X+8100,S*2+2);
        Sign(FVector(X+4800,0,570),FString::Printf(TEXT("CLOCK %d / BOARD AT 9 / RIDE TO 3"),S+1),32,CA::Gold());
        Sign(FVector(X+8000,0,540),TEXT("CHECKPOINT / HEALTH + MAGAZINE RESTORED"),26,CA::Mint());
    }
    for (const auto& P : Campaign::Spawns)
    {
        const FTransform T(FRotator(0,180,0),FVector(P.X,P.Y,P.Z));
        ACAEnemy* Enemy=GetWorld()->SpawnActorDeferred<ACAEnemy>(ACAEnemy::StaticClass(),T,this,nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Enemy) continue;
        Enemy->Kind=static_cast<ECAEnemyKind>(P.Kind); Enemy->Section=P.Section; Enemy->Wave=P.Wave;
        Enemy->MaxHits=Campaign::Profiles[P.Kind].Hits;
        Enemy->FinishSpawning(T);
        CACampaignConstruction::Mark(Enemy,TEXT("ClockCampaign/Enemies"));
    }
    Floor(54050,900,1000); Rail(54050,900,-520); Rail(54050,900,520);
    ACAExitDoor* ExitGate=GetWorld()->SpawnActor<ACAExitDoor>(ACAExitDoor::StaticClass(),FVector(53660,0,0),FRotator::ZeroRotator);
    CACampaignConstruction::Mark(ExitGate,TEXT("ClockCampaign/Extraction"));
    const FTransform ET(FRotator::ZeroRotator,FVector(Campaign::ExitX,0,200));
    ACAZone* Exit=GetWorld()->SpawnActorDeferred<ACAZone>(ACAZone::StaticClass(),ET,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Exit) { Exit->Kind=ECAZoneKind::Exit; Exit->FinishSpawning(ET); }
    CACampaignConstruction::Mark(Exit,TEXT("ClockCampaign/Extraction"));
    Sign(FVector(53750,0,570),TEXT("FINAL EXTRACTION / DEFEAT THE WARDEN AND ALL ROBOTS"),32,CA::Mint());
    Block(FVector(54600,0,260),FVector(80,1100,520),CA::White());
    APlayerStart* Start=GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),FVector(Campaign::StartX,0,110),FRotator::ZeroRotator);
    CACampaignConstruction::Mark(Start,TEXT("ClockCampaign/PlayerStart"));
    bBuilt=true;
    RefreshPresentation();
}
