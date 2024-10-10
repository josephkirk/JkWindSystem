#include "WindSubsystem.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "WindSystemCommon.h"

UWindSimulationSubsystem::UWindSimulationSubsystem()
    : WindSimComponent(nullptr)
{
}

void UWindSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    EnsureWindSimComponentInitialized();

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
    EnsureWindSimComponentInitialized();

    if (WindSimComponent)
    {
        WindSimComponent->SimulationStep(DeltaTime);
    }

    UpdateWindGenerators(DeltaTime);

    return true;
}

void UWindSimulationSubsystem::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    EnsureWindSimComponentInitialized();

    if (WindSimComponent)
    {
        WindSimComponent->AddWindAtLocation(Location, WindVelocity);
    }
}

FVector UWindSimulationSubsystem::GetWindVelocityAtLocation(const FVector& WorldLocation) const
{
    if (!WindSimComponent)
    {
        WINDSYSTEM_LOG_ERROR(TEXT("WindSimComponent is not initialized in GetWindVelocityAtLocation"));
        return FVector::ZeroVector;
    }

    return WindSimComponent->GetWindVelocityAtLocation(WorldLocation);
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

void UWindSimulationSubsystem::EnsureWindSimComponentInitialized()
{
    if (!WindSimComponent)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            AActor* WindSimActor = World->SpawnActor<AActor>();
            WindSimComponent = NewObject<UWindSimulationComponent>(WindSimActor);
            WindSimComponent->RegisterComponent();
            WindSimComponent->InitializeForTesting(); // Call this for both testing and normal initialization
            WINDSYSTEM_LOG(Log, TEXT("WindSimulationComponent created and initialized"));
        }
        else
        {
            WINDSYSTEM_LOG_ERROR(TEXT("Failed to create WindSimulationComponent: World is null"));
        }
    }
}