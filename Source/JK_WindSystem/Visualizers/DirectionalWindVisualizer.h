#pragma once

#include "CoreMinimal.h"
#include "WindSourceVisualizerBase.h"
#include "DirectionalWindVisualizer.generated.h"

class FDirectionalWindVisualizerSceneProxy;

UCLASS()
class JK_WINDSYSTEM_API UDirectionalWindVisualizer : public UWindSourceVisualizerBase
{
    GENERATED_BODY()

public:
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

    // Add a method to get color based on wind strength
    static FLinearColor GetColorForWindStrength(float Strength, float MaxStrength);
};

class FDirectionalWindVisualizerSceneProxy : public FWindSourceVisualizerSceneProxy
{
public:
    FDirectionalWindVisualizerSceneProxy(const UDirectionalWindVisualizer* InComponent);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

protected:
    virtual void GatherVisualizationData() const override;

private:
    mutable FVector Direction;
    mutable EWindShapeType ShapeType;
    mutable float ConeAngle;

    static constexpr float MaxWindStrength = 1000.0f; // You may want to make this configurable

    void DrawArrowHead(FPrimitiveDrawInterface* PDI, const FVector& TipLocation, const FVector& ArrowDirection, float Size, const FLinearColor& Color, ESceneDepthPriorityGroup DepthPriority, float Thickness) const;
};