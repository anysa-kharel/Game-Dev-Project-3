#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UWorld;
class ACACombatGameMode;
namespace CA
{
    inline FLinearColor Navy() { return FLinearColor(0.025f,0.05f,0.08f); }
    inline FLinearColor Steel() { return FLinearColor(0.12f,0.18f,0.23f); }
    inline FLinearColor Cyan() { return FLinearColor(0.025f,0.75f,0.95f); }
    inline FLinearColor Gold() { return FLinearColor(0.95f,0.48f,0.08f); }
    inline FLinearColor Red() { return FLinearColor(0.95f,0.075f,0.045f); }
    inline FLinearColor Mint() { return FLinearColor(0.1f,0.9f,0.5f); }
    inline FLinearColor White() { return FLinearColor(0.8f,0.9f,1); }
    void Mesh(UStaticMeshComponent* Component,const TCHAR* Shape,const FVector& Size,
        const FLinearColor& Color,float Glow=0,bool Solid=true);
    void Tint(UStaticMeshComponent* Component,const FLinearColor& Color,float Glow=0);
    void Sound(UWorld* World,const TCHAR* Name,const FVector& Location,float Volume=0.5f);
    ACACombatGameMode* Game(UWorld* World);
}
