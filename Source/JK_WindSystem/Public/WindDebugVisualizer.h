#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "WindSystemComponent.h"
#include "WindDebugVisualizer.generated.h"

class FWindDebugSceneProxy;

USTRUCT()
struct FWindDebugArrow
{
    GENERATED_BODY()

    FVector Start;
    FVector End;
    FColor Color;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindDebugVisualizer : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UWindDebugVisualizer();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float UpdateInterval = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float ArrowScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    int32 GridResolution = 10;

    void SetVisualizationExtent(const FVector& Extent);
private:
    TArray<FWindDebugArrow> DebugArrows;
    float TimeSinceLastUpdate = 0.0f;

    void UpdateDebugArrows();

    FVector VisualizationExtent;
    
    friend FWindDebugSceneProxy;
};
