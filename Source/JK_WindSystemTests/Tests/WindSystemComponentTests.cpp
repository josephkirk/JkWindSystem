#include "WindSystemTestCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentInitializationTest, "JK_WindSystem.Component.Initialization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentVelocityTest, "JK_WindSystem.Component.VelocityCalculation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemComponentSimulationStepTest, "JK_WindSystem.Component.SimulationStep", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWindSystemComponentInitializationTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();
    
    UWindSimulationComponent* WindComponent = SetupWindSimulation(TestWorld);

    // Test initialization
    TestTrue("WindComponent is valid", IsValid(WindComponent));
    TestTrue("GridSize is set", WindComponent->GetGridSize() > 0);
    TestTrue("CellSize is set", WindComponent->GetCellSize() > 0.0f);
    TestTrue("SimulationFrequency is set", WindComponent->GetSimulationFrequency() > 0.0f);

    // Test if WindGrid is initialized
    FVector TestLocation(100.0f, 100.0f, 100.0f);
    FVector WindVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);
    TestFalse("WindGrid is initialized (non-zero velocity returned)", WindVelocity.IsZero());

    // Clean up
    TestWorld->DestroyActor(WindComponent->GetOwner());
    DestroyTestWorld(TestWorld);
    
    return true;
}

bool FWindSystemComponentVelocityTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();
    
    UWindSimulationComponent* WindComponent = SetupWindSimulation(TestWorld);

    // Ensure WindComponent is valid
    if (!TestNotNull("WindComponent is valid", WindComponent))
    {
        DestroyTestWorld(TestWorld);
        return false;
    }

    // Test wind velocity calculation
    FVector TestLocation(100.0f, 100.0f, 100.0f);
    FVector WindVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    // Check if the returned velocity is valid (not zero and not infinite)
    TestFalse("Wind velocity is not zero", WindVelocity.IsZero());
    TestFalse("Wind velocity is not infinite", WindVelocity.ContainsNaN());

    // Add wind at a specific location and test if it affects nearby locations
    FVector WindSource(200.0f, 200.0f, 200.0f);
    FVector AddedWind(10.0f, 0.0f, 0.0f);
    WindComponent->AddWindAtLocation(WindSource, AddedWind);

    // Perform a simulation step to process the added wind
    WindComponent->SimulationStep(WindTestConstants::DEFAULT_SIMULATION_DELTA_TIME);

    FVector NewWindVelocity = WindComponent->GetWindVelocityAtLocation(WindSource);
    TestTrue("Added wind affects source location", !FMath::IsNearlyEqual(NewWindVelocity.X, WindVelocity.X, 0.01f));

    // Log wind velocities for debugging
    UE_LOG(LogTemp, Log, TEXT("Initial wind velocity: %s"), *WindVelocity.ToString());
    UE_LOG(LogTemp, Log, TEXT("Wind velocity after adding wind: %s"), *NewWindVelocity.ToString());

    // Clean up
    TestWorld->DestroyActor(WindComponent->GetOwner());
    DestroyTestWorld(TestWorld);
    
    return true;
}

bool FWindSystemComponentSimulationStepTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();
    
    UWindSimulationComponent* WindComponent = SetupWindSimulation(TestWorld);

    // Ensure WindComponent is valid
    if (!TestNotNull("WindComponent is valid", WindComponent))
    {
        DestroyTestWorld(TestWorld);
        return false;
    }

    // Record initial state
    FVector TestLocation(100.0f, 100.0f, 100.0f);
    FVector InitialVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    UE_LOG(LogTemp, Log, TEXT("Initial velocity at test point: %s"), *InitialVelocity.ToString());

    // Perform simulation steps
    for (int32 i = 0; i < WindTestConstants::DEFAULT_SIMULATION_STEPS; ++i)
    {
        WindComponent->SimulationStep(WindTestConstants::DEFAULT_SIMULATION_DELTA_TIME);
    }

    // Check if the simulation step changed the wind velocity
    FVector NewVelocity = WindComponent->GetWindVelocityAtLocation(TestLocation);

    UE_LOG(LogTemp, Log, TEXT("New velocity after simulation steps: %s"), *NewVelocity.ToString());

    // We'll consider the test passed if there's any change in velocity, even if small
    TestTrue("Simulation step changes wind velocity", !NewVelocity.Equals(InitialVelocity, 0.0001f));

    // Test if multiple simulation steps lead to stabilization
    FVector PreviousVelocity = NewVelocity;
    bool stabilized = false;
    for (int i = 0; i < 100; ++i)
    {
        WindComponent->SimulationStep(WindTestConstants::DEFAULT_SIMULATION_DELTA_TIME);
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
    TestWorld->DestroyActor(WindComponent->GetOwner());
    DestroyTestWorld(TestWorld);
    
    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS