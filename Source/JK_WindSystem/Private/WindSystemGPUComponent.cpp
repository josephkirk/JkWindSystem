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
#include "WindSystemCommon.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RHIGPUReadback.h"

// // Add the parameter struct after includes and before any function definitions
// struct FCopyTextureToBufferParameters
// {
//     DECLARE_TYPE_LAYOUT(FCopyTextureToBufferParameters, NonVirtual);
    
//     LAYOUT_FIELD(FRDGTextureSRVRef, SourceTexture);
//     LAYOUT_FIELD(FRDGBufferUAVRef, OutputBuffer);
//     LAYOUT_FIELD(FIntVector, Size);
// };

class FWindSimulationCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FWindSimulationCS);
    SHADER_USE_PARAMETER_STRUCT(FWindSimulationCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, VelocityField)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, DensityField)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, Viscosity)
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(int32, SimulationStep)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FCopyTextureToBufferCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FCopyTextureToBufferCS);
    SHADER_USE_PARAMETER_STRUCT(FCopyTextureToBufferCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D, SourceTexture)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FFloat16Color>, OutputBuffer)
        SHADER_PARAMETER(uint32, SizeX)
        SHADER_PARAMETER(uint32, SizeY)
        SHADER_PARAMETER(uint32, SizeZ)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};


// struct FRDGParameters
// {
//     FCopyTextureToBufferCS::FParameters* Parameters;
// };

IMPLEMENT_GLOBAL_SHADER(FCopyTextureToBufferCS, "/JK_WindSystem/CopyTextureToBuffer.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FWindSimulationCS, "/JK_WindSystem/WindSimulationCS.usf", "MainCS", SF_Compute);

UWindGPUSimulationComponent::UWindGPUSimulationComponent()
    : VelocityRenderTarget(nullptr), DensityRenderTarget(nullptr)
{
    //VelocityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
    //VelocityRenderTarget->Init(GridSize, GridSize, GridSize, PF_FloatRGBA);
    //VelocityRenderTarget->UpdateResource();

    //DensityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
    //DensityRenderTarget->Init(GridSize, GridSize, GridSize, PF_R32_FLOAT);
    //DensityRenderTarget->UpdateResource();

    LastDeltaTime = 0;

    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void UWindGPUSimulationComponent::BeginPlay()
{
    Super::BeginPlay();

    // Initialize render targets if not set
    if (!VelocityRenderTarget)
    {
        VelocityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
        VelocityRenderTarget->Init(GridSize, GridSize, GridSize, PF_FloatRGBA);
        VelocityRenderTarget->UpdateResource();
    }

    if (!DensityRenderTarget)
    {
        DensityRenderTarget = NewObject<UTextureRenderTargetVolume>(this);
        DensityRenderTarget->Init(GridSize, GridSize, GridSize, PF_R32_FLOAT);
        DensityRenderTarget->UpdateResource();
    }

    // Start the timer to update the WindGrid
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateWindGridTimerHandle, this, &UWindGPUSimulationComponent::UpdateWindGridFromGPU_Callback, UpdateInterval, true);
    }
}   

void UWindGPUSimulationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear the timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateWindGridTimerHandle);
    }


    Super::EndPlay(EndPlayReason);
}

void UWindGPUSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    LastDeltaTime = DeltaTime;
}

FVector UWindGPUSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    // Implement this based on your specific needs
    return FVector::ZeroVector;
}

void UWindGPUSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    // Implement this based on your specific needs
}

void UWindGPUSimulationComponent::SimulationStep(float DeltaTime)
{
    Execute(VelocityRenderTarget, DensityRenderTarget, DeltaTime);
}

void UWindGPUSimulationComponent::UpdateWindGridFromGPU_Callback()
{
    UpdateWindGridFromGPU();
}
void UWindGPUSimulationComponent::UpdateWindGridFromGPU()
{
    ENQUEUE_RENDER_COMMAND(UpdateWindGridFromGPU)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            UpdateWindGridFromGPU_RenderThread(RHICmdList);
        });
}

void UWindGPUSimulationComponent::UpdateWindGridFromGPU_RenderThread(FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());

    if (!VelocityRenderTarget || !WindGrid)
    {
        return;
    }

    FRDGBuilder GraphBuilder(RHICmdList);
    {
        // Get texture RHI
        FRHITexture* TextureRHI = VelocityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
        FRDGTextureRef SourceTexture = GraphBuilder.RegisterExternalTexture(
            CreateRenderTarget(TextureRHI, TEXT("VelocityTexture"))
        );

        // Create buffer for output
        const int32 BufferSize = VelocityRenderTarget->SizeX * VelocityRenderTarget->SizeY * VelocityRenderTarget->SizeZ;
        FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
            FRDGBufferDesc::CreateStructuredDesc(sizeof(FFloat16Color), BufferSize),
            TEXT("OutputBuffer")
        );
        FRDGBufferUAVRef BufferUAV = GraphBuilder.CreateUAV(OutputBuffer);

        // Get compute shader
        TShaderMapRef<FCopyTextureToBufferCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

        // Create parameters
        auto* PassParameters = GraphBuilder.AllocParameters<FCopyTextureToBufferCS::FParameters>();
        PassParameters->SourceTexture = GraphBuilder.CreateSRV(SourceTexture);
        PassParameters->OutputBuffer = BufferUAV;
        PassParameters->SizeX = VelocityRenderTarget->SizeX;
        PassParameters->SizeY = VelocityRenderTarget->SizeY;
        PassParameters->SizeZ = VelocityRenderTarget->SizeZ;

        // Add compute pass
        GraphBuilder.AddPass(
            RDG_EVENT_NAME("CopyTextureToBuffer"),
            PassParameters,  // Pass the parameters struct
            ERDGPassFlags::Compute,
            [ComputeShader, PassParameters](FRHICommandList& RHICmdList)
            {
                FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters,
                    FComputeShaderUtils::GetGroupCount(
                        FIntVector(PassParameters->SizeX, PassParameters->SizeY, PassParameters->SizeZ),
                        FIntVector(8, 8, 1)
                    )
                );
            }
        );
    }
    GraphBuilder.Execute();
}

void UWindGPUSimulationComponent::Execute(UTextureRenderTargetVolume* InVelocityRenderTarget, UTextureRenderTargetVolume* InDensityRenderTarget, float DeltaTime)
{
    if (!InVelocityRenderTarget || !InDensityRenderTarget)
    {
        return;
    }

    ENQUEUE_RENDER_COMMAND(WindSimulationExecution)(
        [this, InVelocityRenderTarget, InDensityRenderTarget, DeltaTime](FRHICommandListImmediate& RHICmdList)
        {
            Execute_RenderThread(RHICmdList, InVelocityRenderTarget, InDensityRenderTarget, DeltaTime);
        });
}

void UWindGPUSimulationComponent::Execute_RenderThread(FRHICommandListImmediate& RHICmdList, UTextureRenderTargetVolume* InVelocityRenderTarget,UTextureRenderTargetVolume* InDensityRenderTarget, float DeltaTime)
{
    ensure(IsInRenderingThread());

    FRDGBuilder GraphBuilder(RHICmdList);
    {
        TShaderMapRef<FWindSimulationCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

        FRDGTextureRef VelocityField = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InVelocityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("WindSimulation.VelocityField")));

        FRDGTextureRef DensityField = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InDensityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("WindSimulation.DensityField")));
            
        FRDGTextureDesc IntermediateVelocityFieldTextureDesc(
            FRDGTextureDesc::Create3D(
                FIntVector(VelocityField->Desc.Extent.X, VelocityField->Desc.Extent.Y, VelocityField->Desc.Extent.X),
                VelocityField->Desc.Format,
                FClearValueBinding::Green,
                TexCreate_ShaderResource | TexCreate_UAV));

        FRDGTextureDesc IntermediateDensityFieldTextureDesc(
            FRDGTextureDesc::Create3D(
                FIntVector(DensityField->Desc.Extent.X, DensityField->Desc.Extent.Y, DensityField->Desc.Extent.X),
                DensityField->Desc.Format,
                FClearValueBinding::Green,
                TexCreate_ShaderResource | TexCreate_UAV));

        FRDGTextureRef IntermediateVelocityFieldTexture = GraphBuilder.CreateTexture(IntermediateVelocityFieldTextureDesc, TEXT("WindSimulation.IntermediateVelocityField"));
        FRDGTextureRef IntermediateDensityFieldTexture = GraphBuilder.CreateTexture(IntermediateDensityFieldTextureDesc, TEXT("WindSimulation.IntermediateDensityField"));

        FRDGTextureUAVRef VelocityField_UAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(IntermediateVelocityFieldTexture));
        FRDGTextureUAVRef DensityField_UAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(IntermediateDensityFieldTexture));

        FWindSimulationCS::FParameters* Parameters = GraphBuilder.AllocParameters<FWindSimulationCS::FParameters>();
        Parameters->VelocityField = VelocityField_UAV;
        Parameters->DensityField = DensityField_UAV;
        Parameters->DeltaTime = DeltaTime;
        Parameters->Viscosity = Viscosity;
        Parameters->GridSize = FIntVector(GridSize, GridSize, GridSize);

        ClearUnusedGraphResources(ComputeShader, Parameters);
        
        for (int32 Step = 0; Step < 4; ++Step)
        {
            Parameters->SimulationStep = Step;
            
            FComputeShaderUtils::AddPass(
                GraphBuilder,
                RDG_EVENT_NAME("WindSimulation"),
                ERDGPassFlags::Compute,
                ComputeShader,
                Parameters,
                FIntVector(FMath::DivideAndRoundUp(GridSize, 8),
                    FMath::DivideAndRoundUp(GridSize, 8),
                    FMath::DivideAndRoundUp(GridSize, 8))
            );
        }

        // Copy back to RenderTarget
        {
            RDG_EVENT_SCOPE(GraphBuilder, "WindSimulationPostCopy");

            AddCopyTexturePass(GraphBuilder, IntermediateVelocityFieldTexture, VelocityField, FRHICopyTextureInfo());

            AddCopyTexturePass(GraphBuilder, IntermediateDensityFieldTexture, DensityField, FRHICopyTextureInfo());
        }
    }
    GraphBuilder.Execute();
}
