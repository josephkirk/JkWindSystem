
#include "WindGeneratorComponent.h"

UWindGeneratorComponent::UWindGeneratorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWindGeneratorComponent::BeginPlay()
{
    Super::BeginPlay();
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
