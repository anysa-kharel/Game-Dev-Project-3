#include "CAUtil.h"
#include "CACombatGameMode.h"
#include "WayfindingRules.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Sound/SoundBase.h"

void CA::Mesh(UStaticMeshComponent* Component,const TCHAR* Shape,const FVector& Size,
    const FLinearColor& Color,float Glow,bool Solid)
{
    Component->SetMobility(EComponentMobility::Movable);
    const FString Path=FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));
    Component->SetRelativeScale3D(Size/100.0);
    Component->SetCollisionProfileName(Solid ? TEXT("BlockAllDynamic") : TEXT("NoCollision"));
    Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(Glow<1);
    Tint(Component,Color,Glow);
}
void CA::Tint(UStaticMeshComponent* Component,const FLinearColor& Color,float Glow)
{
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/ClockArena/Materials/M_ArenaDaylight.M_ArenaDaylight"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    const bool DaylightMaterial=Material!=nullptr;
    if (!Material) Material=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/ClockArena/Materials/M_Arena.M_Arena"));
    if (!Material) Material=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Material) return;
    Component->SetMaterial(0,Material);
    const FVector4 Value(Color.R,Color.G,Color.B,Color.A);
    // Keep the same ambient floor if preparation has not yet created the new material.
    const float VisibleGlow=Glow+(DaylightMaterial ? 0.0f : Wayfinding::AmbientLift);
    if (!Component->GetWorld() || !Component->GetWorld()->IsGameWorld())
    {
        Component->SetDefaultCustomPrimitiveDataVector4(0,Value);
        Component->SetDefaultCustomPrimitiveDataFloat(4,VisibleGlow);
    }
    Component->SetCustomPrimitiveDataVector4(0,Value);
    Component->SetCustomPrimitiveDataFloat(4,VisibleGlow);
}
void CA::Sound(UWorld* World,const TCHAR* Name,const FVector& Location,float Volume)
{
    if (!World) return;
    // Lower music around existing effects without changing their playback gain.
    if (ACACombatGameMode* Mode=Game(World)) Mode->DuckMusic();
    const FString Path=FString::Printf(TEXT("/Game/ClockArena/Audio/%s.%s"),Name,Name);
    if (USoundBase* Effect=LoadObject<USoundBase>(nullptr,*Path))
        UGameplayStatics::PlaySoundAtLocation(World,Effect,Location,Volume);
}
ACACombatGameMode* CA::Game(UWorld* World)
{
    return World ? Cast<ACACombatGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
}
