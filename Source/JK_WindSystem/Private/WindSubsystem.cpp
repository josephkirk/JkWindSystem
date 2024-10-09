#include "WindSubsystem.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

UWindSimulationSubsystem::UWindSimulationSubsystem()
    : WindSimComponent(nullptr)
{
}

void UWindSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Set up tick function
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWindSimulationSubsystem::Tick), 0.0f);
}

void UWindSimulationSubsystem::Deinitialize()
{
    // Remove tick function
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

    Super::Deinitialize();
}

bool UWindSimulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Only create for game worlds
    if (const UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

bool UWindSimulationSubsystem::Tick(float DeltaTime)
{
    if (WindSimComponent)
    {
        WindSimComponent->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, nullptr);
    }
    return true;
}

void UWindSimulationSubsystem::SetWindSimulationComponent(UWindSimulationComponent* InWindSimComponent)
{
    WindSimComponent = InWindSimComponent;
}

void UWindSimulationSubsystem::RegisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    if (WindGenerator)
    {
        WindGenerators.AddUnique(WindGenerator);
    }
}

void UWindSimulationSubsystem::UnregisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    WindGenerators.Remove(WindGenerator);
}

FVector UWindSimulationSubsystem::GetWindVelocityAtLocation(const FVector& WorldLocation) const
{
    FVector TotalWind = FVector::ZeroVector;
    
    if (WindSimComponent)
    {
        TotalWind = WindSimComponent->GetWindVelocityAtLocation(WorldLocation);
    }

    for (const UWindGeneratorComponent* WindGenerator : WindGenerators)
    {
        if (WindGenerator && WindGenerator->IsActive())
        {
            TotalWind += WindGenerator->GetWindVelocityAtLocation(WorldLocation);
        }
    }

    return TotalWind;
}

