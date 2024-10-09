#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WindSystemComponent.h"
#include "WindSourceComponent.h"
#include "WindSubsystem.generated.h"

UCLASS()
class JK_WINDSYSTEM_API UWindSimulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UWindSimulationSubsystem();

    // USubsystem implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Tick function
    virtual bool Tick(float DeltaTime);

    // Wind query function
    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    FVector GetWindVelocityAtLocation(const FVector& WorldLocation) const;

    // Set the wind simulation component
    void SetWindSimulationComponent(UWindSimulationComponent* InWindSimComponent);

private:
    UPROPERTY()
    UWindSimulationComponent* WindSimComponent;

    FTSTicker::FDelegateHandle TickHandle;

private:
    TArray<UWindGeneratorComponent*> WindGenerators;

public:
    void RegisterWindGenerator(UWindGeneratorComponent* WindGenerator);
    void UnregisterWindGenerator(UWindGeneratorComponent* WindGenerator);

};
