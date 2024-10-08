#include "PointWindSourceComponent.h"

FVector UPointWindGeneratorComponent::GetWindVelocityAtLocation(const FVector& Location) const
{
    FVector Direction = Location - GetComponentLocation();
    float Distance = Direction.Size();
    
    if (Distance > Radius)
        return FVector::ZeroVector;

    Direction.Normalize();
    return Direction * Strength * GetFalloff(Distance);
}
