#include "PointWindGeneratorActor.h"

APointWindGeneratorActor::APointWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<UPointWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
    RootComponent = WindGeneratorComponent;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

    WindVisualizer = CreateDefaultSubobject<UPointWindVisualizer>(TEXT("WindVisualizer"));
    WindVisualizer->SetupAttachment(RootComponent);
}

void APointWindGeneratorActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (WindVisualizer && WindGeneratorComponent)
    {
        WindVisualizer->SetWindSourceComponent(WindGeneratorComponent);
    }
}

void APointWindGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
    
    if (WindVisualizer)
    {
        WindVisualizer->SetVisibility(false);
    }
}

#if WITH_EDITOR
void APointWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (WindVisualizer && WindGeneratorComponent)
    {
        WindVisualizer->SetWindSourceComponent(WindGeneratorComponent);
    }
}
#endif