// WindSystemVisualizer.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WindSystemComponent.h"
#include "DrawDebugHelpers.h"
#include "WindSystemVisualizer.generated.h"

class UWindSimulationSubsystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindDebugVisualizer : public UActorComponent
{
    GENERATED_BODY()

public:
    UWindDebugVisualizer();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // These properties are now controlled by the WindDebugVisualizationActor
    float ArrowScale;
    float ArrowThickness;
    FLinearColor ArrowColor;
    float MinVelocityThreshold;
    bool bDrawArrows;
    bool bDrawVelocityText;
    bool bDrawAdaptiveGrid;
    float TextScale;
    int32 VisualizationResolution;

private:
    void DrawDebugArrow(const UWorld* World, const FVector& Start, const FVector& End, const FVector& Velocity);
    void DrawDebugVelocityText(const UWorld* World, const FVector& Location, const FVector& Velocity);
    void DrawDebugAdaptiveGrid(const UWorld* World);

    UPROPERTY()
    UWindSimulationComponent* WindSimComponent;

    UWindSimulationSubsystem* GetWindSubsystem() const;
};


UCLASS()
class JK_WINDSYSTEM_API AWindDebugVisualizationActor : public AActor
{
    GENERATED_BODY()
    
public:    
    AWindDebugVisualizationActor();

protected:
    virtual void BeginPlay() override;

public:    
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind Debug")
    UWindDebugVisualizer* WindDebugVisualizer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float ArrowScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float ArrowThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    FColor ArrowColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float MinVelocityThreshold = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    bool bDrawArrows = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    bool bDrawVelocityText = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    bool bDrawAdaptiveGrid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float TextScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    int32 VisualizationResolution = 5;

    UFUNCTION(BlueprintCallable, Category = "Wind Debug")
    void UpdateVisualizerSettings();
};
