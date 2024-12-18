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

IMPLEMENT_GLOBAL_SHADER(FWindSimulationCS, "/Plugin/JK_WindSystem/Private/WindSimulationCS.usf", "MainCS", SF_Compute);

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
    //UpdateWindGridFromGPU();
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
        UE_LOG(LogTemp, Error, TEXT("VelocityRenderTarget or WindGrid is null"));
        return;
    }

    FRDGBuilder GraphBuilder(RHICmdList);
    {
        FRDGTextureRef SourceTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(VelocityRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture(), TEXT("VelocityField")));

        FIntVector Size = WindGrid->GetBoundSize();
        uint32 BufferSize = Size.X * Size.Y * Size.Z * sizeof(FFloat16Color);

        // Create a staging buffer for the readback
        FRDGBufferRef StagingBuffer = GraphBuilder.CreateBuffer(
            FRDGBufferDesc::CreateStructuredDesc(sizeof(FFloat16Color), Size.X * Size.Y * Size.Z),
            TEXT("WindSimulationStagingBuffer")
        );

        // Add pass to copy texture data to the staging buffer
        GraphBuilder.AddPass(
            RDG_EVENT_NAME("CopyTextureToBuffer"),
            ERDGPassFlags::Copy,
            [SourceTexture, StagingBuffer, Size, BufferSize](FRHICommandList& RHICmdList)
            {
                FRHIBuffer* Buffer = StagingBuffer->GetRHI();
                FRHITexture* Texture = SourceTexture->GetRHI();

                if (void* MappedData = RHICmdList.LockBuffer(Buffer, 0, BufferSize, RLM_WriteOnly))
                {
                    FFloat16Color* DestData = static_cast<FFloat16Color*>(MappedData);
                    
                    // Create a temporary texture for readback
                    FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create2D(TEXT("TempReadback"))
                        .SetExtent(Size.X, Size.Y)
                        .SetFormat(PF_FloatRGBA)
                        .SetNumMips(1)
                        .SetFlags(TexCreate_ShaderResource);
                    
                    FTexture2DRHIRef TempTexture = RHICreateTexture(Desc);

                    // Copy slice by slice
                    for (int32 SliceIndex = 0; SliceIndex < static_cast<int32>(Size.Z); ++SliceIndex)
                    {
                        uint32 DestOffset = SliceIndex * Size.X * Size.Y;
                        FRHICopyTextureInfo CopyInfo;
                        CopyInfo.Size = FIntVector(Size.X, Size.Y, 1);
                        CopyInfo.SourceSliceIndex = SliceIndex;
                        RHICmdList.CopyTexture(Texture, TempTexture, CopyInfo);
                        
                        uint32 Stride = 0;
                        if (void* TextureData = RHILockTexture2D(TempTexture, 0, RLM_ReadOnly, Stride, true))
                        {
                            FMemory::Memcpy(
                                DestData + DestOffset,
                                TextureData,
                                Size.X * Size.Y * sizeof(FFloat16Color)
                            );
                            RHIUnlockTexture2D(TempTexture, 0, false);
                        }
                    }
                    
                    RHICmdList.UnlockBuffer(Buffer);
                }
            }
        );

        // Create GPU readback object
        FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("WindSimulationReadback"));
        AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, StagingBuffer, BufferSize);

        // Setup async readback
        auto RunnerFunc = [this, GPUBufferReadback, Size](auto&& RunnerFunc) -> void {
            if (GPUBufferReadback->IsReady())
            {
                const FFloat16Color* ColorData = static_cast<const FFloat16Color*>(GPUBufferReadback->Lock(Size.X * Size.Y * Size.Z * sizeof(FFloat16Color)));
                
                if (ColorData)
                {
                    AsyncTask(ENamedThreads::GameThread, [this, ColorData, Size]() {
                        for (int32 Z = 0; Z < Size.Z; ++Z)
                        {
                            for (int32 Y = 0; Y < Size.Y; ++Y)
                            {
                                for (int32 X = 0; X < Size.X; ++X)
                                {
                                    int32 Index = X + Y * Size.X + Z * Size.X * Size.Y;
                                    FFloat16Color Color = ColorData[Index];
                                    FVector Velocity(Color.R.GetFloat(), Color.G.GetFloat(), Color.B.GetFloat());
                                    WindGrid->SetCell(X, Y, Z, Velocity);
                                }
                            }
                        }
                    });
                }
                
                GPUBufferReadback->Unlock();
                delete GPUBufferReadback;
            }
            else
            {
                AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
                    RunnerFunc(RunnerFunc);
                });
            }
        };

        AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
            RunnerFunc(RunnerFunc);
        });
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
