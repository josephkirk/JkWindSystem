#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindFunctionLibrary.generated.h"

UCLASS()
class JK_WINDSYSTEM_API UWindSimulationFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wind Simulation", meta = (WorldContext = "WorldContextObject"))
    static FVector GetWindVelocityAtLocation(const UObject* WorldContextObject, const FVector& WorldLocation);
};
