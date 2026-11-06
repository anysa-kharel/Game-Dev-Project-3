#include "CACombatGameMode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

void ACACombatGameMode::UpdateMusic(float DeltaSeconds)
{
    MusicRetry-=DeltaSeconds;
    if (MusicRetry<=0 && (!MenuMusic || !GameMusic))
    {
        MusicRetry=2;
        auto Create=[this](const TCHAR* Name)->UAudioComponent*
        {
            const FString Path=FString::Printf(TEXT("/Game/ClockArena/Audio/%s.%s"),Name,Name);
            USoundWave* Loop=LoadObject<USoundWave>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
            if (!Loop) return nullptr;
            Loop->bLooping=true;
            UAudioComponent* Audio=UGameplayStatics::CreateSound2D(this,Loop,0,1,0,nullptr,false,false);
            if (Audio)
            {
                Audio->bIsUISound=true;Audio->bIsMusic=true;
                Audio->bOverridePriority=true;Audio->Priority=0.1f;
                Audio->Play();
            }
            return Audio;
        };
        if (!MenuMusic) MenuMusic=Create(TEXT("S_MenuLoop"));
        if (!GameMusic) GameMusic=Create(TEXT("S_GameAmbience"));
    }
    MusicMix.Step(DeltaSeconds,StartFlow.Waiting(),bPaused,bWon || (bReady && !PlayerHealth.Alive()),
        bMusicMuted,MenuMusicVolume,GameplayMusicVolume);
    if (MenuMusic) MenuMusic->SetVolumeMultiplier(static_cast<float>(MusicMix.Menu));
    if (GameMusic) GameMusic->SetVolumeMultiplier(static_cast<float>(MusicMix.Game));
}
void ACACombatGameMode::DuckMusic(float Hold) { MusicMix.Focus(Hold); }
void ACACombatGameMode::ToggleMusic()
{
    bMusicMuted=!bMusicMuted;
    // Mute is immediate, while unmuting returns gently on the next game tick.
    if (bMusicMuted)
    {
        MusicMix.Menu=MusicMix.Game=0;
        if (MenuMusic) MenuMusic->SetVolumeMultiplier(0);
        if (GameMusic) GameMusic->SetVolumeMultiplier(0);
    }
    ApplyPauseMusic();
}
void ACACombatGameMode::ApplyPauseMusic()
{
    // The UI music can continue quietly while the world and its effects are paused.
    // Set the paused level here because the game mode does not tick during pause.
    if (!GameMusic || StartFlow.Waiting()) return;
    if (bPaused && MenuMusic) { MenuMusic->SetVolumeMultiplier(0);MusicMix.Menu=0; }
    if (bMusicMuted) { GameMusic->SetVolumeMultiplier(0);return; }
    if (bPaused)
    {
        MusicMix.Game=FMath::Clamp(GameplayMusicVolume,0.0f,1.0f)*0.35;
        if (MusicMix.FocusLeft>0) MusicMix.Game*=PresentationAudio::DuckFactor;
    }
    GameMusic->SetVolumeMultiplier(static_cast<float>(MusicMix.Game));
}
void ACACombatGameMode::StopMusic()
{
    for (UAudioComponent* Audio : {MenuMusic.Get(),GameMusic.Get()})
        if (IsValid(Audio)) { Audio->Stop();Audio->DestroyComponent(); }
    MenuMusic=nullptr;GameMusic=nullptr;
}
