// PointWindGeneratorComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "WindSourceComponent.generated.h"

class UWindSimulationSubsystem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JK_WINDSYSTEM_API UWindGeneratorComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UWindGeneratorComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wind")
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float Strength = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float Radius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float UpdateFrequency = 0.1f;

    // New function to update wind simulation
    void UpdateWindSimulation(float DeltaTime, UWindSimulationSubsystem* Subsystem);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Wind")
    virtual float GetFalloff(float Distance) const;

private:
    float TimeSinceLastUpdate;

    UWindSimulationSubsystem* GetWindSubsystem() const;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UPointWindGeneratorComponent : public UWindGeneratorComponent
{
    GENERATED_BODY()

public:
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;
};

UENUM(BlueprintType)
enum class EWindShapeType : uint8
{
    Cylinder,
    Cone
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UDirectionalWindGeneratorComponent : public UWindGeneratorComponent
{
    GENERATED_BODY()

public:
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    EWindShapeType ShapeType = EWindShapeType::Cylinder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (EditCondition = "ShapeType == EWindShapeType::Cone", EditConditionHides))
    float ConeAngle = 45.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API UVortexWindGeneratorComponent : public UWindGeneratorComponent
{
    GENERATED_BODY()

public:
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float TangentialStrength = 100.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JK_WINDSYSTEM_API USplineWindGeneratorComponent : public UWindGeneratorComponent
{
    GENERATED_BODY()

public:
    USplineWindGeneratorComponent();

    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wind")
    class USplineComponent* WindSpline;
};
