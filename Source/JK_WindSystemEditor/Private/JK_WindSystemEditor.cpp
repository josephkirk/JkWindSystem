#include "JK_WindSystemEditor.h"
#include "WindSettingsEditorTabManager.h"
#include "WindGenerators.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "WindSystemSettings.h"

#define LOCTEXT_NAMESPACE "FJK_WindSystemEditorModule"

void FJK_WindSystemEditorModule::StartupModule()
{
    RegisterSettings();
    
    FWindSystemMenuCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().OpenWindSettingsConfig,
        FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::OpenWindSettingsEditor),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreatePointWindSource,
        FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::CreatePointWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateDirectionalWindSource,
        FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::CreateDirectionalWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateVortexWindSource,
        FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::CreateVortexWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateSplineWindSource,
        FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::CreateSplineWindSource),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJK_WindSystemEditorModule::RegisterMenus));

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<FExtensibilityManager> ExtensibilityManager = LevelEditorModule.GetToolBarExtensibilityManager();

    TSharedPtr<FUICommandList> CommandList = MakeShareable(new FUICommandList);
    
    // Initialize ToolbarExtender
    ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension(
        "Settings",
        EExtensionHook::After,
        nullptr,
        FToolBarExtensionDelegate::CreateRaw(this, &FJK_WindSystemEditorModule::AddToolBarButtons)
    );
    
    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FJK_WindSystemEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FWindSystemMenuCommands::Unregister();
    UnregisterSettings();
    if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    {
        FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
        TSharedPtr<FExtensibilityManager> ExtensibilityManager = LevelEditorModule.GetToolBarExtensibilityManager();

        LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolbarExtender);
    }
}

void AddWindSourceMenuExtension(UToolMenu* Menu)
{
    FToolMenuSection& Section = Menu->AddSection("WindSources", LOCTEXT("WindSourcesHeader", "Wind Sources"));
    
    Section.AddMenuEntry("CreatPointWindSource",
        LOCTEXT("CreatPointWindSource", "Create Point Wind"),
        LOCTEXT("CreatPointWindSourceTooltip", "Create Point Wind Source"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::CreatePointWindSource))
    );
    // Section.AddMenuEntry(FWindSystemMenuCommands::Get().CreateDirectionalWindSource, PluginCommands);
    // Section.AddMenuEntry(FWindSystemMenuCommands::Get().CreateVortexWindSource, PluginCommands);
    // Section.AddMenuEntry(FWindSystemMenuCommands::Get().CreateSplineWindSource, PluginCommands);
}

void AddWindSystemMenuExtension(UToolMenu* Menu)
{
    FToolMenuSection& Section = Menu->AddSection("WindSystemActions", LOCTEXT("WindSystemActionsHeader", "Wind System Actions"));
    
    Section.AddMenuEntry("OpenWindSettingsConfig",
        LOCTEXT("OpenWindSettingsConfig", "Wind Settings Config"),
        LOCTEXT("OpenWindSettingsConfigTooltip", "Open Wind Settings Configuration"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::OpenWindSettingsEditor))
    );
    
    Section.AddSubMenu(
        "WindSources",
        LOCTEXT("WindSourceSubMenu", "Wind System Sources"),
        LOCTEXT("WindSourceSubMenuTooltip", "Wind System Sources Creator"),
        FNewToolMenuDelegate::CreateStatic(&AddWindSourceMenuExtension)
    );
}

void FJK_WindSystemEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
    FToolMenuSection& Section = Menu->AddSection("WindSystem", LOCTEXT("WindSystemMenu", "Wind System"));

    Section.AddSubMenu(
        "JK Wind System",
        LOCTEXT("WindSystemSubMenu", "Wind System"),
        LOCTEXT("WindSystemSubMenuTooltip", "Options For WindSystem"),
        FNewToolMenuDelegate::CreateStatic(&AddWindSystemMenuExtension)
    );
}



void FJK_WindSystemEditorModule::OpenWindSettingsConfig()
{
    UE_LOG(LogTemp, Log, TEXT("Opening Wind Settings Config..."));
}

void FJK_WindSystemEditorModule::CreatePointWindSource()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        APointWindGeneratorActor* PointWindActor = World->SpawnActor<APointWindGeneratorActor>(Location, Rotation);
        if (PointWindActor)
        {
            UE_LOG(LogTemp, Log, TEXT("Point Wind Source created"));
        }
    }
}

void FJK_WindSystemEditorModule::CreateDirectionalWindSource()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        ADirectionalWindGeneratorActor* DirectionalWindActor = World->SpawnActor<ADirectionalWindGeneratorActor>(Location, Rotation);
        if (DirectionalWindActor)
        {
            UE_LOG(LogTemp, Log, TEXT("Directional Wind Source created"));
        }
    }
}

void FJK_WindSystemEditorModule::CreateVortexWindSource()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        AVortexWindGeneratorActor* VortexWindActor = World->SpawnActor<AVortexWindGeneratorActor>(Location, Rotation);
        if (VortexWindActor)
        {
            UE_LOG(LogTemp, Log, TEXT("Vortex Wind Source created"));
        }
    }
}

void FJK_WindSystemEditorModule::CreateSplineWindSource()
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World)
    {
        FVector Location = FVector::ZeroVector;
        FRotator Rotation = FRotator::ZeroRotator;
        ASplineWindGeneratorActor* SplineWindActor = World->SpawnActor<ASplineWindGeneratorActor>(Location, Rotation);
        if (SplineWindActor)
        {
            UE_LOG(LogTemp, Log, TEXT("Spline Wind Source created"));
        }
    }
}

void FWindSystemMenuCommands::RegisterCommands()
{
    UI_COMMAND(OpenWindSettingsConfig, "Wind Settings Config", "Open Wind Settings Configuration", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CreatePointWindSource, "Point Wind Source", "Create a Point Wind Source", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CreateDirectionalWindSource, "Directional Wind Source", "Create a Directional Wind Source", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CreateVortexWindSource, "Vortex Wind Source", "Create a Vortex Wind Source", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CreateSplineWindSource, "Spline Wind Source", "Create a Spline Wind Source", EUserInterfaceActionType::Button, FInputChord());
}

void FJK_WindSystemEditorModule::AddToolBarButtons(FToolBarBuilder& Builder)
{
    Builder.AddToolBarButton(
        FUIAction(FExecuteAction::CreateStatic(&FJK_WindSystemEditorModule::OpenWindSettingsEditor)),
        NAME_None,
        LOCTEXT("WindSettingsButton", "Wind Settings"),
        LOCTEXT("WindSettingsButtonTooltip", "Open Wind Settings Editor"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings")
    );
}

void FJK_WindSystemEditorModule::OpenWindSettingsEditor()
{
    TSharedRef<FWindSettingsEditorTabManager> WindSettingsEditor(new FWindSettingsEditorTabManager());
    WindSettingsEditor->InitWindSettingsEditor(nullptr);
}

void FJK_WindSystemEditorModule::RegisterSettings()
{

    UnregisterSettings();

    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Wind System",
            LOCTEXT("WindSystemSettingsName", "Wind System"),
            LOCTEXT("WindSystemSettingsDescription", "Configure the Wind System settings"),
            GetMutableDefault<UWindSystemSettings>()
        );
    }
}

void FJK_WindSystemEditorModule::UnregisterSettings()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "Wind System");
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJK_WindSystemEditorModule, JK_WindSystemEditor)