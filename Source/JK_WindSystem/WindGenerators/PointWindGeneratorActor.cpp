#include "PointWindGeneratorActor.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"

APointWindGeneratorActor::APointWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<UPointWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
    RootComponent = WindGeneratorComponent;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    ArrowComponent->SetupAttachment(RootComponent);
    ArrowComponent->bIsEditorOnly = true;
    ArrowComponent->SetVisibility(true);
    ArrowComponent->SetHiddenInGame(true);
#endif

    bShowDebugVisualization = true;
}

void APointWindGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Hide editor-only components in-game
#if WITH_EDITORONLY_DATA
    if (ArrowComponent)
    {
        ArrowComponent->SetVisibility(false);
        ArrowComponent->SetHiddenInGame(true);
    }
#endif
}

void APointWindGeneratorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugVisualization)
    {
        UpdateVisualization();
    }
}

void APointWindGeneratorActor::UpdateVisualization()
{
    if (!WindGeneratorComponent) return;

    FVector Location = GetActorLocation();
    float Radius = WindGeneratorComponent->Radius;
    float Strength = WindGeneratorComponent->Strength;

    // Draw sphere to represent the area of effect
    DrawDebugSphere(GetWorld(), Location, Radius, 32, FColor::Cyan, false, -1.0f, 0, 2.0f);

    // Draw lines to represent wind direction (inward for point wind)
    int32 NumLines = 16;
    for (int32 i = 0; i < NumLines; ++i)
    {
        float Angle = 2.0f * PI * i / NumLines;
        FVector Start = Location + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
        FVector End = Location;
        DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, -1.0f, 0, 2.0f);
    }

    // Update arrow component to show wind strength
#if WITH_EDITORONLY_DATA
    if (ArrowComponent)
    {
        ArrowComponent->SetArrowLength(FMath::Clamp(Strength / 10.0f, 50.0f, 500.0f));
        ArrowComponent->SetArrowColor(FColor::Blue);
    }
#endif
}

#if WITH_EDITOR
void APointWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update visualization when properties change in editor
    UpdateVisualization();
}
#endif