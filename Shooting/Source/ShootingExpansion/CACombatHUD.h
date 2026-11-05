#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CACombatHUD.generated.h"

UCLASS()
class SHOOTINGEXPANSION_API ACACombatHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    float W=1280,H=720,S=1;
    void Label(const FString& Value,float X,float Y,float Size,const FLinearColor& Color);
    void Center(const FString& Value,float Y,float Size,const FLinearColor& Color);
    void Panel(float X,float Y,float Width,float Height,const FLinearColor& Accent);
};
