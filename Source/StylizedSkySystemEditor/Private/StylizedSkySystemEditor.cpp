#include "StylizedSkySystemEditor.h"

#include "StylizedSkyManagerWidget.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FStylizedSkySystemEditorModule"

const FName FStylizedSkySystemEditorModule::SkySystemTabName(TEXT("StylizedSkySystem"));

void FStylizedSkySystemEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SkySystemTabName, 
		FOnSpawnTab::CreateRaw(this, &FStylizedSkySystemEditorModule::OnSpawnSkySystemTab))
		.SetDisplayName(NSLOCTEXT("StylizedSkySystem", "TabTitle", "Stylized Sky System Manager"))
		.SetTooltipText(NSLOCTEXT("StylizedSkySystem", "TabTooltip", "Open Stylized Sky System Manager"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());
	
	RegisterMenuExtensions();
}

void FStylizedSkySystemEditorModule::ShutdownModule()
{
	UnregisterMenuExtensions();
	
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SkySystemTabName);
}

void FStylizedSkySystemEditorModule::RegisterMenuExtensions()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}
    
	UToolMenu* WindowMenu = ToolMenus->FindMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntry(
			"OpenSkySystemWindow",
			NSLOCTEXT("SkySystemPlugin", "MenuEntryTitle", "Sky System Manager"),
			NSLOCTEXT("SkySystemPlugin", "MenuEntryTooltip", "Open Sky System Manager Window"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FStylizedSkySystemEditorModule::OpenSkySystemWindow))
		);
	}
}

void FStylizedSkySystemEditorModule::UnregisterMenuExtensions()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (ToolMenus)
	{
		ToolMenus->RemoveSection("LevelEditor.MainMenu.Window", "OpenSkySystemWindow");
	}
}

void FStylizedSkySystemEditorModule::OpenSkySystemWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SkySystemTabName);
}

TSharedRef<SDockTab> FStylizedSkySystemEditorModule::OnSpawnSkySystemTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// 创建天空系统UI界面
			SNew(SStylizedSkyManagerWidget)
		];
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FStylizedSkySystemEditorModule, StylizedSkySystemEditor)