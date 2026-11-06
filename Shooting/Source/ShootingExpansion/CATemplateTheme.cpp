#include "CACombatGameMode.h"
#include "CATheme.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{
    void AddAmmoCaption(UUserWidget* PlayerUI)
    {
        // Leave the original CurrentAmmo/TotalAmmo text bindings intact.
        UWidgetTree* Tree=PlayerUI->WidgetTree;
        if (!Tree || Tree->FindWidget(TEXT("CAAmmoCaption"))) return;
        UTextBlock* AmmoValue=Cast<UTextBlock>(Tree->FindWidget(TEXT("CurrentAmmo")));
        UCanvasPanel* AmmoCanvas=AmmoValue ? Cast<UCanvasPanel>(AmmoValue->GetParent()) : nullptr;
        UCanvasPanelSlot* AmmoValueSlot=AmmoValue ? Cast<UCanvasPanelSlot>(AmmoValue->Slot) : nullptr;
        if (!AmmoCanvas || !AmmoValueSlot) return;

        UTextBlock* AmmoLabel=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("CAAmmoCaptionText"));
        AmmoLabel->SetText(FText::FromString(TEXT("AMMO")));
        AmmoLabel->SetColorAndOpacity(FSlateColor(CATheme::Gold()));
        AmmoLabel->SetShadowColorAndOpacity(FLinearColor::Transparent);
        FSlateFontInfo AmmoFont=AmmoValue->GetFont();
        AmmoFont.Size=16;AmmoFont.OutlineSettings.OutlineSize=0;
        AmmoLabel->SetFont(AmmoFont);

        UBorder* AmmoCaption=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("CAAmmoCaption"));
        AmmoCaption->SetBrushColor(CATheme::PanelFill());
        AmmoCaption->SetPadding(FMargin(8,3));
        AmmoCaption->SetContent(AmmoLabel);
        AmmoCaption->SetVisibility(ESlateVisibility::HitTestInvisible);
        UCanvasPanelSlot* AmmoCaptionSlot=AmmoCanvas->AddChildToCanvas(AmmoCaption);
        AmmoCaptionSlot->SetAnchors(AmmoValueSlot->GetAnchors());
        AmmoCaptionSlot->SetAlignment(AmmoValueSlot->GetAlignment());
        AmmoCaptionSlot->SetPosition(AmmoValueSlot->GetPosition()+FVector2D(0,-40));
        AmmoCaptionSlot->SetAutoSize(true);
        AmmoCaptionSlot->SetZOrder(AmmoValueSlot->GetZOrder()+1);
    }
}

void ACACombatGameMode::StyleTemplateHUD()
{
    TArray<UUserWidget*> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Widgets,UUserWidget::StaticClass(),true);
    for (UUserWidget* Widget : Widgets)
    {
        if (!IsValid(Widget) || !Widget->WidgetTree ||
            !Widget->GetClass()->GetName().Contains(TEXT("WBP_PlayerUI"))) continue;
        if (ThemedWidgets.ContainsByPredicate([Widget](const TWeakObjectPtr<UUserWidget>& Item){return Item.Get()==Widget;})) continue;
        TArray<UWidget*> WidgetChildren;Widget->WidgetTree->GetAllWidgets(WidgetChildren);
        for (UWidget* Child : WidgetChildren)
        {
            if (UTextBlock* Text=Cast<UTextBlock>(Child))
            {
                Text->SetColorAndOpacity(FSlateColor(CATheme::White()));
                Text->SetShadowColorAndOpacity(FLinearColor::Transparent);
                FSlateFontInfo Font=Text->GetFont();
                Font.OutlineSettings.OutlineSize=0;Text->SetFont(Font);
            }
            else if (UImage* Image=Cast<UImage>(Child)) Image->SetColorAndOpacity(CATheme::Gold());
            else if (UProgressBar* Bar=Cast<UProgressBar>(Child)) Bar->SetFillColorAndOpacity(CATheme::Gold());
            else if (UBorder* Border=Cast<UBorder>(Child)) Border->SetBrushColor(CATheme::Ink());
        }
        AddAmmoCaption(Widget);
        ThemedWidgets.Add(Widget);
    }
}
