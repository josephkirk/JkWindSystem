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
    void OpenWindSettingsConfig();
    static void OpenWindSettingsEditor();
    static void CreatePointWindSource();
    static void CreateDirectionalWindSource();
    static void CreateVortexWindSource();
    static void CreateSplineWindSource();
private:
    void RegisterMenus();


    void AddToolBarButtons(FToolBarBuilder& Builder);
    
    void RegisterSettings();
    void UnregisterSettings();
private:
    TSharedPtr<class FUICommandList> PluginCommands;
    TSharedPtr<FExtender> ToolbarExtender;
};

class FWindSystemMenuCommands : public TCommands<FWindSystemMenuCommands>
{
public:
    FWindSystemMenuCommands()
        : TCommands<FWindSystemMenuCommands>(
            TEXT("WindSystemMenu"),
            NSLOCTEXT("Contexts", "WindSystemMenu", "Wind System Menu"),
            NAME_None,
            FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> OpenWindSettingsConfig;
    TSharedPtr<FUICommandInfo> CreatePointWindSource;
    TSharedPtr<FUICommandInfo> CreateDirectionalWindSource;
    TSharedPtr<FUICommandInfo> CreateVortexWindSource;
    TSharedPtr<FUICommandInfo> CreateSplineWindSource;
};