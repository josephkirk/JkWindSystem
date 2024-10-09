// WindSystemComponent.cpp
#include "WindSystemComponent.h"
#include "Async/ParallelFor.h"
#include "Math/UnrealMathSSE.h"
#include "WindSystemLog.h"

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
    // Stop the simulation worker
    if (SimulationWorker)
    {
        SimulationWorker->Stop();
        
        if (SimulationThread)
        {
            SimulationThread->WaitForCompletion();
            delete SimulationThread;
            SimulationThread = nullptr;
        }
        
        delete SimulationWorker;
        SimulationWorker = nullptr;
    }

    // Clear timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }

    // Unregister from any subsystems
    // if (UWindSystemSubsystem* WindSystem = GetWorld()->GetSubsystem<UWindSystemSubsystem>())
    // {
    //     WindSystem->UnregisterWindComponent(this);
    // }

    // Unbind delegates
    OnWindCellUpdated.Clear();

    // Clear the grid data
    AdaptiveGrid.Empty();

    // Clear references to other objects
    // WindSimComponent = nullptr;

    // Call the base class EndPlay
    Super::EndPlay(EndPlayReason);
}

void UWindSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // Main thread work, if any
}

void UWindSimulationComponent::InitializeGrid()
{
    int32 TotalCells = BaseGridSize * BaseGridSize * BaseGridSize;
    AdaptiveGrid.SetNum(TotalCells);

    for (int32 i = 0; i < TotalCells; ++i)
    {
        AdaptiveGrid[i].Velocity = FVector::ZeroVector;
    }

    WINDSYSTEM_LOG(Log, TEXT("Wind Simulation Grid Initialized: %d cells"), TotalCells);
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

FVector UWindSimulationComponent::GetWindVelocityAtLocation(const FVector& WorldLocation) const
{
    int32 X, Y, Z;
    GetGridCell(WorldLocation, X, Y, Z);

    // Ensure we're within the grid bounds
    if (X < 0 || X >= BaseGridSize || Y < 0 || Y >= BaseGridSize || Z < 0 || Z >= BaseGridSize)
    {
        WINDSYSTEM_LOG_WARNING(TEXT("GetWindVelocityAtLocation: Location out of grid bounds: %s"), *WorldLocation.ToString());
        return FVector::ZeroVector;
    }

    int32 Index = IX(X, Y, Z);
    if (Index < 0 || Index >= AdaptiveGrid.Num())
    {
        WINDSYSTEM_LOG_ERROR(TEXT("GetWindVelocityAtLocation: Invalid grid index %d for location %s"), Index, *WorldLocation.ToString());
        return FVector::ZeroVector;
    }

    // Simple trilinear interpolation for smoother results
    FVector LocalPos = GetComponentTransform().InverseTransformPosition(WorldLocation);
    FVector CellPos(
        (LocalPos.X / CellSize) - X,
        (LocalPos.Y / CellSize) - Y,
        (LocalPos.Z / CellSize) - Z
    );

    FVector InterpolatedVelocity = FVector::ZeroVector;
    for (int32 dz = 0; dz <= 1; ++dz)
    {
        for (int32 dy = 0; dy <= 1; ++dy)
        {
            for (int32 dx = 0; dx <= 1; ++dx)
            {
                int32 NX = FMath::Clamp(X + dx, 0, BaseGridSize - 1);
                int32 NY = FMath::Clamp(Y + dy, 0, BaseGridSize - 1);
                int32 NZ = FMath::Clamp(Z + dz, 0, BaseGridSize - 1);

                int32 NeighborIndex = IX(NX, NY, NZ);
                if (NeighborIndex >= 0 && NeighborIndex < AdaptiveGrid.Num())
                {
                    float Weight = (dx == 0 ? 1 - CellPos.X : CellPos.X) *
                        (dy == 0 ? 1 - CellPos.Y : CellPos.Y) *
                        (dz == 0 ? 1 - CellPos.Z : CellPos.Z);

                    InterpolatedVelocity += AdaptiveGrid[NeighborIndex].Velocity * Weight;
                }
            }
        }
    }

    return InterpolatedVelocity;
}

void UWindSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    int32 X, Y, Z;
    GetGridCell(Location, X, Y, Z);

    // Add the wind velocity to the current cell
    int32 Index = IX(X, Y, Z);
    if (Index >= 0 && Index < AdaptiveGrid.Num())
    {
        AdaptiveGrid[Index].Velocity += WindVelocity;

        // Optionally, distribute some of the wind to neighboring cells for smoother effect
        float DistributionFactor = 0.1f;
        for (int32 dz = -1; dz <= 1; dz++)
        {
            for (int32 dy = -1; dy <= 1; dy++)
            {
                for (int32 dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0 && dz == 0) continue;

                    int32 NX = FMath::Clamp(X + dx, 0, BaseGridSize - 1);
                    int32 NY = FMath::Clamp(Y + dy, 0, BaseGridSize - 1);
                    int32 NZ = FMath::Clamp(Z + dz, 0, BaseGridSize - 1);

                    int32 NeighborIndex = IX(NX, NY, NZ);
                    if (NeighborIndex >= 0 && NeighborIndex < AdaptiveGrid.Num())
                    {
                        AdaptiveGrid[NeighborIndex].Velocity += WindVelocity * DistributionFactor;
                    }
                }
            }
        }

        // Trigger the OnWindCellUpdated event
        OnWindCellUpdated.Broadcast(Location, AdaptiveGrid[Index].Velocity, CellSize);
    }
}

// FWindSimulationWorker implementation
FWindSimulationWorker::FWindSimulationWorker(UWindSimulationComponent* InOwner)
    : Owner(InOwner), bShouldRun(true)
{
}

void UWindSimulationComponent::GetGridCell(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ) const
{
    FVector LocalPos = GetComponentTransform().InverseTransformPosition(WorldLocation);

    OutX = FMath::FloorToInt(LocalPos.X / CellSize);
    OutY = FMath::FloorToInt(LocalPos.Y / CellSize);
    OutZ = FMath::FloorToInt(LocalPos.Z / CellSize);

    // Log if we're outside the grid bounds
    if (OutX < 0 || OutX >= BaseGridSize || OutY < 0 || OutY >= BaseGridSize || OutZ < 0 || OutZ >= BaseGridSize)
    {
        WINDSYSTEM_LOG_WARNING(TEXT("GetGridCell: Location out of grid bounds: World(%s), Local(%s), Grid(%d, %d, %d)"),
            *WorldLocation.ToString(), *LocalPos.ToString(), OutX, OutY, OutZ);
    }
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
