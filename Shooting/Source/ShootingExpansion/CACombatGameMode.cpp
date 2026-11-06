#include "CACombatGameMode.h"
#include "CAArenaActors.h"
#include "CAArenaBuilder.h"
#include "CACombatHUD.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include <cfloat>

ACACombatGameMode::ACACombatGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    // Use the uploaded template's actual player, gun, animations and controller.
    static ConstructorHelpers::FClassFinder<APawn> PawnBP(
        TEXT("/Game/InfimaGames/FreeFPSTemplate/Core/BP_Character"));
    static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(
        TEXT("/Game/InfimaGames/FreeFPSTemplate/Core/BP_Player_Controller"));
    static ConstructorHelpers::FClassFinder<AActor> BulletBP(
        TEXT("/Game/InfimaGames/FreeFPSTemplate/Core/BP_Bullet"));
    if (PawnBP.Succeeded()) DefaultPawnClass=PawnBP.Class;
    if (ControllerBP.Succeeded()) PlayerControllerClass=ControllerBP.Class;
    if (BulletBP.Succeeded()) TemplateBulletClass=BulletBP.Class;
    HUDClass=ACACombatHUD::StaticClass();
}
void ACACombatGameMode::BeginPlay()
{
    Super::BeginPlay();
    TActorIterator<ACAArenaBuilder> Existing(GetWorld());
    ACAArenaBuilder* Builder=Existing ? *Existing : nullptr;
    bCampaign=Builder ? Builder->bCampaignLayout : true;
    if (!Builder) Builder=GetWorld()->SpawnActor<ACAArenaBuilder>();
    if (Builder && !Builder->bBuilt)
    {
        if (bCampaign) Builder->BuildCampaign(); else Builder->BuildArena();
    }
    else if (Builder) Builder->RefreshPresentation();
    for (TActorIterator<ACAEnemy> It(GetWorld());It;++It) Enemies.Add(*It);
    TActorIterator<ACAClockBridge> Bridges(GetWorld());
    Clock=Bridges ? *Bridges : nullptr;
    for (TActorIterator<ACAClockBridge> It(GetWorld());It;++It) Clocks.Add(*It);
    TotalEnemies=Enemies.Num();
    BulletSpawnHandle=GetWorld()->AddOnActorSpawnedHandler(
        FOnActorSpawned::FDelegate::CreateUObject(this,&ACACombatGameMode::ActorSpawned));
}
void ACACombatGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld()) GetWorld()->RemoveOnActorSpawnedHandler(BulletSpawnHandle);
    Super::EndPlay(EndPlayReason);
}
void ACACombatGameMode::ActorSpawned(AActor* Actor)
{
    if (IsActive() && Actor && TemplateBulletClass && Actor->IsA(TemplateBulletClass))
        CA::Sound(GetWorld(),TEXT("S_PlayerShot"),Actor->GetActorLocation(),0.4f);
}
void ACACombatGameMode::SetupPlayer()
{
    APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);
    PlayerPawn=PC ? PC->GetPawn() : nullptr;
    if (!PlayerPawn) return;
    bReady=true;
    PlayerPawn->OnTakeAnyDamage.AddDynamic(this,&ACACombatGameMode::PlayerDamaged);
    if (ACharacter* Character=Cast<ACharacter>(PlayerPawn))
    {
        Character->GetCharacterMovement()->bIgnoreBaseRotation=true;
        Character->GetCharacterMovement()->bImpartBaseAngularVelocity=false;
        for (ACAClockBridge* Bridge : Clocks)
            if (IsValid(Bridge)) Character->GetCharacterMovement()->AddTickPrerequisiteActor(Bridge);
    }
    EnableInput(PC);
    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::P,IE_Pressed,this,&ACACombatGameMode::TogglePause).bExecuteWhenPaused=true;
        InputComponent->BindKey(EKeys::Enter,IE_Pressed,this,&ACACombatGameMode::Confirm).bExecuteWhenPaused=true;
        InputComponent->BindKey(EKeys::BackSpace,IE_Pressed,this,&ACACombatGameMode::RetryCheckpoint);
        InputComponent->BindKey(EKeys::F1,IE_Pressed,this,&ACACombatGameMode::ToggleHelp).bExecuteWhenPaused=true;
    }
    PC->bShowMouseCursor=false;
    FInputModeGameOnly Mode; Mode.SetConsumeCaptureMouseDown(false); PC->SetInputMode(Mode);
    RestorePlayer();
    Say(bCampaign ? TEXT("CITADEL RUN / 5 SECTORS / 4 CLOCK BRIDGES / Clear each squad, then follow the flags. F1: guide.")
        : TEXT("CLEAR THE YARD, THEN FOLLOW GOLD ARROWS / FLAG 1 > CLOCK HAND > FLAG 2 > EXIT. F1: help."),8);
}
void ACACombatGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bReady) { SetupPlayer(); return; }
    if (bPaused || bWon) return;
    HitMarker=FMath::Max(0.0f,HitMarker-DeltaSeconds);
    DamageFlash=FMath::Max(0.0f,DamageFlash-DeltaSeconds);
    MessageTime=FMath::Max(0.0f,MessageTime-DeltaSeconds);
    if (!PlayerHealth.Alive())
    {
        RecoveryTime-=DeltaSeconds;
        if (RecoveryTime<=0) RestorePlayer();
        return;
    }
    RunSeconds+=DeltaSeconds;
    StepCampaign();
    PlayerHealth.Step(DeltaSeconds);
    if (PlayerPawn && PlayerPawn->GetActorLocation().Z<-700)
        PlayerDamaged(PlayerPawn,1000,nullptr,nullptr,nullptr);
}
int32 ACACombatGameMode::RemainingEnemies() const
{
    return FMath::Max(0,TotalEnemies-Defeated);
}
FVector ACACombatGameMode::PlayerAimPoint() const
{
    if (!PlayerPawn) return FVector::ZeroVector;
    FVector Location=PlayerPawn->GetPawnViewLocation();
    FRotator Rotation;
    if (const APlayerController* PC=Cast<APlayerController>(PlayerPawn->GetController()))
        PC->GetPlayerViewPoint(Location,Rotation);
    return Location-FVector(0,0,15);
}
int32 ACACombatGameMode::RemainingInSection(int32 Section) const
{
    int32 Count=0;
    for (ACAEnemy* Enemy : Enemies)
        if (IsValid(Enemy) && !Enemy->bDead && Enemy->Section==Section) ++Count;
    return Count;
}
void ACACombatGameMode::EnemyDefeated(ACAEnemy* Enemy)
{
    if (!Enemy || !Enemies.Contains(Enemy)) return;
    ++Defeated;
    if (bCampaign)
    {
        if (RemainingInSection(ActiveSection())==0)
            Say(ActiveSection()==5 ? TEXT("WARDEN AND ALL SQUADS CLEARED / Follow the GREEN FINAL EXIT.")
                : TEXT("BOTH WAVES CLEARED / The bridge gate is opening. Follow GOLD arrows to the next flag."),6);
        return;
    }
    if (RemainingEnemies()==0) Say(TEXT("ALL HOSTILES CLEARED / Follow the EXIT marker to the GREEN exit flag."),6);
    else if (RemainingInSection(1)==0 && CheckpointIndex<2)
        Say(TEXT("YARD CLEARED / Follow GOLD floor arrows to FLAG 1, then board the gold clock hand."),6);
}
void ACACombatGameMode::SaveCheckpoint(int32 Index)
{
    if (!IsActive() || Index<=CheckpointIndex || Index>(bCampaign ? Campaign::Checkpoints : 2)) return;
    if (bCampaign && !Campaign::CanSave(CheckpointIndex,Index,RemainingInSection(ActiveSection()))) return;
    CheckpointIndex=Index; PlayerHealth.Reset(); RefillMagazine();
    CA::Sound(GetWorld(),TEXT("S_Checkpoint"),PlayerPawn->GetActorLocation(),0.6f);
    if (bCampaign)
    {
        Say(FString::Printf(TEXT("FLAG %d SAVED / Health and magazine restored. %s"),Index,
            Index%2 ? TEXT("Wait for the GOLD hand, then ride from 9 to 3.") : TEXT("Next sector ahead. Watch the new enemy silhouettes.")),6);
        return;
    }
    Say(Index==1 ? TEXT("FLAG 1 SAVED / Your flag is GREEN. Board the GOLD hand at 9, then ride to FLAG 2.")
        : TEXT("FLAG 2 SAVED / Clear the remaining robots, then follow the GREEN EXIT flag."),7);
}
void ACACombatGameMode::RefillMagazine()
{
    if (!PlayerPawn) return;
    FNumericProperty* Current=FindFProperty<FNumericProperty>(PlayerPawn->GetClass(),TEXT("Current_Ammunition"));
    FNumericProperty* Total=FindFProperty<FNumericProperty>(PlayerPawn->GetClass(),TEXT("Total_Ammunition"));
    if (!Current || !Total) return;
    const void* Source=Total->ContainerPtrToValuePtr<void>(PlayerPawn);
    void* Target=Current->ContainerPtrToValuePtr<void>(PlayerPawn);
    const double Value=Total->IsInteger() ? static_cast<double>(Total->GetSignedIntPropertyValue(Source))
        : Total->GetFloatingPointPropertyValue(Source);
    if (Current->IsInteger()) Current->SetIntPropertyValue(Target,static_cast<int64>(Value));
    else Current->SetFloatingPointPropertyValue(Target,Value);
}
void ACACombatGameMode::StopPlayerFire()
{
    if (!PlayerPawn) return;
    if (FBoolProperty* Holding=FindFProperty<FBoolProperty>(PlayerPawn->GetClass(),TEXT("bHolding_Shoot")))
        Holding->SetPropertyValue_InContainer(PlayerPawn,false);
}
void ACACombatGameMode::RestorePlayer()
{
    if (!PlayerPawn) return;
    PlayerHealth.Reset(); RecoveryTime=0; StopPlayerFire();
    FVector Location(-2150,0,110);
    if (CheckpointIndex==1) Location=FVector(2850,0,110);
    if (CheckpointIndex==2) Location=FVector(5800,0,110);
    if (bCampaign) Location=FVector(CheckpointIndex>0 ? Campaign::FlagX(CheckpointIndex) : Campaign::StartX,0,110);
    if (ACharacter* Character=Cast<ACharacter>(PlayerPawn))
    {
        Character->SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr),NAME_None,true);
        Character->GetCharacterMovement()->StopMovementImmediately();
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    PlayerPawn->SetActorLocationAndRotation(Location,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
    APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);
    if (PC) { PC->SetControlRotation(FRotator::ZeroRotator); PlayerPawn->EnableInput(PC); }
    RefillMagazine();
    if (!bCampaign && Clock && CheckpointIndex==1) Clock->ResetClock();
    if (bCampaign && CheckpointIndex%2)
        for (ACAClockBridge* Bridge : Clocks)
            if (IsValid(Bridge) && Bridge->SectionBefore==ActiveSection()) Bridge->ResetClock();
    for (ACAEnemy* Enemy : Enemies)
        if (IsValid(Enemy) && !Enemy->bDead) Enemy->ResetGuard();
    for (TActorIterator<ACABolt> It(GetWorld());It;++It) It->Destroy();
}
void ACACombatGameMode::PlayerDamaged(AActor* Actor,float Damage,const UDamageType* Type,
    AController* InstigatorController,AActor* Causer)
{
    if (!IsActive() || !PlayerHealth.Damage(Damage)) return;
    DamageFlash=0.35f;
    CA::Sound(GetWorld(),TEXT("S_PlayerHit"),PlayerPawn->GetActorLocation(),0.45f);
    if (!PlayerHealth.Alive())
    {
        ++Retries; RecoveryTime=1.4f; StopPlayerFire();
        if (APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0)) PlayerPawn->DisableInput(PC);
        if (ACharacter* Character=Cast<ACharacter>(PlayerPawn))
        {
            Character->GetCharacterMovement()->StopMovementImmediately();
            Character->GetCharacterMovement()->DisableMovement();
        }
    }
}
void ACACombatGameMode::RetryCheckpoint()
{
    if (!IsActive()) return;
    ++Retries; RestorePlayer(); Say(TEXT("CHECKPOINT RETRY / Defeated enemies stay defeated."),3);
}
void ACACombatGameMode::TogglePause()
{
    if (!bReady || bWon || !PlayerHealth.Alive()) return;
    bPaused=!bPaused;
    UGameplayStatics::SetGamePaused(GetWorld(),bPaused);
}
void ACACombatGameMode::Confirm()
{
    if (bWon)
    {
        UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this,true)));
        return;
    }
    if (bPaused) TogglePause();
}
void ACACombatGameMode::ToggleHelp() { bHelp=!bHelp; }
void ACACombatGameMode::TryWin()
{
    if (!IsActive()) return;
    if (RemainingEnemies()>0 || TotalEnemies==0 || (bCampaign && CheckpointIndex<Campaign::Checkpoints))
    {
        Say(TEXT("Clear the remaining robots first. The NEXT marker points toward your objective."),5); return;
    }
    bWon=true; StopPlayerFire();
    if (APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0)) PlayerPawn->DisableInput(PC);
    CA::Sound(GetWorld(),TEXT("S_Win"),PlayerPawn->GetActorLocation(),0.7f);
}
void ACACombatGameMode::Say(const FString& Text,float Duration) { Message=Text; MessageTime=Duration; }
Wayfinding::Stage ACACombatGameMode::GuidanceStage() const
{
    if (bCampaign) return RemainingEnemies()==0 ? Wayfinding::Stage::Exit : Wayfinding::Stage::ClearFoundry;
    return Wayfinding::NextStage(CheckpointIndex,RemainingInSection(1),RemainingInSection(2),
        PlayerPawn ? PlayerPawn->GetActorLocation().X : -2150);
}
FString ACACombatGameMode::Objective() const
{
    if (bCampaign) return CampaignObjective();
    switch (GuidanceStage())
    {
    case Wayfinding::Stage::ClearYard: return TEXT("CLEAR THE YARD / THEN REACH FLAG 1");
    case Wayfinding::Stage::FlagOne: return TEXT("FOLLOW GOLD ARROWS TO CHECKPOINT FLAG 1");
    case Wayfinding::Stage::BoardClock: return Clock && !CombatRules::AtDock(Clock->Angle(),false)
        ? TEXT("WAIT AT 9 / BOARD THE HAND WHEN IT ARRIVES") : TEXT("BOARD GOLD HAND AT 9 / RIDE TO FLAG 2");
    case Wayfinding::Stage::FlagTwo: return TEXT("STAY ON HAND TO 3 / THEN REACH FLAG 2");
    case Wayfinding::Stage::ClearFoundry: return TEXT("CLEAR THE FOUNDRY / THEN REACH THE EXIT");
    case Wayfinding::Stage::ReturnToYard: return TEXT("YARD ROBOTS REMAIN / RIDE CLOCK BACK");
    default: return TEXT("EXIT OPEN / REACH THE GREEN EXIT FLAG");
    }
}
FString ACACombatGameMode::WaypointLabel() const
{
    if (bCampaign) return CampaignWaypointLabel();
    switch (GuidanceStage())
    {
    case Wayfinding::Stage::ClearYard: return TEXT("YARD ROBOT");
    case Wayfinding::Stage::FlagOne: return TEXT("FLAG 1 / CHECKPOINT");
    case Wayfinding::Stage::BoardClock: return Clock && !CombatRules::AtDock(Clock->Angle(),false)
        ? TEXT("WAIT FOR GOLD HAND") : TEXT("BOARD GOLD HAND");
    case Wayfinding::Stage::FlagTwo: return TEXT("FLAG 2 / FAR DOCK");
    case Wayfinding::Stage::ClearFoundry: return TEXT("FOUNDRY ROBOT");
    case Wayfinding::Stage::ReturnToYard: return PlayerPawn && PlayerPawn->GetActorLocation().X>5200
        && Clock && !CombatRules::AtDock(Clock->Angle(),true)
        ? TEXT("WAIT FOR RETURN HAND") : TEXT("RIDE BACK TO YARD");
    default: return TEXT("GREEN EXIT FLAG");
    }
}
FVector ACACombatGameMode::Waypoint() const
{
    if (bCampaign) return CampaignWaypoint();
    auto FlagLocation=[this](int32 Index,bool IsExit,const FVector& Fallback)
    {
        for (TActorIterator<ACAZone> It(GetWorld());It;++It)
            if ((IsExit && It->Kind==ECAZoneKind::Exit) ||
                (!IsExit && It->Kind==ECAZoneKind::Checkpoint && It->CheckpointIndex==Index))
                return It->GetActorLocation()+FVector(0,-180,400);
        return Fallback;
    };
    const double PlayerX=PlayerPawn ? PlayerPawn->GetActorLocation().X : -2150;
    const FVector Center=Clock ? Clock->GetActorLocation() : FVector(4200,0,0);
    const float Radius=Clock ? Clock->HandLength : 1200;
    const bool NearReady=!Clock || CombatRules::AtDock(Clock->Angle(),false);
    const bool FarReady=!Clock || CombatRules::AtDock(Clock->Angle(),true);
    switch (GuidanceStage())
    {
    case Wayfinding::Stage::FlagOne:
        return FlagLocation(1,false,FVector(2760,0,600));
    case Wayfinding::Stage::BoardClock:
        return Center+FVector(Wayfinding::BoardingOffset(Radius,false,NearReady),0,180);
    case Wayfinding::Stage::FlagTwo:
        return FlagLocation(2,false,FVector(5680,0,600));
    case Wayfinding::Stage::ReturnToYard:
        return Center+FVector(PlayerX>5200 ? Wayfinding::BoardingOffset(Radius,true,FarReady)
            : Wayfinding::BoardingOffset(Radius,false,false),0,180);
    case Wayfinding::Stage::Exit:
        return FlagLocation(0,true,FVector(9780,0,600));
    default:
        break;
    }
    const int32 Section=GuidanceStage()==Wayfinding::Stage::ClearYard ? 1 : 2;
    const ACAEnemy* Closest=nullptr;
    double Distance=DBL_MAX;
    for (const ACAEnemy* Enemy : Enemies)
    {
        if (!IsValid(Enemy) || Enemy->bDead || Enemy->Section!=Section) continue;
        const double D=FVector::DistSquared(PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector,
            Enemy->GetActorLocation());
        if (D<Distance) { Closest=Enemy; Distance=D; }
    }
    return Closest ? Closest->GetActorLocation()+FVector(0,0,130)
        : FlagLocation(0,true,FVector(9780,0,600));
}
