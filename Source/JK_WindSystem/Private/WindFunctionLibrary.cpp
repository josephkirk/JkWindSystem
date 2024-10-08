#include "WindFunctionLibrary.h"
#include "WindSubsystem.h"

FVector UWindSimulationFunctionLibrary::GetWindVelocityAtLocation(const UObject* WorldContextObject, const FVector& WorldLocation)
{
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
    {
        if (UWindSimulationSubsystem* WindSubsystem = World->GetSubsystem<UWindSimulationSubsystem>())
        {
            return WindSubsystem->GetWindVelocityAtLocation(WorldLocation);
        }
    }
    return FVector::ZeroVector;
}
