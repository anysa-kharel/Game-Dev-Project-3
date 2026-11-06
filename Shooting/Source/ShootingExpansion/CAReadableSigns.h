#pragma once
#include "CoreMinimal.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class ATextRenderActor;
class UWorld;

namespace CAReadability
{
    FLinearColor Ink();
    void Lettering(UTextRenderComponent* Text);
    void Surface(UStaticMeshComponent* Mesh,const FLinearColor& Color);
    void Style(ATextRenderActor* Sign);
    void Refresh(UWorld* World);
}
