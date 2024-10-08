#pragma once

#include "CoreMinimal.h"
#include "WindGeneratorComponent.h"
#include "PointWindSourceComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UPointWindGeneratorComponent : public UWindGeneratorComponent
{
    GENERATED_BODY()

public:
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;
};
