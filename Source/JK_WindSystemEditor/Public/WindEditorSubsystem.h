#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "WindSystemSettings.h"
#include "WindEditorSubsystem.generated.h"

UCLASS(MinimalAPI)
class UWindSettingsEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    float GetMaxAllowedWindVelocity() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    void SetMaxAllowedWindVelocity(float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    int32 GetGridSize() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    void SetGridSize(int32 NewValue);

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    float GetCellSize() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    void SetCellSize(float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    float GetViscosity() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    void SetViscosity(float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    float GetSimulationFrequency() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Settings")
    void SetSimulationFrequency(float NewValue);

private:
    UWindSystemSettings* GetWindSettings() const;
};