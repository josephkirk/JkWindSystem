#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/CriticalSection.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "WindSystemSettings.h"
#include "WindSystemComponent.generated.h"

class UWindSimulationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWindCellUpdated, const FVector&, CellCenter, const FVector&, WindVelocity, float, CellSize);

class FWindGrid
{
public:
    FWindGrid(int32 Size, float InCellSize);

    FVector GetCell(int32 X, int32 Y, int32 Z) const;
    void SetCell(int32 X, int32 Y, int32 Z, const FVector& Value);

    int32 GetSize() const { return GridSize; }
    float GetCellSize() const { return CellSize; }

    TArray<FVector>& GetGridData() { return Grid; }
    // Add a const version
    const TArray<FVector>& GetGridData() const { return Grid; }

private:
    TArray<FVector> Grid;
    int32 GridSize;
    float CellSize;

    FORCEINLINE int32 GetIndex(int32 X, int32 Y, int32 Z) const;
    FORCEINLINE bool IsValidIndex(int32 X, int32 Y, int32 Z) const;

friend UWindSimulationComponent;
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

UCLASS(ClassGroup = (Custom), HideCategories = (Rendering, Replication, Collision, HLOD, Mobile, Physics, Mobility, VirtualTexture, ComponentTick))
class JK_WINDSYSTEM_API UWindSimulationComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UWindSimulationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    virtual void AddWindAtLocation(const FVector& Location, const FVector& WindVelocity);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    float GetSimulationFrequency() const { return SimulationFrequency; }

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    int32 GetGridSize() const { return GridSize; }

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    float GetCellSize() const { return CellSize; }

    void virtual SimulationStep(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation|Testing")
    void InitializeForTesting();

    UFUNCTION(BlueprintImplementableEvent, Category = "Wind Simulation")
    void StartSimulation();

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    float GetMaxAllowedWindVelocity() const;

    void UpdateGridCenter(const FVector& NewCenter);
private:
    TSharedPtr<FWindGrid> WindGrid;
    TSharedPtr<FWindGrid> TempGrid;
    float Viscosity;
    float SimulationFrequency;

    FWindSimulationWorker* SimulationWorker;
    FRunnableThread* SimulationThread;

    mutable FCriticalSection SimulationLock;

    UPROPERTY()
    int32 GridSize;

    UPROPERTY()
    float CellSize;
    FVector GridCenter;
    FVector PreviousGridCenter;
    bool bIsBroadcasting = false;

    const UWindSystemSettings* GetSettings() const;
    void SwapGrids();
    void BroadcastWindUpdates();
    void HandleGridMovement();
    void InitializeGrid();
    bool IsGridInitialized() const { return WindGrid != nullptr; }
    void Diffuse(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, float Diff, float Dt);
    void Project(TSharedPtr<FWindGrid> Velocity, TSharedPtr<FWindGrid> P, TSharedPtr<FWindGrid> Div);
    void SetBoundary(TSharedPtr<FWindGrid> Field);
    void Advect(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, const TSharedPtr<FWindGrid> Velocity, float Dt);
    void ApplySIMDOperations(TSharedPtr<FWindGrid> Grid, float Scalar);
    FVector InterpolateVelocity(const FVector& Position) const;
};
