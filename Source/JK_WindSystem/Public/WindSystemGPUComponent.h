#pragma once

#include "CoreMinimal.h"
#include "WindSystemComponent.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RenderGraphResources.h"
#include "WindSystemCommon.h"
#include "WindSystemGPUComponent.generated.h"

#define MAX_GRID_SIZE 128 // Adjust this value as needed

UCLASS()
class JK_WINDSYSTEM_API UWindGPUSimulationComponent : public UWindSimulationComponent
{
    GENERATED_BODY()

public:
    UWindGPUSimulationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SimulationStep(float DeltaTime) override;

    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const override;
    virtual void AddWindAtLocation(const FVector& Location, const FVector& WindVelocity) override;

private:
    FTexture3DRHIRef VelocityFieldTexture;
    FTexture3DRHIRef DensityFieldTexture;
    FRHIUnorderedAccessView* VelocityFieldUAV;
    FRHIUnorderedAccessView* DensityFieldUAV;
    FRHIShaderResourceView* VelocityFieldSRV;
    FRHIShaderResourceView* DensityFieldSRV;

    float GridUpdateInterval;
    float TimeSinceLastGridUpdate;

    void InitializeGPUResources();
    void ReleaseGPUResources();
    void DispatchComputeShader(FRHICommandListImmediate& RHICmdList, float DeltaTime);
    void UpdateGPUTexture(int32 X, int32 Y, int32 Z, const FVector& Velocity, float Density);
    void UpdateGridFromGPU();
};