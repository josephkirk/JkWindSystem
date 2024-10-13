#include "WindSettingsEditorTabManager.h"
#include "WindSettingsEditorWidget.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "EditorStyleSet.h"
#include "WindSystemSettings.h"

const FName FWindSettingsEditorTabManager::WindSettingsTabId(TEXT("WindSettings"));

FWindSettingsEditorTabManager::FWindSettingsEditorTabManager()
{
}

void FWindSettingsEditorTabManager::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(WindSettingsTabId, FOnSpawnTab::CreateSP(this, &FWindSettingsEditorTabManager::SpawnTab_WindSettings))
        .SetDisplayName(FText::FromString("Wind Settings"))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FWindSettingsEditorTabManager::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

    InTabManager->UnregisterTabSpawner(WindSettingsTabId);
}

void FWindSettingsEditorTabManager::InitWindSettingsEditor(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_WindSettingsEditor_Layout")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab(WindSettingsTabId, ETabState::OpenedTab)
            )
        );

    const bool bCreateDefaultStandaloneMenu = true;
    const bool bCreateDefaultToolbar = true;
    UWindSystemSettings* WindSettings = GetMutableDefault<UWindSystemSettings>();
    TArray<UObject*> ObjectsToEdit;
    ObjectsToEdit.Add(WindSettings);

    FAssetEditorToolkit::InitAssetEditor(
        EToolkitMode::Standalone,
        InitToolkitHost,
        FName("WindSettingsEditor"),
        StandaloneDefaultLayout,
        bCreateDefaultStandaloneMenu,
        bCreateDefaultToolbar,
        ObjectsToEdit
    );
}

TSharedRef<SDockTab> FWindSettingsEditorTabManager::SpawnTab_WindSettings(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SWindSettingsEditorWidget)
        ];
}