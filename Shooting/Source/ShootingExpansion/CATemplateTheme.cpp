#include "CACombatGameMode.h"
#include "CATheme.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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
        ThemedWidgets.Add(Widget);
    }
}
