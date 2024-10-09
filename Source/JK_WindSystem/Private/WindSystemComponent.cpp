#include "WindSystemComponent.h"
#include "DrawDebugHelpers.h"
//#include "WindSubsystem.h"

UWindSimulationComponent::UWindSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame
    bShowDebugVisualization = false;
}

void UWindSimulationComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeGrid();

}

void UWindSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    SimulationTimer += DeltaTime;
    if (SimulationTimer >= 1.0f / SimulationFrequency)
    {
        StepSimulation(SimulationTimer);
        SimulationTimer = 0.0f;
    }
}

void UWindSimulationComponent::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    int32 X, Y, Z;
    GetGridCell(Location, X, Y, Z);

    // Add the wind velocity to the current cell
    VelocityGrid[IX(X, Y, Z)] += WindVelocity;

    // Optionally, distribute some of the wind to neighboring cells for smoother effect
    float DistributionFactor = 0.1f;
    for (int32 dz = -1; dz <= 1; dz++)
    {
        for (int32 dy = -1; dy <= 1; dy++)
        {
            for (int32 dx = -1; dx <= 1; dx++)
            {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                int32 NX = FMath::Clamp(X + dx, 0, GridSizeX - 1);
                int32 NY = FMath::Clamp(Y + dy, 0, GridSizeY - 1);
                int32 NZ = FMath::Clamp(Z + dz, 0, GridSizeZ - 1);
                
                VelocityGrid[IX(NX, NY, NZ)] += WindVelocity * DistributionFactor;
            }
        }
    }
}

void UWindSimulationComponent::GetGridCell(const FVector& Location, int32& OutX, int32& OutY, int32& OutZ) const
{
    FVector LocalPos = GetOwner()->GetActorTransform().InverseTransformPosition(Location);
    OutX = FMath::Clamp(FMath::FloorToInt(LocalPos.X / CellSize), 0, GridSizeX - 1);
    OutY = FMath::Clamp(FMath::FloorToInt(LocalPos.Y / CellSize), 0, GridSizeY - 1);
    OutZ = FMath::Clamp(FMath::FloorToInt(LocalPos.Z / CellSize), 0, GridSizeZ - 1);
}

void UWindSimulationComponent::NotifyCellUpdated(int32 X, int32 Y, int32 Z)
{
    if (!bShowDebugVisualization || !OnWindCellUpdated.IsBound())
    {
        return;
    }

    FVector CellCenter = FVector(X + 0.5f, Y + 0.5f, Z + 0.5f) * CellSize;
    FVector WorldLocation = GetOwner()->GetActorTransform().TransformPosition(CellCenter);
    FVector WindVelocity = VelocityGrid[IX(X, Y, Z)];

    OnWindCellUpdated.Broadcast(WorldLocation, WindVelocity, CellSize);
}

void UWindSimulationComponent::InitializeGrid()
{
    int32 TotalCells = GridSizeX * GridSizeY * GridSizeZ;
    VelocityGrid.Init(FVector::ZeroVector, TotalCells);
    PreviousVelocityGrid.Init(FVector::ZeroVector, TotalCells);
}

void UWindSimulationComponent::StepSimulation(float DeltaTime)
{
    AddSources();
    Diffuse(PreviousVelocityGrid, VelocityGrid, Viscosity, DeltaTime);
    Project(PreviousVelocityGrid, PreviousVelocityGrid, PreviousVelocityGrid, VelocityGrid, VelocityGrid);
    Advect(VelocityGrid, PreviousVelocityGrid, PreviousVelocityGrid, DeltaTime);
    Project(VelocityGrid, VelocityGrid, VelocityGrid, PreviousVelocityGrid, PreviousVelocityGrid);
}

void UWindSimulationComponent::AddSources()
{
    // Add wind sources here
    // For example, add a constant wind in the X direction
    // for (int32 Z = 0; Z < GridSizeZ; ++Z)
    // {
    //     for (int32 Y = 0; Y < GridSizeY; ++Y)
    //     {
    //         VelocityGrid[IX(0, Y, Z)] += FVector(10.0f, 0.0f, 0.0f);
    //     }
    // }
}

void UWindSimulationComponent::Diffuse(TArray<FVector>& Dst, const TArray<FVector>& Src, float Diff, float Dt)
{
    float A = Dt * Diff * GridSizeX * GridSizeY * GridSizeZ;
    for (int32 K = 0; K < 20; ++K) // Gauss-Seidel relaxation
    {
        for (int32 Z = 1; Z < GridSizeZ - 1; ++Z)
        {
            for (int32 Y = 1; Y < GridSizeY - 1; ++Y)
            {
                for (int32 X = 1; X < GridSizeX - 1; ++X)
                {
                    Dst[IX(X, Y, Z)] = (Src[IX(X, Y, Z)] +
                        A * (Dst[IX(X - 1, Y, Z)] + Dst[IX(X + 1, Y, Z)] +
                            Dst[IX(X, Y - 1, Z)] + Dst[IX(X, Y + 1, Z)] +
                            Dst[IX(X, Y, Z - 1)] + Dst[IX(X, Y, Z + 1)])) / (1 + 6 * A);
                }
            }
        }
    }
}

void UWindSimulationComponent::Project(TArray<FVector>& VelocityX, TArray<FVector>& VelocityY, TArray<FVector>& VelocityZ, TArray<FVector>& P, TArray<FVector>& Div)
{
    // Implementation of the Project step
    // This is a simplified version and may need to be expanded for better accuracy
    for (int32 Z = 1; Z < GridSizeZ - 1; ++Z)
    {
        for (int32 Y = 1; Y < GridSizeY - 1; ++Y)
        {
            for (int32 X = 1; X < GridSizeX - 1; ++X)
            {
                Div[IX(X, Y, Z)] = FVector(-0.5f * (
                    VelocityX[IX(X + 1, Y, Z)].X - VelocityX[IX(X - 1, Y, Z)].X +
                    VelocityY[IX(X, Y + 1, Z)].Y - VelocityY[IX(X, Y - 1, Z)].Y +
                    VelocityZ[IX(X, Y, Z + 1)].Z - VelocityZ[IX(X, Y, Z - 1)].Z) / GridSizeX);
                P[IX(X, Y, Z)] = FVector(0,0,0);
            }
        }
    }

    // Gauss-Seidel relaxation
    for (int32 K = 0; K < 20; ++K)
    {
        for (int32 Z = 1; Z < GridSizeZ - 1; ++Z)
        {
            for (int32 Y = 1; Y < GridSizeY - 1; ++Y)
            {
                for (int32 X = 1; X < GridSizeX - 1; ++X)
                {
                    P[IX(X, Y, Z)] = (Div[IX(X, Y, Z)] +
                        P[IX(X - 1, Y, Z)] + P[IX(X + 1, Y, Z)] +
                        P[IX(X, Y - 1, Z)] + P[IX(X, Y + 1, Z)] +
                        P[IX(X, Y, Z - 1)] + P[IX(X, Y, Z + 1)]) / 6.0f;
                }
            }
        }
    }

    for (int32 Z = 1; Z < GridSizeZ - 1; ++Z)
    {
        for (int32 Y = 1; Y < GridSizeY - 1; ++Y)
        {
            for (int32 X = 1; X < GridSizeX - 1; ++X)
            {
                VelocityX[IX(X, Y, Z)] -= FVector(0.5f * (P[IX(X + 1, Y, Z)].X - P[IX(X - 1, Y, Z)].X) * GridSizeX, 0, 0);
                VelocityY[IX(X, Y, Z)] -= FVector(0, 0.5f * (P[IX(X, Y + 1, Z)].Y - P[IX(X, Y - 1, Z)].Y) * GridSizeY, 0);
                VelocityZ[IX(X, Y, Z)] -= FVector(0, 0, 0.5f * (P[IX(X, Y, Z + 1)].Z - P[IX(X, Y, Z - 1)].Z) * GridSizeZ);
                NotifyCellUpdated(X, Y, Z);
            }
        }
    }
}

void UWindSimulationComponent::Advect(TArray<FVector>& Dst, const TArray<FVector>& Src, const TArray<FVector>& Velocity, float Dt)
{
    float Dt0 = Dt * GridSizeX;
    for (int32 Z = 1; Z < GridSizeZ - 1; ++Z)
    {
        for (int32 Y = 1; Y < GridSizeY - 1; ++Y)
        {
            for (int32 X = 1; X < GridSizeX - 1; ++X)
            {
                float X0 = X - Dt0 * Velocity[IX(X, Y, Z)].X;
                float Y0 = Y - Dt0 * Velocity[IX(X, Y, Z)].Y;
                float Z0 = Z - Dt0 * Velocity[IX(X, Y, Z)].Z;

                int32 I0 = FMath::FloorToInt(X0);
                int32 I1 = I0 + 1;
                int32 J0 = FMath::FloorToInt(Y0);
                int32 J1 = J0 + 1;
                int32 K0 = FMath::FloorToInt(Z0);
                int32 K1 = K0 + 1;

                float S1 = X0 - I0;
                float S0 = 1 - S1;
                float T1 = Y0 - J0;
                float T0 = 1 - T1;
                float U1 = Z0 - K0;
                float U0 = 1 - U1;

                I0 = FMath::Clamp(I0, 0, GridSizeX - 1);
                I1 = FMath::Clamp(I1, 0, GridSizeX - 1);
                J0 = FMath::Clamp(J0, 0, GridSizeY - 1);
                J1 = FMath::Clamp(J1, 0, GridSizeY - 1);
                K0 = FMath::Clamp(K0, 0, GridSizeZ - 1);
                K1 = FMath::Clamp(K1, 0, GridSizeZ - 1);

                Dst[IX(X, Y, Z)] =
                    S0 * (T0 * (U0 * Src[IX(I0, J0, K0)] + U1 * Src[IX(I0, J0, K1)]) +
                        T1 * (U0 * Src[IX(I0, J1, K0)] + U1 * Src[IX(I0, J1, K1)])) +
                    S1 * (T0 * (U0 * Src[IX(I1, J0, K0)] + U1 * Src[IX(I1, J0, K1)]) +
                        T1 * (U0 * Src[IX(I1, J1, K0)] + U1 * Src[IX(I1, J1, K1)]));
            }
        }
    }
}

FVector UWindSimulationComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector LocalPos = GetOwner()->GetActorTransform().InverseTransformPosition(Location);
    int32 X = FMath::Clamp(FMath::FloorToInt(LocalPos.X / CellSize), 0, GridSizeX - 1);
    int32 Y = FMath::Clamp(FMath::FloorToInt(LocalPos.Y / CellSize), 0, GridSizeY - 1);
    int32 Z = FMath::Clamp(FMath::FloorToInt(LocalPos.Z / CellSize), 0, GridSizeZ - 1);

    return VelocityGrid[IX(X, Y, Z)];
}

int32 UWindSimulationComponent::IX(int32 X, int32 Y, int32 Z) const
{
    return X + GridSizeX * (Y + GridSizeY * Z);
}
