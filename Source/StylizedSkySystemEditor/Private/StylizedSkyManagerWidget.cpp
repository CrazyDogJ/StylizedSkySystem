// Fill out your copyright notice in the Description page of Project Settings.


#include "StylizedSkyManagerWidget.h"
#include "StylizedSkyActor.h"
#include "EngineUtils.h"

void SStylizedSkyManagerWidget::Construct(const FArguments& InArgs)
{
	// Detail view panel.
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SStylizedSkyManagerWidget::GetIsPropertyVisible));
	DetailsView->SetDisableCustomDetailLayouts(true);	// This is to not have transforms and other special viewer
	
	ChildSlot
	[
		SNew(SBorder)
		.Padding(10.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
			.Orientation(Orient_Vertical)
			.ScrollBarAlwaysVisible(false)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 10)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("StylizedSkySystem", "WindowTitle", "Stylized Sky System Manager"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Justification(ETextJustify::Center)
				]
			]
			+ SScrollBox::Slot()
			[
				CreateButtonPanel()
			]
            + SScrollBox::Slot()
			[
				DetailsView->AsShared()
			]
		]
	];
}

void SStylizedSkyManagerWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const auto FoundActor = FindSkySystemActor();
	DetailsView->SetObject(FoundActor);
	ButtonCreateStylizedSkyActor->SetVisibility(FoundActor ? EVisibility::Collapsed : EVisibility::Visible);
}

bool SStylizedSkyManagerWidget::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	const bool bVisible = PropertyAndParent.Property.HasMetaData("PropertyVisible");
	
	bool bIsParentPropertyVisible = false;
	for (const auto ParentProperty : PropertyAndParent.ParentProperties)
	{
		if (ParentProperty->HasMetaData("PropertyVisible"))
		{
			bIsParentPropertyVisible = true;
			break;
		}
	}
	
	return bVisible || bIsParentPropertyVisible;
}

FReply SStylizedSkyManagerWidget::OnButtonCreateStylizedSkyActor()
{
	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FReply::Handled();
	}

	const FTransform Transform(FVector(0.0f, 0.0f, 0.0f));
	Cast<AStylizedSkyActor>(GEditor->AddActor(World->GetCurrentLevel(), AStylizedSkyActor::StaticClass(), Transform));

	return FReply::Handled();
}

AStylizedSkyActor* SStylizedSkyManagerWidget::FindSkySystemActor() const
{
	AStylizedSkyActor* FoundActor = nullptr;

	if (const UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (TActorIterator<AStylizedSkyActor> It(World, AStylizedSkyActor::StaticClass()); It)
		{
			FoundActor = *It;
			return FoundActor;
		}
	}
    
	return FoundActor;
}

TSharedRef<SWidget> SStylizedSkyManagerWidget::CreateButtonPanel()
{
	auto NewButton = 
		SNew(SButton)
			.Text(NSLOCTEXT("StylizedSkySystem", "CreateSkySystem", "Create Sky System"))
			.OnClicked(this, &SStylizedSkyManagerWidget::OnButtonCreateStylizedSkyActor)
			.HAlign(HAlign_Center);

	ButtonCreateStylizedSkyActor = NewButton;
	return NewButton;
}
