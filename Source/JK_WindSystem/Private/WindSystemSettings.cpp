#include "WindSystemSettings.h"

UWindSystemSettings::UWindSystemSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MaxAllowedWindVelocity = 1000000.0f;
    GridSize = 32; // Default value
    CellSize = 100.0f; // Default value
    Viscosity = 0.1f;
    SimulationFrequency = 60.0f;
}