#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "RenderGraphDefinitions.h"
#include "RHICommandList.h"
#include "WindSimulation.generated.h"

/**
 * Wind Simulation UObject that manages 3D fluid simulation using compute shaders
 * Interfaces with WindSimulationPCS.usf compute shader for GPU-based wind simulation
 */
UCLASS(BlueprintType, Blueprintable)
class JK_WINDSYSTEM_API UWindSimulation : public UObject
{
	GENERATED_BODY()

public:
	// Constructor
	UWindSimulation();

	// Simulation parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	float DeltaTime = 0.016f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	float Viscosity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	float MaxVelocity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	FIntVector GridSize = FIntVector(64, 64, 64);

	// Render targets for simulation data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	TObjectPtr<UTextureRenderTargetVolume> VelocityRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	TObjectPtr<UTextureRenderTargetVolume> DensityRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	TObjectPtr<UTextureRenderTargetVolume> PressureRenderTarget;

	// Pressure solver parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation", meta = (ClampMin = "1", ClampMax = "50"))
	int32 PressureSolverIterations = 20;

	// Public interface functions
	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	void Initialize(int32 InGridSizeX = 64, int32 InGridSizeY = 64, int32 InGridSizeZ = 64);

	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	void StepSimulation(float InDeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	void ResetSimulation();

	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	FVector GetVelocityAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	void AddVelocityAtLocation(const FVector& WorldLocation, const FVector& Velocity);

	// Advanced simulation control
	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	void SetSimulationParameters(float InViscosity, float InMaxVelocity);

	UFUNCTION(BlueprintCallable, Category = "Wind Simulation")
	bool IsInitialized() const { return bIsInitialized; }

protected:
	// Internal state
	bool bIsInitialized = false;
	int32 CurrentSimulationStep = 0;

	// Internal simulation functions
	void ExecuteSimulationStep(int32 SimulationStep, float InDeltaTime);
	void CreateRenderTargets();
	void DispatchComputeShader(int32 SimulationStep, float InDeltaTime);

	// Render thread execution
	void ExecuteSimulationOnRenderThread(FRHICommandListImmediate& RHICmdList, int32 SimulationStep, float InDeltaTime);
	void ExecuteVelocityInjectionOnRenderThread(FRHICommandListImmediate& RHICmdList, const FIntVector& GridCoord, const FVector& Velocity, float InjectionRadius);
	
	// Pressure solver methods
	void ExecutePressureSolveOnRenderThread(FRHICommandListImmediate& RHICmdList);
	void ExecutePressureApplyOnRenderThread(FRHICommandListImmediate& RHICmdList);

	// Utility functions
	FIntVector WorldLocationToGridCoord(const FVector& WorldLocation) const;
	FVector GridCoordToWorldLocation(const FIntVector& GridCoord) const;
	bool IsValidGridCoord(const FIntVector& GridCoord) const;

	// Simulation bounds (can be customized)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	FVector SimulationBounds = FVector(1000.0f, 1000.0f, 1000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind Simulation")
	FVector SimulationOrigin = FVector::ZeroVector;
};
