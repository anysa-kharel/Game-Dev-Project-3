#include "CACombatGameMode.h"
#include "CAArenaActors.h"
#include "CAEnemy.h"
#include "CAUtil.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include <cfloat>

int32 ACACombatGameMode::ActiveSection() const
{ return bCampaign ? Campaign::SectionForCheckpoint(CheckpointIndex) : CheckpointIndex<2 ? 1 : 2; }
int32 ACACombatGameMode::ActiveWave() const
{
    if (!bCampaign) return 0;
    for (const ACAEnemy* E : Enemies)
        if (IsValid(E) && !E->bDead && E->Section==ActiveSection() && E->Wave==0) return 0;
    return 1;
}
bool ACACombatGameMode::EnemyEnabled(const ACAEnemy* Enemy) const
{
    if (!Enemy || !IsActive()) return false;
    return bCampaign ? Enemy->Section==ActiveSection() && Enemy->Wave==ActiveWave()
        : Enemy->Section!=2 || CheckpointIndex>=2;
}
void ACACombatGameMode::StepCampaign()
{
    if (!bCampaign || !PlayerPawn) return;
    double Nearest=DBL_MAX;
    for (ACAClockBridge* Bridge : Clocks)
        if (IsValid(Bridge))
        {
            const double D=FVector::DistSquared(PlayerPawn->GetActorLocation(),Bridge->GetActorLocation());
            if (D<Nearest) { Clock=Bridge;Nearest=D; }
        }
    const int32 S=ActiveSection(),Wave=ActiveWave();
    if ((S!=AnnouncedSection || Wave!=AnnouncedWave) && RemainingInSection(S)>0)
    {
        AnnouncedSection=S;AnnouncedWave=Wave;
        Say(FString::Printf(TEXT("%s / WAVE %d OF 2 / %s"),UTF8_TO_TCHAR(Campaign::Names[S-1]),Wave+1,
            Wave==0 ? TEXT("Clear the first squad. Use the side routes.") : TEXT("Reinforcements! Watch for the new enemy types.")),5);
        CA::Sound(GetWorld(),TEXT("S_WaveAlert"),PlayerPawn->GetActorLocation(),0.35f);
    }
}
FString ACACombatGameMode::CampaignObjective() const
{
    const int32 S=ActiveSection();
    const auto Goal=Campaign::NextGoal(CheckpointIndex,RemainingInSection(S),PlayerPawn ? PlayerPawn->GetActorLocation().X : Campaign::StartX);
    switch (Goal)
    {
    case Campaign::Goal::Fight:
        return FString::Printf(TEXT("%d/5 / %s / WAVE %d OF 2"),S,UTF8_TO_TCHAR(Campaign::Names[S-1]),ActiveWave()+1);
    case Campaign::Goal::NearFlag:
        for (ACAClockBridge* Bridge : Clocks)
            if (IsValid(Bridge) && Bridge->SectionBefore==S && PlayerPawn &&
                ClockLessons::AtApproach(PlayerPawn->GetActorLocation().X-Bridge->GetActorLocation().X) &&
                (S==2 || S==3)) return Bridge->LessonHint();
        return FString::Printf(TEXT("SECTOR CLEAR / WALK THROUGH FLAG %d"),CheckpointIndex+1);
    case Campaign::Goal::Board: case Campaign::Goal::FarFlag:
        for (ACAClockBridge* Bridge : Clocks)
            if (IsValid(Bridge) && Bridge->SectionBefore==S) return Bridge->LessonHint();
        return FString::Printf(TEXT("CLOCK %d / FOLLOW THE GOLD FLAG"),S);
    default: return TEXT("CITADEL SECURED / REACH THE GOLD EXIT");
    }
}
FString ACACombatGameMode::CampaignWaypointLabel() const
{
    const auto Goal=Campaign::NextGoal(CheckpointIndex,RemainingInSection(ActiveSection()),PlayerPawn ? PlayerPawn->GetActorLocation().X : Campaign::StartX);
    switch (Goal)
    {
    case Campaign::Goal::Fight: return TEXT("ACTIVE SQUAD / CLEAR ROBOTS");
    case Campaign::Goal::NearFlag: case Campaign::Goal::FarFlag:
        return FString::Printf(TEXT("FLAG %d / CHECKPOINT"),CheckpointIndex+1);
    case Campaign::Goal::Board:
        return Clock && !CombatRules::AtDock(Clock->Angle(),false) ? TEXT("WAIT FOR THE GOLD HAND") : TEXT("STEP ONTO THE GOLD HAND");
    default: return TEXT("GOLD FINAL EXIT");
    }
}
FVector ACACombatGameMode::CampaignWaypoint() const
{
    const FVector P=PlayerPawn ? PlayerPawn->GetActorLocation() : FVector(Campaign::StartX,0,110);
    const auto Goal=Campaign::NextGoal(CheckpointIndex,RemainingInSection(ActiveSection()),P.X);
    if (Goal==Campaign::Goal::Fight)
    {
        ACAEnemy* Nearest=nullptr; double Distance=DBL_MAX;
        for (ACAEnemy* Enemy : Enemies)
            if (IsValid(Enemy) && !Enemy->bDead && Enemy->Section==ActiveSection() && Enemy->Wave==ActiveWave())
            {
                const double D=FVector::DistSquared(P,Enemy->GetActorLocation());
                if (D<Distance) { Distance=D;Nearest=Enemy; }
            }
        if (Nearest) return Nearest->GetActorLocation()+FVector(0,0,150);
    }
    if (Goal==Campaign::Goal::Board)
        for (ACAClockBridge* Bridge : Clocks)
            if (IsValid(Bridge) && Bridge->SectionBefore==ActiveSection())
                return Bridge->GetActorLocation()+FVector(Wayfinding::BoardingOffset(Bridge->HandLength,false,
                    CombatRules::AtDock(Bridge->Angle(),false)),0,180);
    for (TActorIterator<ACAZone> It(GetWorld());It;++It)
        if ((Goal==Campaign::Goal::Exit && It->Kind==ECAZoneKind::Exit) ||
            (Goal!=Campaign::Goal::Exit && It->Kind==ECAZoneKind::Checkpoint && It->CheckpointIndex==CheckpointIndex+1))
            return It->GetActorLocation()+FVector(0,-180,400);
    return FVector(Campaign::ExitX,0,500);
}
