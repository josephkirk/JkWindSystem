#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "WindSourceComponent.h"
#include "WindSourceVisualizerBase.generated.h"

class FWindSourceVisualizerSceneProxy;

UCLASS(Abstract)
class JK_WINDSYSTEM_API UWindSourceVisualizerBase : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UWindSourceVisualizerBase();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    void SetWindSourceComponent(UWindGeneratorComponent* InWindSource);

    // Make WindSource accessible to derived classes
    UWindGeneratorComponent* GetWindSource() const { return WindSource; }

protected:
    UPROPERTY(Transient)
    UWindGeneratorComponent* WindSource;

    virtual void UpdateVisualization();

    friend class FWindSourceVisualizerSceneProxy;
};

class FWindSourceVisualizerSceneProxy : public FPrimitiveSceneProxy
{
public:
    FWindSourceVisualizerSceneProxy(const UWindSourceVisualizerBase* InComponent);

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
    virtual SIZE_T GetTypeHash() const override;
    virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }

protected:
    virtual void GatherVisualizationData() const = 0;

    const UWindSourceVisualizerBase* VisualizerComponent;
    mutable FVector ComponentLocation;
    mutable float Radius;
    mutable float Strength;
};