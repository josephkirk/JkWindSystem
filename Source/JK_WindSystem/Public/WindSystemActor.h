#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "WindSystemComponent.h"
#include "WindSystemVisualizer.h"
#include "WindSystemActor.generated.h"

UCLASS(NotPlaceable, Hidden)
class JK_WINDSYSTEM_API AWindSystemActor : public AActor
{
    GENERATED_BODY()

public:
    AWindSystemActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind System")
    UWindSimulationComponent* WindSimulationComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind System")
    bool bShowWindVisualization;

private:

    UPROPERTY(VisibleAnywhere, Category = "Wind System | Visualizer")
    UWindSystemVisualizer* WindSystemVisualizer;

protected:
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS()
class JK_WINDSYSTEM_API AGPUWindSystemActor : public AWindSystemActor
{
    GENERATED_BODY()

public:
    AGPUWindSystemActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};