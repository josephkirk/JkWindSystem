#pragma once

#include "CoreMinimal.h"
#include "WindSimulationComponent.h"
#include "RHI.h"
#include "RHIResources.h"
#include "WindSystemGPUComponent.generated.h"

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

private:

    FStructuredBufferRHIRef WindGridBuffer;
    FUnorderedAccessViewRHIRef WindGridUAV;
    FShaderResourceViewRHIRef WindGridSRV;

    void InitializeGPUResources();
    void ReleaseGPUResources();
    void DispatchComputeShader(FRHICommandList& RHICmdList);

    void UpdateGridFromGPU();
    float TimeSinceLastGridUpdate;
    float GridUpdateInterval;
}