#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindDebugVisualizer.h"
#include "WindDebugVisualizerActor.generated.h"

UCLASS(Blueprintable)
class JK_WINDSYSTEM_API AWindDebugVisualizerActor : public AActor
{
    GENERATED_BODY()

public:
    AWindDebugVisualizerActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind Debug")
    UWindDebugVisualizer* WindDebugVisualizer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float UpdateInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    float ArrowScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    int32 GridResolution = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Debug")
    FVector VisualizationExtent = FVector(1000.0f);

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Wind Debug")
    void UpdateVisualizerSettings();
};