#include "WindSubsystem.h"
#include "WindSystemActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "WindZoneVolumeComponent.h"

UWindSimulationSubsystem::UWindSimulationSubsystem()
    : WindSystemActor(nullptr)
{
}

void UWindSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    EnsureWindSystemActorInitialized();

    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWindSimulationSubsystem::Tick), 0.0f);
}

void UWindSimulationSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

    DestroyWindSystemActor();

    Super::Deinitialize();
}

bool UWindSimulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (const UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

bool UWindSimulationSubsystem::Tick(float DeltaTime)
{
    EnsureWindSystemActorInitialized();

    if (WindSystemActor && WindSystemActor->WindSimulationComponent)
    {
        WindSystemActor->WindSimulationComponent->SimulationStep(DeltaTime);
    }

    UpdateWindGenerators(DeltaTime);

    return true;
}

FVector UWindSimulationSubsystem::GetWindVelocityAtLocation(const FVector& WorldLocation) const
{
    if (WindSystemActor && WindSystemActor->WindSimulationComponent)
    {
        FVector WindVelocity = WindSystemActor->WindSimulationComponent->GetWindVelocityAtLocation(WorldLocation);
        for (const UWindZoneVolumeComponent* Modifier : WindZones)
        {
            WindVelocity = Modifier->ModifyWindVelocity(WindVelocity, WorldLocation);
        }
        return WindVelocity;
    }
    return FVector::ZeroVector;
}

void UWindSimulationSubsystem::AddWindAtLocation(const FVector& Location, const FVector& WindVelocity)
{
    if (WindSystemActor && WindSystemActor->WindSimulationComponent)
    {
        WindSystemActor->WindSimulationComponent->AddWindAtLocation(Location, WindVelocity);
    }
}

void UWindSimulationSubsystem::RegisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    FScopeLock Lock(&GeneratorsLock);
    WindGenerators.AddUnique(WindGenerator);
}

void UWindSimulationSubsystem::UnregisterWindGenerator(UWindGeneratorComponent* WindGenerator)
{
    FScopeLock Lock(&GeneratorsLock);
    WindGenerators.Remove(WindGenerator);
}

void UWindSimulationSubsystem::RegisterWindZone(UWindZoneVolumeComponent* Modifier)
{
    WindZones.AddUnique(Modifier);
}

void UWindSimulationSubsystem::UnregisterWindZone(UWindZoneVolumeComponent* Modifier)
{
    WindZones.Remove(Modifier);
}

void UWindSimulationSubsystem::UpdateWindGenerators(float DeltaTime)
{
    FScopeLock Lock(&GeneratorsLock);
    for (UWindGeneratorComponent* WindGenerator : WindGenerators)
    {
        if (WindGenerator && WindGenerator->IsActive())
        {
            WindGenerator->UpdateWindSimulation(DeltaTime, this);
        }
    }
}

void UWindSimulationSubsystem::EnsureWindSystemActorInitialized()
{
    if (!WindSystemActor)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            // Check if a WindSystemActor already exists in the world
            for (TActorIterator<AWindSystemActor> It(World); It; ++It)
            {
                WindSystemActor = *It;
                break;
            }

            // If no WindSystemActor exists, create a new one
            if (!WindSystemActor)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                WindSystemActor = World->SpawnActor<AWindSystemActor>(SpawnParams);
            }

            if (WindSystemActor)
            {
                WindSystemActor->WindSimulationComponent->InitializeForTesting();
            }
        }
    }
}

void UWindSimulationSubsystem::DestroyWindSystemActor()
{
    if (WindSystemActor)
    {
        WindSystemActor->Destroy();
        WindSystemActor = nullptr;
    }
}