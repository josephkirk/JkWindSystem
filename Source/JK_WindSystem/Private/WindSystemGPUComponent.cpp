#include "WindSystemGPUComponent.h"
#include "ShaderCompilerCore.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"

class FWindSimulationCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FWindSimulationCS);
    SHADER_USE_PARAMETER_STRUCT(FWindSimulationCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVector4f>, WindGrid)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, Viscosity)
        SHADER_PARAMETER(FIntVector, GridSize)
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
    GridSize = GetSettings()->GridSize; // Default value
    CellSize = GetSettings()->CellSize; // Default value
    Viscosity = GetSettings()->Viscosity;
    SimulationFrequency = GetSettings()->SimulationFrequency;
    bAutoActivate = true;
    GridUpdateInterval = 1.0f / 30.0f; // Update 30 times per second, adjust as needed
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
    // Update Grid From GPU Compute
    TimeSinceLastGridUpdate += DeltaTime;
    if (TimeSinceLastGridUpdate >= GridUpdateInterval)
    {
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
    FVector GridPos = LocalPos / WindGrid->GetCellSize();

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

    // ... existing validation code ...

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
        UpdateGPUTexture(X, Y, Z, NewVelocity);

        WINDSYSTEM_LOG_VERBOSE(TEXT("Wind added at location: Pos=%s, NewVelocity=%s"), 
            *Location.ToString(), *NewVelocity.ToString());
    }
    else
    {
        WINDSYSTEM_LOG_WARNING(TEXT("Attempted to add wind outside the grid bounds"));
    }
}

void UWindGPUSimulationComponent::UpdateGPUTexture(int32 X, int32 Y, int32 Z, const FVector& Velocity)
{
    ENQUEUE_RENDER_COMMAND(UpdateWindVelocity)(
        [this, X, Y, Z, Velocity](FRHICommandListImmediate& RHICmdList)
        {
            int32 Size = WindGrid->GetSize();
            uint32 SrcPitch, SlicePitch;
            FFloat16Color* Data = (FFloat16Color*)RHILockTexture3D(VelocityFieldTexture, 0, RLM_WriteOnly, SrcPitch, SlicePitch, 1, 1, 1, X, Y, Z);
            
            *Data = FFloat16Color(Velocity.X, Velocity.Y, Velocity.Z, 0.0f);

            RHIUnlockTexture3D(VelocityFieldTexture, 0, Z);
        }
    );
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

            // Read data from GPU
            uint32 SrcPitch, SlicePitch;
            FFloat16Color* Data = (FFloat16Color*)RHILockTexture3D(VelocityFieldTexture, 0, RLM_ReadOnly, SrcPitch, SlicePitch, Size, Size, Size);

            // Copy data to CPU grid
            for (int32 Z = 0; Z < Size; ++Z)
            {
                for (int32 Y = 0; Y < Size; ++Y)
                {
                    for (int32 X = 0; X < Size; ++X)
                    {
                        int32 Index = X + Y * Size + Z * Size * Size;
                        GridData[Index] = FVector(Data[Index].R, Data[Index].G, Data[Index].B);
                    }
                }
            }

            RHIUnlockTexture3D(VelocityFieldTexture, 0, 0);
        }
    );
}

void UWindGPUSimulationComponent::InitializeGPUResources()
{
    FRHIResourceCreateInfo CreateInfo;
    WindGridBuffer = RHICreateStructuredBuffer(sizeof(FVector4f), GridSize * GridSize * GridSize * sizeof(FVector4f), BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);
    WindGridUAV = RHICreateUnorderedAccessView(WindGridBuffer, false, false);
    WindGridSRV = RHICreateShaderResourceView(WindGridBuffer, sizeof(FVector4f), PF_A32B32G32R32F);
}

void UWindGPUSimulationComponent::ReleaseGPUResources()
{
    WindGridBuffer.SafeRelease();
    WindGridUAV.SafeRelease();
    WindGridSRV.SafeRelease();
}

void UWindGPUSimulationComponent::DispatchComputeShader(FRHICommandList& RHICmdList)
{
    TShaderMapRef<FWindSimulationCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    FWindSimulationCS::FParameters Parameters;
    Parameters.WindGrid = WindGridUAV;
    Parameters.DeltaTime = 1.0f / SimulationFrequency;
    Parameters.Viscosity = Viscosity;
    Parameters.GridSize = FIntVector(GridSize, GridSize, GridSize);

    FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters,
        FIntVector(FMath::DivideAndRoundUp(GridSize, 8),
                   FMath::DivideAndRoundUp(GridSize, 8),
                   FMath::DivideAndRoundUp(GridSize, 8)));
}

void UWindGPUSimulationComponent::SimulationStep(float DeltaTime)
{
    ENQUEUE_RENDER_COMMAND(WindSimulationStep)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            DispatchComputeShader(RHICmdList);
        }
    );
}