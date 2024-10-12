#include "WindSystemActor.h"

AWindSystemActor::AWindSystemActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindSimulationComponent = CreateDefaultSubobject<UWindSimulationComponent>(TEXT("WindSimulationComponent"));
    RootComponent = WindSimulationComponent;

    WindGridVisualizer = CreateDefaultSubobject<UWindGridVisualizer>(TEXT("WindGridVisualizer"));
    WindGridVisualizer->SetupAttachment(RootComponent);

    bShowWindVisualization = true;
}

void AWindSystemActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // You can add any per-frame updates here if needed
    WindGridVisualizer->SetVisibility(bShowWindVisualization);
}

#if WITH_EDITOR
void AWindSystemActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AWindSystemActor, bShowWindVisualization))
    {
        WindGridVisualizer->SetVisibility(bShowWindVisualization);
    }
}
#endif