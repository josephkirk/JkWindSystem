#include "WindGridVisualizerActor.h"

AWindGridVisualizerActor::AWindGridVisualizerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    GridVisualizer = CreateDefaultSubobject<UWindGridVisualizer>(TEXT("GridVisualizer"));
    GridVisualizer->SetupAttachment(RootComponent);
}