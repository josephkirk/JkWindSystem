#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "WindZoneVolumeComponent.generated.h"

UENUM(BlueprintType)
enum class EWindZoneType : uint8
{
    FreezeSimulation,
    ZeroWind,
    Redirection
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UWindZoneVolumeComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UWindZoneVolumeComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Modifier")
    EWindZoneType ModifierType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Modifier", meta = (EditCondition = "ModifierType == EWindZoneType::Redirection", EditConditionHides, MakeEditWidget))
    FTransform RedirectionTransform;

    UFUNCTION(BlueprintCallable, Category = "Wind Modifier")
    FVector ModifyWindVelocity(const FVector& OriginalVelocity, const FVector& Location) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Modifier")
    bool IsPointInside(const FVector& Point) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    class UWindSimulationSubsystem* WindSubsystem;
};