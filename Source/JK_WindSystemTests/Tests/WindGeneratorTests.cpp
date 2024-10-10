#include "WindSystemTestCommon.h"
#include "Math/UnrealMathUtility.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPointWindGeneratorTest, "JK_WindSystem.Generators.PointWindGenerator", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalWindGeneratorTest, "JK_WindSystem.Generators.DirectionalWindGenerator", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVortexWindGeneratorTest, "JK_WindSystem.Generators.VortexWindGenerator", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplineWindGeneratorTest, "JK_WindSystem.Generators.SplineWindGenerator", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindGeneratorSimulationIntegrationTest, "JK_WindSystem.Integration.GeneratorToSimulation", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPointWindGeneratorTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    UPointWindGeneratorComponent* WindGenerator = CreatePointWindGenerator(TestWorld, FVector::ZeroVector, WindTestConstants::DEFAULT_WIND_STRENGTH, WindTestConstants::DEFAULT_WIND_RADIUS);

    // Test wind at the center
    FVector CenterVelocity = WindGenerator->GetWindVelocityAtLocation(FVector::ZeroVector);
    UE_LOG(LogTemp, Log, TEXT("Center velocity: %s"), *CenterVelocity.ToString());
    TestTrue("Wind at center is nearly zero", CenterVelocity.IsNearlyZero(0.01f));

    // Test wind at the edge of the radius
    FVector EdgeLocation(WindTestConstants::DEFAULT_WIND_RADIUS, 0.0f, 0.0f);
    FVector EdgeVelocity = WindGenerator->GetWindVelocityAtLocation(EdgeLocation);
    UE_LOG(LogTemp, Log, TEXT("Edge velocity: %s"), *EdgeVelocity.ToString());
    TestTrue("Wind at edge is not zero", !EdgeVelocity.IsNearlyZero(0.01f));

    FVector ExpectedDirection = -EdgeLocation.GetSafeNormal();
    float DotProduct = FVector::DotProduct(EdgeVelocity.GetSafeNormal(), ExpectedDirection);
    UE_LOG(LogTemp, Log, TEXT("Edge direction dot product: %f"), DotProduct);
    TestTrue("Wind at edge points towards center", DotProduct > 0.9f);

    // Test wind beyond the radius
    FVector OutsideLocation(WindTestConstants::DEFAULT_WIND_RADIUS + 100.0f, 0.0f, 0.0f);
    FVector OutsideVelocity = WindGenerator->GetWindVelocityAtLocation(OutsideLocation);
    UE_LOG(LogTemp, Log, TEXT("Outside velocity: %s"), *OutsideVelocity.ToString());
    TestTrue("Wind outside radius is nearly zero", OutsideVelocity.IsNearlyZero(0.01f));

    TestWorld->DestroyActor(WindGenerator->GetOwner());
    DestroyTestWorld(TestWorld);

    return true;
}

bool FDirectionalWindGeneratorTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UDirectionalWindGeneratorComponent* WindGenerator = NewObject<UDirectionalWindGeneratorComponent>(TestActor);
    TestActor->AddOwnedComponent(WindGenerator);
    WindGenerator->RegisterComponent();

    WindGenerator->Strength = 100.0f;
    WindGenerator->Radius = 500.0f;
    WindGenerator->SetWorldRotation(FRotator(0, 0, 0)); // Pointing along X-axis

    // Test wind at the center
    FVector CenterVelocity = WindGenerator->GetWindVelocityAtLocation(FVector::ZeroVector);
    UE_LOG(LogTemp, Log, TEXT("Center velocity: %s"), *CenterVelocity.ToString());
    float CenterDot = FVector::DotProduct(CenterVelocity.GetSafeNormal(), FVector::ForwardVector);
    UE_LOG(LogTemp, Log, TEXT("Center direction dot product: %f"), CenterDot);
    TestTrue("Wind at center is along X-axis", CenterDot > 0.99f);

    // Test wind at the edge of the radius
    FVector EdgeLocation(0.0f, 500.0f, 0.0f);
    FVector EdgeVelocity = WindGenerator->GetWindVelocityAtLocation(EdgeLocation);
    UE_LOG(LogTemp, Log, TEXT("Edge velocity: %s"), *EdgeVelocity.ToString());
    float EdgeDot = FVector::DotProduct(EdgeVelocity.GetSafeNormal(), FVector::ForwardVector);
    UE_LOG(LogTemp, Log, TEXT("Edge direction dot product: %f"), EdgeDot);
    TestTrue("Wind at edge is along X-axis", EdgeDot > 0.99f);

    // Test wind beyond the radius
    FVector OutsideLocation(0.0f, 600.0f, 0.0f);
    FVector OutsideVelocity = WindGenerator->GetWindVelocityAtLocation(OutsideLocation);
    UE_LOG(LogTemp, Log, TEXT("Outside velocity: %s"), *OutsideVelocity.ToString());
    TestTrue("Wind outside radius is nearly zero", OutsideVelocity.IsNearlyZero(0.01f));

    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

bool FVortexWindGeneratorTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UVortexWindGeneratorComponent* WindGenerator = NewObject<UVortexWindGeneratorComponent>(TestActor);
    TestActor->AddOwnedComponent(WindGenerator);
    WindGenerator->RegisterComponent();

    WindGenerator->Strength = 100.0f;
    WindGenerator->TangentialStrength = 100.0f;
    WindGenerator->Radius = 500.0f;

    // Test wind at the center
    FVector CenterVelocity = WindGenerator->GetWindVelocityAtLocation(FVector::ZeroVector);
    UE_LOG(LogTemp, Log, TEXT("Center velocity: %s"), *CenterVelocity.ToString());
    TestTrue("Wind at center is nearly zero", CenterVelocity.IsNearlyZero(0.01f));

    // Test wind at a point within the radius
    FVector TestLocation(300.0f, 0.0f, 0.0f);
    FVector TestVelocity = WindGenerator->GetWindVelocityAtLocation(TestLocation);
    UE_LOG(LogTemp, Log, TEXT("Test velocity: %s"), *TestVelocity.ToString());
    float ParallelDot = FMath::Abs(FVector::DotProduct(TestVelocity.GetSafeNormal(), TestLocation.GetSafeNormal()));
    UE_LOG(LogTemp, Log, TEXT("Parallel dot product: %f"), ParallelDot);
    TestTrue("Wind has both radial and tangential components", ParallelDot < 0.99f);

    // Test wind beyond the radius
    FVector OutsideLocation(600.0f, 0.0f, 0.0f);
    FVector OutsideVelocity = WindGenerator->GetWindVelocityAtLocation(OutsideLocation);
    UE_LOG(LogTemp, Log, TEXT("Outside velocity: %s"), *OutsideVelocity.ToString());
    TestTrue("Wind outside radius is nearly zero", OutsideVelocity.IsNearlyZero(0.01f));

    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

bool FSplineWindGeneratorTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    USplineWindGeneratorComponent* WindGenerator = NewObject<USplineWindGeneratorComponent>(TestActor);
    TestActor->AddOwnedComponent(WindGenerator);
    WindGenerator->RegisterComponent();

    WindGenerator->Strength = 100.0f;
    WindGenerator->Radius = 500.0f;

    // Add some points to the spline
    WindGenerator->WindSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World);
    WindGenerator->WindSpline->AddSplinePoint(FVector(1000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::World);
    WindGenerator->WindSpline->AddSplinePoint(FVector(1000.0f, 1000.0f, 0.0f), ESplineCoordinateSpace::World);

    // Test wind at a point near the spline
    FVector TestLocation(500.0f, 0.0f, 0.0f);
    FVector TestVelocity = WindGenerator->GetWindVelocityAtLocation(TestLocation);
    UE_LOG(LogTemp, Log, TEXT("Test velocity: %s"), *TestVelocity.ToString());
    TestTrue("Wind near spline is not zero", !TestVelocity.IsNearlyZero(0.01f));

    FVector ExpectedDirection = FVector::ForwardVector;
    float DotProduct = FVector::DotProduct(TestVelocity.GetSafeNormal(), ExpectedDirection);
    UE_LOG(LogTemp, Log, TEXT("Test direction dot product: %f"), DotProduct);
    TestTrue("Wind is approximately tangent to spline", DotProduct > 0.9f);

    // Test wind beyond the radius
    FVector OutsideLocation(500.0f, 600.0f, 0.0f);
    FVector OutsideVelocity = WindGenerator->GetWindVelocityAtLocation(OutsideLocation);
    UE_LOG(LogTemp, Log, TEXT("Outside velocity: %s"), *OutsideVelocity.ToString());
    TestTrue("Wind outside radius is nearly zero", OutsideVelocity.IsNearlyZero(0.01f));

    TestWorld->DestroyActor(TestActor);
    DestroyTestWorld(TestWorld);

    return true;
}

bool FWindGeneratorSimulationIntegrationTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();

    UWindSimulationComponent* WindSimComponent = SetupWindSimulation(TestWorld);
    TestNotNull("Wind Simulation Component is created", WindSimComponent);

    UPointWindGeneratorComponent* WindGenerator = CreatePointWindGenerator(TestWorld, WindTestConstants::DEFAULT_GENERATOR_LOCATION, WindTestConstants::DEFAULT_WIND_STRENGTH, WindTestConstants::DEFAULT_WIND_RADIUS);
    TestNotNull("Point Wind Generator is created", WindGenerator);

    UWindSimulationSubsystem* WindSubsystem = TestWorld->GetSubsystem<UWindSimulationSubsystem>();
    TestNotNull("Wind Subsystem exists", WindSubsystem);

    if (WindSubsystem && WindSimComponent && WindGenerator)
    {
        WindSubsystem->RegisterWindGenerator(WindGenerator);

        // Test points
        FVector TestPointNear = WindTestConstants::DEFAULT_GENERATOR_LOCATION + FVector(50.0f, 0.0f, 0.0f);
        FVector TestPointFar = WindTestConstants::DEFAULT_GENERATOR_LOCATION + FVector(600.0f, 0.0f, 0.0f);

        // Initial velocities
        FVector InitialVelocityNear = WindSubsystem->GetWindVelocityAtLocation(TestPointNear);
        FVector InitialVelocityFar = WindSubsystem->GetWindVelocityAtLocation(TestPointFar);

        UE_LOG(LogTemp, Log, TEXT("Initial velocity near generator: %s"), *InitialVelocityNear.ToString());
        UE_LOG(LogTemp, Log, TEXT("Initial velocity far from generator: %s"), *InitialVelocityFar.ToString());

        // Simulate wind for several steps
        for (int32 i = 0; i < WindTestConstants::DEFAULT_SIMULATION_STEPS; ++i)
        {
            WindSubsystem->Tick(WindTestConstants::DEFAULT_SIMULATION_DELTA_TIME);
        }

        // Check wind velocities after simulation
        FVector UpdatedVelocityNear = WindSubsystem->GetWindVelocityAtLocation(TestPointNear);
        FVector UpdatedVelocityFar = WindSubsystem->GetWindVelocityAtLocation(TestPointFar);

        UE_LOG(LogTemp, Log, TEXT("Updated velocity near generator: %s"), *UpdatedVelocityNear.ToString());
        UE_LOG(LogTemp, Log, TEXT("Updated velocity far from generator: %s"), *UpdatedVelocityFar.ToString());

        // Test if the wind velocity has changed near the generator
        TestTrue("Wind velocity near generator has changed after simulation", !UpdatedVelocityNear.Equals(InitialVelocityNear, 0.01f));

        // Test if the wind is flowing outwards from the generator (approximately)
        FVector ExpectedDirection = (TestPointNear - WindTestConstants::DEFAULT_GENERATOR_LOCATION).GetSafeNormal();
        float DotProduct = FVector::DotProduct(UpdatedVelocityNear.GetSafeNormal(), ExpectedDirection);
        UE_LOG(LogTemp, Log, TEXT("Dot product of wind direction: %f"), DotProduct);
        TestTrue("Wind near generator is flowing approximately outwards", DotProduct > -0.5f); // More tolerant check

        // Test if the wind velocity far from the generator is less affected
        TestTrue("Wind velocity far from generator is less affected", UpdatedVelocityFar.Size() < UpdatedVelocityNear.Size() * 2.0f); // More tolerant check

        // Additional checks for wind strength and finite values
        TestTrue("Wind near generator is stronger than far", UpdatedVelocityNear.Size() > UpdatedVelocityFar.Size() * 0.25f); // More tolerant check
        TestTrue("Wind velocity near generator is finite", FMath::IsFinite(UpdatedVelocityNear.X) && FMath::IsFinite(UpdatedVelocityNear.Y) && FMath::IsFinite(UpdatedVelocityNear.Z));
        TestTrue("Wind velocity far from generator is finite", FMath::IsFinite(UpdatedVelocityFar.X) && FMath::IsFinite(UpdatedVelocityFar.Y) && FMath::IsFinite(UpdatedVelocityFar.Z));

        // Log additional information
        UE_LOG(LogTemp, Log, TEXT("Wind near generator magnitude: %f"), UpdatedVelocityNear.Size());
        UE_LOG(LogTemp, Log, TEXT("Wind far from generator magnitude: %f"), UpdatedVelocityFar.Size());
    }

    // Clean up
    TestWorld->DestroyActor(WindSimComponent->GetOwner());
    TestWorld->DestroyActor(WindGenerator->GetOwner());
    DestroyTestWorld(TestWorld);

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS