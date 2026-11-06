#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioPresentationRules.h"
#include "ClockLessonRules.h"
#include "CAArenaActors.generated.h"
class UStaticMeshComponent;
class USceneComponent;
class UBoxComponent;
class USphereComponent;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class UTextRenderComponent;
class UAudioComponent;

UCLASS(Blueprintable)
class SHOOTINGEXPANSION_API ACAScenery : public AActor
{
    GENERATED_BODY()
public:
    ACAScenery();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Arena") FString Shape=TEXT("Cube");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Arena") FVector Dimensions=FVector(100);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Arena") FLinearColor Color=FLinearColor(0.1f,0.15f,0.2f);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Arena") bool bSolid=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Arena") float Glow=0;
};

UCLASS(Blueprintable)
class SHOOTINGEXPANSION_API ACAClockBridge : public AActor
{
    GENERATED_BODY()
public:
    ACAClockBridge();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Pivot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hand;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hub;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Clock") float HandLength=1200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Clock") float HandWidth=260;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Clock") float Period=28;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Clock") int32 SectionBefore=0;
    double Age=0;
    bool bStarted=false;
    float Angle() const;
    void ResetClock();
    int32 LessonStage() const { return ClockLessons::Stage(SectionBefore); }
    FString LessonHint() const;
    bool LessonGeometryReady() const { return bLessonGeometryBuilt; }
private:
    void BuildLessonGeometry();
    void UpdateLessonGeometry();
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> LessonBeams;
    float StandingCapsuleHeight=0,CrouchedCapsuleHeight=0,CurrentBeamClearance=150;
    bool bPassedLessonBeam=false,bLessonGeometryBuilt=false;
};

UENUM(BlueprintType)
enum class ECAZoneKind : uint8 { Checkpoint, Exit };

UCLASS(Blueprintable)
class SHOOTINGEXPANSION_API ACAZone : public AActor
{
    GENERATED_BODY()
public:
    ACAZone();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Zone") void RefreshMarker();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Beacon;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FlagPole;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flag;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FlagStripe;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FlagCap;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> FlagFront;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> FlagBack;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zone") ECAZoneKind Kind=ECAZoneKind::Checkpoint;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zone") int32 CheckpointIndex=1;
    UFUNCTION() void Enter(UPrimitiveComponent* Component,AActor* OtherActor,
        UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& Hit);
private:
    int32 LastMarkerState=-1;
    void UpdateMarker();
};

UCLASS()
class SHOOTINGEXPANSION_API ACAExitDoor : public AActor
{
    GENERATED_BODY()
public:
    ACAExitDoor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Panel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Light;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Status;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StatusBoard;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> GateOpenAudio;
    void RefreshSign();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Gate") int32 RequiredSection=0;
    float Open=0;
    PresentationAudio::GateCue OpeningCue;
};

UCLASS()
class SHOOTINGEXPANSION_API ACABolt : public AActor
{
    GENERATED_BODY()
public:
    ACABolt();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat") float Damage=12;
    bool bSpent=false;
    UFUNCTION() void Impact(UPrimitiveComponent* Component,AActor* OtherActor,
        UPrimitiveComponent* OtherComp,FVector NormalImpulse,const FHitResult& Hit);
};
