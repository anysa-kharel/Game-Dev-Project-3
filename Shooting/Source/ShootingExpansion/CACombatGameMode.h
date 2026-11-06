#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CombatRules.h"
#include "WayfindingRules.h"
#include "CampaignRules.h"
#include "CAInterfaceRules.h"
#include "CACombatGameMode.generated.h"
class ACAEnemy;
class ACAClockBridge;
class UDamageType;
class UUserWidget;

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
    UPROPERTY() TArray<TObjectPtr<ACAClockBridge>> Clocks;
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
    bool bCampaign=false;
    CAInterface::StartGate StartFlow;
    int32 AnnouncedSection=0,AnnouncedWave=-1;
    FString Message;
    bool IsActive() const { return bReady && StartFlow.Running() && !bPaused && !bWon && PlayerHealth.Alive(); }
    void RequestStart();
    int32 RemainingEnemies() const;
    int32 RemainingInSection(int32 Section) const;
    int32 ActiveSection() const;
    int32 ActiveWave() const;
    bool EnemyEnabled(const ACAEnemy* Enemy) const;
    FString CampaignObjective() const;
    FString CampaignWaypointLabel() const;
    FVector CampaignWaypoint() const;
    void StepCampaign();
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
    Wayfinding::Stage GuidanceStage() const;
    FString WaypointLabel() const;
    FVector Waypoint() const;
    FVector PlayerAimPoint() const;
    UFUNCTION() void PlayerDamaged(AActor* Actor,float Damage,const UDamageType* Type,
        AController* InstigatorController,AActor* Causer);
private:
    void SetupPlayer();
    void StopPlayerFire();
    void RefillMagazine();
    void ActorSpawned(AActor* Actor);
    void ShowControls();
    void BeginRun();
    void HideTemplateHUD();
    void RestoreTemplateHUD();
    bool bPriorClickEvents=false,bPriorMouseOver=false;
    TArray<TWeakObjectPtr<UUserWidget>> IntroWidgets;
    TArray<uint8> IntroVisibility;
    FDelegateHandle BulletSpawnHandle;
};
