#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatRules.h"
#include "CACombatGameMode.generated.h"
class ACAEnemy;
class ACAClockBridge;
class UDamageType;

UCLASS()
class SHOOTINGEXPANSION_API ACACombatGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACACombatGameMode();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY() TObjectPtr<APawn> PlayerPawn;
    UPROPERTY() TObjectPtr<ACAClockBridge> Clock;
    UPROPERTY() TArray<TObjectPtr<ACAEnemy>> Enemies;
    UPROPERTY() TSubclassOf<AActor> TemplateBulletClass;
    CombatRules::Health PlayerHealth;
    int32 CheckpointIndex=0;
    int32 TotalEnemies=0;
    int32 Defeated=0;
    int32 Retries=0;
    float RunSeconds=0;
    float HitMarker=0;
    float DamageFlash=0;
    float MessageTime=0;
    float RecoveryTime=0;
    bool bReady=false;
    bool bPaused=false;
    bool bWon=false;
    bool bHelp=false;
    FString Message;
    bool IsActive() const { return bReady && !bPaused && !bWon && PlayerHealth.Alive(); }
    int32 RemainingEnemies() const;
    int32 RemainingInSection(int32 Section) const;
    void EnemyDefeated(ACAEnemy* Enemy);
    void SaveCheckpoint(int32 Index);
    void RestorePlayer();
    void RetryCheckpoint();
    void TogglePause();
    void Confirm();
    void ToggleHelp();
    void TryWin();
    void Say(const FString& Text,float Duration=4);
    FString Objective() const;
    FVector Waypoint() const;
    FVector PlayerAimPoint() const;
    UFUNCTION() void PlayerDamaged(AActor* Actor,float Damage,const UDamageType* Type,
        AController* InstigatorController,AActor* Causer);
private:
    void SetupPlayer();
    void StopPlayerFire();
    void RefillMagazine();
    void ActorSpawned(AActor* Actor);
    FDelegateHandle BulletSpawnHandle;
};
