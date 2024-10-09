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

    // Create the WindSimulationComponent
    if (UWorld* World = GetWorld())
    {
        AActor* WindSimActor = World->SpawnActor<AActor>();
        WindSimComponent = NewObject<UWindSimulationComponent>(WindSimActor);
        WindSimComponent->RegisterComponent();
    }

    // Set up tick function
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWindSimulationSubsystem::Tick), 0.0f);
}

void UWindSimulationSubsystem::Deinitialize()
{
    // Remove tick function
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

    // Clean up WindSimulationComponent
    if (WindSimComponent)
    {
        WindSimComponent->DestroyComponent();
        WindSimComponent = nullptr;
    }

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

    UpdateWindGenerators(DeltaTime);

    return true;
}

void UWindSimulationSubsystem::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    if (WindSimComponent)
    {
        WindSimComponent->AddWindAtLocation(Location, WindVelocity);
    }
}

FVector UWindSimulationSubsystem::GetWindVelocityAtLocation(const FVector& WorldLocation) const
{
    if (WindSimComponent)
    {
        return WindSimComponent->GetWindVelocityAtLocation(WorldLocation);
    }
    return FVector::ZeroVector;
}

void UWindSimulationSubsystem::RegisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    WindGenerators.AddUnique(WindGenerator);
}

void UWindSimulationSubsystem::UnregisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    WindGenerators.Remove(WindGenerator);
}

void UWindSimulationSubsystem::UpdateWindGenerators(float DeltaTime)
{
    for (UWindGeneratorComponent* WindGenerator : WindGenerators)
    {
        if (WindGenerator && WindGenerator->IsActive())
        {
            WindGenerator->UpdateWindSimulation(DeltaTime, this);
        }
    }
}

