#include "WindSystemComponent.h"
#include "Async/ParallelFor.h"
#include "Math/UnrealMathSSE.h"
#include "WindSystemLog.h"

FWindGrid::FWindGrid(int32 Size, float InCellSize) : GridSize(Size), CellSize(InCellSize)
{
    Grid.SetNum(Size * Size * Size);
}

FVector& FWindGrid::GetCell(int32 X, int32 Y, int32 Z)
{
    return Grid[GetIndex(X, Y, Z)];
}

const FVector& FWindGrid::GetCell(int32 X, int32 Y, int32 Z) const
{
    return Grid[GetIndex(X, Y, Z)];
}

void FWindGrid::SetCell(int32 X, int32 Y, int32 Z, const FVector& Value)
{
    Grid[GetIndex(X, Y, Z)] = Value;
}

int32 FWindGrid::GetIndex(int32 X, int32 Y, int32 Z) const
{
    X = FMath::Clamp(X, 0, GridSize - 1);
    Y = FMath::Clamp(Y, 0, GridSize - 1);
    Z = FMath::Clamp(Z, 0, GridSize - 1);
    return X + Y * GridSize + Z * GridSize * GridSize;
}

UWindSimulationComponent::UWindSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
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
        
        if (SimulationThread)
        {
            SimulationThread->WaitForCompletion();
            delete SimulationThread;
            SimulationThread = nullptr;
        }
        
        delete SimulationWorker;
        SimulationWorker = nullptr;
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
    const int32 BaseGridSize = 32;
    const float CellSize = 100.0f;
    WindGrid = MakeShared<FWindGrid>(BaseGridSize, CellSize);

    WINDSYSTEM_LOG(Log, TEXT("Wind Simulation Grid Initialized: %d cells"), BaseGridSize * BaseGridSize * BaseGridSize);
}

void UWindSimulationComponent::SimulationStep(float DeltaTime)
{
    TSharedPtr<FWindGrid> TempGrid = MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize());

    Diffuse(TempGrid, WindGrid, Viscosity, DeltaTime);
    Project(TempGrid, MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()), MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()));
    Advect(WindGrid, TempGrid, TempGrid, DeltaTime);
    Project(WindGrid, MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()), MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()));

    ApplySIMDOperations(WindGrid, DeltaTime);
}

void UWindSimulationComponent::Diffuse(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, float Diff, float Dt)
{
    float a = Dt * Diff * (Src->GetSize() - 2) * (Src->GetSize() - 2);
    int32 Size = Src->GetSize();

    ParallelFor(Size, [&](int32 K)
        {
            for (int32 J = 1; J < Size - 1; J++)
            {
                for (int32 I = 1; I < Size - 1; I++)
                {
                    FVector NewValue = (Src->GetCell(I, J, K) +
                        a * (Src->GetCell(I - 1, J, K) + Src->GetCell(I + 1, J, K) +
                            Src->GetCell(I, J - 1, K) + Src->GetCell(I, J + 1, K) +
                            Src->GetCell(I, J, K - 1) + Src->GetCell(I, J, K + 1))) / (1 + 6 * a);
                    Dst->SetCell(I, J, K, NewValue);
                }
            }
        });

    SetBoundary(Dst);
}

void UWindSimulationComponent::Project(TSharedPtr<FWindGrid> Velocity, TSharedPtr<FWindGrid> P, TSharedPtr<FWindGrid> Div)
{
    int32 Size = Velocity->GetSize();
    double H = 1.0 / (Size - 2);

    ParallelFor(Size, [&](int32 K)
        {
            for (int32 J = 1; J < Size - 1; J++)
            {
                for (int32 I = 1; I < Size - 1; I++)
                {
                    double DivValue = -0.5 * H * (
                        Velocity->GetCell(I + 1, J, K).X - Velocity->GetCell(I - 1, J, K).X +
                        Velocity->GetCell(I, J + 1, K).Y - Velocity->GetCell(I, J - 1, K).Y +
                        Velocity->GetCell(I, J, K + 1).Z - Velocity->GetCell(I, J, K - 1).Z
                        );
                    Div->SetCell(I, J, K, FVector(DivValue));
                    P->SetCell(I, J, K, FVector::ZeroVector);
                }
            }
        });

    SetBoundary(Div);
    SetBoundary(P);

    for (int32 K = 0; K < 20; K++)
    {
        ParallelFor(Size, [&](int32 K)
            {
                for (int32 J = 1; J < Size - 1; J++)
                {
                    for (int32 I = 1; I < Size - 1; I++)
                    {
                        double PValue = (Div->GetCell(I, J, K).X +
                            P->GetCell(I - 1, J, K).X + P->GetCell(I + 1, J, K).X +
                            P->GetCell(I, J - 1, K).X + P->GetCell(I, J + 1, K).X +
                            P->GetCell(I, J, K - 1).X + P->GetCell(I, J, K + 1).X) / 6.0;
                        P->SetCell(I, J, K, FVector(PValue));
                    }
                }
            });
        SetBoundary(P);
    }

    ParallelFor(Size, [&](int32 K)
        {
            for (int32 J = 1; J < Size - 1; J++)
            {
                for (int32 I = 1; I < Size - 1; I++)
                {
                    FVector Vel = Velocity->GetCell(I, J, K);
                    Vel.X -= 0.5 * (P->GetCell(I + 1, J, K).X - P->GetCell(I - 1, J, K).X) / H;
                    Vel.Y -= 0.5 * (P->GetCell(I, J + 1, K).X - P->GetCell(I, J - 1, K).X) / H;
                    Vel.Z -= 0.5 * (P->GetCell(I, J, K + 1).X - P->GetCell(I, J, K - 1).X) / H;
                    Velocity->SetCell(I, J, K, Vel);
                }
            }
        });

    SetBoundary(Velocity);
}

void UWindSimulationComponent::SetBoundary(TSharedPtr<FWindGrid> Field)
{
    int32 Size = Field->GetSize();

    ParallelFor(Size, [&](int32 I)
    {
        for (int32 J = 1; J < Size - 1; J++)
        {
            Field->SetCell(I, J, 0, Field->GetCell(I, J, 1));
            Field->SetCell(I, J, Size - 1, Field->GetCell(I, J, Size - 2));
        }
    });

    ParallelFor(Size, [&](int32 K)
    {
        for (int32 I = 1; I < Size - 1; I++)
        {
            Field->SetCell(I, 0, K, Field->GetCell(I, 1, K));
            Field->SetCell(I, Size - 1, K, Field->GetCell(I, Size - 2, K));
        }
    });

    ParallelFor(Size, [&](int32 J)
    {
        for (int32 K = 1; K < Size - 1; K++)
        {
            Field->SetCell(0, J, K, Field->GetCell(1, J, K));
            Field->SetCell(Size - 1, J, K, Field->GetCell(Size - 2, J, K));
        }
    });

    // Set corner values
    Field->SetCell(0, 0, 0, (Field->GetCell(1, 0, 0) + Field->GetCell(0, 1, 0) + Field->GetCell(0, 0, 1)) / 3.0f);
    Field->SetCell(0, Size - 1, 0, (Field->GetCell(1, Size - 1, 0) + Field->GetCell(0, Size - 2, 0) + Field->GetCell(0, Size - 1, 1)) / 3.0f);
    Field->SetCell(0, 0, Size - 1, (Field->GetCell(1, 0, Size - 1) + Field->GetCell(0, 1, Size - 1) + Field->GetCell(0, 0, Size - 2)) / 3.0f);
    Field->SetCell(0, Size - 1, Size - 1, (Field->GetCell(1, Size - 1, Size - 1) + Field->GetCell(0, Size - 2, Size - 1) + Field->GetCell(0, Size - 1, Size - 2)) / 3.0f);
    Field->SetCell(Size - 1, 0, 0, (Field->GetCell(Size - 2, 0, 0) + Field->GetCell(Size - 1, 1, 0) + Field->GetCell(Size - 1, 0, 1)) / 3.0f);
    Field->SetCell(Size - 1, Size - 1, 0, (Field->GetCell(Size - 2, Size - 1, 0) + Field->GetCell(Size - 1, Size - 2, 0) + Field->GetCell(Size - 1, Size - 1, 1)) / 3.0f);
    Field->SetCell(Size - 1, 0, Size - 1, (Field->GetCell(Size - 2, 0, Size - 1) + Field->GetCell(Size - 1, 1, Size - 1) + Field->GetCell(Size - 1, 0, Size - 2)) / 3.0f);
    Field->SetCell(Size - 1, Size - 1, Size - 1, (Field->GetCell(Size - 2, Size - 1, Size - 1) + Field->GetCell(Size - 1, Size - 2, Size - 1) + Field->GetCell(Size - 1, Size - 1, Size - 2)) / 3.0f);
}

void UWindSimulationComponent::Advect(TSharedPtr<FWindGrid> Dst, const TSharedPtr<FWindGrid> Src, const TSharedPtr<FWindGrid> Velocity, float Dt)
{
    int32 Size = Src->GetSize();
    float Dt0 = Dt * (Size - 2);

    ParallelFor(Size, [&](int32 K)
    {
        for (int32 J = 1; J < Size - 1; J++)
        {
            for (int32 I = 1; I < Size - 1; I++)
            {
                FVector Pos = FVector(I, J, K) - Dt0 * Velocity->GetCell(I, J, K);
                
                Pos.X = FMath::Clamp(Pos.X, 0.5f, Size - 1.5f);
                int32 I0 = FMath::FloorToInt(Pos.X);
                int32 I1 = I0 + 1;
                
                Pos.Y = FMath::Clamp(Pos.Y, 0.5f, Size - 1.5f);
                int32 J0 = FMath::FloorToInt(Pos.Y);
                int32 J1 = J0 + 1;
                
                Pos.Z = FMath::Clamp(Pos.Z, 0.5f, Size - 1.5f);
                int32 K0 = FMath::FloorToInt(Pos.Z);
                int32 K1 = K0 + 1;

                float S1 = Pos.X - I0;
                float S0 = 1 - S1;
                float T1 = Pos.Y - J0;
                float T0 = 1 - T1;
                float U1 = Pos.Z - K0;
                float U0 = 1 - U1;

                Dst->SetCell(I, J, K,
                    S0 * (T0 * (U0 * Src->GetCell(I0, J0, K0) + U1 * Src->GetCell(I0, J0, K1)) +
                          T1 * (U0 * Src->GetCell(I0, J1, K0) + U1 * Src->GetCell(I0, J1, K1))) +
                    S1 * (T0 * (U0 * Src->GetCell(I1, J0, K0) + U1 * Src->GetCell(I1, J0, K1)) +
                          T1 * (U0 * Src->GetCell(I1, J1, K0) + U1 * Src->GetCell(I1, J1, K1)))
                );
            }
        }
    });

    SetBoundary(Dst);
}

void UWindSimulationComponent::ApplySIMDOperations(TSharedPtr<FWindGrid> Grid, float Scalar)
{
    int32 Size = Grid->GetSize();
    int32 TotalCells = Size * Size * Size;
    int32 VectorCount = TotalCells / 4;

    TArray<FVector>& GridData = Grid->GetGridData();

    // Use SIMD operations for blocks of 4 vectors
    for (int32 i = 0; i < VectorCount; i++)
    {
        VectorRegister4Float* VectorPtr = reinterpret_cast<VectorRegister4Float*>(&GridData[i * 4]);
        VectorRegister4Float ScalarVec = VectorSetFloat1(Scalar);
        *VectorPtr = VectorMultiply(*VectorPtr, ScalarVec);
    }

    // Handle remaining elements
    for (int32 i = VectorCount * 4; i < TotalCells; i++)
    {
        GridData[i] *= Scalar;
    }
}

FVector UWindSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    if (!WindGrid)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return FVector::ZeroVector;
    }

    FVector LocalPos = GetComponentTransform().InverseTransformPosition(Location);
    FVector GridPos = LocalPos / WindGrid->GetCellSize();

    return InterpolateVelocity(GridPos);
}

FVector UWindSimulationComponent::InterpolateVelocity(const FVector& Position) const
{
    int32 Size = WindGrid->GetSize();
    int32 X0 = FMath::FloorToInt(Position.X);
    int32 Y0 = FMath::FloorToInt(Position.Y);
    int32 Z0 = FMath::FloorToInt(Position.Z);
    int32 X1 = FMath::Min(X0 + 1, Size - 1);
    int32 Y1 = FMath::Min(Y0 + 1, Size - 1);
    int32 Z1 = FMath::Min(Z0 + 1, Size - 1);

    float Sx = Position.X - X0;
    float Sy = Position.Y - Y0;
    float Sz = Position.Z - Z0;

    FVector C000 = WindGrid->GetCell(X0, Y0, Z0);
    FVector C100 = WindGrid->GetCell(X1, Y0, Z0);
    FVector C010 = WindGrid->GetCell(X0, Y1, Z0);
    FVector C110 = WindGrid->GetCell(X1, Y1, Z0);
    FVector C001 = WindGrid->GetCell(X0, Y0, Z1);
    FVector C101 = WindGrid->GetCell(X1, Y0, Z1);
    FVector C011 = WindGrid->GetCell(X0, Y1, Z1);
    FVector C111 = WindGrid->GetCell(X1, Y1, Z1);

    return FMath::Lerp(
        FMath::Lerp(
            FMath::Lerp(C000, C100, Sx),
            FMath::Lerp(C010, C110, Sx),
            Sy),
        FMath::Lerp(
            FMath::Lerp(C001, C101, Sx),
            FMath::Lerp(C011, C111, Sx),
            Sy),
        Sz
    );
}

void UWindSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    if (!WindGrid)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return;
    }

    FVector LocalPos = GetComponentTransform().InverseTransformPosition(Location);
    FVector GridPos = LocalPos / WindGrid->GetCellSize();

    int32 X = FMath::FloorToInt(GridPos.X);
    int32 Y = FMath::FloorToInt(GridPos.Y);
    int32 Z = FMath::FloorToInt(GridPos.Z);

    int32 Size = WindGrid->GetSize();
    if (X >= 0 && X < Size && Y >= 0 && Y < Size && Z >= 0 && Z < Size)
    {
        FVector CurrentVelocity = WindGrid->GetCell(X, Y, Z);
        FVector NewVelocity = CurrentVelocity + WindVelocity;
        WindGrid->SetCell(X, Y, Z, NewVelocity);

        // Notify listeners about the updated cell
        OnWindCellUpdated.Broadcast(Location, NewVelocity, WindGrid->GetCellSize());
    }
    else
    {
        WINDSYSTEM_LOG_WARNING(TEXT("Attempted to add wind outside the grid bounds"));
    }
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