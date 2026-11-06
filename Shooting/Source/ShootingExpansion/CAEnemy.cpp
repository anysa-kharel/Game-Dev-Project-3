#include "CAEnemy.h"
#include "CAArenaActors.h"
#include "CACombatGameMode.h"
#include "CAUtil.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

FString ACAEnemy::TypeName() const
{
    const TCHAR* Names[]={TEXT("GUARD"),TEXT("RUSHER"),TEXT("BURST SENTRY"),TEXT("SKIRMISHER"),TEXT("SHIELD GUARD"),TEXT("BOMBER"),TEXT("WARDEN")};
    return Names[FMath::Clamp(static_cast<int32>(Kind),0,6)];
}
FLinearColor ACAEnemy::TypeColor() const
{
    const FLinearColor Colors[]={CA::Red(),CA::Gold(),CA::Cyan(),FLinearColor(0.65f,0.2f,1),
        FLinearColor(0.1f,0.4f,1),FLinearColor(1,0.28f,0.025f),CA::Gold()};
    return Colors[FMath::Clamp(static_cast<int32>(Kind),0,6)];
}
ACAEnemy::ACAEnemy()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(43,88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    AIControllerClass=AAIController::StaticClass(); AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=false;
    GetCharacterMovement()->MaxStepHeight=38; GetCharacterMovement()->BrakingDecelerationWalking=1800;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(GetCapsuleComponent());
    Head=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head")); Head->SetupAttachment(GetCapsuleComponent());
    Eye=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eye")); Eye->SetupAttachment(Head);
    Gun=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cannon")); Gun->SetupAttachment(Body);
    LeftLeg=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeg")); LeftLeg->SetupAttachment(GetCapsuleComponent());
    RightLeg=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeg")); RightLeg->SetupAttachment(GetCapsuleComponent());
    Shield=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontShield")); Shield->SetupAttachment(GetCapsuleComponent());
    Detail=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoleSilhouette")); Detail->SetupAttachment(GetCapsuleComponent());
    DangerRing=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BomberWarning")); DangerRing->SetupAttachment(GetCapsuleComponent());
}
void ACAEnemy::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const bool Boss=Kind==ECAEnemyKind::Warden;
    GetCapsuleComponent()->SetCapsuleSize(Boss ? 70 : 43,Boss ? 130 : 88);
    FVector BodySize=Boss ? FVector(130,130,130) : Kind==ECAEnemyKind::Scout ? FVector(58,58,65)
        : Kind==ECAEnemyKind::ShieldGuard ? FVector(96,90,100) : FVector(78,78,80);
    const TCHAR* BodyShape=Kind==ECAEnemyKind::Bomber ? TEXT("Sphere")
        : Kind==ECAEnemyKind::Skirmisher || Kind==ECAEnemyKind::Sentry ? TEXT("Cube") : TEXT("Cylinder");
    CA::Mesh(Body,BodyShape,BodySize,TypeColor(),0,false); Body->SetRelativeLocation(FVector(0,0,-9));
    const FVector HeadSize=Boss ? FVector(90,90,75) : Kind==ECAEnemyKind::Sentry ? FVector(90,110,45) : FVector(72);
    CA::Mesh(Head,Kind==ECAEnemyKind::Skirmisher ? TEXT("Cube") : TEXT("Sphere"),HeadSize,CA::Steel(),0,false);
    Head->SetRelativeLocation(FVector(0,0,Boss ? 87 : 51));
    CA::Mesh(Eye,TEXT("Cube"),FVector(8,50,13),TypeColor(),2,false);
    Eye->SetRelativeLocation(FVector(48,0,0)); Eye->SetRelativeScale3D(FVector(8/HeadSize.X,50/HeadSize.Y,13/HeadSize.Z));
    CA::Mesh(Gun,TEXT("Cube"),FVector(76,23,24),CA::Navy(),0,false);
    Gun->SetRelativeLocation(FVector(50,-25,6)); Gun->SetRelativeScale3D(FVector(76/BodySize.X,23/BodySize.Y,24/BodySize.Z));
    Gun->SetVisibility(Kind!=ECAEnemyKind::Scout && Kind!=ECAEnemyKind::Bomber);
    for (UStaticMeshComponent* Leg : {LeftLeg.Get(),RightLeg.Get()})
        CA::Mesh(Leg,TEXT("Cube"),Boss ? FVector(40,40,80) : FVector(26,26,56),CA::Steel(),0,false);
    LeftLeg->SetRelativeLocation(FVector(0,Boss ? -38 : -23,Boss ? -90 : -60));
    RightLeg->SetRelativeLocation(FVector(0,Boss ? 38 : 23,Boss ? -90 : -60));
    CA::Mesh(Shield,TEXT("Cube"),FVector(12,130,145),TypeColor(),0.45f,false);
    Shield->SetRelativeLocation(FVector(76,0,6)); Shield->SetVisibility(Kind==ECAEnemyKind::ShieldGuard);
    CA::Mesh(Detail,Boss ? TEXT("Cylinder") : TEXT("Cube"),Boss ? FVector(170,170,24) : FVector(74,18,18),TypeColor(),0.5f,false);
    Detail->SetRelativeLocation(Boss ? FVector(0,0,135) : FVector(75,28,-5));
    Detail->SetVisibility(Boss || Kind==ECAEnemyKind::Sentry);
    CA::Mesh(DangerRing,TEXT("Cylinder"),FVector(860,860,5),TypeColor(),0.5f,false);
    DangerRing->SetRelativeLocation(FVector(0,0,-84)); DangerRing->SetCastShadow(false); DangerRing->SetVisibility(false);
}
void ACAEnemy::BeginPlay()
{
    Super::BeginPlay(); Home=GetActorLocation(); LastSeen=Home; HitsLeft=MaxHits; ShieldHits=2;
    GetCharacterMovement()->MaxWalkSpeed=Campaign::Profiles[FMath::Clamp(static_cast<int32>(Kind),0,6)].Speed;
    TacticalAttack.Cooldown=1.5+FMath::Abs(Home.Y)*0.001;
}
bool ACAEnemy::SeesPlayer() const
{
    const ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn) return false;
    const FVector Start=GetActorLocation()+FVector(0,0,50),Target=G->PlayerAimPoint();
    const double SightRange=Kind==ECAEnemyKind::Warden ? 3500 : 3100;
    if (FVector::DistSquared(Start,Target)>FMath::Square(SightRange)) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySight),false,this);
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Start,Target,ECC_Visibility,Params);
    return !Blocked || Hit.GetActor()==G->PlayerPawn;
}
void ACAEnemy::WalkToward(const FVector& Goal,float Scale)
{
    FVector Desired=Goal-GetActorLocation(); Desired.Z=0;
    if (Desired.SizeSquared()<100) return;
    Desired.Normalize();
    FVector Best=FVector::ZeroVector;
    float BestScore=-100;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySteering),false,this);
    for (float Angle : {0.0f,40.0f,-40.0f,80.0f,-80.0f,120.0f,-120.0f})
    {
        const FVector Direction=Desired.RotateAngleAxis(Angle,FVector::UpVector);
        const FVector Start=GetActorLocation()-FVector(0,0,25);
        FHitResult Obstacle;
        if (GetWorld()->SweepSingleByChannel(Obstacle,Start,Start+Direction*180,
            FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(GetCapsuleComponent()->GetScaledCapsuleRadius()+5),Params)) continue;
        const FVector FloorAhead=GetActorLocation()+Direction*210;
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor,FloorAhead+FVector(0,0,90),
            FloorAhead-FVector(0,0,245),ECC_Visibility,Params)) continue;
        if (Floor.ImpactNormal.Z<0.65) continue;
        const float Score=static_cast<float>(FVector::DotProduct(Direction,Desired));
        if (Score>BestScore) { BestScore=Score; Best=Direction; }
    }
    if (!Best.IsNearlyZero()) AddMovementInput(Best,Scale);
}
void ACAEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead) return;
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (G && G->bCampaign)
    {
        const bool Enabled=G->EnemyEnabled(this);
        if (!Enabled && !bDormant)
        {
            bDormant=true; SetActorHiddenInGame(true);
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            GetCharacterMovement()->StopMovementImmediately(); GetCharacterMovement()->DisableMovement();
        }
        if (!Enabled) return;
        if (bDormant)
        {
            bDormant=false; SetActorHiddenInGame(false);
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            GetCharacterMovement()->SetMovementMode(MOVE_Walking); TacticalAttack.Reset();
            TacticalAttack.Cooldown=1.2+FMath::Abs(Home.Y)*0.001;
        }
    }
    if (!G || !G->IsActive() || !G->PlayerPawn || G->RunSeconds<2.5f || !G->EnemyEnabled(this))
    { GetCharacterMovement()->StopMovementImmediately(); return; }
    if (GetActorLocation().Z<-650) { Die(GetActorLocation()-FVector(0,0,1)); return; }
    AnimationAge+=DeltaSeconds;HitFlash=FMath::Max(0.0f,HitFlash-DeltaSeconds);
    const bool Sight=SeesPlayer(); TimeSinceSeen+=DeltaSeconds;
    if (Sight) { bAlert=true;LastSeen=G->PlayerPawn->GetActorLocation();TimeSinceSeen=0; }
    if (TimeSinceSeen>8) bAlert=false;
    const FVector ToPlayer=G->PlayerPawn->GetActorLocation()-GetActorLocation();
    const double Distance=ToPlayer.Size();
    if (bAlert)
    {
        FVector Facing=LastSeen-GetActorLocation();Facing.Z=0;
        if (!Facing.IsNearlyZero()) SetActorRotation(FRotator(0,Facing.Rotation().Yaw,0));
        if (Kind==ECAEnemyKind::Scout || Kind==ECAEnemyKind::Bomber)
        {
            if (!TacticalAttack.Charging) WalkToward(LastSeen);
            else GetCharacterMovement()->StopMovementImmediately();
        }
        else if (Kind==ECAEnemyKind::Skirmisher)
        {
            if (!Sight || Distance>1800) WalkToward(LastSeen);
            else if (Distance<650) WalkToward(GetActorLocation()-ToPlayer,0.8f);
            else WalkToward(GetActorLocation()+GetActorRightVector()*FMath::Sin(AnimationAge*1.7f)*650,1);
        }
        else if (Kind!=ECAEnemyKind::Sentry)
        {
            if (!Sight || Distance>1300) WalkToward(LastSeen);
            else if (Distance<480 && Kind!=ECAEnemyKind::ShieldGuard) WalkToward(GetActorLocation()-ToPlayer,0.6f);
            else WalkToward(GetActorLocation()+GetActorRightVector()*FMath::Sin(AnimationAge*0.65f)*260,0.4f);
        }
    }
    else if (Kind!=ECAEnemyKind::Sentry)
        WalkToward(Home+FVector(FMath::Cos(AnimationAge*0.3f)*130,FMath::Sin(AnimationAge*0.3f)*130,0),0.3f);
    Campaign::Profile Profile=Campaign::Profiles[FMath::Clamp(static_cast<int32>(Kind),0,6)];
    if (Kind==ECAEnemyKind::Warden && HitsLeft<=MaxHits/2) Profile.Cooldown=1.4;
    if (TacticalAttack.Step(DeltaSeconds,Sight && bAlert && Distance<Profile.Range,Profile))
    {
        if (Kind==ECAEnemyKind::Scout) UGameplayStatics::ApplyDamage(G->PlayerPawn,16,GetController(),this,nullptr);
        else if (Kind==ECAEnemyKind::Bomber)
        {
            if (Distance<430 && SeesPlayer()) UGameplayStatics::ApplyDamage(G->PlayerPawn,24,GetController(),this,nullptr);
            Die(GetActorLocation()-ToPlayer);return;
        }
        else Fire();
    }
    const bool Charging=TacticalAttack.Charging;
    CA::Tint(Eye,HitFlash>0 ? CA::White() : Charging ? CA::Gold() : TypeColor(),Charging ? 4 : 1.5f);
    Shield->SetVisibility(Kind==ECAEnemyKind::ShieldGuard && ShieldHits>0 && !Charging);
    CA::Tint(Shield,HitFlash>0 ? CA::White() : TypeColor(),0.45f);
    DangerRing->SetVisibility(Kind==ECAEnemyKind::Bomber && Charging);
    if (Kind==ECAEnemyKind::Warden) Detail->SetRelativeRotation(FRotator(0,AnimationAge*60,0));
    const float Swing=FMath::Sin(AnimationAge*12)*FMath::Min(1.0f,static_cast<float>(GetVelocity().Size2D()/140))*25;
    LeftLeg->SetRelativeRotation(FRotator(Swing,0,0));RightLeg->SetRelativeRotation(FRotator(-Swing,0,0));
}
void ACAEnemy::Fire()
{
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn || !SeesPlayer()) return;
    const FVector Muzzle=GetActorLocation()+GetActorForwardVector()*(Kind==ECAEnemyKind::Warden ? 120 : 88)+FVector(0,0,10);
    const FRotator Aim=(G->PlayerAimPoint()-Muzzle).Rotation();
    const int32 Count=Campaign::BurstCount(static_cast<int32>(Kind),HitsLeft,MaxHits);
    for (int32 N=0;N<Count;++N)
    {
        const float Offset=(N-(Count-1)*0.5f)*(Kind==ECAEnemyKind::Warden ? 7.0f : 5.0f);
        const FTransform Transform(FRotator(Aim.Pitch,Aim.Yaw+Offset,0),Muzzle);
        ACABolt* Bolt=GetWorld()->SpawnActorDeferred<ACABolt>(ACABolt::StaticClass(),Transform,this,this,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Bolt) { Bolt->Damage=Kind==ECAEnemyKind::Warden ? 10 : Kind==ECAEnemyKind::Skirmisher ? 9 : 12; Bolt->FinishSpawning(Transform); }
    }
    CA::Sound(GetWorld(),TEXT("S_EnemyFire"),Muzzle,0.3f);
}
float ACAEnemy::TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser)
{
    if (bDead || DamageAmount<=0) return 0;
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->IsActive() || !G->EnemyEnabled(this)) return 0;
    const FVector Source=G->PlayerPawn ? G->PlayerPawn->GetActorLocation() : LastSeen;
    if (Kind==ECAEnemyKind::ShieldGuard && Campaign::ShieldBlocks(
        FVector::DotProduct(GetActorForwardVector(),(Source-GetActorLocation()).GetSafeNormal()),ShieldHits,TacticalAttack.Charging))
    {
        if (ShieldHits==2) G->Say(TEXT("SHIELD BLOCKED / Flank the blue guard, or shoot when its shield lowers."),4);
        --ShieldHits;HitFlash=0.12f;G->HitMarker=0.08f;
        CA::Tint(Shield,CA::White(),1.1f);CA::Sound(GetWorld(),TEXT("S_EnemyHit"),GetActorLocation(),0.3f);
        return 0;
    }
    Super::TakeDamage(DamageAmount,DamageEvent,EventInstigator,DamageCauser);
    --HitsLeft;HitFlash=0.16f;bAlert=true;TimeSinceSeen=0;LastSeen=Source;G->HitMarker=0.16f;
    const FVector From=IsValid(DamageCauser) ? DamageCauser->GetActorLocation() : Source;
    if (HitsLeft<=0) Die(From); else CA::Sound(GetWorld(),TEXT("S_EnemyHit"),GetActorLocation(),0.25f);
    return DamageAmount;
}
void ACAEnemy::Die(const FVector& From)
{
    if (bDead) return;
    bDead=true;DangerRing->SetVisibility(false);
    GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector Impulse=(GetActorLocation()-From).GetSafeNormal()*260+FVector(0,0,190);
    for (UStaticMeshComponent* Part : {Body.Get(),Head.Get(),LeftLeg.Get(),RightLeg.Get(),Detail.Get(),Shield.Get()})
    {
        if (!Part->IsVisible()) continue;
        Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Part->SetCollisionProfileName(TEXT("PhysicsActor"));Part->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
        Part->SetSimulatePhysics(true);Part->AddImpulse(Impulse,NAME_None,true);
    }
    CA::Tint(Eye,CA::Navy(),0);
    if (ACACombatGameMode* G=CA::Game(GetWorld())) G->EnemyDefeated(this);
    CA::Sound(GetWorld(),TEXT("S_EnemyDown"),GetActorLocation(),0.4f);SetLifeSpan(4);
}
void ACAEnemy::ResetGuard()
{
    if (bDead) return;
    SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr),NAME_None,true);
    SetActorLocation(Home,false,nullptr,ETeleportType::TeleportPhysics);
    GetCharacterMovement()->StopMovementImmediately();
    if (!bDormant) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    HitsLeft=MaxHits;ShieldHits=2;bAlert=false;TimeSinceSeen=100;TacticalAttack.Reset();
}
