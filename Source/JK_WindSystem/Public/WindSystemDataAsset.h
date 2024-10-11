#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WindSystemDataAsset.generated.h"

UCLASS(BlueprintType)
class JK_WINDSYSTEM_API UWindSettingsDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings")
    float GlobalWindStrength = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings")
    FVector GlobalWindDirection = FVector(1.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings")
    float TurbulenceStrength = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings")
    float TurbulenceFrequency = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings")
    bool bEnableGusts = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings", meta = (EditCondition = "bEnableGusts"))
    float GustStrength = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Settings", meta = (EditCondition = "bEnableGusts"))
    float GustFrequency = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation", meta = (ClampMin = "8", ClampMax = "256"))
    int32 GridSize = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation", meta = (ClampMin = "10.0", ClampMax = "1000.0"))
    float CellSize = 100.0f;
};