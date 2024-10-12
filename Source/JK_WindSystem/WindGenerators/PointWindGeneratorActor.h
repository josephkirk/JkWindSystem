#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSourceComponent.h"
#include "Components/BillboardComponent.h"
#include "PointWindVisualizer.h"
#include "PointWindGeneratorActor.generated.h"

UCLASS(BlueprintType, Placeable, Meta=(DisplayName="Point Wind Generator", Category="Wind System"))
class JK_WINDSYSTEM_API APointWindGeneratorActor : public AActor
{
    GENERATED_BODY()

public:
    APointWindGeneratorActor();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
    UPointWindGeneratorComponent* WindGeneratorComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UPointWindVisualizer* WindVisualizer;

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};