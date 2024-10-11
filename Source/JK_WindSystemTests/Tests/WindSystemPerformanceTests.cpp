#include "WindSystemTestCommon.h"
#include "Misc/Timespan.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemPerformanceTest, "JK_WindSystem.Performance.StressTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::StressFilter | EAutomationTestFlags::LowPriority)

bool FWindSystemPerformanceTest::RunTest(const FString& Parameters)
{
    UWorld* TestWorld = CreateTestWorld();
    UWindSimulationComponent* WindComponent = SetupWindSimulation(TestWorld);
    UWindSimulationSubsystem* WindSubsystem = TestWorld->GetSubsystem<UWindSimulationSubsystem>();

    if (!TestNotNull("Wind Simulation Component is created", WindComponent) ||
        !TestNotNull("Wind Subsystem exists", WindSubsystem))
    {
        DestroyTestWorld(TestWorld);
        return false;
    }

    // Performance test parameters
    const int32 NumWindGenerators = 100;
    const int32 NumSimulationSteps = 1000;
    const int32 NumSamplePoints = 1000;
    const float SimulationDeltaTime = 1.0f / 60.0f; // Simulating at 60 FPS

    // Create wind generators
    TArray<UWindGeneratorComponent*> Generators;
    for (int32 i = 0; i < NumWindGenerators; ++i)
    {
        FVector Location = FVector(FMath::RandRange(-5000.f, 5000.f), FMath::RandRange(-5000.f, 5000.f), FMath::RandRange(-5000.f, 5000.f));
        UPointWindGeneratorComponent* Generator = CreatePointWindGenerator(TestWorld, Location, FMath::RandRange(50.f, 200.f), FMath::RandRange(200.f, 1000.f));
        WindSubsystem->RegisterWindGenerator(Generator);
        Generators.Add(Generator);
    }

    // Prepare sample points
    TArray<FVector> SamplePoints;
    for (int32 i = 0; i < NumSamplePoints; ++i)
    {
        SamplePoints.Add(FVector(FMath::RandRange(-5000.f, 5000.f), FMath::RandRange(-5000.f, 5000.f), FMath::RandRange(-5000.f, 5000.f)));
    }

    // Run simulation and measure performance
    double TotalSimulationTime = 0.0;
    double TotalSamplingTime = 0.0;

    for (int32 Step = 0; Step < NumSimulationSteps; ++Step)
    {
        // Measure simulation step time
        double StartTime = FPlatformTime::Seconds();
        WindSubsystem->Tick(SimulationDeltaTime);
        double EndTime = FPlatformTime::Seconds();
        TotalSimulationTime += EndTime - StartTime;

        // Measure sampling time
        StartTime = FPlatformTime::Seconds();
        for (const FVector& SamplePoint : SamplePoints)
        {
            WindSubsystem->GetWindVelocityAtLocation(SamplePoint);
        }
        EndTime = FPlatformTime::Seconds();
        TotalSamplingTime += EndTime - StartTime;

        // Every 100 steps, log progress
        if (Step % 100 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Completed %d simulation steps"), Step);
        }
    }

    // Calculate and log performance metrics
    double AverageSimulationTime = TotalSimulationTime / NumSimulationSteps;
    double AverageSamplingTime = TotalSamplingTime / NumSimulationSteps;
    double SimulationFPS = 1.0 / AverageSimulationTime;
    double SamplesPerSecond = NumSamplePoints / AverageSamplingTime;

    UE_LOG(LogTemp, Log, TEXT("Performance Test Results:"));
    UE_LOG(LogTemp, Log, TEXT("Number of Wind Generators: %d"), NumWindGenerators);
    UE_LOG(LogTemp, Log, TEXT("Number of Simulation Steps: %d"), NumSimulationSteps);
    UE_LOG(LogTemp, Log, TEXT("Number of Sample Points: %d"), NumSamplePoints);
    UE_LOG(LogTemp, Log, TEXT("Average Simulation Time: %.6f seconds"), AverageSimulationTime);
    UE_LOG(LogTemp, Log, TEXT("Average Sampling Time: %.6f seconds"), AverageSamplingTime);
    UE_LOG(LogTemp, Log, TEXT("Simulation FPS: %.2f"), SimulationFPS);
    UE_LOG(LogTemp, Log, TEXT("Samples per Second: %.2f"), SamplesPerSecond);

    // Define performance thresholds
    const double MinAcceptableSimulationFPS = 30.0;
    const double MinAcceptableSamplesPerSecond = 100000.0; // 100k samples per second

    // Check if performance meets the thresholds
    TestTrue("Simulation FPS meets minimum threshold", SimulationFPS >= MinAcceptableSimulationFPS);
    TestTrue("Sampling performance meets minimum threshold", SamplesPerSecond >= MinAcceptableSamplesPerSecond);

    // Clean up
    for (UWindGeneratorComponent* Generator : Generators)
    {
        TestWorld->DestroyActor(Generator->GetOwner());
    }
    TestWorld->DestroyActor(WindComponent->GetOwner());
    DestroyTestWorld(TestWorld);

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS