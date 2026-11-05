#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CAArenaBuilder.generated.h"

UCLASS(Blueprintable)
class SHOOTINGEXPANSION_API ACAArenaBuilder : public AActor
{
    GENERATED_BODY()
public:
    ACAArenaBuilder();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Arena") bool bBuilt=false;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Arena") void BuildArena();
private:
    void Block(const FVector& Location,const FVector& Size,const FLinearColor& Color,
        bool Solid=true,float Glow=0,const FRotator& Rotation=FRotator::ZeroRotator,
        const TCHAR* Shape=TEXT("Cube"));
    void Sign(const FVector& Location,const FString& Text,float Size=36,
        const FLinearColor& Color=FLinearColor::White,const FRotator& Rotation=FRotator(0,180,0));
    void Floor(float X,float Length,float Width=3400,float Height=0);
    void Rail(float X,float Length,float Y,float Height=0);
    void Room(float X,float Length);
    void Gallery(float Start,float End,float Y);
    void Checkpoint(float X,int32 Index);
};
