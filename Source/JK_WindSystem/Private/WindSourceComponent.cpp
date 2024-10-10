#include "WindSourceComponent.h"
#include "WindSubsystem.h"
#include "WindSystemLog.h"

UWindGeneratorComponent::UWindGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    TimeSinceLastUpdate = 0.0f;
}

void UWindGeneratorComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWindSimulationSubsystem* Subsystem = GetWindSubsystem())
    {
        Subsystem->RegisterWindGenerator(this);
    }
}

void UWindGeneratorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWindSimulationSubsystem* Subsystem = GetWindSubsystem())
    {
        Subsystem->UnregisterWindGenerator(this);
    }
    Super::EndPlay(EndPlayReason);
}

void UWindGeneratorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // The actual update is now handled by the subsystem
}

void UWindGeneratorComponent::UpdateWindSimulation(float DeltaTime, UWindSimulationSubsystem* Subsystem)
{
    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateFrequency)
    {
        const int32 SampleCount = 10;
        for (int32 i = 0; i < SampleCount; ++i)
        {
            FVector RandomOffset = FMath::VRand() * FMath::RandRange(0.0f, Radius);
            FVector SampleLocation = GetComponentLocation() + RandomOffset;
            FVector WindVelocity = GetWindVelocityAtLocation(SampleLocation);

            WINDSYSTEM_LOG(Verbose, TEXT("WindGeneratorComponent: Adding wind at location %s with velocity %s"), *SampleLocation.ToString(), *WindVelocity.ToString());
            Subsystem->AddWindAtLocation(SampleLocation, WindVelocity);
        }
        TimeSinceLastUpdate = 0.0f;
    }
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

UWindSimulationSubsystem* UWindGeneratorComponent::GetWindSubsystem() const
{
    if (UWorld* World = GetWorld())
    {
        return World->GetSubsystem<UWindSimulationSubsystem>();
    }
    return nullptr;
}

FVector UPointWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector Direction = GetComponentLocation() - Location;  // Changed to point towards center
    float Distance = Direction.Size();

    if (Distance > Radius)
        return FVector::ZeroVector;

    if (FMath::IsNearlyZero(Distance))
        return FVector::ZeroVector;

    Direction /= Distance;  // Normalize
    float StrengthAtDistance = Strength * GetFalloff(Distance);
    return Direction * StrengthAtDistance;
}

FVector UDirectionalWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector ToLocation = Location - GetComponentLocation();
    float Distance = ToLocation.Size();

    if (Distance > Radius)
        return FVector::ZeroVector;

    FVector Direction = GetForwardVector();

    if (ShapeType == EWindShapeType::Cone)
    {
        float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, ToLocation.GetSafeNormal())));
        if (Angle > ConeAngle)
            return FVector::ZeroVector;
    }

    float StrengthAtDistance = Strength * GetFalloff(Distance);
    return Direction * StrengthAtDistance;
}

FVector UVortexWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector ToLocation = Location - GetComponentLocation();
    float Distance = ToLocation.Size();

    if (Distance > Radius)
        return FVector::ZeroVector;

    FVector Tangent = FVector::CrossProduct(GetUpVector(), ToLocation).GetSafeNormal();
    FVector Radial = ToLocation.GetSafeNormal();

    float StrengthAtDistance = Strength * GetFalloff(Distance);
    float TangentialStrengthAtDistance = TangentialStrength * GetFalloff(Distance);

    return (Tangent * TangentialStrengthAtDistance + Radial * StrengthAtDistance);
}

USplineWindGeneratorComponent::USplineWindGeneratorComponent()
{
    WindSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WindSpline"));
    WindSpline->SetupAttachment(this);
}

FVector USplineWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    float SplineInput = WindSpline->FindInputKeyClosestToWorldLocation(Location);
    FVector ClosestPoint = WindSpline->GetLocationAtSplineInputKey(SplineInput, ESplineCoordinateSpace::World);
    float ClosestDistance = FVector::Distance(Location, ClosestPoint);

    if (ClosestDistance > Radius)
        return FVector::ZeroVector;

    FVector SplineTangent = WindSpline->GetTangentAtSplineInputKey(SplineInput, ESplineCoordinateSpace::World).GetSafeNormal();
    float StrengthAtDistance = Strength * GetFalloff(ClosestDistance);
    return SplineTangent * StrengthAtDistance;
}