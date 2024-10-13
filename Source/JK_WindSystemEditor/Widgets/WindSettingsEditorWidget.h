#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSpinBox.h"
#include "WindSystemSettings.h"

class SWindSettingsEditorWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWindSettingsEditorWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UWindSystemSettings> WindSettings;

    TSharedPtr<SSpinBox<float>> MaxWindVelocitySpinBox;
    TSharedPtr<SSpinBox<int32>> GridSizeSpinBox;
    TSharedPtr<SSpinBox<float>> CellSizeSpinBox;
    TSharedPtr<SSpinBox<float>> ViscositySpinBox;
    TSharedPtr<SSpinBox<float>> SimulationFrequencySpinBox;

    void OnMaxWindVelocityChanged(float NewValue);
    void OnGridSizeChanged(int32 NewValue);
    void OnCellSizeChanged(float NewValue);
    void OnViscosityChanged(float NewValue);
    void OnSimulationFrequencyChanged(float NewValue);
};