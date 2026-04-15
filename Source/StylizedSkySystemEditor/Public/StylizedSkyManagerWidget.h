#pragma once

#include "CoreMinimal.h"

class AStylizedSkyActor;

/**
 * Stylized sky manager widget.
 */
class STYLIZEDSKYSYSTEMEDITOR_API SStylizedSkyManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStylizedSkyManagerWidget) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Create stylized sky system actor button */
	TSharedPtr<class SButton> ButtonCreateStylizedSkyActor;

	/** Detail panel prt */
	TSharedPtr<class IDetailsView> DetailsView;

	/** Detail panel property visible. */
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	
	/** Create reply */
	FReply OnButtonCreateStylizedSkyActor();
    
	/** 查找场景中的天空系统Actor */
	AStylizedSkyActor* FindSkySystemActor() const;
    
	/** 创建按钮面板 */
	TSharedRef<SWidget> CreateButtonPanel();
};
