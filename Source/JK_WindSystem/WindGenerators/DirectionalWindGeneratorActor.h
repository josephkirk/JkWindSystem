#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSourceComponent.h"
#include "Components/BillboardComponent.h"
#include "DirectionalWindVisualizer.h"
#include "DirectionalWindGeneratorActor.generated.h"

UCLASS(BlueprintType, Placeable, Meta=(DisplayName="Directional Wind Generator", Category="Wind System"))
class JK_WINDSYSTEM_API ADirectionalWindGeneratorActor : public AActor
{
    GENERATED_BODY()

public:
    ADirectionalWindGeneratorActor();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
    UDirectionalWindGeneratorComponent* WindGeneratorComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UDirectionalWindVisualizer* WindVisualizer;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, Category = "Visualization")
    class UArrowComponent* ArrowComponent;
#endif

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};