#include "WindSystemComponent.h"
#include "Async/ParallelFor.h"
#include "Math/UnrealMathSSE.h"
#include "WindSystemCommon.h"
#include "Misc/ScopeLock.h"

FWindGrid::FWindGrid(int32 Size, float InCellSize)
    : GridSize(Size), CellSize(InCellSize)
{
    Grid.SetNumZeroed(Size * Size * Size);
}

FORCEINLINE FVector FWindGrid::GetCell(int32 X, int32 Y, int32 Z) const
{
    return IsValidIndex(X, Y, Z) ? Grid[GetIndex(X, Y, Z)] : FVector::ZeroVector;
}

FORCEINLINE void FWindGrid::SetCell(int32 X, int32 Y, int32 Z, const FVector& Value)
{
    if (IsValidIndex(X, Y, Z))
    {
        Grid[GetIndex(X, Y, Z)] = Value;
    }
}

FORCEINLINE int32 FWindGrid::GetIndex(int32 X, int32 Y, int32 Z) const
{
    return X + Y * GridSize + Z * GridSize * GridSize;
}

FORCEINLINE bool FWindGrid::IsValidIndex(int32 X, int32 Y, int32 Z) const
{
    return X >= 0 && X < GridSize && Y >= 0 && Y < GridSize && Z >= 0 && Z < GridSize;
}

UWindSimulationComponent::UWindSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    GridSize = GetSettings()->GridSize; // Default value
    CellSize = GetSettings()->CellSize; // Default value
    Viscosity = GetSettings()->Viscosity;
    SimulationFrequency = GetSettings()->SimulationFrequency;
    bAutoActivate = true;
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

void UWindSimulationComponent::UpdateGridCenter(const FVector& NewCenter)
{
    PreviousGridCenter = GridCenter;
    GridCenter = NewCenter;
}

float UWindSimulationComponent::GetMaxAllowedWindVelocity() const
{
    return GetSettings()->MaxAllowedWindVelocity;
}

const UWindSystemSettings* UWindSimulationComponent::GetSettings() const
{
    return GetDefault<UWindSystemSettings>();
}

void UWindSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // Main thread work, if any
}

void UWindSimulationComponent::InitializeGrid()
{
    if (WindGrid)
    {
        WINDSYSTEM_LOG(Warning, TEXT("WindGrid is already initialized. Skipping initialization."));
        return;
    }

    WindGrid = MakeShared<FWindGrid>(GridSize, CellSize);
    TempGrid = MakeShared<FWindGrid>(GridSize, CellSize);

    // Initialize with zero values (already done in FWindGrid constructor)
    WINDSYSTEM_LOG(Log, TEXT("Wind Simulation Grid Initialized: %d cells"), GridSize * GridSize * GridSize);
}

void UWindSimulationComponent::InitializeForTesting()
{
    // Call the private InitializeGrid method
    InitializeGrid();

    // Perform any other initialization normally done in BeginPlay
    StartSimulation();

    // Add any additional initialization needed for testing
    // For example, you might want to set up some initial wind conditions
    WINDSYSTEM_LOG(Log, TEXT("Wind Simulation Component initialized for testing"));
}

void UWindSimulationComponent::SwapGrids()
{
    Swap(WindGrid, TempGrid);
}

void UWindSimulationComponent::SimulationStep(float DeltaTime)
{
    FScopeLock Lock(&SimulationLock);

    if (!IsGridInitialized())
    {
        WINDSYSTEM_LOG_WARNING(TEXT("WindGrid is not initialized"));
        return;
    }

    HandleGridMovement();
    // Use TempGrid for intermediate calculations
    Diffuse(TempGrid, WindGrid, Viscosity, DeltaTime);
    Project(TempGrid, MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()), MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()));
    Advect(WindGrid, TempGrid, TempGrid, DeltaTime);
    Project(WindGrid, MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()), MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize()));

    ApplySIMDOperations(WindGrid, DeltaTime);

    // BroadcastWindUpdates();
}

void UWindSimulationComponent::HandleGridMovement()
{
        // Handle grid movement
    FVector GridMovement = GridCenter - PreviousGridCenter;
    if (!GridMovement.IsNearlyZero())
    {
        FVector GridMovementCells = GridMovement / CellSize;
        int32 ShiftX = FMath::FloorToInt(GridMovementCells.X);
        int32 ShiftY = FMath::FloorToInt(GridMovementCells.Y);
        int32 ShiftZ = FMath::FloorToInt(GridMovementCells.Z);

        TSharedPtr<FWindGrid> NewGrid = MakeShared<FWindGrid>(WindGrid->GetSize(), WindGrid->GetCellSize());
        
        for (int32 x = 0; x < WindGrid->GetSize(); ++x)
        {
            for (int32 y = 0; y < WindGrid->GetSize(); ++y)
            {
                for (int32 z = 0; z < WindGrid->GetSize(); ++z)
                {
                    int32 OldX = x - ShiftX;
                    int32 OldY = y - ShiftY;
                    int32 OldZ = z - ShiftZ;

                    if (WindGrid->IsValidIndex(OldX, OldY, OldZ))
                    {
                        NewGrid->SetCell(x, y, z, WindGrid->GetCell(OldX, OldY, OldZ));
                    }
                    else
                    {
                        NewGrid->SetCell(x, y, z, FVector::ZeroVector);
                    }
                }
            }
        }

        WindGrid = NewGrid;
    }
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

void UWindSimulationComponent::ApplySIMDOperations(TSharedPtr<FWindGrid> Grid, float DeltaTime)
{
    if (!Grid)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("ApplySIMDOperations: Grid is null"));
        return;
    }

    int32 Size = Grid->GetSize();
    int32 TotalCells = Size * Size * Size;
    int32 VectorCount = TotalCells / 4; // Number of 4-wide vectors

    TArray<FVector>& GridData = Grid->GetGridData();
    
    // Convert DeltaTime to a vector for SIMD operations
    VectorRegister4Float DeltaTimeVec = VectorSetFloat1(DeltaTime);
    
    // Constants for clamping
    VectorRegister4Float MaxSpeedVec = VectorSetFloat1(1000.0f); // Maximum allowed wind speed
    VectorRegister4Float MinSpeedVec = VectorSetFloat1(-1000.0f); // Minimum allowed wind speed

    // Process 4 vectors at a time using SIMD
    for (int32 i = 0; i < VectorCount; i++)
    {
        // Load 4 FVector's (12 floats) into 3 SIMD vectors
        VectorRegister4Float* VectorPtr = reinterpret_cast<VectorRegister4Float*>(&GridData[i * 4]);
        VectorRegister4Float Vx = VectorPtr[0];
        VectorRegister4Float Vy = VectorPtr[1];
        VectorRegister4Float Vz = VectorPtr[2];

        // Apply decay (simulate drag)
        const VectorRegister4Float DecayFactor = VectorSetFloat1(0.99f);
        Vx = VectorMultiply(Vx, DecayFactor);
        Vy = VectorMultiply(Vy, DecayFactor);
        Vz = VectorMultiply(Vz, DecayFactor);

        // Apply some force (e.g., global wind)
        const VectorRegister4Float GlobalWindForce = VectorSetFloat1(0.1f);
        Vx = VectorAdd(Vx, VectorMultiply(GlobalWindForce, DeltaTimeVec));

        // Clamp velocities to prevent extreme values
        Vx = VectorMin(VectorMax(Vx, MinSpeedVec), MaxSpeedVec);
        Vy = VectorMin(VectorMax(Vy, MinSpeedVec), MaxSpeedVec);
        Vz = VectorMin(VectorMax(Vz, MinSpeedVec), MaxSpeedVec);

        // Store the results back
        VectorPtr[0] = Vx;
        VectorPtr[1] = Vy;
        VectorPtr[2] = Vz;
    }

    // Handle remaining elements (if any)
    for (int32 i = VectorCount * 4; i < TotalCells; i++)
    {
        FVector& WindVelocity = GridData[i];
        
        // Apply decay
        WindVelocity *= 0.99f;
        
        // Apply global wind force
        WindVelocity.X += 0.1f * DeltaTime;
        
        // Clamp velocity
        WindVelocity.X = FMath::Clamp(WindVelocity.X, -1000.0f, 1000.0f);
        WindVelocity.Y = FMath::Clamp(WindVelocity.Y, -1000.0f, 1000.0f);
        WindVelocity.Z = FMath::Clamp(WindVelocity.Z, -1000.0f, 1000.0f);
    }
}

FVector UWindSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FScopeLock Lock(&SimulationLock);

    if (!IsGridInitialized())
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindGrid is not initialized"));
        return FVector::ZeroVector;
    }

    FVector LocalPos = Location - GridCenter;
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
      /*  if (CurrentVelocity.ContainsNaN()) {
            CurrentVelocity = FVector::ZeroVector;
        }*/
        FVector NewVelocity = CurrentVelocity + WindVelocity;
        
        // Clamp the new velocity to prevent extreme values
        if (NewVelocity.SizeSquared() > FMath::Square(GetMaxAllowedWindVelocity()))
        {
            NewVelocity = NewVelocity.GetSafeNormal() * GetMaxAllowedWindVelocity();
            //WINDSYSTEM_LOG_WARNING(TEXT("Wind velocity clamped at location: %s"), *Location.ToString());
        }

        WindGrid->SetCell(X, Y, Z, NewVelocity);

        WINDSYSTEM_LOG_VERBOSE(TEXT("Wind added at location: Pos=%s, NewVelocity=%s"), 
            *Location.ToString(), *NewVelocity.ToString());
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