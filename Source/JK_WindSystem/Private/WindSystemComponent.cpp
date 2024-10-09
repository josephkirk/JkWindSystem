// WindSystemComponent.cpp
#include "WindSystemComponent.h"
#include "Async/ParallelFor.h"
#include "Math/UnrealMathSSE.h"

UWindSimulationComponent::UWindSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    BaseGridSize = 32;
    CellSize = 100.0f;
    Viscosity = 0.1f;
    SimulationFrequency = 60.0f;
}

void UWindSimulationComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeGrid();

    SimulationWorker = new FWindSimulationWorker(this);
    SimulationThread = FRunnableThread::Create(SimulationWorker, TEXT("WindSimulationThread"));
}

void UWindSimulationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SimulationWorker)
    {
        SimulationWorker->Stop();
        SimulationThread->WaitForCompletion();
        delete SimulationWorker;
        delete SimulationThread;
    }

    Super::EndPlay(EndPlayReason);
}

void UWindSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // Main thread work, if any
}

void UWindSimulationComponent::InitializeGrid()
{
    AdaptiveGrid.SetNum(BaseGridSize * BaseGridSize * BaseGridSize);
}

void UWindSimulationComponent::SimulationStep(float DeltaTime)
{
    UpdateAdaptiveGrid();

    TArray<FVector> TempGrid;
    TempGrid.SetNum(AdaptiveGrid.Num());

    // Multi-threaded diffusion
    ParallelFor(AdaptiveGrid.Num(), [&](int32 Index)
        {
            FAdaptiveGridCell& Cell = AdaptiveGrid[Index];
            TArray<FVector> CellVelocity = { Cell.Velocity };
            TArray<FVector> TempVelocity;
            TempVelocity.SetNum(1);
            Diffuse(TempVelocity, CellVelocity, Viscosity, DeltaTime);
            TempGrid[Index] = TempVelocity[0];
        });

    // Apply SIMD operations
    ApplySIMDOperations(TempGrid, DeltaTime);

    // Multi-threaded advection
    ParallelFor(AdaptiveGrid.Num(), [&](int32 Index)
        {
            FAdaptiveGridCell& Cell = AdaptiveGrid[Index];
            TArray<FVector> CellVelocity = { Cell.Velocity };
            TArray<FVector> TempVelocity = { TempGrid[Index] };
            Advect(CellVelocity, TempVelocity, TempVelocity, DeltaTime);
            Cell.Velocity = CellVelocity[0];
        });

    // Project step (not easily parallelizable due to its iterative nature)
    TArray<FVector> ProjectX, ProjectY, ProjectZ, P, Div;
    ProjectX.SetNum(AdaptiveGrid.Num());
    ProjectY.SetNum(AdaptiveGrid.Num());
    ProjectZ.SetNum(AdaptiveGrid.Num());
    P.SetNum(AdaptiveGrid.Num());
    Div.SetNum(AdaptiveGrid.Num());

    for (int32 i = 0; i < AdaptiveGrid.Num(); ++i)
    {
        ProjectX[i] = FVector(AdaptiveGrid[i].Velocity.X, 0, 0);
        ProjectY[i] = FVector(0, AdaptiveGrid[i].Velocity.Y, 0);
        ProjectZ[i] = FVector(0, 0, AdaptiveGrid[i].Velocity.Z);
    }

    Project(ProjectX, ProjectY, ProjectZ, P, Div);

    for (int32 i = 0; i < AdaptiveGrid.Num(); ++i)
    {
        AdaptiveGrid[i].Velocity = FVector(ProjectX[i].X, ProjectY[i].Y, ProjectZ[i].Z);
    }
}

void UWindSimulationComponent::UpdateAdaptiveGrid()
{
    for (FAdaptiveGridCell& Cell : AdaptiveGrid)
    {
        if (Cell.Velocity.Size() > Cell.SubdivisionThreshold)
        {
            Subdivide(Cell);
        }
        else if (Cell.Velocity.Size() < Cell.MergeThreshold && Cell.Children.Num() > 0)
        {
            Merge(Cell);
        }
    }
}

void UWindSimulationComponent::Subdivide(FAdaptiveGridCell& Cell)
{
    if (Cell.Children.Num() == 0)
    {
        for (int32 i = 0; i < 8; ++i)
        {
            Cell.Children.Add(new FAdaptiveGridCell());
        }
    }
}

void UWindSimulationComponent::Merge(FAdaptiveGridCell& ParentCell)
{
    // Average child velocities
    FVector AverageVelocity = FVector::ZeroVector;
    for (const FAdaptiveGridCell* Child : ParentCell.Children)
    {
        AverageVelocity += Child->Velocity;
    }
    ParentCell.Velocity = AverageVelocity / ParentCell.Children.Num();

    // Clear children
    for (FAdaptiveGridCell* Child : ParentCell.Children)
    {
        delete Child;
    }
    ParentCell.Children.Empty();
}

void UWindSimulationComponent::Diffuse(TArray<FVector>& Dst, const TArray<FVector>& Src, float Diff, float Dt)
{
    // Simplified diffusion for demonstration
    for (int32 i = 0; i < Src.Num(); ++i)
    {
        Dst[i] = Src[i] + Diff * Dt * (Src[i] - Src[i]);
    }
}

void UWindSimulationComponent::Project(TArray<FVector>& VelocityX, TArray<FVector>& VelocityY, TArray<FVector>& VelocityZ, TArray<FVector>& P, TArray<FVector>& Div)
{
    float h = CellSize;
    float h2 = h * h;

    // Calculate divergence
    for (int32 k = 1; k < BaseGridSize - 1; k++)
    {
        for (int32 j = 1; j < BaseGridSize - 1; j++)
        {
            for (int32 i = 1; i < BaseGridSize - 1; i++)
            {
                int32 Index = IX(i, j, k);
                Div[Index].X = -0.5f * h * (
                    VelocityX[IX(i + 1, j, k)].X - VelocityX[IX(i - 1, j, k)].X +
                    VelocityY[IX(i, j + 1, k)].Y - VelocityY[IX(i, j - 1, k)].Y +
                    VelocityZ[IX(i, j, k + 1)].Z - VelocityZ[IX(i, j, k - 1)].Z
                    );
                P[Index] = FVector::ZeroVector;
            }
        }
    }

    SetBoundary(Div);
    SetBoundary(P);

    // Solve Poisson equation
    for (int32 Iteration = 0; Iteration < 20; Iteration++)
    {
        for (int32 k = 1; k < BaseGridSize - 1; k++)
        {
            for (int32 j = 1; j < BaseGridSize - 1; j++)
            {
                for (int32 i = 1; i < BaseGridSize - 1; i++)
                {
                    int32 Index = IX(i, j, k);
                    P[Index].X = (Div[Index].X +
                        P[IX(i - 1, j, k)].X + P[IX(i + 1, j, k)].X +
                        P[IX(i, j - 1, k)].X + P[IX(i, j + 1, k)].X +
                        P[IX(i, j, k - 1)].X + P[IX(i, j, k + 1)].X) / 6.0f;
                }
            }
        }
        SetBoundary(P);
    }

    // Subtract pressure gradient
    for (int32 k = 1; k < BaseGridSize - 1; k++)
    {
        for (int32 j = 1; j < BaseGridSize - 1; j++)
        {
            for (int32 i = 1; i < BaseGridSize - 1; i++)
            {
                int32 Index = IX(i, j, k);
                VelocityX[Index].X -= 0.5f * (P[IX(i + 1, j, k)].X - P[IX(i - 1, j, k)].X) / h;
                VelocityY[Index].Y -= 0.5f * (P[IX(i, j + 1, k)].X - P[IX(i, j - 1, k)].X) / h;
                VelocityZ[Index].Z -= 0.5f * (P[IX(i, j, k + 1)].X - P[IX(i, j, k - 1)].X) / h;
            }
        }
    }

    SetBoundary(VelocityX);
    SetBoundary(VelocityY);
    SetBoundary(VelocityZ);
}

// Helper function to handle boundaries
void UWindSimulationComponent::SetBoundary(TArray<FVector>& Field)
{
    for (int32 j = 1; j < BaseGridSize - 1; j++)
    {
        for (int32 i = 1; i < BaseGridSize - 1; i++)
        {
            // Set z-facing boundaries
            Field[IX(i, j, 0)] = Field[IX(i, j, 1)];
            Field[IX(i, j, BaseGridSize - 1)] = Field[IX(i, j, BaseGridSize - 2)];
        }
    }

    for (int32 k = 1; k < BaseGridSize - 1; k++)
    {
        for (int32 i = 1; i < BaseGridSize - 1; i++)
        {
            // Set y-facing boundaries
            Field[IX(i, 0, k)] = Field[IX(i, 1, k)];
            Field[IX(i, BaseGridSize - 1, k)] = Field[IX(i, BaseGridSize - 2, k)];
        }
    }

    for (int32 k = 1; k < BaseGridSize - 1; k++)
    {
        for (int32 j = 1; j < BaseGridSize - 1; j++)
        {
            // Set x-facing boundaries
            Field[IX(0, j, k)] = Field[IX(1, j, k)];
            Field[IX(BaseGridSize - 1, j, k)] = Field[IX(BaseGridSize - 2, j, k)];
        }
    }

    // Set corner values
    Field[IX(0, 0, 0)] = (Field[IX(1, 0, 0)] + Field[IX(0, 1, 0)] + Field[IX(0, 0, 1)]) / 3.0f;
    Field[IX(0, BaseGridSize - 1, 0)] = (Field[IX(1, BaseGridSize - 1, 0)] + Field[IX(0, BaseGridSize - 2, 0)] + Field[IX(0, BaseGridSize - 1, 1)]) / 3.0f;
    Field[IX(0, 0, BaseGridSize - 1)] = (Field[IX(1, 0, BaseGridSize - 1)] + Field[IX(0, 1, BaseGridSize - 1)] + Field[IX(0, 0, BaseGridSize - 2)]) / 3.0f;
    Field[IX(0, BaseGridSize - 1, BaseGridSize - 1)] = (Field[IX(1, BaseGridSize - 1, BaseGridSize - 1)] + Field[IX(0, BaseGridSize - 2, BaseGridSize - 1)] + Field[IX(0, BaseGridSize - 1, BaseGridSize - 2)]) / 3.0f;
    Field[IX(BaseGridSize - 1, 0, 0)] = (Field[IX(BaseGridSize - 2, 0, 0)] + Field[IX(BaseGridSize - 1, 1, 0)] + Field[IX(BaseGridSize - 1, 0, 1)]) / 3.0f;
    Field[IX(BaseGridSize - 1, BaseGridSize - 1, 0)] = (Field[IX(BaseGridSize - 2, BaseGridSize - 1, 0)] + Field[IX(BaseGridSize - 1, BaseGridSize - 2, 0)] + Field[IX(BaseGridSize - 1, BaseGridSize - 1, 1)]) / 3.0f;
    Field[IX(BaseGridSize - 1, 0, BaseGridSize - 1)] = (Field[IX(BaseGridSize - 2, 0, BaseGridSize - 1)] + Field[IX(BaseGridSize - 1, 1, BaseGridSize - 1)] + Field[IX(BaseGridSize - 1, 0, BaseGridSize - 2)]) / 3.0f;
    Field[IX(BaseGridSize - 1, BaseGridSize - 1, BaseGridSize - 1)] = (Field[IX(BaseGridSize - 2, BaseGridSize - 1, BaseGridSize - 1)] + Field[IX(BaseGridSize - 1, BaseGridSize - 2, BaseGridSize - 1)] + Field[IX(BaseGridSize - 1, BaseGridSize - 1, BaseGridSize - 2)]) / 3.0f;
}

// Helper function to calculate 3D array index
int32 UWindSimulationComponent::IX(int32 x, int32 y, int32 z) const
{
    return x + y * BaseGridSize + z * BaseGridSize * BaseGridSize;
}

void UWindSimulationComponent::Advect(TArray<FVector>& Dst, const TArray<FVector>& Src, const TArray<FVector>& Velocity, float Dt)
{
    // Simplified advection for demonstration
    for (int32 i = 0; i < Src.Num(); ++i)
    {
        Dst[i] = Src[i] + Velocity[i] * Dt;
    }
}

void UWindSimulationComponent::ApplySIMDOperations(TArray<FVector>& Vectors, float Scalar)
{
    const int32 VectorCount = Vectors.Num();
    const int32 NumChunks = VectorCount / 4;

    for (int32 ChunkIndex = 0; ChunkIndex < NumChunks; ChunkIndex++)
    {
        float* Data = reinterpret_cast<float*>(&Vectors[ChunkIndex * 4]);
        VectorRegister4Float VecChunk = VectorLoadAligned(Data);
        VectorRegister4Float ScalarReg = VectorSetFloat1(Scalar);
        VectorRegister4Float Result = VectorMultiply(VecChunk, ScalarReg);
        VectorStoreAligned(Result, Data);
    }

    // Handle remaining elements
    for (int32 i = NumChunks * 4; i < VectorCount; ++i)
    {
        Vectors[i] *= Scalar;
    }
}

FVector UWindSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    // Implement interpolation based on adaptive grid
    // This is a simplified version
    int32 Index = FMath::FloorToInt(Location.X / CellSize) +
        FMath::FloorToInt(Location.Y / CellSize) * BaseGridSize +
        FMath::FloorToInt(Location.Z / CellSize) * BaseGridSize * BaseGridSize;
    Index = FMath::Clamp(Index, 0, AdaptiveGrid.Num() - 1);
    return AdaptiveGrid[Index].Velocity;
}

void UWindSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    int32 Index = FMath::FloorToInt(Location.X / CellSize) +
        FMath::FloorToInt(Location.Y / CellSize) * BaseGridSize +
        FMath::FloorToInt(Location.Z / CellSize) * BaseGridSize * BaseGridSize;
    Index = FMath::Clamp(Index, 0, AdaptiveGrid.Num() - 1);
    AdaptiveGrid[Index].Velocity += WindVelocity;
}

// FWindSimulationWorker implementation
FWindSimulationWorker::FWindSimulationWorker(UWindSimulationComponent* InOwner)
    : Owner(InOwner), bShouldRun(true)
{
}

uint32 FWindSimulationWorker::Run()
{
    while (bShouldRun)
    {
        Owner->SimulationStep(1.0f / Owner->GetSimulationFrequency());
        FPlatformProcess::Sleep(1.0f / Owner->GetSimulationFrequency());
    }
    return 0;
}

void FWindSimulationWorker::Stop()
{
    bShouldRun = false;
}
