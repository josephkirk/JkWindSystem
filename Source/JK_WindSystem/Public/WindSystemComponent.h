#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WindSystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWindCellUpdated, const FVector&, CellCenter, const FVector&, WindVelocity, float, CellSize);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindSimulationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
    UWindSimulationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UPROPERTY(BlueprintAssignable, Category = "Wind Simulation|Debug")
    FOnWindCellUpdated OnWindCellUpdated;

public:
    UPROPERTY(EditAnywhere, Category = "Wind Simulation|Debug")
    bool bShowDebugVisualization;

    UPROPERTY(EditAnywhere, Category = "Wind Simulation|Debug")
    float DebugArrowScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Wind Simulation|Debug")
    float DebugArrowThickness = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Wind Simulation|Debug")
    FColor DebugArrowColor = FColor::Cyan;

protected:
    virtual void BeginPlay() override;
    //virtual void EndPlay() override;

private:
    // Grid dimensions
    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    int32 GridSizeX = 256;

    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    int32 GridSizeY = 256;

    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    int32 GridSizeZ = 4;

    // Simulation parameters
    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    float CellSize = 128.0f; // Size of each grid cell in world units

    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    float SimulationFrequency = 1.0f; // How often to update the simulation (in seconds)

    UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    float Viscosity = 0.1f;

    // Wind grid
    TArray<FVector> VelocityGrid;
    TArray<FVector> PreviousVelocityGrid;

    float SimulationTimer = 0.0f;

private:
    void NotifyCellUpdated(int32 X, int32 Y, int32 Z);
    void InitializeGrid();
    void StepSimulation(float DeltaTime);
    void AddSources();
    void Diffuse(TArray<FVector>& Dst, const TArray<FVector>& Src, float Diff, float Dt);
    void Project(TArray<FVector>& VelocityX, TArray<FVector>& VelocityY, TArray<FVector>& VelocityZ, TArray<FVector>& P, TArray<FVector>& Div);
    void Advect(TArray<FVector>& Dst, const TArray<FVector>& Src, const TArray<FVector>& Velocity, float Dt);
    int32 IX(int32 X, int32 Y, int32 Z) const;
};
