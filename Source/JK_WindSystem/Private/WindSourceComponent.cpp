// PointWindGeneratorComponent.cpp
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
        // Sample points within the wind generator's influence
        const int32 SampleCount = 10;
        for (int32 i = 0; i < SampleCount; ++i)
        {
            FVector RandomOffset = FMath::VRand() * FMath::RandRange(0.0f, Radius);
            FVector SampleLocation = GetComponentLocation() + RandomOffset;
            FVector WindVelocity = GetWindVelocityAtLocation(SampleLocation);

            // WINDSYSTEM_LOG(Log, TEXT("WindGeneratorComponent: Adding wind at location %s with velocity %s"), *SampleLocation.ToString(), *WindVelocity.ToString());
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
