#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindZoneVolumeComponent.h"
#include "WindZoneVolumeActor.generated.h"

UCLASS()
class JK_WINDSYSTEM_API AWindZoneVolumeActor : public AActor
{
    GENERATED_BODY()

public:
    AWindZoneVolumeActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind Modifier")
    UWindZoneVolumeComponent* WindZoneComponent;
};