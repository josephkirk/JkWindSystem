// WindSystemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "WindSystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWindCellUpdated, const FVector&, CellCenter, const FVector&, WindVelocity, float, CellSize);

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

USTRUCT()
struct FAdaptiveGridCell
{
    GENERATED_BODY()

    FVector Velocity;
    TArray<FAdaptiveGridCell*> Children;
    float SubdivisionThreshold;
    float MergeThreshold;

    FAdaptiveGridCell() : Velocity(FVector::ZeroVector), SubdivisionThreshold(1.0f), MergeThreshold(0.1f) {}
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindSimulationComponent : public UActorComponent
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

    //UPROPERTY(EditAnywhere, Category = "Wind Simulation")
    //float SimulationFrequency;

    void SimulationStep(float DeltaTime);

private:

    TArray<FAdaptiveGridCell> AdaptiveGrid;
    int32 BaseGridSize;
    float CellSize;
    float Viscosity;
    float SimulationFrequency;

    FWindSimulationWorker* SimulationWorker;
    FRunnableThread* SimulationThread;

    void InitializeGrid();
    void UpdateAdaptiveGrid();
    void Subdivide(FAdaptiveGridCell& Cell);
    void Merge(FAdaptiveGridCell& ParentCell);
    void Diffuse(TArray<FVector>& Dst, const TArray<FVector>& Src, float Diff, float Dt);
    void Project(TArray<FVector>& VelocityX, TArray<FVector>& VelocityY, TArray<FVector>& VelocityZ, TArray<FVector>& P, TArray<FVector>& Div);
    void SetBoundary(TArray<FVector>& Field);
    int32 IX(int32 x, int32 y, int32 z) const;
    void Advect(TArray<FVector>& Dst, const TArray<FVector>& Src, const TArray<FVector>& Velocity, float Dt);

    void ApplySIMDOperations(TArray<FVector>& Vectors, float Scalar);
};
