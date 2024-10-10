#include "WindSystemVisualizer.h"
#include "WindSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "WindSystemCommon.h"

UWindDebugVisualizer::UWindDebugVisualizer()
{
    PrimaryComponentTick.bCanEverTick = true;
    bIsVisualizationActive = true;
    bAutoActivate = true;
}

void UWindDebugVisualizer::BeginPlay()
{
    Super::BeginPlay();

    UWindSimulationSubsystem* WindSubsystem = GetWindSubsystem();
    if (WindSubsystem)
    {
        WindSimComponent = WindSubsystem->GetWindSimComponent();
        if (WindSimComponent)
        {
            WindSimComponent->OnWindCellUpdated.AddDynamic(this, &UWindDebugVisualizer::OnWindCellUpdated);
            WINDSYSTEM_LOG(Log, TEXT("WindDebugVisualizer: Successfully bound to WindSimComponent"));
        }
        else
        {
            WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: Failed to get WindSimComponent from Subsystem"));
        }
    }
    else
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: Failed to get WindSimulationSubsystem"));
    }
}

void UWindDebugVisualizer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WindSimComponent)
    {
        WindSimComponent->OnWindCellUpdated.RemoveDynamic(this, &UWindDebugVisualizer::OnWindCellUpdated);
    }

    Super::EndPlay(EndPlayReason);
    
    bIsVisualizationActive = false;
}

void UWindDebugVisualizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsVisualizationActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: World is null in TickComponent"));
        return;
    }

    FWindCellUpdate Update;
    while (WindUpdates.Dequeue(Update))
    {
        ProcessWindUpdate(World, Update);
    }

    if (bDrawAdaptiveGrid)
    {
        DrawDebugAdaptiveGrid(World);
    }

    // Always draw some debug info to ensure the visualizer is working
    DrawDebugPoint(World, GetOwner()->GetActorLocation(), 10.0f, FColor::Red, false, -1.0f, 0);
    DrawDebugString(World, GetOwner()->GetActorLocation(), TEXT("WindDebugVisualizer"), nullptr, FColor::White, 0.0f, true);
}

void UWindDebugVisualizer::OnWindCellUpdated(const FVector& CellCenter, const FVector& WindVelocity, float CellSize)
{
    if (!bIsVisualizationActive)
    {
        return;
    }

    FWindCellUpdate NewUpdate;
    NewUpdate.CellCenter = CellCenter;
    NewUpdate.WindVelocity = WindVelocity;
    NewUpdate.CellSize = CellSize;

    WindUpdates.Enqueue(NewUpdate);
}

void UWindDebugVisualizer::ProcessWindUpdate(UWorld* World, const FWindCellUpdate& Update)
{
    float VelocityMagnitude = Update.WindVelocity.Size();
    
    // Check for NaN or inf values
    if (!IsVectorFinite(Update.WindVelocity))
    {
        WINDSYSTEM_LOG_ERROR(TEXT("Invalid wind velocity detected: %s"), *Update.WindVelocity.ToString());
        return;
    }

    // Clamp extremely large values
    const float MaxAllowedMagnitude = 1000000.0f; // Adjust this value as needed
    if (VelocityMagnitude > MaxAllowedMagnitude)
    {
        //WINDSYSTEM_LOG_WARNING(TEXT("Wind velocity magnitude exceeds maximum allowed value. Clamping."));
        FVector ClampedVelocity = Update.WindVelocity.GetSafeNormal() * MaxAllowedMagnitude;
        VelocityMagnitude = MaxAllowedMagnitude;
        
        // Use the clampedVelocity for further processing
        if (bDrawArrows)
        {
            FVector End = Update.CellCenter + ClampedVelocity * ArrowScale;
            DrawDebugArrow(World, Update.CellCenter, End, ClampedVelocity);
        }

        if (bDrawVelocityText)
        {
            DrawDebugVelocityText(World, Update.CellCenter, ClampedVelocity);
        }
    }
    else if (VelocityMagnitude >= MinVelocityThreshold)
    {
        if (bDrawArrows)
        {
            FVector End = Update.CellCenter + Update.WindVelocity * ArrowScale;
            DrawDebugArrow(World, Update.CellCenter, End, Update.WindVelocity);
        }

        if (bDrawVelocityText)
        {
            DrawDebugVelocityText(World, Update.CellCenter, Update.WindVelocity);
        }
    }

    WINDSYSTEM_LOG_VERBOSE(TEXT("Processing wind update: Center=%s, Velocity=%s, Magnitude=%f"), 
        *Update.CellCenter.ToString(), *Update.WindVelocity.ToString(), VelocityMagnitude);
}

void UWindDebugVisualizer::DrawDebugArrow(const UWorld* World, const FVector& Start, const FVector& End, const FVector& Velocity)
{
    if (!World)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("DrawDebugArrow: World is null"));
        return;
    }

    if (!IsVectorFinite(Start) || !IsVectorFinite(End))
    {
        WINDSYSTEM_LOG_ERROR(TEXT("DrawDebugArrow: Invalid Start or End point"));
        return;
    }

    DrawDebugDirectionalArrow(World, Start, End, 20.0f, ArrowColor, false, -1.0f, 0, ArrowThickness);
    WINDSYSTEM_LOG_VERBOSE(TEXT("Drew debug arrow: Start=%s, End=%s, Velocity=%s"), 
        *Start.ToString(), *End.ToString(), *Velocity.ToString());
}

void UWindDebugVisualizer::DrawDebugVelocityText(const UWorld* World, const FVector& Location, const FVector& Velocity)
{
    FString VelocityText = FString::Printf(TEXT("%.2f, %.2f, %.2f"), Velocity.X, Velocity.Y, Velocity.Z);
    DrawDebugString(World, Location, VelocityText, nullptr, ArrowColor, 0.0f, true, TextScale);
    WINDSYSTEM_LOG_VERBOSE(TEXT("Drew debug text: Location=%s, Text=%s"), *Location.ToString(), *VelocityText);
}

void UWindDebugVisualizer::DrawDebugAdaptiveGrid(const UWorld* World)
{
    if (!World || !WindSimComponent)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: World or WindSimComponent is null in DrawDebugAdaptiveGrid"));
        return;
    }

    UWindSimulationSubsystem* WindSubsystem = GetWindSubsystem();
    if (!WindSubsystem)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: Failed to get WindSimulationSubsystem in DrawDebugAdaptiveGrid"));
        return;
    }

    FVector Origin = WindSimComponent->GetComponentLocation();
    float CellSize = WindSimComponent->GetCellSize();
    int32 GridSize = WindSimComponent->GetBaseGridSize();
    float TotalSize = CellSize * GridSize;

    WINDSYSTEM_LOG_VERBOSE(TEXT("Drawing adaptive grid: Origin=%s, CellSize=%f, GridSize=%d, TotalSize=%f"),
        *Origin.ToString(), CellSize, GridSize, TotalSize);

    for (int32 z = 0; z < VisualizationResolution; ++z)
    {
        for (int32 y = 0; y < VisualizationResolution; ++y)
        {
            for (int32 x = 0; x < VisualizationResolution; ++x)
            {
                FVector CellCenter = Origin + FVector(
                    x * TotalSize / VisualizationResolution,
                    y * TotalSize / VisualizationResolution,
                    z * TotalSize / VisualizationResolution
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

                DrawDebugBox(World, CellCenter, FVector(TotalSize / VisualizationResolution / 2), FQuat::Identity, FColor::White, false, -1, 0, 1);
            }
        }
    }
}

UWindSimulationSubsystem* UWindDebugVisualizer::GetWindSubsystem() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: World is null in GetWindSubsystem"));
        return nullptr;
    }

    UWindSimulationSubsystem* Subsystem = World->GetSubsystem<UWindSimulationSubsystem>();
    if (!Subsystem)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindDebugVisualizer: Failed to get WindSimulationSubsystem"));
    }
    return Subsystem;
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
