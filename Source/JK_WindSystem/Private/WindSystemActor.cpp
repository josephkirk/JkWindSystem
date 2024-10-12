#include "WindSystemActor.h"

AWindSystemActor::AWindSystemActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindSimulationComponent = CreateDefaultSubobject<UWindSimulationComponent>(TEXT("WindSimulationComponent"));
    RootComponent = WindSimulationComponent;

    WindSystemVisualizer = CreateDefaultSubobject<UWindSystemVisualizer>(TEXT("WindSystemVisualizer"));
    WindSystemVisualizer->SetupAttachment(RootComponent);

    bShowWindVisualization = true;
}

void AWindSystemActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // You can add any per-frame updates here if needed
    WindSystemVisualizer->SetVisibility(bShowWindVisualization);
}

#if WITH_EDITOR
void AWindSystemActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AWindSystemActor, bShowWindVisualization))
    {
        WindSystemVisualizer->SetVisibility(bShowWindVisualization);
    }
}
#endif