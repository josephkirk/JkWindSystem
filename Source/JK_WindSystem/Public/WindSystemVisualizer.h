// WindDebugVisualizer.h

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "WindSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "WindSystemVisualizer.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UWindDebugVisualizer : public UActorComponent
{
    GENERATED_BODY()

public:    
    UWindDebugVisualizer();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float ArrowScale = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float ArrowThickness = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    FColor ArrowColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float DebugDrawDuration = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float MinVelocityThreshold = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDrawArrows = true;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDrawVelocityText = false;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float TextScale = 1.0f;

private:
    UFUNCTION()
    void OnWindCellUpdated(const FVector& CellCenter, const FVector& WindVelocity, float CellSize);

    void DrawDebugArrow(const UWorld* World, const FVector& Start, const FVector& End);
    void DrawDebugVelocityText(const UWorld* World, const FVector& Location, const FVector& Velocity);

    UPROPERTY()
    UWindSimulationComponent* WindSimComponent;
};

