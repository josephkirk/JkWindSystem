#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WindSystemSettings.generated.h"

UCLASS(config=JK_WindSystem, defaultconfig)
class JK_WINDSYSTEM_API UWindSystemSettings : public UObject
{
    GENERATED_BODY()

public:
    UWindSystemSettings(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(config, EditAnywhere, Category = "Wind Simulation", meta = (ClampMin = "1.0"))
    float MaxAllowedWindVelocity;

    UPROPERTY(config, EditAnywhere, Category = "Wind Simulation", meta = (ClampMin = "1.0"))
    float GridSize;

    UPROPERTY(config, EditAnywhere, Category = "Wind Simulation", meta = (ClampMin = "1.0"))
    float CellSize;

    UPROPERTY(config, EditAnywhere, Category = "Wind Simulation", meta = (ClampMin = "1.0"))
    float Viscosity;

    UPROPERTY(config, EditAnywhere, Category = "Wind Simulation", meta = (ClampMin = "1.0"))
    float SimulationFrequency;
};