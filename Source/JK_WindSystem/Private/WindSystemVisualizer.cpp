#include "WindSystemVisualizer.h"

UWindDebugVisualizer::UWindDebugVisualizer()
{
    PrimaryComponentTick.bCanEverTick = false;
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

void UWindDebugVisualizer::OnWindCellUpdated(const FVector& CellCenter, const FVector& WindVelocity, float CellSize)
{
    UWorld* World = GetWorld();
    if (World && World->IsEditorWorld())
    {
        float VelocityMagnitude = WindVelocity.Size();
        if (VelocityMagnitude >= MinVelocityThreshold)
        {
            if (bDrawArrows)
            {
                FVector End = CellCenter + WindVelocity * ArrowScale;
                DrawDebugArrow(World, CellCenter, End);
            }

            if (bDrawVelocityText)
            {
                DrawDebugVelocityText(World, CellCenter, WindVelocity);
            }
        }
    }
}

void UWindDebugVisualizer::DrawDebugArrow(const UWorld* World, const FVector& Start, const FVector& End)
{
    DrawDebugDirectionalArrow(
        World,
        Start,
        End,
        20.0f, // Arrow Size
        ArrowColor,
        false, // Persistent Lines
        DebugDrawDuration,
        0, // DepthPriority
        ArrowThickness
    );
}

void UWindDebugVisualizer::DrawDebugVelocityText(const UWorld* World, const FVector& Location, const FVector& Velocity)
{
    FString VelocityText = FString::Printf(TEXT("%.2f, %.2f, %.2f"), Velocity.X, Velocity.Y, Velocity.Z);
    DrawDebugString(
        World,
        Location,
        VelocityText,
        nullptr, // Actor
        ArrowColor,
        DebugDrawDuration,
        false, // bDrawShadow
        TextScale
    );
}

