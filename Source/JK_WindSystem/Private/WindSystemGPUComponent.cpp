#include "WindSystemGPUComponent.h"
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

class FWindSimulationCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FWindSimulationCS);
    SHADER_USE_PARAMETER_STRUCT(FWindSimulationCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, DensityField)
        SHADER_PARAMETER_SRV(Texture3D<float4>, PrevVelocityField)
        SHADER_PARAMETER_SRV(Texture3D<float>, PrevDensityField)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, Viscosity)
        SHADER_PARAMETER(float, Diffusion)
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(int32, SimulationStep)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};


IMPLEMENT_GLOBAL_SHADER(FWindSimulationCS, "/Plugin/JK_WindSystem/Private/WindSimulationCS.usf", "MainCS", SF_Compute);

UWindGPUSimulationComponent::UWindGPUSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    GridUpdateInterval = 1.0f / 30.0f; // Update 60 times per second
    TimeSinceLastGridUpdate = 0.0f;
}

void UWindGPUSimulationComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeGPUResources();
}

void UWindGPUSimulationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseGPUResources();
    Super::EndPlay(EndPlayReason);
}

void UWindGPUSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TimeSinceLastGridUpdate += DeltaTime;
    if (TimeSinceLastGridUpdate >= GridUpdateInterval)
    {
        SimulationStep(TimeSinceLastGridUpdate);
        UpdateGridFromGPU();
        TimeSinceLastGridUpdate = 0.0f;
    }
}

FVector UWindGPUSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FScopeLock Lock(&SimulationLock);

    if (!IsGridInitialized())
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return FVector::ZeroVector;
    }

    FVector LocalPos = Location - GridCenter;
    FVector GridPos = LocalPos / CellSize;

    return WindGrid->GetCell(
        FMath::FloorToInt(GridPos.X),
        FMath::FloorToInt(GridPos.Y),
        FMath::FloorToInt(GridPos.Z)
    );
}

void UWindGPUSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    FScopeLock Lock(&SimulationLock);
    if (!IsGridInitialized())
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return;
    }

    if (!IsVectorFinite(WindVelocity))
    {
        WINDSYSTEM_LOG_ERROR(TEXT("Invalid wind velocity provided: %s"), *WindVelocity.ToString());
        return;
    }

    FVector LocalPos = Location - GridCenter;
    FVector GridPos = LocalPos / CellSize;

    int32 X = FMath::FloorToInt(GridPos.X);
    int32 Y = FMath::FloorToInt(GridPos.Y);
    int32 Z = FMath::FloorToInt(GridPos.Z);

    int32 Size = WindGrid->GetSize();
    if (X >= 0 && X < Size && Y >= 0 && Y < Size && Z >= 0 && Z < Size)
    {
        FVector CurrentVelocity = WindGrid->GetCell(X, Y, Z);
        FVector NewVelocity = CurrentVelocity + WindVelocity;

        // Clamp the new velocity to prevent extreme values    
        if (NewVelocity.SizeSquared() > FMath::Square(GetMaxAllowedWindVelocity()))
        {
            NewVelocity = NewVelocity.GetSafeNormal() * GetMaxAllowedWindVelocity();
        }

        WindGrid->SetCell(X, Y, Z, NewVelocity);

        // Also update the GPU texture
        UpdateGPUTexture(X, Y, Z, NewVelocity, 1.0f);

        WINDSYSTEM_LOG_VERBOSE(TEXT("Wind added at location: Pos=%s, NewVelocity=%s"), 
            *Location.ToString(), *NewVelocity.ToString());
    }
    else
    {
        WINDSYSTEM_LOG_WARNING(TEXT("Attempted to add wind outside the grid bounds"));
    }
}

void UWindGPUSimulationComponent::UpdateGPUTexture(int32 X, int32 Y, int32 Z, const FVector& Velocity, float Density)
{
    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

    FFloat16Color VelocityData;
    VelocityData.R = Velocity.X;
    VelocityData.G = Velocity.Y;
    VelocityData.B = Velocity.Z;
    VelocityData.A = 0.0f;

    FUpdateTextureRegion3D UpdateRegion(X, Y, Z, 0, 0, 0, 1, 1, 1);

    RHICmdList.UpdateTexture3D(VelocityFieldTexture, 0, UpdateRegion, sizeof(FFloat16Color), sizeof(FFloat16Color), (uint8*)&VelocityData);
    
    float DensityData = Density;
    RHICmdList.UpdateTexture3D(DensityFieldTexture, 0, UpdateRegion, sizeof(float), sizeof(float), (uint8*)&DensityData);
}

void UWindGPUSimulationComponent::UpdateGridFromGPU()
{
    if (!IsGridInitialized())
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return;
    }

    ENQUEUE_RENDER_COMMAND(UpdateWindGrid)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            int32 Size = WindGrid->GetSize();
            TArray<FVector>& GridData = WindGrid->GetGridData();

            // FTexture3DRHIRef LocalVelocityFieldTexture = VelocityFieldTexture;
            TArray<FLinearColor> ColorData;
            ColorData.SetNum(Size * Size * Size);

            FReadSurfaceDataFlags ReadFlags(RCM_MinMax);
            RHICmdList.ReadSurfaceData(VelocityFieldTexture, FIntRect(0, 0, Size, Size), ColorData, ReadFlags);

            // Copy data to CPU grid
            for (int32 Index = 0; Index < GridData.Num(); ++Index)
            {
                GridData[Index] = FVector(ColorData[Index].R, ColorData[Index].G, ColorData[Index].B);
            }
        }
    );
}

void UWindGPUSimulationComponent::InitializeGPUResources()
{
    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
    FRDGBuilder GraphBuilder(RHICmdList);

    FRDGTextureDesc VelocityDesc = FRDGTextureDesc::Create3D(
        FIntVector(GridSize, GridSize, GridSize),
        PF_FloatRGBA,
        FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV
    );

    FRDGTextureDesc DensityDesc = FRDGTextureDesc::Create3D(
        FIntVector(GridSize, GridSize, GridSize),
        PF_R32_FLOAT,
        FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV
    );

    FRDGTexture* RDGVelocityFieldTexture = GraphBuilder.CreateTexture(VelocityDesc, TEXT("VelocityFieldTexture"));
    FRDGTexture* RDGDensityFieldTexture = GraphBuilder.CreateTexture(DensityDesc, TEXT("DensityFieldTexture"));

    GraphBuilder.Execute();

    VelocityFieldTexture = RDGVelocityFieldTexture->GetRHI();
    DensityFieldTexture = RDGDensityFieldTexture->GetRHI();

    VelocityFieldUAV = RHICmdList.CreateUnorderedAccessView(VelocityFieldTexture, 0);
    DensityFieldUAV = RHICmdList.CreateUnorderedAccessView(DensityFieldTexture, 0);

    FRHITextureSRVCreateInfo VelocitySRVCreateInfo;
    VelocitySRVCreateInfo.NumMipLevels = 1;
    VelocityFieldSRV = RHICmdList.CreateShaderResourceView(VelocityFieldTexture, VelocitySRVCreateInfo);

    FRHITextureSRVCreateInfo DensitySRVCreateInfo;
    DensitySRVCreateInfo.NumMipLevels = 1;
    DensityFieldSRV = RHICmdList.CreateShaderResourceView(DensityFieldTexture, DensitySRVCreateInfo);
}

void UWindGPUSimulationComponent::DispatchComputeShader(FRHICommandListImmediate& RHICmdList, float DeltaTime)
{
    FRDGBuilder GraphBuilder(RHICmdList);

    TShaderMapRef<FWindSimulationCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FWindSimulationCS::FParameters* Parameters = GraphBuilder.AllocParameters<FWindSimulationCS::FParameters>();
    Parameters->VelocityField = GraphBuilder.CreateUAV(GraphBuilder.RegisterExternalTexture(CreateRenderTarget(VelocityFieldTexture, TEXT("VelocityField"))));
    Parameters->DensityField = GraphBuilder.CreateUAV(GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityFieldTexture, TEXT("DensityField"))));
    Parameters->PrevVelocityField = VelocityFieldSRV;
    Parameters->PrevDensityField = DensityFieldSRV;
    Parameters->DeltaTime = DeltaTime;
    Parameters->Viscosity = Viscosity;
    Parameters->Diffusion = 0.1f; // Adjust as needed
    Parameters->GridSize = FIntVector(GridSize, GridSize, GridSize);

    // Perform multiple simulation steps
    for (int32 Step = 0; Step < 4; ++Step)
    {
        Parameters->SimulationStep = Step;

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("WindSimulation"),
            ComputeShader,
            Parameters,
            FIntVector(FMath::DivideAndRoundUp(GridSize, 8),
                FMath::DivideAndRoundUp(GridSize, 8),
                FMath::DivideAndRoundUp(GridSize, 8))
        );
    }

    GraphBuilder.Execute();
}

void UWindGPUSimulationComponent::SimulationStep(float DeltaTime)
{
    ENQUEUE_RENDER_COMMAND(WindSimulationStep)(
        [this, DeltaTime](FRHICommandListImmediate& RHICmdList)
        {
            DispatchComputeShader(RHICmdList, DeltaTime);
        }
    );
}

void UWindGPUSimulationComponent::ReleaseGPUResources()
{
    if (VelocityFieldUAV)
    {
        VelocityFieldUAV->Release();
        VelocityFieldUAV = nullptr;
    }
    if (DensityFieldUAV)
    {
        DensityFieldUAV->Release();
        DensityFieldUAV = nullptr;
    }
    if (VelocityFieldSRV)
    {
        VelocityFieldSRV->Release();
        VelocityFieldSRV = nullptr;
    }
    if (DensityFieldSRV)
    {
        DensityFieldSRV->Release();
        DensityFieldSRV = nullptr;
    }
    VelocityFieldTexture.SafeRelease();
    DensityFieldTexture.SafeRelease();
}