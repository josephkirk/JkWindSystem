#include "DirectionalWindGeneratorActor.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"

ADirectionalWindGeneratorActor::ADirectionalWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<UDirectionalWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
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

void ADirectionalWindGeneratorActor::BeginPlay()
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

void ADirectionalWindGeneratorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugVisualization)
    {
        UpdateVisualization();
    }
}

void ADirectionalWindGeneratorActor::UpdateVisualization()
{
    if (!WindGeneratorComponent) return;

    FVector Location = GetActorLocation();
    FVector Direction = GetActorForwardVector();
    float Radius = WindGeneratorComponent->Radius;
    float Strength = WindGeneratorComponent->Strength;

    // Draw cylinder to represent the area of effect
    DrawDebugCylinder(GetWorld(), Location, Location + Direction * Radius, Radius, 32, FColor::Cyan, false, -1.0f, 0, 2.0f);

    // Draw lines to represent wind direction
    int32 NumLines = 16;
    float LineLength = Radius * 0.8f;
    for (int32 i = 0; i < NumLines; ++i)
    {
        float Angle = 2.0f * PI * i / NumLines;
        FVector Offset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
        FVector Start = Location + Offset;
        FVector End = Start + Direction * LineLength;
        DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, -1.0f, 0, 2.0f);
    }

    // Update arrow component to show wind direction and strength
#if WITH_EDITORONLY_DATA
    if (ArrowComponent)
    {
        ArrowComponent->SetArrowLength(FMath::Clamp(Strength / 10.0f, 50.0f, 500.0f));
        ArrowComponent->SetArrowColor(FColor::Blue);
        ArrowComponent->SetWorldRotation(Direction.Rotation());
    }
#endif
}

#if WITH_EDITOR
void ADirectionalWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update visualization when properties change in editor
    UpdateVisualization();
}
#endif