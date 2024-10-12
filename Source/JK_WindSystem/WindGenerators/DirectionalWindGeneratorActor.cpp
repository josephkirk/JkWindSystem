#include "DirectionalWindGeneratorActor.h"
#include "Components/ArrowComponent.h"

ADirectionalWindGeneratorActor::ADirectionalWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<UDirectionalWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
    RootComponent = WindGeneratorComponent;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

    WindVisualizer = CreateDefaultSubobject<UDirectionalWindVisualizer>(TEXT("WindVisualizer"));
    WindVisualizer->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    ArrowComponent->SetupAttachment(RootComponent);
    ArrowComponent->bIsEditorOnly = true;
    ArrowComponent->SetVisibility(true);
    ArrowComponent->SetHiddenInGame(true);
#endif
}

void ADirectionalWindGeneratorActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (WindVisualizer && WindGeneratorComponent)
    {
        WindVisualizer->SetWindSourceComponent(WindGeneratorComponent);
    }

#if WITH_EDITORONLY_DATA
    if (ArrowComponent)
    {
        ArrowComponent->SetArrowColor(FColor::Blue);
        ArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
    }
#endif
}

void ADirectionalWindGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
    
    if (WindVisualizer)
    {
        WindVisualizer->SetVisibility(false);
    }

#if WITH_EDITORONLY_DATA
    if (ArrowComponent)
    {
        ArrowComponent->SetVisibility(false);
        ArrowComponent->SetHiddenInGame(true);
    }
#endif
}

#if WITH_EDITOR
void ADirectionalWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (WindVisualizer && WindGeneratorComponent)
    {
        WindVisualizer->SetWindSourceComponent(WindGeneratorComponent);
    }

#if WITH_EDITORONLY_DATA
    if (ArrowComponent && WindGeneratorComponent)
    {
        ArrowComponent->SetArrowColor(FColor::Blue);
        ArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
    }
#endif
}
#endif