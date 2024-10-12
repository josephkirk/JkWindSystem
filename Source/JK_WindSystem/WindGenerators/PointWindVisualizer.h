#pragma once

#include "CoreMinimal.h"
#include "WindSourceVisualizerBase.h"
#include "PointWindVisualizer.generated.h"

class FPointWindVisualizerSceneProxy;

UCLASS()
class JK_WINDSYSTEM_API UPointWindVisualizer : public UWindSourceVisualizerBase
{
    GENERATED_BODY()

public:
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

    // Add a method to get color based on wind strength
    static FLinearColor GetColorForWindStrength(float Strength, float MaxStrength);
};

class FPointWindVisualizerSceneProxy : public FWindSourceVisualizerSceneProxy
{
public:
    FPointWindVisualizerSceneProxy(const UPointWindVisualizer* InComponent);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
    virtual void GatherVisualizationData() const override;

private:
    static constexpr float MaxWindStrength = 1000.0f; // You may want to make this configurable
};