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
    if (!Builder) Builder=GetWorld()->SpawnActor<ACAArenaBuilder>();
    if (Builder && !Builder->bBuilt) Builder->BuildArena();
    for (TActorIterator<ACAEnemy> It(GetWorld());It;++It) Enemies.Add(*It);
    TActorIterator<ACAClockBridge> Bridges(GetWorld());
    Clock=Bridges ? *Bridges : nullptr;
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
        if (Clock) Character->GetCharacterMovement()->AddTickPrerequisiteActor(Clock);
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
    Say(TEXT("CLEAR THE YARD / Use cover. Red robots shoot; gold scouts rush. F1 shows controls."),8);
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
    if (RemainingEnemies()==0) Say(TEXT("ALL HOSTILES CLEARED / The extraction door is open."),6);
    else if (RemainingInSection(1)==0 && CheckpointIndex<2)
        Say(TEXT("YARD CLEARED / Follow the cyan route and ride the giant clock hand."),6);
}
void ACACombatGameMode::SaveCheckpoint(int32 Index)
{
    if (!IsActive() || Index<=CheckpointIndex || Index>2) return;
    CheckpointIndex=Index; PlayerHealth.Reset();
    CA::Sound(GetWorld(),TEXT("S_Checkpoint"),PlayerPawn->GetActorLocation(),0.6f);
    Say(Index==1 ? TEXT("CHECKPOINT / Board the GOLD clock hand at the 9 o'clock dock. It pauses to let you board.")
        : TEXT("CHECKPOINT / FOUNDRY / Use the crates, shelters and raised galleries. Clear the remaining robots."),7);
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
    if (Clock && CheckpointIndex==1) Clock->ResetClock();
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
    if (RemainingEnemies()>0 || TotalEnemies==0)
    {
        Say(TEXT("Clear the remaining robots first. The NEXT marker points toward your objective."),5); return;
    }
    bWon=true; StopPlayerFire();
    if (APlayerController* PC=UGameplayStatics::GetPlayerController(GetWorld(),0)) PlayerPawn->DisableInput(PC);
    CA::Sound(GetWorld(),TEXT("S_Win"),PlayerPawn->GetActorLocation(),0.7f);
}
void ACACombatGameMode::Say(const FString& Text,float Duration) { Message=Text; MessageTime=Duration; }
FString ACACombatGameMode::Objective() const
{
    if (RemainingInSection(1)>0 && CheckpointIndex<2) return TEXT("01 / CLEAR THE LOADING YARD");
    if (CheckpointIndex<2) return TEXT("02 / RIDE THE CLOCK HAND TO THE FAR PATH");
    if (RemainingEnemies()>0) return TEXT("03 / CLEAR THE FOUNDRY");
    return TEXT("ALL CLEAR / REACH THE GREEN EXTRACTION BEACON");
}
FVector ACACombatGameMode::Waypoint() const
{
    if (!PlayerPawn) return FVector(-1000,0,150);
    if (CheckpointIndex<2 && RemainingInSection(1)==0)
        return PlayerPawn->GetActorLocation().X<3250 ? FVector(3070,0,120) : FVector(5600,0,120);
    const ACAEnemy* Closest=nullptr;
    double Distance=DBL_MAX;
    for (const ACAEnemy* Enemy : Enemies)
    {
        if (!IsValid(Enemy) || Enemy->bDead || (CheckpointIndex<2 && Enemy->Section!=1)) continue;
        const double D=FVector::DistSquared(PlayerPawn->GetActorLocation(),Enemy->GetActorLocation());
        if (D<Distance) { Closest=Enemy; Distance=D; }
    }
    return Closest ? Closest->GetActorLocation()+FVector(0,0,130) : FVector(9650,0,180);
}
