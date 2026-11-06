#include "CAArenaActors.h"
#include "CACombatGameMode.h"
#include "CAUtil.h"
#include "CombatRules.h"
#include "WayfindingRules.h"
#include "CampaignRules.h"
#include "CAReadableSigns.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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
    if (Tags.Contains(TEXT("CAReadabilityPanel"))) CAReadability::Surface(Mesh,Color);
    Mesh->SetCullDistance(15000);
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
    if (SectionBefore>0)
    {
        const FVector P=G->PlayerPawn->GetActorLocation()-GetActorLocation();
        if (Campaign::Boarded(P.X,P.Y,P.Z,HandWidth) && G->RemainingInSection(SectionBefore)==0
            && G->CheckpointIndex>=SectionBefore*2-1) bStarted=true;
    }
    else if (G->PlayerPawn->GetActorLocation().X>2550) bStarted=true;
    if (bStarted) Age+=DeltaSeconds;
    Pivot->SetRelativeRotation(FRotator(0,Angle(),0));
}

ACAZone::ACAZone()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickInterval=0.15f;
    Trigger=CreateDefaultSubobject<UBoxComponent>(TEXT("Zone")); SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(120,460,200));
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->SetGenerateOverlapEvents(true);
    Trigger->OnComponentBeginOverlap.AddDynamic(this,&ACAZone::Enter);
    Beacon=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon")); Beacon->SetupAttachment(Trigger);
    FlagPole=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointFlagPole")); FlagPole->SetupAttachment(Trigger);
    Flag=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointFlag")); Flag->SetupAttachment(Trigger);
    FlagStripe=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagStatusStripe")); FlagStripe->SetupAttachment(Trigger);
    FlagCap=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointFlagCap")); FlagCap->SetupAttachment(Trigger);
    FlagFront=CreateDefaultSubobject<UTextRenderComponent>(TEXT("FlagLabelFront")); FlagFront->SetupAttachment(Trigger);
    FlagBack=CreateDefaultSubobject<UTextRenderComponent>(TEXT("FlagLabelBack")); FlagBack->SetupAttachment(Trigger);
}
void ACAZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshMarker();
}
void ACAZone::RefreshMarker()
{
    const FLinearColor Color=Kind==ECAZoneKind::Exit ? CA::Gold() : CA::Cyan();
    CA::Mesh(Beacon,TEXT("Cube"),FVector(220,840,5),Color,0.45f,false);
    Beacon->SetRelativeLocation(FVector(0,0,-196));
    CA::Mesh(FlagPole,TEXT("Cylinder"),FVector(12,12,Wayfinding::FlagHeight),CA::White(),0.1f,false);
    FlagPole->SetRelativeLocation(FVector(0,-340,Wayfinding::FlagHeight*0.5f-200));
    CA::Mesh(Flag,TEXT("Cube"),FVector(16,300,180),CAReadability::Ink(),0,false);
    Flag->SetRelativeLocation(FVector(0,-180,400));
    CAReadability::Surface(Flag,CAReadability::Ink());
    CA::Mesh(FlagStripe,TEXT("Cube"),FVector(22,12,180),Color,0,false);
    FlagStripe->SetRelativeLocation(FVector(0,-324,400));
    CAReadability::Surface(FlagStripe,Color);
    CA::Mesh(FlagCap,TEXT("Sphere"),FVector(25),Color,1.0f,false);
    FlagCap->SetRelativeLocation(FVector(0,-340,Wayfinding::FlagHeight-200));
    for (UTextRenderComponent* Text : {FlagFront.Get(),FlagBack.Get()})
    {
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetVerticalAlignment(EVRTA_TextCenter);
        Text->SetWorldSize(42);
        CAReadability::Lettering(Text);
    }
    FlagFront->SetRelativeLocation(FVector(-10,-180,400));
    FlagFront->SetRelativeRotation(FRotator(0,180,0));
    FlagBack->SetRelativeLocation(FVector(10,-180,400));
    FlagBack->SetRelativeRotation(FRotator::ZeroRotator);
    LastMarkerState=-1;
    UpdateMarker();
}
void ACAZone::BeginPlay()
{
    Super::BeginPlay();
    RefreshMarker();
}
void ACAZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMarker();
}
void ACAZone::UpdateMarker()
{
    const ACACombatGameMode* G=CA::Game(GetWorld());
    const int32 Saved=G ? G->CheckpointIndex : 0;
    // The editor and the first BeginPlay callbacks must not show an unlocked exit.
    const int32 Remaining=G && G->bReady && G->TotalEnemies>0 ? G->RemainingEnemies() : 12;
    const auto State=Wayfinding::StateFor(CheckpointIndex,Saved,Remaining,Kind==ECAZoneKind::Exit);
    const int32 Key=static_cast<int32>(State);
    if (Key==LastMarkerState) return;
    LastMarkerState=Key;
    const bool Green=State==Wayfinding::FlagState::Saved || State==Wayfinding::FlagState::Open;
    const FLinearColor Color=Green ? CA::Mint() : State==Wayfinding::FlagState::Locked ? CA::Gold() : CA::Cyan();
    const FString Label=Kind==ECAZoneKind::Exit
        ? (Green ? TEXT("EXIT\nGO HERE") : TEXT("EXIT\nLOCKED"))
        : FString::Printf(TEXT("FLAG %d\n%s"),CheckpointIndex,Green ? TEXT("SAVED") : TEXT("CHECKPOINT"));
    FlagFront->SetText(FText::FromString(Label));
    FlagBack->SetText(FText::FromString(Label));
    CAReadability::Surface(Flag,CAReadability::Ink());
    CAReadability::Surface(FlagStripe,Color);
    CA::Tint(FlagCap,Color,1.0f);
    CA::Tint(Beacon,Color,0.45f);
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
    Status=CreateDefaultSubobject<UTextRenderComponent>(TEXT("GateStatus")); Status->SetupAttachment(Root);
    StatusBoard=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateStatusBoard")); StatusBoard->SetupAttachment(Root);
}
void ACAExitDoor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    CA::Mesh(Panel,TEXT("Cube"),FVector(50,1000,310),CA::Steel());
    Panel->SetRelativeLocation(FVector(0,0,155));
    CA::Mesh(Light,TEXT("Cube"),FVector(70,1100,25),CA::Red(),2,false);
    Light->SetRelativeLocation(FVector(0,0,345));
    RefreshSign();
    Status->SetText(FText::FromString(RequiredSection>0 ? TEXT("CLEAR BOTH WAVES") : TEXT("FINAL EXTRACTION")));
}
void ACAExitDoor::RefreshSign()
{
    CA::Mesh(StatusBoard,TEXT("Cube"),FVector(12,650,94),CAReadability::Ink(),0,false);
    StatusBoard->SetRelativeLocation(FVector(-32,0,430));
    CAReadability::Surface(StatusBoard,CAReadability::Ink());
    Status->SetRelativeLocation(FVector(-40,0,430));Status->SetRelativeRotation(FRotator(0,180,0));
    Status->SetHorizontalAlignment(EHTA_Center);Status->SetVerticalAlignment(EVRTA_TextCenter);Status->SetWorldSize(30);
    CAReadability::Lettering(Status);
}
void ACAExitDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const ACACombatGameMode* G=CA::Game(GetWorld());
    if (!G || !G->IsActive()) return;
    const int32 Remaining=RequiredSection>0 ? G->RemainingInSection(RequiredSection) : G->RemainingEnemies();
    if (Remaining>0) return;
    Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Open=FMath::Min(1.0f,Open+DeltaSeconds);
    Panel->SetRelativeLocation(FVector(0,0,155+350*Open));
    CA::Tint(Light,CA::Mint(),2);
    Status->SetText(FText::FromString(RequiredSection>0 ? TEXT("CLOCK BRIDGE OPEN") : TEXT("EXIT OPEN")));
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
    if (OtherActor==GetOwner() || bSpent) return;
    bSpent=true;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Movement->StopMovementImmediately();Visual->SetVisibility(false);
    if (ACACombatGameMode* G=CA::Game(GetWorld()))
        if (G->IsActive() && OtherActor==G->PlayerPawn)
            UGameplayStatics::ApplyDamage(OtherActor,Damage,GetInstigatorController(),this,nullptr);
    SetLifeSpan(0.05f);
}
