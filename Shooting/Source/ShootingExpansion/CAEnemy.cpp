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

ACAEnemy::ACAEnemy()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(43,88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    AIControllerClass=AAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=false;
    GetCharacterMovement()->MaxStepHeight=38;
    GetCharacterMovement()->MaxWalkSpeed=170;
    GetCharacterMovement()->BrakingDecelerationWalking=1800;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(GetCapsuleComponent());
    Head=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head")); Head->SetupAttachment(GetCapsuleComponent());
    Eye=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eye")); Eye->SetupAttachment(Head);
    Gun=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cannon")); Gun->SetupAttachment(Body);
    LeftLeg=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeg")); LeftLeg->SetupAttachment(GetCapsuleComponent());
    RightLeg=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeg")); RightLeg->SetupAttachment(GetCapsuleComponent());
}
void ACAEnemy::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FLinearColor Armor=Kind==ECAEnemyKind::Scout ? CA::Gold() : CA::Red();
    CA::Mesh(Body,TEXT("Cylinder"),FVector(78,78,80),Armor,0,false);
    Body->SetRelativeLocation(FVector(0,0,-9));
    CA::Mesh(Head,TEXT("Sphere"),FVector(72),CA::Steel(),0,false);
    Head->SetRelativeLocation(FVector(0,0,51));
    CA::Mesh(Eye,TEXT("Cube"),FVector(8,50,13),CA::Red(),3,false);
    Eye->SetRelativeLocation(FVector(48,0,0));
    Eye->SetRelativeScale3D(FVector(8,50,13)/72.0);
    CA::Mesh(Gun,TEXT("Cube"),FVector(76,23,24),CA::Navy(),0,false);
    Gun->SetRelativeLocation(FVector(50,-25,6));
    Gun->SetRelativeScale3D(FVector(76.0/78,23.0/78,24.0/80));
    for (UStaticMeshComponent* Leg : {LeftLeg.Get(),RightLeg.Get()})
        CA::Mesh(Leg,TEXT("Cube"),FVector(26,26,56),CA::Steel(),0,false);
    LeftLeg->SetRelativeLocation(FVector(0,-23,-60));
    RightLeg->SetRelativeLocation(FVector(0,23,-60));
}
void ACAEnemy::BeginPlay()
{
    Super::BeginPlay();
    Home=GetActorLocation(); LastSeen=Home; HitsLeft=MaxHits;
    GetCharacterMovement()->MaxWalkSpeed=Kind==ECAEnemyKind::Scout ? 255 : 170;
    AttackState.Reset(1.5+(FMath::Abs(Home.Y)*0.001));
}
bool ACAEnemy::SeesPlayer() const
{
    const ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn) return false;
    const FVector Start=GetActorLocation()+FVector(0,0,50);
    const FVector Target=G->PlayerAimPoint();
    if (FVector::DistSquared(Start,Target)>FMath::Square(2600.0)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySight),false,this);
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
            FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(38),Params)) continue;
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
    if (!G || !G->IsActive() || !G->PlayerPawn || G->RunSeconds<2.5f ||
        (Section==2 && G->CheckpointIndex<2))
    {
        GetCharacterMovement()->StopMovementImmediately(); return;
    }
    if (GetActorLocation().Z<-650) { Die(GetActorLocation()-FVector(0,0,1)); return; }
    AnimationAge+=DeltaSeconds; HitFlash=FMath::Max(0.0f,HitFlash-DeltaSeconds);
    const bool Sight=SeesPlayer();
    TimeSinceSeen+=DeltaSeconds;
    if (Sight) { bAlert=true; LastSeen=G->PlayerPawn->GetActorLocation(); TimeSinceSeen=0; }
    if (TimeSinceSeen>7) bAlert=false;
    const FVector ToPlayer=G->PlayerPawn->GetActorLocation()-GetActorLocation();
    const float Distance=static_cast<float>(ToPlayer.Size());
    if (bAlert)
    {
        FVector Facing=LastSeen-GetActorLocation(); Facing.Z=0;
        if (!Facing.IsNearlyZero()) SetActorRotation(FRotator(0,Facing.Rotation().Yaw,0));
        if (Kind==ECAEnemyKind::Scout) WalkToward(LastSeen,1);
        else if (Kind==ECAEnemyKind::Guard)
        {
            if (!Sight || Distance>1150) WalkToward(LastSeen);
            else if (Distance<530) WalkToward(GetActorLocation()-ToPlayer,0.7f);
            else
            {
                const FVector Side=GetActorRightVector()*FMath::Sin(AnimationAge*0.65f)*300;
                WalkToward(GetActorLocation()+Side,0.5f);
            }
        }
    }
    else if (Kind!=ECAEnemyKind::Sentry)
        WalkToward(Home+FVector(FMath::Cos(AnimationAge*0.3f)*140,FMath::Sin(AnimationAge*0.3f)*140,0),0.35f);
    const bool Melee=Kind==ECAEnemyKind::Scout;
    if (AttackState.Step(DeltaSeconds,Sight && bAlert,Distance<(Melee ? 175 : 2400),Melee))
    {
        if (Melee)
        {
            UGameplayStatics::ApplyDamage(G->PlayerPawn,16,GetController(),this,nullptr);
            CA::Sound(GetWorld(),TEXT("S_EnemyFire"),GetActorLocation(),0.25f);
        }
        else Fire();
    }
    CA::Tint(Eye,HitFlash>0 ? CA::White() : AttackState.Charging ? CA::Gold() : CA::Red(),
        AttackState.Charging ? 6 : 3);
    const float Swing=FMath::Sin(AnimationAge*12)*FMath::Min(1.0f,static_cast<float>(GetVelocity().Size2D()/140))*25;
    LeftLeg->SetRelativeRotation(FRotator(Swing,0,0));
    RightLeg->SetRelativeRotation(FRotator(-Swing,0,0));
}
void ACAEnemy::Fire()
{
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->PlayerPawn || !SeesPlayer()) return;
    const FVector Muzzle=GetActorLocation()+GetActorForwardVector()*88+FVector(0,0,10);
    const FVector Target=G->PlayerAimPoint();
    FActorSpawnParameters Params;
    Params.Owner=this; Params.Instigator=this;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ACABolt>(ACABolt::StaticClass(),Muzzle,(Target-Muzzle).Rotation(),Params);
    CA::Sound(GetWorld(),TEXT("S_EnemyFire"),Muzzle,0.35f);
}
float ACAEnemy::TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,
    AController* EventInstigator,AActor* DamageCauser)
{
    if (bDead || DamageAmount<=0) return 0;
    ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->IsActive()) return 0;
    Super::TakeDamage(DamageAmount,DamageEvent,EventInstigator,DamageCauser);
    --HitsLeft; HitFlash=0.16f;
    bAlert=true; TimeSinceSeen=0;
    if (G->PlayerPawn) LastSeen=G->PlayerPawn->GetActorLocation();
    G->HitMarker=0.16f;
    const FVector From=DamageCauser ? DamageCauser->GetActorLocation() : LastSeen;
    if (HitsLeft<=0) Die(From);
    else CA::Sound(GetWorld(),TEXT("S_EnemyHit"),GetActorLocation(),0.25f);
    return DamageAmount;
}
void ACAEnemy::Die(const FVector& From)
{
    if (bDead) return;
    bDead=true;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FVector Impulse=(GetActorLocation()-From).GetSafeNormal()*260+FVector(0,0,190);
    for (UStaticMeshComponent* Part : {Body.Get(),Head.Get(),LeftLeg.Get(),RightLeg.Get()})
    {
        Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Part->SetCollisionProfileName(TEXT("PhysicsActor"));
        Part->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
        Part->SetSimulatePhysics(true);
        Part->AddImpulse(Impulse,NAME_None,true);
    }
    CA::Tint(Eye,CA::Navy(),0);
    if (ACACombatGameMode* G=CA::Game(GetWorld())) G->EnemyDefeated(this);
    CA::Sound(GetWorld(),TEXT("S_EnemyDown"),GetActorLocation(),0.45f);
    SetLifeSpan(4);
}
void ACAEnemy::ResetGuard()
{
    if (bDead) return;
    SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr),NAME_None,true);
    SetActorLocation(Home,false,nullptr,ETeleportType::TeleportPhysics);
    GetCharacterMovement()->StopMovementImmediately();
    HitsLeft=MaxHits; bAlert=false; TimeSinceSeen=100; AttackState.Reset(2);
}
