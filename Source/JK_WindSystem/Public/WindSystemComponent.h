#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "WindSystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWindCellUpdated, const FVector&, CellCenter, const FVector&, WindVelocity, float, CellSize);

class FWindGrid
{
public:
    FWindGrid(int32 Size, float CellSize);

    FVector& GetCell(int32 X, int32 Y, int32 Z);
    const FVector& GetCell(int32 X, int32 Y, int32 Z) const;
    void SetCell(int32 X, int32 Y, int32 Z, const FVector& Value);

    int32 GetSize() const { return GridSize; }
    float GetCellSize() const { return CellSize; }

    TArray<FVector>& GetGridData() { return Grid; }
    const TArray<FVector>& GetGridData() const { return Grid; }

private:
    TArray<FVector> Grid;
    int32 GridSize;
    float CellSize;

    int32 GetIndex(int32 X, int32 Y, int32 Z) const;
};

class FWindSimulationWorker : public FRunnable
{
public:
    FWindSimulationWorker(class UWindSimulationComponent* InOwner);
    virtual uint32 Run() override;
    void Stop();

private:
    class UWindSimulationComponent* Owner;
    FThreadSafeBool bShouldRun;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindSimulationComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UWindSimulationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    void AddWindAtLocation(const FVector& Location, const FVector& WindVelocity);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    float GetSimulationFrequency() const { return SimulationFrequency; }

    UPROPERTY(BlueprintAssignable, Category = "Wind Simulation|Debug")
    FOnWindCellUpdated OnWindCellUpdated;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    int32 GetBaseGridSize() const;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    float GetCellSize() const;

    void SimulationStep(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation|Testing")
    void InitializeForTesting();

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    void StartSimulation();

private:
    TSharedPtr<FWindGrid> WindGrid;
    float Viscosity;
    float SimulationFrequency;

    FWindSimulationWorker* SimulationWorker;
    FRunnableThread* SimulationThread;
    
    UPROPERTY()
    int32 BaseGridSize;

    UPROPERTY()
    float CellSize;

    bool bIsBroadcasting = false;

    void BroadcastWindUpdates();
    void InitializeGrid();
    bool IsGridInitialized() const { return WindGrid != nullptr; }
    void Diffuse(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, float Diff, float Dt);
    void Project(TSharedPtr<FWindGrid> Velocity, TSharedPtr<FWindGrid> P, TSharedPtr<FWindGrid> Div);
    void SetBoundary(TSharedPtr<FWindGrid> Field);
    void Advect(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, const TSharedPtr<FWindGrid> Velocity, float Dt);
    void ApplySIMDOperations(TSharedPtr<FWindGrid> Grid, float Scalar);
    FVector InterpolateVelocity(const FVector& Position) const;
};
