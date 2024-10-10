#include "SplineWindGeneratorActor.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"

ASplineWindGeneratorActor::ASplineWindGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = true;

    WindGeneratorComponent = CreateDefaultSubobject<USplineWindGeneratorComponent>(TEXT("WindGeneratorComponent"));
    RootComponent = WindGeneratorComponent;

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

    bShowDebugVisualization = true;
}

void ASplineWindGeneratorActor::BeginPlay()
{
    Super::BeginPlay();
}

void ASplineWindGeneratorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugVisualization)
    {
        UpdateVisualization();
    }
}

void ASplineWindGeneratorActor::UpdateVisualization()
{
    if (!WindGeneratorComponent || !WindGeneratorComponent->WindSpline) return;

    USplineComponent* Spline = WindGeneratorComponent->WindSpline;
    float Radius = WindGeneratorComponent->Radius;
    float Strength = WindGeneratorComponent->Strength;

    // Draw spline
    for (int32 i = 0; i < Spline->GetNumberOfSplinePoints() - 1; ++i)
    {
        FVector Start = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector End = Spline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1.0f, 0, 4.0f);
    }

    // Draw wind effect along the spline
    float SplineLength = Spline->GetSplineLength();
    int32 NumSegments = FMath::Max(static_cast<int32>(SplineLength / 50.0f), 10); // Adjust number of segments based on spline length

    for (int32 i = 0; i < NumSegments; ++i)
    {
        float Distance = (static_cast<float>(i) / NumSegments) * SplineLength;
        FVector Location = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

        // Calculate rotation based on tangent
        FRotator TangentRotation = Tangent.Rotation();
        FVector UpVector = TangentRotation.RotateVector(FVector::UpVector);
        FVector RightVector = TangentRotation.RotateVector(FVector::RightVector);

        // Draw cross-section of wind effect
        DrawDebugCircle(GetWorld(), Location, Radius, 16, FColor::Cyan, false, -1.0f, 0, 2.0f, UpVector, RightVector);

        // Draw wind direction
        DrawDebugLine(GetWorld(), Location, Location + Tangent.GetSafeNormal() * Radius * 0.8f, FColor::Blue, false, -1.0f, 0, 2.0f);
    }
}

#if WITH_EDITOR
void ASplineWindGeneratorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update visualization when properties change in editor
    UpdateVisualization();
}
#endif