#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

//-----------------------------------------------------------------------------
// 1. Velocity Injection (VelocityInjection.usf)
//    Velocity Injection Compute Shader Class
//-----------------------------------------------------------------------------
class FVelocityInjectionCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FVelocityInjectionCS);
	SHADER_USE_PARAMETER_STRUCT(FVelocityInjectionCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
		SHADER_PARAMETER(FIntVector, GridCoord)
		SHADER_PARAMETER(FVector3f, Velocity)
		SHADER_PARAMETER(float, InjectionRadius)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
//-----------------------------------------------------------------------------
// 2. Wind Simulation (WindSimulationCS.usf)
//    Wind Simulation Compute Shader Class
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
//  Pressure Solve Shader (WindPressureSolveCS.usf)
//    Performs one iteration of the Jacobi pressure solve.
//-----------------------------------------------------------------------------
class FWindPressureSolveCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FWindPressureSolveCS);
	SHADER_USE_PARAMETER_STRUCT(FWindPressureSolveCS, FGlobalShader);

	// Define the parameters that match the HLSL file
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
		SHADER_PARAMETER_RDG_TEXTURE(RWTexture3D<float>, PressureFieldRead)
		SHADER_PARAMETER(FIntVector, GridSize)

		// Output
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, PressureFieldWrite)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// This shader should be compiled for all platforms that support compute shaders.
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		// This must match the #define in the .usf file
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), 8);
	}
};


//-----------------------------------------------------------------------------
// Pressure Apply Shader (WindPressureApplyCS.usf)
//    Subtracts the final pressure gradient from the velocity field.
//-----------------------------------------------------------------------------
class FWindPressureApplyCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FWindPressureApplyCS);
	SHADER_USE_PARAMETER_STRUCT(FWindPressureApplyCS, FGlobalShader);

	// Define the parameters that match the HLSL file
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// Input
		SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float>, PressureField)
		SHADER_PARAMETER(FIntVector, GridSize)

		// Output
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_PER_GROUP"), 8);
	}
};