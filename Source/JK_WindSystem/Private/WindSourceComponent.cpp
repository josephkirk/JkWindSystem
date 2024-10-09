// PointWindGeneratorComponent.cpp
#include "WindSourceComponent.h"
#include "WindSubsystem.h"

UWindGeneratorComponent::UWindGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWindGeneratorComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (UWindSimulationSubsystem* WindSubsystem = World->GetSubsystem<UWindSimulationSubsystem>())
        {
            WindSubsystem->RegisterWindGenerator(this);
        }
    }
}

void UWindGeneratorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UWindSimulationSubsystem* WindSubsystem = World->GetSubsystem<UWindSimulationSubsystem>())
        {
            WindSubsystem->UnregisterWindGenerator(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UWindGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FVector UWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    // Base class returns no wind
    return FVector::ZeroVector;
}

float UWindGeneratorComponent::GetFalloff(float Distance) const
{
    return FMath::Clamp(1.0f - (Distance / Radius), 0.0f, 1.0f);
}

FVector UPointWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector Direction = Location - GetComponentLocation();
    float Distance = Direction.Size();
    
    if (Distance > Radius)
        return FVector::ZeroVector;

    Direction.Normalize();
    return Direction * Strength * GetFalloff(Distance);
}

FVector UDirectionalWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector Direction = GetForwardVector();
    FVector ToLocation = Location - GetComponentLocation();
    float Distance = ToLocation.Size();

    if (Distance > Radius)
        return FVector::ZeroVector;

    if (ShapeType == EWindShapeType::Cone)
    {
        float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, ToLocation.GetSafeNormal())));
        if (Angle > ConeAngle)
            return FVector::ZeroVector;
    }

    return Direction * Strength * GetFalloff(Distance);
}

FVector UVortexWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector ToLocation = Location - GetComponentLocation();
    float Distance = ToLocation.Size();

    if (Distance > Radius)
        return FVector::ZeroVector;

    FVector Tangent = FVector::CrossProduct(GetUpVector(), ToLocation).GetSafeNormal();
    FVector Outward = ToLocation.GetSafeNormal();

    return (Tangent * TangentialStrength + Outward * Strength) * GetFalloff(Distance);
}

USplineWindGeneratorComponent::USplineWindGeneratorComponent()
{
    WindSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WindSpline"));
    WindSpline->SetupAttachment(this);
}

FVector USplineWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    float ClosestDistance = TNumericLimits<float>::Max();
    float SplineInput = WindSpline->FindInputKeyClosestToWorldLocation(Location);

    // Calculate the closest distance manually
    FVector ClosestPoint = WindSpline->GetLocationAtSplineInputKey(SplineInput, ESplineCoordinateSpace::World);
    ClosestDistance = FVector::Distance(Location, ClosestPoint);

    if (ClosestDistance > Radius)
        return FVector::ZeroVector;

    FVector SplineTangent = WindSpline->GetTangentAtDistanceAlongSpline(SplineInput * WindSpline->GetSplineLength(), ESplineCoordinateSpace::World);
    return SplineTangent.GetSafeNormal() * Strength * GetFalloff(ClosestDistance);
}
