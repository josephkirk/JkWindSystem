// WindSystemVisualizer.cpp
#include "WindSystemVisualizer.h"
#include "WindSubsystem.h"
#include "Kismet/GameplayStatics.h"

UWindDebugVisualizer::UWindDebugVisualizer()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWindDebugVisualizer::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner)
    {
        WindSimComponent = Owner->FindComponentByClass<UWindSimulationComponent>();
        if (WindSimComponent)
        {
            WindSimComponent->OnWindCellUpdated.AddDynamic(this, &UWindDebugVisualizer::OnWindCellUpdated);
        }
    }
}

void UWindDebugVisualizer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WindSimComponent)
    {
        WindSimComponent->OnWindCellUpdated.RemoveDynamic(this, &UWindDebugVisualizer::OnWindCellUpdated);
    }

    Super::EndPlay(EndPlayReason);
}

void UWindDebugVisualizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDrawAdaptiveGrid)
    {
        DrawDebugAdaptiveGrid(GetWorld());
    }
}

void UWindDebugVisualizer::OnWindCellUpdated(const FVector& CellCenter, const FVector& WindVelocity, float CellSize)
{
    UWorld* World = GetWorld();
    if (World && !World->IsNetMode(NM_DedicatedServer))
    {
        float VelocityMagnitude = WindVelocity.Size();
        if (VelocityMagnitude >= MinVelocityThreshold)
        {
            if (bDrawArrows)
            {
                FVector End = CellCenter + WindVelocity * ArrowScale;
                DrawDebugArrow(World, CellCenter, End, WindVelocity);
            }

            if (bDrawVelocityText)
            {
                DrawDebugVelocityText(World, CellCenter, WindVelocity);
            }
        }
    }
}

void UWindDebugVisualizer::DrawDebugArrow(const UWorld* World, const FVector& Start, const FVector& End, const FVector& Velocity)
{
    DrawDebugDirectionalArrow(World, Start, End, 20.0f, ArrowColor, false, -1.0f, 0, ArrowThickness);
}

void UWindDebugVisualizer::DrawDebugVelocityText(const UWorld* World, const FVector& Location, const FVector& Velocity)
{
    FString VelocityText = FString::Printf(TEXT("%.2f, %.2f, %.2f"), Velocity.X, Velocity.Y, Velocity.Z);
    DrawDebugString(World, Location, VelocityText, nullptr, ArrowColor, 0.0f, false, TextScale);
}

void UWindDebugVisualizer::DrawDebugAdaptiveGrid(const UWorld* World)
{
    if (!World || !WindSimComponent)
        return;

    UWindSimulationSubsystem* WindSubsystem = GetWindSubsystem();
    if (!WindSubsystem)
        return;

    FVector Origin = GetOwner()->GetActorLocation();
    float GridSize = WindSimComponent->GetCellSize() * WindSimComponent->GetBaseGridSize();

    for (int32 z = 0; z < VisualizationResolution; ++z)
    {
        for (int32 y = 0; y < VisualizationResolution; ++y)
        {
            for (int32 x = 0; x < VisualizationResolution; ++x)
            {
                FVector CellCenter = Origin + FVector(
                    x * GridSize / VisualizationResolution,
                    y * GridSize / VisualizationResolution,
                    z * GridSize / VisualizationResolution
                );

                FVector WindVelocity = WindSubsystem->GetWindVelocityAtLocation(CellCenter);
                
                if (WindVelocity.Size() >= MinVelocityThreshold)
                {
                    if (bDrawArrows)
                    {
                        FVector End = CellCenter + WindVelocity * ArrowScale;
                        DrawDebugArrow(World, CellCenter, End, WindVelocity);
                    }

                    if (bDrawVelocityText)
                    {
                        DrawDebugVelocityText(World, CellCenter, WindVelocity);
                    }
                }

                DrawDebugBox(World, CellCenter, FVector(GridSize / VisualizationResolution / 2), FQuat::Identity, FColor::White, false, -1, 0, 1);
            }
        }
    }
}

UWindSimulationSubsystem* UWindDebugVisualizer::GetWindSubsystem() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetSubsystem<UWindSimulationSubsystem>();
    }
    return nullptr;
}

// WindDebugVisualizationActor

AWindDebugVisualizationActor::AWindDebugVisualizationActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindDebugVisualizer = CreateDefaultSubobject<UWindDebugVisualizer>(TEXT("WindDebugVisualizer"));
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void AWindDebugVisualizationActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateVisualizerSettings();
}

void AWindDebugVisualizationActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AWindDebugVisualizationActor::UpdateVisualizerSettings()
{
    if (WindDebugVisualizer)
    {
        WindDebugVisualizer->ArrowScale = ArrowScale;
        WindDebugVisualizer->ArrowThickness = ArrowThickness;
        WindDebugVisualizer->ArrowColor = ArrowColor;
        WindDebugVisualizer->MinVelocityThreshold = MinVelocityThreshold;
        WindDebugVisualizer->bDrawArrows = bDrawArrows;
        WindDebugVisualizer->bDrawVelocityText = bDrawVelocityText;
        WindDebugVisualizer->bDrawAdaptiveGrid = bDrawAdaptiveGrid;
        WindDebugVisualizer->TextScale = TextScale;
        WindDebugVisualizer->VisualizationResolution = VisualizationResolution;
    }
}
