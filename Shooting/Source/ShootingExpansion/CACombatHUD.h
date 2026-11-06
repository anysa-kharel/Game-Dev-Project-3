#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CACombatHUD.generated.h"
class UTexture2D;

UCLASS()
class SHOOTINGEXPANSION_API ACACombatHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
private:
    // A reflected reference keeps the transient runtime image alive during the menu.
    UPROPERTY(Transient) TObjectPtr<UTexture2D> ControlsBackdrop;
    bool bBackdropChecked=false;
    float W=1280,H=720,S=1;
    void Label(const FString& Value,float X,float Y,float Size,const FLinearColor& Color);
    void Center(const FString& Value,float Y,float Size,const FLinearColor& Color);
    void Panel(float X,float Y,float Width,float Height,const FLinearColor& Accent);
    void DrawControls();
    void MenuText(const FString& Value,float X,float Y,float Height,const FLinearColor& Color,
        float MaxWidth=0,bool Bold=false,float WidthScale=1.0f,bool Centered=false);
};
