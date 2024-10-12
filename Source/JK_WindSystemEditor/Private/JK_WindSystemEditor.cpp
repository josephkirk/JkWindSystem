#include "JK_WindSystemEditor.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "PointWindGeneratorActor.h"
#include "DirectionalWindGeneratorActor.h"
#include "VortexWindGeneratorActor.h"
#include "SplineWindGeneratorActor.h"

#define LOCTEXT_NAMESPACE "FJK_WindSystemEditorModule"

void FJK_WindSystemEditorModule::StartupModule()
{
    FWindSystemMenuCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().OpenWindSettingsConfig,
        FExecuteAction::CreateRaw(this, &FJK_WindSystemEditorModule::OpenWindSettingsConfig),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreatePointWindSource,
        FExecuteAction::CreateRaw(this, &FJK_WindSystemEditorModule::CreatePointWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateDirectionalWindSource,
        FExecuteAction::CreateRaw(this, &FJK_WindSystemEditorModule::CreateDirectionalWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateVortexWindSource,
        FExecuteAction::CreateRaw(this, &FJK_WindSystemEditorModule::CreateVortexWindSource),
        FCanExecuteAction());

    PluginCommands->MapAction(
        FWindSystemMenuCommands::Get().CreateSplineWindSource,
        FExecuteAction::CreateRaw(this, &FJK_WindSystemEditorModule::CreateSplineWindSource),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJK_WindSystemEditorModule::RegisterMenus));
}

void FJK_WindSystemEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FWindSystemMenuCommands::Unregister();
}

void FJK_WindSystemEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
    FToolMenuSection& Section = Menu->AddSection("WindSystem", LOCTEXT("WindSystemMenu", "Wind System"));

    Section.AddSubMenu(
        "JK Wind System",
        LOCTEXT("WindSystemSubMenu", "Wind System"),
        LOCTEXT("WindSystemSubMenuTooltip", "Option For WindSystem"),
        FNewMenuDelegate::CreateRaw(this, &FJK_WindSystemEditorModule::AddWindSystemMenuExtension)
    );
}

void FJK_WindSystemEditorModule::AddWindSystemMenuExtension(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(FWindSystemMenuCommands::Get().OpenWindSettingsConfig);
    MenuBuilder.AddSubMenu(
        LOCTEXT("WindSourceSubMenu", "Wind System Sources"),
        LOCTEXT("WindSourceSubMenuTooltip", "WindSystem System Sources Creator"),
        FNewMenuDelegate::CreateRaw(this, &FJK_WindSystemEditorModule::AddWindSourceMenuExtension)
    );
}

void FJK_WindSystemEditorModule::AddWindSourceMenuExtension(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(FWindSystemMenuCommands::Get().CreatePointWindSource);
    MenuBuilder.AddMenuEntry(FWindSystemMenuCommands::Get().CreateDirectionalWindSource);
    MenuBuilder.AddMenuEntry(FWindSystemMenuCommands::Get().CreateVortexWindSource);
    MenuBuilder.AddMenuEntry(FWindSystemMenuCommands::Get().CreateSplineWindSource);
}

void FJK_WindSystemEditorModule::OpenWindSettingsConfig()
{
    UE_LOG(LogTemp, Warning, TEXT("Opening Wind Settings Config..."));
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
            UE_LOG(LogTemp, Warning, TEXT("Point Wind Source created"));
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
            UE_LOG(LogTemp, Warning, TEXT("Directional Wind Source created"));
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
            UE_LOG(LogTemp, Warning, TEXT("Vortex Wind Source created"));
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
            UE_LOG(LogTemp, Warning, TEXT("Spline Wind Source created"));
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FJK_WindSystemEditorModule, JK_WindSystemEditor)