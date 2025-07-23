#include "WindSimulation/WindSimulation.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "ShaderCompilerCore.h"
#include "RHIDefinitions.h"
#include "RHIStaticStates.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "RenderTargetPool.h"

IMPLEMENT_GLOBAL_SHADER(FAdvanceWindSimulationCS, "/JK_WindSystem/WindSimulationPCS.usf", "MainCS", SF_Compute);

// Wind Simulation Compute Shader Class
class FAdvanceWindSimulationCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FAdvanceWindSimulationCS);
	SHADER_USE_PARAMETER_STRUCT(FAdvanceWindSimulationCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, DensityField)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, Viscosity)
		SHADER_PARAMETER(FIntVector, GridSize)
		SHADER_PARAMETER(int32, SimulationStep)
		SHADER_PARAMETER(float, MaxVelocity)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};


// UWindSimulation Implementation
UWindSimulation::UWindSimulation()
{
	DeltaTime = 0.016f;
	Viscosity = 0.01f;
	MaxVelocity = 10.0f;
	GridSize = FIntVector(64, 64, 64);
	bIsInitialized = false;
	CurrentSimulationStep = 0;
	SimulationBounds = FVector(1000.0f, 1000.0f, 1000.0f);
	SimulationOrigin = FVector::ZeroVector;
}

void UWindSimulation::Initialize(int32 InGridSizeX, int32 InGridSizeY, int32 InGridSizeZ)
{
	GridSize = FIntVector(InGridSizeX, InGridSizeY, InGridSizeZ);
	
	// Create render targets
	CreateRenderTargets();
	
	bIsInitialized = true;
	CurrentSimulationStep = 0;
	
	UE_LOG(LogTemp, Log, TEXT("WindSimulation initialized with grid size: %dx%dx%d"), InGridSizeX, InGridSizeY, InGridSizeZ);
}

void UWindSimulation::CreateRenderTargets()
{
	// Create velocity render target (float4 for RGBA/XYZW)
	VelocityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
	VelocityRenderTarget->Init(GridSize.X, GridSize.Y, GridSize.Z, EPixelFormat::PF_A32B32G32R32F);
	VelocityRenderTarget->ClearColor = FLinearColor::Black;
	VelocityRenderTarget->UpdateResource();
	
	// Create density render target (float for single channel)
	DensityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
	DensityRenderTarget->Init(GridSize.X, GridSize.Y, GridSize.Z, EPixelFormat::PF_R32_FLOAT);
	DensityRenderTarget->ClearColor = FLinearColor::Black;
	DensityRenderTarget->UpdateResource();
}

void UWindSimulation::StepSimulation(float InDeltaTime)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("WindSimulation::StepSimulation called but simulation not initialized!"));
		return;
	}
	
	// Execute all 4 simulation steps: Diffuse -> Project -> Advect -> Project
	for (int32 Step = 0; Step < 4; Step++)
	{
		ExecuteSimulationStep(Step, InDeltaTime);
	}
}

void UWindSimulation::ExecuteSimulationStep(int32 SimulationStep, float InDeltaTime)
{
	if (!VelocityRenderTarget || !DensityRenderTarget)
	{
		return;
	}
	
	// Execute on render thread
	ENQUEUE_RENDER_COMMAND(WindSimulationStep)(
		[this, SimulationStep, InDeltaTime](FRHICommandListImmediate& RHICmdList)
		{
			ExecuteSimulationOnRenderThread(RHICmdList, SimulationStep, InDeltaTime);
		});
}

void UWindSimulation::ExecuteSimulationOnRenderThread(FRHICommandListImmediate& RHICmdList, int32 SimulationStep, float InDeltaTime)
{
	check(IsInRenderingThread());
	
	FRDGBuilder GraphBuilder(RHICmdList);
	
	// Create RDG textures from render targets
	FRDGTextureRef VelocityTexture = GraphBuilder.RegisterExternalTexture(
		CreateRenderTarget(VelocityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("VelocityField")));
	
	FRDGTextureRef DensityTexture = GraphBuilder.RegisterExternalTexture(
		CreateRenderTarget(DensityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("DensityField")));
	
	// Create UAVs
	FRDGTextureUAVRef VelocityUAV = GraphBuilder.CreateUAV(VelocityTexture);
	FRDGTextureUAVRef DensityUAV = GraphBuilder.CreateUAV(DensityTexture);
	
	// Set up compute shader parameters
	FAdvanceWindSimulationCS::FParameters* Parameters = GraphBuilder.AllocParameters<FAdvanceWindSimulationCS::FParameters>();
	Parameters->VelocityField = VelocityUAV;
	Parameters->DensityField = DensityUAV;
	Parameters->DeltaTime = InDeltaTime;
	Parameters->Viscosity = Viscosity;
	Parameters->GridSize = GridSize;
	Parameters->SimulationStep = SimulationStep;
	Parameters->MaxVelocity = MaxVelocity;
	
	// Get compute shader
	TShaderMapRef<FAdvanceWindSimulationCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	
	// Calculate dispatch count (8x8x8 thread groups as defined in shader)
	const int32 ThreadGroupSize = 8;
	FIntVector DispatchCount;
	DispatchCount.X = FMath::DivideAndRoundUp(GridSize.X, ThreadGroupSize);
	DispatchCount.Y = FMath::DivideAndRoundUp(GridSize.Y, ThreadGroupSize);
	DispatchCount.Z = FMath::DivideAndRoundUp(GridSize.Z, ThreadGroupSize);
	
	// Add compute pass
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("WindSimulationStep_%d", SimulationStep),
		ComputeShader,
		Parameters,
		DispatchCount
	);
	
	GraphBuilder.Execute();
}

void UWindSimulation::ResetSimulation()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	// Clear render targets
	if (VelocityRenderTarget)
	{
		VelocityRenderTarget->ClearColor = FLinearColor::Black;
		VelocityRenderTarget->UpdateResource();
	}
	
	if (DensityRenderTarget)
	{
		DensityRenderTarget->ClearColor = FLinearColor::Black;
		DensityRenderTarget->UpdateResource();
	}
	
	CurrentSimulationStep = 0;
	UE_LOG(LogTemp, Log, TEXT("WindSimulation reset"));
}

FVector UWindSimulation::GetVelocityAtLocation(const FVector& WorldLocation) const
{
	if (!bIsInitialized)
	{
		return FVector::ZeroVector;
	}
	
	FIntVector GridCoord = WorldLocationToGridCoord(WorldLocation);
	if (!IsValidGridCoord(GridCoord))
	{
		return FVector::ZeroVector;
	}
	
	// TODO: Implement GPU readback for getting velocity at specific location
	// This is a simplified implementation - in practice you'd need to read back from GPU
	return FVector::ZeroVector;
}

void UWindSimulation::AddVelocityAtLocation(const FVector& WorldLocation, const FVector& Velocity)
{
	if (!bIsInitialized)
	{
		return;
	}
	
	FIntVector GridCoord = WorldLocationToGridCoord(WorldLocation);
	if (!IsValidGridCoord(GridCoord))
	{
		return;
	}
	
	// TODO: Implement GPU write for adding velocity at specific location
	// This would require a separate compute shader or CPU->GPU data transfer
}

void UWindSimulation::SetSimulationParameters(float InViscosity, float InMaxVelocity)
{
	Viscosity = InViscosity;
	MaxVelocity = InMaxVelocity;
}

FIntVector UWindSimulation::WorldLocationToGridCoord(const FVector& WorldLocation) const
{
	FVector RelativeLocation = WorldLocation - SimulationOrigin;
	FVector NormalizedLocation = RelativeLocation / SimulationBounds;
	
	// Clamp to [0,1] range and convert to grid coordinates
	NormalizedLocation = NormalizedLocation.BoundToBox(FVector::ZeroVector, FVector::OneVector);
	
	FIntVector GridCoord;
	GridCoord.X = FMath::FloorToInt(NormalizedLocation.X * (GridSize.X - 1));
	GridCoord.Y = FMath::FloorToInt(NormalizedLocation.Y * (GridSize.Y - 1));
	GridCoord.Z = FMath::FloorToInt(NormalizedLocation.Z * (GridSize.Z - 1));
	
	return GridCoord;
}

FVector UWindSimulation::GridCoordToWorldLocation(const FIntVector& GridCoord) const
{
	FVector NormalizedLocation;
	NormalizedLocation.X = (float)GridCoord.X / (GridSize.X - 1);
	NormalizedLocation.Y = (float)GridCoord.Y / (GridSize.Y - 1);
	NormalizedLocation.Z = (float)GridCoord.Z / (GridSize.Z - 1);
	
	FVector WorldLocation = SimulationOrigin + (NormalizedLocation * SimulationBounds);
	return WorldLocation;
}

bool UWindSimulation::IsValidGridCoord(const FIntVector& GridCoord) const
{
	return GridCoord.X >= 0 && GridCoord.X < GridSize.X &&
		   GridCoord.Y >= 0 && GridCoord.Y < GridSize.Y &&
		   GridCoord.Z >= 0 && GridCoord.Z < GridSize.Z;
}

