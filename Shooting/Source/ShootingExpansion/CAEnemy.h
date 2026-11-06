#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatRules.h"
#include "CampaignRules.h"
#include "CAEnemy.generated.h"
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECAEnemyKind : uint8 { Guard, Scout, Sentry, Skirmisher, ShieldGuard, Bomber, Warden };

UCLASS(Blueprintable)
class SHOOTINGEXPANSION_API ACAEnemy : public ACharacter
{
    GENERATED_BODY()
public:
    ACAEnemy();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,
        AController* EventInstigator,AActor* DamageCauser) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Eye;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gun;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LeftLeg;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RightLeg;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shield;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Detail;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DangerRing;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Enemy") ECAEnemyKind Kind=ECAEnemyKind::Guard;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Enemy") int32 Section=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Enemy") int32 Wave=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Enemy") int32 MaxHits=3;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Enemy") int32 HitsLeft=3;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Enemy") bool bDead=false;
    FVector Home=FVector::ZeroVector;
    FVector LastSeen=FVector::ZeroVector;
    bool bAlert=false;
    float TimeSinceSeen=100;
    float AnimationAge=0;
    float HitFlash=0;
    CombatRules::Attack AttackState;
    Campaign::Charge TacticalAttack;
    int32 ShieldHits=2;
    bool bDormant=false;
    FString TypeName() const;
    FLinearColor TypeColor() const;
    bool SeesPlayer() const;
    void WalkToward(const FVector& Goal,float Scale=1);
    void Fire();
    void ResetGuard();
    void Die(const FVector& From);
};
