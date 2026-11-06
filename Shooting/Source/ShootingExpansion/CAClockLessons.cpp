#include "CAArenaActors.h"
#include "CAArenaBuilder.h"
#include "CACombatGameMode.h"
#include "CAReadableSigns.h"
#include "CATheme.h"
#include "CAUtil.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

namespace
{
    const FName CoursePart=TEXT("CAClockLessonPart");
    FName DockTag(int32 Stage) { return FName(*FString::Printf(TEXT("CAClockLessonDock%d"),Stage)); }
    bool OriginalDockPiece(const ACAScenery* Piece,const ACAClockBridge* Bridge)
    {
        const FVector Offset=Piece->GetActorLocation()-Bridge->GetActorLocation();
        return ClockLessons::OriginalDockPiece(Offset.X,Offset.Y,Offset.Z,Piece->Dimensions.X);
    }
}

void ACAClockBridge::BuildLessonGeometry()
{
    bLessonGeometryBuilt=false;LessonBeams.Empty();
    TArray<UActorComponent*> OldLessonParts;
    GetComponents(OldLessonParts);
    for (UActorComponent* Piece : OldLessonParts)
        if (Piece && Piece->ComponentHasTag(CoursePart)) Piece->DestroyComponent();
    const int32 Stage=LessonStage();
    if (!Stage) return;

    auto CourseMesh=[this](USceneComponent* Parent,const FVector& Position,const FVector& Size,
        const FLinearColor& Tint,bool Solid)->UStaticMeshComponent*
    {
        UStaticMeshComponent* Piece=NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(Piece);Piece->ComponentTags.Add(CoursePart);
        Piece->SetupAttachment(Parent);
        CA::Mesh(Piece,TEXT("Cube"),Size,Tint,0,Solid);
        Piece->SetRelativeLocation(Position);Piece->RegisterComponent();
        if (!Solid) { Piece->SetCastShadow(false);CAReadability::Surface(Piece,Tint); }
        return Piece;
    };
    auto CourseSign=[this,&CourseMesh](const FVector& Position,const FString& Caption,float Width,float Height,float FontSize)
    {
        CourseMesh(Root,Position+FVector(10,0,0),FVector(16,Width,Height),CATheme::Ink(),false);
        CourseMesh(Root,Position+FVector(0,-Width*0.5f+5,0),FVector(20,8,Height),CATheme::Gold(),false);
        UTextRenderComponent* Words=NewObject<UTextRenderComponent>(this);
        AddInstanceComponent(Words);Words->ComponentTags.Add(CoursePart);
        Words->SetupAttachment(Root);Words->SetMobility(EComponentMobility::Movable);
        Words->SetRelativeLocation(Position);Words->SetRelativeRotation(FRotator(0,180,0));
        Words->SetHorizontalAlignment(EHTA_Center);Words->SetVerticalAlignment(EVRTA_TextCenter);
        Words->SetWorldSize(FontSize);Words->SetText(FText::FromString(Caption));
        CAReadability::Lettering(Words);Words->RegisterComponent();
    };

    // The first bridge stays simple. Later ones reuse the same controls.
    const TCHAR* Titles[]={TEXT(""),TEXT("CLOCK 1 / LEARN TO RIDE"),TEXT("CLOCK 2 / CROUCH ON THE HAND"),
        TEXT("CLOCK 3 / JUMP TO THE DOCK"),TEXT("CLOCK 4 / CROUCH, THEN JUMP")};
    CourseSign(FVector(-1550,0,430),Titles[Stage],1450,90,35);
    CourseMesh(Pivot,FVector(HandLength-35,0,5),FVector(65,HandWidth-30,8),CATheme::Highlight(),false);

    if (Stage==2)
    {
        // Safe floor underneath: players learn crouching before the moving beam.
        UStaticMeshComponent* PracticeBeam=CourseMesh(Root,FVector(ClockLessons::PracticeX,0,
            CurrentBeamClearance+ClockLessons::BeamHeight*0.5),
            FVector(180,1040,ClockLessons::BeamHeight),CATheme::Gold(),true);
        LessonBeams.Add(PracticeBeam);
        for (int32 Side : {-1,1})
            CourseMesh(Root,FVector(ClockLessons::PracticeX,Side*550,125),FVector(180,45,250),CATheme::Muted(),true);
        CourseSign(FVector(-2480,0,320),TEXT("SAFE PRACTICE / C TO CROUCH UNDER THE ARCH"),1500,72,28);
    }
    if (Stage==3)
    {
        // Higher than the template's normal step height, with safe floor all around.
        CourseMesh(Root,FVector(ClockLessons::PracticeX,0,ClockLessons::PracticeTop*0.5),
            FVector(ClockLessons::PracticeWidth,1040,ClockLessons::PracticeTop),CATheme::Gold(),true);
        CourseMesh(Root,FVector(-2490,0,4),FVector(18,900,6),CATheme::Highlight(),false);
        CourseSign(FVector(-2600,0,320),TEXT("SAFE PRACTICE / HOLD W, THEN SPACE AT THE GOLD STRIPE"),1800,72,28);
    }
    if (ClockLessons::Crouch(Stage))
    {
        // Fixed to the root, so the hand rotates underneath this gantry at 12.
        UStaticMeshComponent* RidingBeam=CourseMesh(Root,FVector(0,-600,
            CurrentBeamClearance+ClockLessons::BeamHeight*0.5),
            FVector(ClockLessons::BeamThickness,1600,ClockLessons::BeamHeight),CATheme::Gold(),true);
        LessonBeams.Add(RidingBeam);
        for (int32 Side : {-1,1})
            CourseMesh(Root,FVector(Side*90,-1440,130),FVector(50,50,260),CATheme::Muted(),true);
        CourseSign(FVector(-1100,-350,340),TEXT("LOW GANTRY AT 12 / C TO CROUCH / STAY LOW UNTIL CLEAR"),1700,72,26);
    }
    if (ClockLessons::Jump(Stage))
    {
        const auto Dock=ClockLessons::Landing(HandLength);
        CourseMesh(Root,FVector(Dock.Start+100,0,5),FVector(22,900,8),CATheme::Highlight(),false);
        CourseSign(FVector(Dock.Start+300,0,330),TEXT("LAND HERE / KEEP W HELD, THEN SPACE AT THE GOLD TIP"),1900,80,27);
        CourseSign(FVector(-1200,0,290),TEXT("THE HAND WAITS AT 3 / WALK TO THE GOLD TIP, THEN JUMP"),1850,76,26);
    }
    else CourseSign(FVector(-1200,0,290),TEXT("BOARD AT 9 / RIDE TO 3 / STEP ONTO THE NEXT DOCK"),1600,76,28);
    bLessonGeometryBuilt=true;
}

void ACAClockBridge::UpdateLessonGeometry()
{
    if (!LessonStage()) return;
    ACharacter* Rider=UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
    if (!Rider || !Rider->GetCapsuleComponent()) return;
    const float FullHeight=Rider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()*2;
    // The template uses bCrouching + SetCapsuleHalfHeight, not native crouch state.
    const FBoolProperty* CrouchFlag=FindFProperty<FBoolProperty>(Rider->GetClass(),TEXT("bCrouching"));
    const bool IsLow=CrouchFlag && CrouchFlag->GetPropertyValue_InContainer(Rider);
    if (!IsLow) StandingCapsuleHeight=FMath::Max(StandingCapsuleHeight,FullHeight);
    else if (StandingCapsuleHeight>0)
        CrouchedCapsuleHeight=CrouchedCapsuleHeight>0 ? FMath::Min(CrouchedCapsuleHeight,FullHeight) : FullHeight;
    const float Clearance=static_cast<float>(ClockLessons::Clearance(StandingCapsuleHeight,CrouchedCapsuleHeight));
    if (FMath::Abs(Clearance-CurrentBeamClearance)>0.5f)
    {
        CurrentBeamClearance=Clearance;
        for (UStaticMeshComponent* Beam : LessonBeams)
            if (IsValid(Beam))
            {
                FVector Position=Beam->GetRelativeLocation();
                Position.Z=Clearance+ClockLessons::BeamHeight*0.5;
                Beam->SetRelativeLocation(Position);
            }
    }
    const FVector Offset=Rider->GetActorLocation()-GetActorLocation();
    if (bStarted && Rider->GetMovementBase()==Hand.Get() &&
        ClockLessons::ClearOfBeam(Offset.X,Offset.Y,Rider->GetCapsuleComponent()->GetScaledCapsuleRadius()))
        bPassedLessonBeam=true;
    if (bStarted && Age>Period*0.8 && CombatRules::AtDock(Angle(),false)) bPassedLessonBeam=false;
}

FString ACAClockBridge::LessonHint() const
{
    ACharacter* Rider=UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
    const double X=Rider ? Rider->GetActorLocation().X-GetActorLocation().X : -2000;
    const bool Riding=Rider && Rider->GetMovementBase()==Hand.Get();
    const auto Cue=ClockLessons::Hint(LessonStage(),X,bStarted,Riding,bPassedLessonBeam,
        CombatRules::AtDock(Angle(),true));
    switch (Cue)
    {
    case ClockLessons::Cue::PracticeCrouch: return TEXT("SAFE PRACTICE / C: CROUCH UNDER THE ARCH");
    case ClockLessons::Cue::PracticeJump: return TEXT("SAFE PRACTICE / W + SPACE: JUMP THE LOW BLOCK");
    case ClockLessons::Cue::Board: return ClockLessons::Crouch(LessonStage())
        ? TEXT("BOARD AT 9 / C: CROUCH BEFORE THE LOW GANTRY") : TEXT("BOARD THE GOLD HAND AT 9 / RIDE TO 3");
    case ClockLessons::Cue::Crouch: return TEXT("C: STAY CROUCHED / RIDE UNDER THE LOW GANTRY");
    case ClockLessons::Cue::Stand: return ClockLessons::Jump(LessonStage())
        ? TEXT("BEAM CLEARED / STAND UP / MOVE TOWARD THE GOLD TIP") : TEXT("BEAM CLEARED / STAND UP / RIDE TO 3 AND STEP OFF");
    case ClockLessons::Cue::JumpExit: return TEXT("HAND PARKED / KEEP W HELD, THEN SPACE AT THE GOLD TIP");
    default: return ClockLessons::Jump(LessonStage()) ? TEXT("RIDE TO 3 / THE HAND WILL WAIT FOR YOUR JUMP")
        : TEXT("RIDE TO 3 / STEP OFF AT THE GOLD FLAG");
    }
}

void ACAArenaBuilder::RefreshClockLessons()
{
    if (!GetWorld() || !bCampaignLayout) return;
    for (TActorIterator<ACAClockBridge> Bridge(GetWorld());Bridge;++Bridge)
    {
        const int32 Stage=Bridge->LessonStage();
        if (!ClockLessons::Jump(Stage)) continue;
        const auto Dock=ClockLessons::Landing(Bridge->HandLength);
        const FName Marker=DockTag(Stage);
        for (TActorIterator<ACAScenery> It(GetWorld());It;++It)
        {
            ACAScenery* Piece=*It;
            if (!Piece->Tags.Contains(Marker) && !OriginalDockPiece(Piece,*Bridge)) continue;
            Piece->Tags.AddUnique(Marker);
            Piece->Dimensions.X=Dock.Length;
            FVector Position=Piece->GetActorLocation();Position.X=Bridge->GetActorLocation().X+Dock.Center;
            Piece->SetActorLocation(Position);Piece->OnConstruction(Piece->GetActorTransform());
        }
    }
}

bool ACAArenaBuilder::ValidateClockLessons() const
{
    if (!GetWorld() || !bCampaignLayout) return false;
    int32 Seen[5]={0,0,0,0,0};
    for (TActorIterator<ACAClockBridge> Bridge(GetWorld());Bridge;++Bridge)
    {
        const int32 Stage=Bridge->LessonStage();
        if (!Stage) continue;
        if (!Bridge->LessonGeometryReady()) return false;
        ++Seen[Stage];
        if (!ClockLessons::Jump(Stage)) continue;
        int32 DockPieces=0;const auto Dock=ClockLessons::Landing(Bridge->HandLength);
        for (TActorIterator<ACAScenery> It(GetWorld());It;++It)
            if (It->Tags.Contains(DockTag(Stage)))
            {
                const double X=It->GetActorLocation().X-Bridge->GetActorLocation().X;
                if (FMath::Abs(X-Dock.Center)>1 || FMath::Abs(It->Dimensions.X-Dock.Length)>1) return false;
                ++DockPieces;
            }
        // Floor, two floor stripes, two rails and two rail caps must all be trimmed.
        if (DockPieces!=7) return false;
    }
    for (int32 Stage=1;Stage<=4;++Stage) if (Seen[Stage]!=1) return false;
    return true;
}
