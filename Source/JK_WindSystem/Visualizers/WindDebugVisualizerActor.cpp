#include "WindDebugVisualizerActor.h"

AWindDebugVisualizerActor::AWindDebugVisualizerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    WindDebugVisualizer = CreateDefaultSubobject<UWindDebugVisualizer>(TEXT("WindDebugVisualizer"));
    WindDebugVisualizer->SetupAttachment(RootComponent);
}

void AWindDebugVisualizerActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateVisualizerSettings();
}

void AWindDebugVisualizerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AWindDebugVisualizerActor::UpdateVisualizerSettings()
{
    if (WindDebugVisualizer)
    {
        WindDebugVisualizer->UpdateInterval = UpdateInterval;
        WindDebugVisualizer->ArrowScale = ArrowScale;
        WindDebugVisualizer->GridResolution = GridResolution;
        WindDebugVisualizer->SetRelativeScale3D(VisualizationExtent / 1000.0f); // Assuming default extent is 1000
    }
}
