#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSystemComponent.h"
#include "WindSystemVisualizer.h"
#include "WindSystemActor.generated.h"

UCLASS(NotPlaceable, Hidden)
class JK_WINDSYSTEM_API AWindSystemActor : public AActor
{
    GENERATED_BODY()

public:
    AWindSystemActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind System")
    UWindSimulationComponent* WindSimulationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind System")
    UWindSystemVisualizer* WindSystemVisualizer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind System")
    bool bShowWindVisualization;

protected:
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};