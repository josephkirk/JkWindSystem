#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "WindSystemSettings.h"
#include "WindGridVisualizer.generated.h"

class FWindGridSceneProxy;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindGridVisualizer : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UWindGridVisualizer();

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    UPROPERTY(EditAnywhere, Category = "Wind Grid Visualization")
    FLinearColor GridColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Wind Grid Visualization")
    float LineThickness = 1.0f;

    const UWindSystemSettings* GetWindSettings() const;
};