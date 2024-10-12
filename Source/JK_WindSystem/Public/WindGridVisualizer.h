#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "WindGridVisualizer.generated.h"

class UWindSimulationComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindGridVisualizer : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UWindGridVisualizer();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    UPROPERTY(EditAnywhere, Category = "Visualization")
    float ArrowScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Visualization")
    float UpdateInterval = 0.1f;

    TArray<FVector> GridPoints;
    TArray<FVector> WindVelocities;

private:
    float TimeSinceLastUpdate;

    void UpdateGridData();
};

class FWindGridSceneProxy : public FPrimitiveSceneProxy
{
public:
    FWindGridSceneProxy(const UWindGridVisualizer* InComponent);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
    virtual uint32 GetMemoryFootprint() const override;
    virtual SIZE_T GetTypeHash() const override;

private:
    TArray<FVector> GridPoints;
    TArray<FVector> WindVelocities;
    float ArrowScale;
};