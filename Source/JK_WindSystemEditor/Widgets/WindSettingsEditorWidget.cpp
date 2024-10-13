#include "WindSettingsEditorWidget.h"
#include "WindSystemSettings.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

void SWindSettingsEditorWidget::Construct(const FArguments& InArgs)
{
    WindSettings = GetMutableDefault<UWindSystemSettings>();

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Max Wind Velocity:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SAssignNew(MaxWindVelocitySpinBox, SSpinBox<float>)
                .Value(WindSettings->MaxAllowedWindVelocity)
                .OnValueChanged(this, &SWindSettingsEditorWidget::OnMaxWindVelocityChanged)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Grid Size:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SAssignNew(GridSizeSpinBox, SSpinBox<int32>)
                .Value(WindSettings->GridSize)
                .OnValueChanged(this, &SWindSettingsEditorWidget::OnGridSizeChanged)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Cell Size:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SAssignNew(CellSizeSpinBox, SSpinBox<float>)
                .Value(WindSettings->CellSize)
                .OnValueChanged(this, &SWindSettingsEditorWidget::OnCellSizeChanged)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Viscosity:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SAssignNew(ViscositySpinBox, SSpinBox<float>)
                .Value(WindSettings->Viscosity)
                .OnValueChanged(this, &SWindSettingsEditorWidget::OnViscosityChanged)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Simulation Frequency:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SAssignNew(SimulationFrequencySpinBox, SSpinBox<float>)
                .Value(WindSettings->SimulationFrequency)
                .OnValueChanged(this, &SWindSettingsEditorWidget::OnSimulationFrequencyChanged)
            ]
        ]
    ];
}

void SWindSettingsEditorWidget::OnMaxWindVelocityChanged(float NewValue)
{
    if (WindSettings.IsValid())
    {
        WindSettings->MaxAllowedWindVelocity = NewValue;
        WindSettings->SaveConfig();
    }
}

void SWindSettingsEditorWidget::OnGridSizeChanged(int32 NewValue)
{
    if (WindSettings.IsValid())
    {
        WindSettings->GridSize = NewValue;
        WindSettings->SaveConfig();
    }
}

void SWindSettingsEditorWidget::OnCellSizeChanged(float NewValue)
{
    if (WindSettings.IsValid())
    {
        WindSettings->CellSize = NewValue;
        WindSettings->SaveConfig();
    }
}

void SWindSettingsEditorWidget::OnViscosityChanged(float NewValue)
{
    if (WindSettings.IsValid())
    {
        WindSettings->Viscosity = NewValue;
        WindSettings->SaveConfig();
    }
}

void SWindSettingsEditorWidget::OnSimulationFrequencyChanged(float NewValue)
{
    if (WindSettings.IsValid())
    {
        WindSettings->SimulationFrequency = NewValue;
        WindSettings->SaveConfig();
    }
}