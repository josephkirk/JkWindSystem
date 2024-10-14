#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WindSourceComponent.h"
#include "WindSubsystem.generated.h"

class AWindSystemActor;
class UWindGeneratorComponent;
class UWindZoneVolumeComponent;
UCLASS()
class JK_WINDSYSTEM_API UWindSimulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UWindSimulationSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    virtual bool Tick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    FVector GetWindVelocityAtLocation(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    void AddWindAtLocation(const FVector& Location, const FVector& WindVelocity);

    void RegisterWindGenerator(UWindGeneratorComponent* WindGenerator);
    void UnregisterWindGenerator(UWindGeneratorComponent* WindGenerator);

    void RegisterWindZone(UWindZoneVolumeComponent* Modifier);
    void UnregisterWindZone(UWindZoneVolumeComponent* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
    AWindSystemActor* GetWindSystemActor() const { return WindSystemActor; }

private:
    UPROPERTY()
    TArray<UWindZoneVolumeComponent*> WindZones;

    UPROPERTY()
    AWindSystemActor* WindSystemActor;

    UPROPERTY()
    TArray<UWindGeneratorComponent*> WindGenerators;
    FCriticalSection GeneratorsLock;

    FTSTicker::FDelegateHandle TickHandle;

    void UpdateWindGenerators(float DeltaTime);
    void EnsureWindSystemActorInitialized();
    void DestroyWindSystemActor();
};