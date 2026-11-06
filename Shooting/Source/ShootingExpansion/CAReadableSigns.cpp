#include "CAReadableSigns.h"
#include "CAArenaActors.h"
#include "CAInterfaceRules.h"
#include "CAUtil.h"
#include "CATheme.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

FLinearColor CAReadability::Ink()
{ return CATheme::Ink(); }
void CAReadability::Lettering(UTextRenderComponent* Text)
{
    if (!Text) return;
    Text->SetTextRenderColor(CATheme::White().ToFColor(true));Text->SetCastShadow(false);
    if (UMaterialInterface* FontMaterial=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque.DefaultTextMaterialOpaque"),nullptr,LOAD_NoWarn|LOAD_Quiet))
        Text->SetTextMaterial(FontMaterial);
}
void CAReadability::Surface(UStaticMeshComponent* Mesh,const FLinearColor& Color)
{
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/ClockArena/Materials/M_SignSurface.M_SignSurface"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if (Material) Mesh->SetMaterial(0,Material); else CA::Tint(Mesh,Color,0);
    const FVector4 Value(Color.R,Color.G,Color.B,1);
    if (!Mesh->GetWorld() || !Mesh->GetWorld()->IsGameWorld()) Mesh->SetDefaultCustomPrimitiveDataVector4(0,Value);
    Mesh->SetCustomPrimitiveDataVector4(0,Value);Mesh->SetCastShadow(false);
}
namespace CAReadableDetail
{
    ACAScenery* Part(ATextRenderActor* Owner,FName Tag,const FVector& Size,const FVector& Offset,const FLinearColor& Color)
    {
        ACAScenery* Found=nullptr;
        for (TActorIterator<ACAScenery> It(Owner->GetWorld());It;++It)
            if (It->GetOwner()==Owner && It->Tags.Contains(Tag))
            {
                if (!Found) Found=*It; else It->Destroy();
            }
        FTransform T=Owner->GetTextRender()->GetComponentTransform();
        T.SetLocation(T.TransformPosition(Offset));
        if (!Found)
        {
            Found=Owner->GetWorld()->SpawnActorDeferred<ACAScenery>(ACAScenery::StaticClass(),T,Owner,nullptr,
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!Found) return nullptr;
            Found->Tags.Add(Tag);Found->Tags.Add(TEXT("CAReadabilityPanel"));Found->Tags.Add(TEXT("CAExpansion"));
            Found->Dimensions=Size;Found->Color=Color;Found->bSolid=false;
            Found->FinishSpawning(T);
        }
        else
        {
            Found->Dimensions=Size;Found->Color=Color;Found->bSolid=false;Found->SetActorTransform(T);
            Found->OnConstruction(T);
        }
        Found->AttachToComponent(Owner->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
        CAReadability::Surface(Found->Mesh,Color);
#if WITH_EDITOR
        Found->SetFolderPath(FName(TEXT("ClockArena/ReadableSigns")));
        Found->SetActorLabel(Tag==TEXT("CAInkBoard") ? TEXT("Sign / Ink Backplate") : TEXT("Sign / Accent Edge"));
#endif
        return Found;
    }
}
void CAReadability::Style(ATextRenderActor* Sign)
{
    if (!Sign) return;
    UTextRenderComponent* Text=Sign->GetTextRender();
    Text->SetHorizontalAlignment(EHTA_Center);Text->SetVerticalAlignment(EVRTA_TextCenter);
    Lettering(Text);
    const FVector Size=Text->GetTextLocalSize();
    const float Width=FMath::Max(static_cast<float>(Size.Y),Text->WorldSize*2)+64;
    const float Height=FMath::Max(static_cast<float>(Size.Z),Text->WorldSize)+36;
    CAReadableDetail::Part(Sign,TEXT("CAInkBoard"),FVector(8,Width,Height),FVector(-8,0,0),Ink());
    CAReadableDetail::Part(Sign,TEXT("CAInkEdge"),FVector(2,Width,7),FVector(-3,0,-Height*0.5f+4),
        CATheme::Gold());
}
void CAReadability::Refresh(UWorld* World)
{
    if (!World) return;
    TArray<ACAScenery*> Orphans;
    for (TActorIterator<ACAScenery> It(World);It;++It)
        if (It->Tags.Contains(TEXT("CAReadabilityPanel")) && !IsValid(It->GetOwner())) Orphans.Add(*It);
    for (ACAScenery* Part : Orphans) Part->Destroy();
    for (TActorIterator<ATextRenderActor> It(World);It;++It)
        if (It->Tags.Contains(TEXT("CAExpansion"))) Style(*It);
}
