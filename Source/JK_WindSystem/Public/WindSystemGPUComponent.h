#pragma once

#include "CoreMinimal.h"
#include "WindSystemComponent.h"
#include "RenderGraphResources.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "WindSystemGPUComponent.generated.h"

class FRDGBuilder;
class UTextureRenderTargetVolume;

UCLASS()
class JK_WINDSYSTEM_API UWindGPUSimulationComponent : public UWindSimulationComponent
{
    GENERATED_BODY()

public:
    UWindGPUSimulationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;
    virtual void AddWindAtLocation(const FVector& Location, const FVector& WindVelocity) override;

    virtual void SimulationStep(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
    UTextureRenderTargetVolume* VelocityRenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
    UTextureRenderTargetVolume* DensityRenderTarget;

private:
    void Execute(UTextureRenderTargetVolume* InVelocityRenderTarget, UTextureRenderTargetVolume* InDensityRenderTargetfloat,float DeltaTime);
    void Execute_RenderThread(FRHICommandListImmediate& RHICmdList, UTextureRenderTargetVolume* InVelocityRenderTarget, UTextureRenderTargetVolume* InDensityRenderTarget, float DeltaTime);

    float LastDeltaTime;
};