#include "WindZoneVolumeComponent.h"
#include "WindSubsystem.h"
#include "Kismet/GameplayStatics.h"

UWindZoneVolumeComponent::UWindZoneVolumeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    ModifierType = EWindZoneType::FreezeSimulation;
    RedirectionTransform = FTransform::Identity;
}

void UWindZoneVolumeComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        WindSubsystem = World->GetSubsystem<UWindSimulationSubsystem>();
        if (WindSubsystem)
        {
            WindSubsystem->RegisterWindZone(this);
        }
    }
}

void UWindZoneVolumeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WindSubsystem)
    {
        WindSubsystem->UnregisterWindZone(this);
    }
    Super::EndPlay(EndPlayReason);
}

bool UWindZoneVolumeComponent::IsPointInside(const FVector& Point) const
{
    FVector LocalPoint = GetComponentTransform().InverseTransformPosition(Point);
    FVector Extent = GetUnscaledBoxExtent();

    return FMath::Abs(LocalPoint.X) <= Extent.X &&
        FMath::Abs(LocalPoint.Y) <= Extent.Y &&
        FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

FVector UWindZoneVolumeComponent::ModifyWindVelocity(const FVector& OriginalVelocity, const FVector& Location) const
{
    if (!IsPointInside(Location))
    {
        return OriginalVelocity;
    }

    switch (ModifierType)
    {
        case EWindZoneType::FreezeSimulation:
            return FVector::ZeroVector;
        case EWindZoneType::ZeroWind:
            return FVector::ZeroVector;
        case EWindZoneType::Redirection:
            return RedirectionTransform.TransformVector(OriginalVelocity);
        default:
            return OriginalVelocity;
    }
}