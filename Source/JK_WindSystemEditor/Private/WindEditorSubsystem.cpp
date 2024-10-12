#include "WindEditorSubsystem.h"
#include "ISettingsModule.h"

void UWindSettingsEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Register the settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Wind System",
            FText::FromString(TEXT("Wind System")),
            FText::FromString(TEXT("Configure the Wind System settings")),
            GetMutableDefault<UWindSystemSettings>()
        );
    }
}

void UWindSettingsEditorSubsystem::Deinitialize()
{
    // Unregister the settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "Wind System");
    }

    Super::Deinitialize();
}

float UWindSettingsEditorSubsystem::GetMaxAllowedWindVelocity() const
{
    return GetWindSettings()->MaxAllowedWindVelocity;
}

void UWindSettingsEditorSubsystem::SetMaxAllowedWindVelocity(float NewValue)
{
    GetWindSettings()->MaxAllowedWindVelocity = NewValue;
    GetWindSettings()->SaveConfig();
}

int32 UWindSettingsEditorSubsystem::GetGridSize() const
{
    return GetWindSettings()->GridSize;
}

void UWindSettingsEditorSubsystem::SetGridSize(int32 NewValue)
{
    GetWindSettings()->GridSize = NewValue;
    GetWindSettings()->SaveConfig();
}

float UWindSettingsEditorSubsystem::GetCellSize() const
{
    return GetWindSettings()->CellSize;
}

void UWindSettingsEditorSubsystem::SetCellSize(float NewValue)
{
    GetWindSettings()->CellSize = NewValue;
    GetWindSettings()->SaveConfig();
}

float UWindSettingsEditorSubsystem::GetViscosity() const
{
    return GetWindSettings()->Viscosity;
}

void UWindSettingsEditorSubsystem::SetViscosity(float NewValue)
{
    GetWindSettings()->Viscosity = NewValue;
    GetWindSettings()->SaveConfig();
}

float UWindSettingsEditorSubsystem::GetSimulationFrequency() const
{
    return GetWindSettings()->SimulationFrequency;
}

void UWindSettingsEditorSubsystem::SetSimulationFrequency(float NewValue)
{
    GetWindSettings()->SimulationFrequency = NewValue;
    GetWindSettings()->SaveConfig();
}

UWindSystemSettings* UWindSettingsEditorSubsystem::GetWindSettings() const
{
    return GetMutableDefault<UWindSystemSettings>();
}