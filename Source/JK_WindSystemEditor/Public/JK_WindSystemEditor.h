#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"
#include "WindEditorSubsystem.h"
#include "ToolMenus.h"

class FToolBarBuilder;
class FMenuBuilder;

class FJK_WindSystemEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void AddWindSystemMenuExtension(FMenuBuilder& MenuBuilder);
    void AddWindSourceMenuExtension(FMenuBuilder& MenuBuilder);
    void OpenWindSettingsConfig();
    void CreatePointWindSource();
    void CreateDirectionalWindSource();
    void CreateVortexWindSource();
    void CreateSplineWindSource();

    TSharedPtr<class FUICommandList> PluginCommands;
};

class FWindSystemMenuCommands : public TCommands<FWindSystemMenuCommands>
{
public:
    FWindSystemMenuCommands()
        : TCommands<FWindSystemMenuCommands>(TEXT("WindSystemMenu"), NSLOCTEXT("Contexts", "WindSystemMenu", "Wind System Menu"), NAME_None, FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> OpenWindSettingsConfig;
    TSharedPtr<FUICommandInfo> CreatePointWindSource;
    TSharedPtr<FUICommandInfo> CreateDirectionalWindSource;
    TSharedPtr<FUICommandInfo> CreateVortexWindSource;
    TSharedPtr<FUICommandInfo> CreateSplineWindSource;
};