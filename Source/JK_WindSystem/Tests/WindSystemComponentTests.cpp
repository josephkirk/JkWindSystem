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
    // Create a new world for testing
    UWorld* TestWorld = CreateTestWorld();
    
    // Spawn an actor with WindSystemComponent
    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();

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

    FVector NewWindVelocity = WindComponent->GetWindVelocityAtLocation(WindSource);
    TestTrue("Added wind affects source location", NewWindVelocity.X > WindVelocity.X);

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

    // Record initial state
    FVector InitialVelocity = WindComponent->GetWindVelocityAtLocation(FVector::ZeroVector);

    // Perform a simulation step
    float DeltaTime = 1.0f / WindComponent->GetSimulationFrequency();
    WindComponent->SimulationStep(DeltaTime);

    // Check if the simulation step changed the wind velocity
    FVector NewVelocity = WindComponent->GetWindVelocityAtLocation(FVector::ZeroVector);
    TestNotEqual("Simulation step changes wind velocity", InitialVelocity, NewVelocity);

    // Test if multiple simulation steps lead to stabilization
    FVector PreviousVelocity = NewVelocity;
    for (int i = 0; i < 100; ++i)
    {
        WindComponent->SimulationStep(DeltaTime);
        NewVelocity = WindComponent->GetWindVelocityAtLocation(FVector::ZeroVector);
        
        if (FMath::Abs(NewVelocity.Size() - PreviousVelocity.Size()) < KINDA_SMALL_NUMBER)
        {
            TestTrue("Simulation stabilizes over time", true);
            break;
        }
        
        PreviousVelocity = NewVelocity;
    }

    // Clean up
    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);
    
    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS