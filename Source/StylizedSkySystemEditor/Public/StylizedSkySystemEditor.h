#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FStylizedSkySystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenuExtensions();

    void UnregisterMenuExtensions();
    
    void OpenSkySystemWindow();

    TSharedRef<SDockTab> OnSpawnSkySystemTab(const FSpawnTabArgs& Args);

    TSharedPtr<FExtender> MenuExtender;

    static const FName SkySystemTabName;
};
