#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindGridVisualizer.h"
#include "WindGridVisualizerActor.generated.h"

UCLASS()
class JK_WINDSYSTEM_API AWindGridVisualizerActor : public AActor
{
    GENERATED_BODY()

public:
    AWindGridVisualizerActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind Grid Visualization")
    UWindGridVisualizer* GridVisualizer;
};