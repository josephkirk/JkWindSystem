#include "WindGenerators/VortexWindGeneratorActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

AVortexWindGeneratorActor::AVortexWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<UVortexWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
    RootComponent = WindGeneratorComponent;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
    VortexMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VortexMesh"));
    VortexMeshComponent->SetupAttachment(RootComponent);
    VortexMeshComponent->bIsEditorOnly = true;
    VortexMeshComponent->SetVisibility(true);
    VortexMeshComponent->SetHiddenInGame(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> VortexMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (VortexMeshAsset.Succeeded())
    {
        VortexMeshComponent->SetStaticMesh(VortexMeshAsset.Object);
    }
#endif

    bShowDebugVisualization = true;
    VisualizationAngle = 0.0f;
}

void AVortexWindGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Hide editor-only components in-game
#if WITH_EDITORONLY_DATA
    if (VortexMeshComponent)
    {
        VortexMeshComponent->SetVisibility(false);
        VortexMeshComponent->SetHiddenInGame(true);
    }
#endif
}

void AVortexWindGeneratorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugVisualization)
    {
        VisualizationAngle += DeltaTime * 2.0f; // Rotate visualization
        if (VisualizationAngle > 2 * PI)
        {
            VisualizationAngle -= 2 * PI;
        }
        UpdateVisualization();
    }
}

void AVortexWindGeneratorActor::UpdateVisualization()
{
    if (!WindGeneratorComponent) return;

    FVector Location = GetActorLocation();
    float Radius = WindGeneratorComponent->Radius;
    float Strength = WindGeneratorComponent->Strength;
    float TangentialStrength = WindGeneratorComponent->TangentialStrength;

    // Draw cylinder to represent the area of effect
    DrawDebugCylinder(GetWorld(), Location - FVector(0, 0, Radius), Location + FVector(0, 0, Radius), Radius, 32, FColor::Cyan, false, -1.0f, 0, 2.0f);

    // Draw spiral lines to represent vortex wind
    int32 NumSpirals = 3;
    int32 PointsPerSpiral = 64;
    float HeightStep = 2 * Radius / PointsPerSpiral;

    for (int32 Spiral = 0; Spiral < NumSpirals; ++Spiral)
    {
        float SpiralOffset = (2.0f * PI * Spiral) / NumSpirals + VisualizationAngle;
        FVector PrevPoint = FVector::ZeroVector;

        for (int32 Point = 0; Point <= PointsPerSpiral; ++Point)
        {
            float t = static_cast<float>(Point) / PointsPerSpiral;
            float Height = -Radius + 2 * Radius * t;
            float Angle = SpiralOffset + t * 4 * PI; // 2 full rotations

            FVector CurrentPoint = Location + FVector(
                FMath::Cos(Angle) * Radius * (1 - t * 0.5f), // Spiral inwards slightly
                FMath::Sin(Angle) * Radius * (1 - t * 0.5f),
                Height
            );

            if (Point > 0)
            {
                DrawDebugLine(GetWorld(), PrevPoint, CurrentPoint, FColor::Blue, false, -1.0f, 0, 2.0f);
            }

            PrevPoint = CurrentPoint;
        }
    }

    // Update vortex mesh to show wind strength and direction
#if WITH_EDITORONLY_DATA
    if (VortexMeshComponent)
    {
        float MeshScale = FMath::Clamp(Strength / 100.0f, 0.5f, 2.0f);
        VortexMeshComponent->SetWorldScale3D(FVector(MeshScale, MeshScale, Radius / 50.0f)); // Adjust height to match radius
        VortexMeshComponent->SetWorldRotation(FRotator(0, VisualizationAngle * 57.2958f, 0)); // Convert radians to degrees
    }
#endif
}

#if WITH_EDITOR
void AVortexWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update visualization when properties change in editor
    UpdateVisualization();
}
#endif