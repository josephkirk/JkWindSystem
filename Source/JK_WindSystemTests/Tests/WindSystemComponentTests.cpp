#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "WindSystemComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentInitializationTest, "JK_WindSystem.Component.Initialization", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentVelocityTest, "JK_WindSystem.Component.VelocityCalculation", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentSimulationStepTest, "JK_WindSystem.Component.SimulationStep", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

UWorld* CreateTestWorld()
{
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(TestWorld);
    return TestWorld;
}

void DestroyTestWorld(UWorld* TestWorld)
{
    if (TestWorld)
    {
        GEngine->DestroyWorldContext(TestWorld);
        TestWorld->DestroyWorld(false);
    }
}

bool FWindSystemComponentInitializationTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();

    // Initialize the component for testing
    WindComponent->InitializeForTesting();

    // Test initialization
    TestTrue("BaseGridSize is set", WindComponent->GetBaseGridSize() > 0);
    TestTrue("CellSize is set", WindComponent->GetCellSize() > 0.0f);
    TestTrue("SimulationFrequency is set", WindComponent->GetSimulationFrequency() > 0.0f);

    // Clean up
    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

bool FWindSystemComponentVelocityTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();

    // Initialize the component for testing
    WindComponent->InitializeForTesting();

    // Test wind velocity calculation
    FVector TestLocation(100.0f, 100.0f, 100.0f);
    FVector WindVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    // Log the initial wind velocity
    UE_LOG(LogTemp, Log, TEXT("Initial wind velocity at (%f, %f, %f): (%f, %f, %f)"),
        TestLocation.X, TestLocation.Y, TestLocation.Z,
        WindVelocity.X, WindVelocity.Y, WindVelocity.Z);

    // Check if the returned velocity is valid (not zero and not infinite)
    TestFalse("Wind velocity is not zero", WindVelocity.IsZero());
    TestFalse("Wind velocity is not infinite", WindVelocity.ContainsNaN());

    // Add wind at a specific location and test if it affects nearby locations
    FVector WindSource(200.0f, 200.0f, 200.0f);
    FVector AddedWind(10.0f, 0.0f, 0.0f);
    WindComponent->AddWindAtLocation(WindSource, AddedWind);

    // Wait for a short time to allow the simulation to process the added wind
    FPlatformProcess::Sleep(0.1f);

    FVector NewWindVelocity = WindComponent->GetWindVelocityAtLocation(WindSource);

    // Log the new wind velocity
    UE_LOG(LogTemp, Log, TEXT("New wind velocity at (%f, %f, %f) after adding wind: (%f, %f, %f)"),
        WindSource.X, WindSource.Y, WindSource.Z,
        NewWindVelocity.X, NewWindVelocity.Y, NewWindVelocity.Z);

    // Check if the added wind has any effect
    TestTrue("Added wind affects source location", !FMath::IsNearlyEqual(NewWindVelocity.X, WindVelocity.X, 0.01f));

    // Clean up
    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

bool FWindSystemComponentSimulationStepTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();

    // Initialize the component for testing
    WindComponent->InitializeForTesting();

    // Record initial state
    FVector TestLocation(100.0f, 100.0f, 100.0f);
    FVector InitialVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    UE_LOG(LogTemp, Log, TEXT("Initial velocity at (%f, %f, %f): (%f, %f, %f)"),
        TestLocation.X, TestLocation.Y, TestLocation.Z,
        InitialVelocity.X, InitialVelocity.Y, InitialVelocity.Z);

    // Perform a simulation step
    float DeltaTime = 1.0f / WindComponent->GetSimulationFrequency();
    WindComponent->SimulationStep(DeltaTime);

    // Check if the simulation step changed the wind velocity
    FVector NewVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    UE_LOG(LogTemp, Log, TEXT("New velocity after simulation step: (%f, %f, %f)"),
        NewVelocity.X, NewVelocity.Y, NewVelocity.Z);

    // We'll consider the test passed if there's any change in velocity, even if small
    TestTrue("Simulation step changes wind velocity", !NewVelocity.Equals(InitialVelocity, 0.0001f));

    // Test if multiple simulation steps lead to stabilization
    FVector PreviousVelocity = NewVelocity;
    bool stabilized = false;
    for (int i = 0; i < 100; ++i)
    {
        WindComponent->SimulationStep(DeltaTime);
        NewVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

        if (NewVelocity.Equals(PreviousVelocity, 0.0001f))
        {
            UE_LOG(LogTemp, Log, TEXT("Simulation stabilized after %d steps"), i);
            stabilized = true;
            break;
        }

        PreviousVelocity = NewVelocity;
    }

    TestTrue("Simulation stabilizes over time", stabilized);

    // Clean up
    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS