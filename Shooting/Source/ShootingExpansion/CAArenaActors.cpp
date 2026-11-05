#include "CAArenaActors.h"
#include "CACombatGameMode.h"
#include "CAUtil.h"
#include "CombatRules.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ACAScenery::ACAScenery()
{
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Geometry"));
    SetRootComponent(Mesh);
}
void ACAScenery::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CA::Mesh(Mesh,*Shape,Dimensions,Color,Glow,bSolid);
}

ACAClockBridge::ACAClockBridge()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    Pivot=CreateDefaultSubobject<USceneComponent>(TEXT("RotatingPivot")); Pivot->SetupAttachment(Root);
    Hand=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RideableMinuteHand")); Hand->SetupAttachment(Pivot);
    Hub=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClockHub")); Hub->SetupAttachment(Root);
}
void ACAClockBridge::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CA::Mesh(Hand,TEXT("Cube"),FVector(HandLength,HandWidth,45),CA::Gold(),0.25f);
    Hand->SetRelativeLocation(FVector(HandLength*0.5f,0,-22.5f));
    Hand->ComponentTags.AddUnique(TEXT("ClockHand"));
    CA::Mesh(Hub,TEXT("Cylinder"),FVector(180,180,80),CA::Steel());
    Hub->SetRelativeLocation(FVector(0,0,-45));
    TArray<UStaticMeshComponent*> Components;
    GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Part : Components)
        if (Part && Part->ComponentHasTag(TEXT("ClockDecor"))) Part->DestroyComponent();
    auto Part=[this](const FVector& Location,const FVector& Size,const FLinearColor& Color,bool Solid)
    {
        UStaticMeshComponent* Mesh=NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(Mesh); Mesh->ComponentTags.Add(TEXT("ClockDecor"));
        Mesh->SetupAttachment(Pivot);
        CA::Mesh(Mesh,TEXT("Cube"),Size,Color,0.8f,Solid);
        Mesh->SetRelativeLocation(Location); Mesh->RegisterComponent();
    };
    Part(FVector(HandLength*0.5f,0,3),FVector(HandLength-50,12,6),CA::Cyan(),false);
    for (int32 Side : {-1,1})
        Part(FVector(HandLength*0.5f,Side*(HandWidth*0.5f+8),45),
            FVector(HandLength-220,16,90),CA::Steel(),true);
    Pivot->SetRelativeRotation(FRotator(0,Angle(),0));
}
void ACAClockBridge::BeginPlay()
{
    Super::BeginPlay(); ResetClock();
    if (ACharacter* Player=UGameplayStatics::GetPlayerCharacter(GetWorld(),0))
        Player->GetCharacterMovement()->AddTickPrerequisiteActor(this);
}
float ACAClockBridge::Angle() const
{
    return static_cast<float>(CombatRules::ClockAngle(Age,Period));
}
void ACAClockBridge::ResetClock()
{
    Age=0; bStarted=false;
    Pivot->SetRelativeRotation(FRotator(0,180,0));
}
void ACAClockBridge::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->IsActive() || !G->PlayerPawn) return;
    if (G->PlayerPawn->GetActorLocation().X>2550) bStarted=true;
    if (bStarted) Age+=DeltaSeconds;
    Pivot->SetRelativeRotation(FRotator(0,Angle(),0));
}

ACAZone::ACAZone()
{
    Trigger=CreateDefaultSubobject<UBoxComponent>(TEXT("Zone")); SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(120,460,200));
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->SetGenerateOverlapEvents(true);
    Trigger->OnComponentBeginOverlap.AddDynamic(this,&ACAZone::Enter);
    Beacon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon")); Beacon->SetupAttachment(Trigger);
}
void ACAZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CA::Mesh(Beacon,TEXT("Cube"),FVector(14,820,8),Kind==ECAZoneKind::Exit ? CA::Mint() : CA::Cyan(),1.8f,false);
    Beacon->SetRelativeLocation(FVector(0,0,-190));
}
void ACAZone::Enter(UPrimitiveComponent* Component,AActor* OtherActor,
    UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& Hit)
{
    if (ACACombatGameMode* G=CA::Game(GetWorld()))
        if (OtherActor==G->PlayerPawn)
        {
            if (Kind==ECAZoneKind::Exit) G->TryWin();
            else G->SaveCheckpoint(CheckpointIndex);
        }
}

ACAExitDoor::ACAExitDoor()
{
    PrimaryActorTick.bCanEverTick=true;
    Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    Panel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExitDoor")); Panel->SetupAttachment(Root);
    Light=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLight")); Light->SetupAttachment(Root);
}
void ACAExitDoor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CA::Mesh(Panel,TEXT("Cube"),FVector(50,1000,310),CA::Steel());
    Panel->SetRelativeLocation(FVector(0,0,155));
    CA::Mesh(Light,TEXT("Cube"),FVector(70,1100,25),CA::Red(),2,false);
    Light->SetRelativeLocation(FVector(0,0,345));
}
void ACAExitDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->IsActive() || G->RemainingEnemies()>0) return;
    Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Open=FMath::Min(1.0f,Open+DeltaSeconds);
    Panel->SetRelativeLocation(FVector(0,0,155+350*Open));
    CA::Tint(Light,CA::Mint(),2);
}

ACABolt::ACABolt()
{
    Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(9); SetRootComponent(Collision);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetGenerateOverlapEvents(false);
    Collision->OnComponentHit.AddDynamic(this,&ACABolt::Impact);
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RedBolt")); Visual->SetupAttachment(Collision);
    Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent=Collision;
    Movement->InitialSpeed=1150; Movement->MaxSpeed=1150;
    Movement->Velocity=FVector(1150,0,0);
    Movement->bInitialVelocityInLocalSpace=true;
    Movement->bSweepCollision=true;
    Movement->ProjectileGravityScale=0; Movement->bRotationFollowsVelocity=true;
    InitialLifeSpan=4;
}
void ACABolt::BeginPlay()
{
    Super::BeginPlay();
    CA::Mesh(Visual,TEXT("Sphere"),FVector(38,16,16),CA::Red(),5,false);
    if (GetOwner()) Collision->IgnoreActorWhenMoving(GetOwner(),true);
}
void ACABolt::Impact(UPrimitiveComponent* Component,AActor* OtherActor,
    UPrimitiveComponent* OtherComp,FVector NormalImpulse,const FHitResult& Hit)
{
    if (OtherActor==GetOwner()) return;
    if (ACACombatGameMode* G=CA::Game(GetWorld()))
        if (G->IsActive() && OtherActor==G->PlayerPawn)
            UGameplayStatics::ApplyDamage(OtherActor,12,GetInstigatorController(),this,nullptr);
    Destroy();
}
