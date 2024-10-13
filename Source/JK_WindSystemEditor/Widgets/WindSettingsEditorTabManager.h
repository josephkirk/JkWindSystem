#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"

class FWindSettingsEditorTabManager : public FAssetEditorToolkit
{
public:
    FWindSettingsEditorTabManager();

    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

    void InitWindSettingsEditor(const TSharedPtr<IToolkitHost>& InitToolkitHost);

    // FAssetEditorToolkit interface
    virtual FName GetToolkitFName() const override { return FName("WindSettingsEditor"); }
    virtual FText GetBaseToolkitName() const override { return FText::FromString("Wind Settings Editor"); }
    virtual FString GetWorldCentricTabPrefix() const override { return "WindSettingsEditor"; }
    virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f); }

    static const FName WindSettingsTabId;

private:
    TSharedRef<SDockTab> SpawnTab_WindSettings(const FSpawnTabArgs& Args);
};