#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Tests/AutomationEditorCommon.h"
#include "WindSystemComponent.h"
#include "WindSourceComponent.h"
#include "WindSubsystem.h"
#include "WindSystemSettings.h"
#include "ProfilingDebugging/CsvProfiler.h"

// Utility function to create a test world
inline UWorld* CreateTestWorld()
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(TestWorld);
    return TestWorld;
}

// Utility function to destroy the test world
inline void DestroyTestWorld(UWorld* TestWorld)
{
    if (TestWorld)
    {
        GEngine->DestroyWorldContext(TestWorld);
        TestWorld->DestroyWorld(false);
    }
}

// Utility function to create an actor with a specific component
template<typename TComponent>
inline AActor* CreateActorWithComponent(UWorld* World, TComponent*& OutComponent)
{
    AActor* Actor = World->SpawnActor<AActor>();
    OutComponent = NewObject<TComponent>(Actor);
    Actor->AddOwnedComponent(OutComponent);
    OutComponent->RegisterComponent();
    return Actor;
}

// Utility function to set up a basic wind simulation
inline UWindSimulationComponent* SetupWindSimulation(UWorld* World)
{
    UWindSimulationComponent* WindSimComponent;
    AActor* SimulationActor = CreateActorWithComponent(World, WindSimComponent);

    // Ensure the component is registered
    WindSimComponent->RegisterComponent();

    // Manually initialize the component for testing
    WindSimComponent->InitializeForTesting();

    // If needed, you can simulate some effects of BeginPlay here
    // For example, if there are any timers or other setup normally done in BeginPlay:
    // WindSimComponent->StartSimulation(); // Assuming you have such a method

    return WindSimComponent;
}

// Utility function to create a point wind generator
inline UPointWindGeneratorComponent* CreatePointWindGenerator(UWorld* World, const FVector& Location, float Strength, float Radius)
{
    UPointWindGeneratorComponent* WindGenerator;
    AActor* GeneratorActor = CreateActorWithComponent(World, WindGenerator);
    GeneratorActor->SetActorLocation(Location);
    WindGenerator->Strength = Strength;
    WindGenerator->Radius = Radius;
    return WindGenerator;
}

// Utility function to simulate wind for a number of steps
inline void SimulateWind(UWindSimulationSubsystem* Subsystem, int32 Steps, float DeltaTime)
{
    for (int32 i = 0; i < Steps; ++i)
    {
        Subsystem->Tick(DeltaTime);
    }
}

// Constants for common test values
namespace WindTestConstants
{
    const float DEFAULT_WIND_STRENGTH = 100.0f;
    const float DEFAULT_WIND_RADIUS = 500.0f;
    const FVector DEFAULT_GENERATOR_LOCATION(250.0f, 250.0f, 250.0f);
    const int32 DEFAULT_SIMULATION_STEPS = 10;
    const float DEFAULT_SIMULATION_DELTA_TIME = 1.0f / 60.0f;
}

// Add any other common functions or utilities here as needed