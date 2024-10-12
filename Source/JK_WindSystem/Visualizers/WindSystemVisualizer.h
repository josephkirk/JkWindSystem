#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "WindSystemVisualizer.generated.h"

class UWindSimulationComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindSystemVisualizer : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UWindSystemVisualizer();

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

class FWindSystemFieldSceneProxy : public FPrimitiveSceneProxy
{
public:
    FWindSystemFieldSceneProxy(const UWindSystemVisualizer* InComponent);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
    virtual uint32 GetMemoryFootprint() const override;
    virtual SIZE_T GetTypeHash() const override;

private:
    TArray<FVector> GridPoints;
    TArray<FVector> WindVelocities;
    float ArrowScale;
};