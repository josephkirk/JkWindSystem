#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindSourceComponent.h"
#include "Components/BillboardComponent.h"
#include "DirectionalWindGeneratorActor.generated.h"

UCLASS(BlueprintType, Placeable, Meta=(DisplayName="Directional Wind Generator", Category="Wind System"))
class JK_WINDSYSTEM_API ADirectionalWindGeneratorActor : public AActor
{
    GENERATED_BODY()

public:
    ADirectionalWindGeneratorActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
    UDirectionalWindGeneratorComponent* WindGeneratorComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UBillboardComponent* BillboardComponent;

#if WITH_EDITORONLY_DATA
    UPROPERTY(VisibleAnywhere, Category = "Visualization")
    class UArrowComponent* ArrowComponent;
#endif

    UPROPERTY(EditAnywhere, Category = "Visualization")
    bool bShowDebugVisualization;

protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void UpdateVisualization();
};