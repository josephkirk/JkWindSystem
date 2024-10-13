#include "WindSystemTestCommon.h"
#include "Misc/Timespan.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS
CSV_DEFINE_CATEGORY(WindSystem, true);
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemLargeScalePerformanceTest, "JK_WindSystem.Performance.1KmCubeUnder2ms", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::PerfFilter | EAutomationTestFlags::HighPriority)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemStressTest, "JK_WindSystem.Performance.StressTest", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::PerfFilter | EAutomationTestFlags::LowPriority)
struct FTestConfiguration
{
    int32 GridSize;
    float CellSize;
    int32 NumWindSources;
    float SimulationDuration;
};

float RunStressTest(UWorld* TestWorld, const FTestConfiguration& Config)
{
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(WindSystemStressTest);
    UWindSystemSettings* WindSettings = GetMutableDefault<UWindSystemSettings>();
    if (!WindSettings)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WindSystemSettings"));
        return -1.0f;
    }

    // Set test settings
    WindSettings->GridSize = Config.GridSize;
    WindSettings->CellSize = Config.CellSize;
    WindSettings->PostEditChange();

    // Create and initialize WindSimulationComponent
    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();
    WindComponent->InitializeForTesting();

    UWindSimulationSubsystem* WindSubsystem = TestWorld->GetSubsystem<UWindSimulationSubsystem>();
    if (!WindSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WindSimulationSubsystem"));
        return -1.0f;
    }

    // Create wind sources
    TArray<UWindGeneratorComponent*> WindSources;
    for (int32 i = 0; i < Config.NumWindSources; ++i)
    {
        FVector Location(
            FMath::RandRange(0.0f, Config.GridSize * Config.CellSize),
            FMath::RandRange(0.0f, Config.GridSize * Config.CellSize),
            FMath::RandRange(0.0f, Config.GridSize * Config.CellSize)
        );
        UPointWindGeneratorComponent* WindSource = CreatePointWindGenerator(TestWorld, Location, 50.0f, 200.0f);
        WindSubsystem->RegisterWindGenerator(WindSource);
        WindSources.Add(WindSource);
    }

    // Warm-up
    for (int32 i = 0; i < 10; ++i)
    {
        WindSubsystem->Tick(1.0f / 60.0f);
    }

    // Stress test
    double TotalTime = 0.0;
    int32 NumIterations = FMath::CeilToInt(Config.SimulationDuration * 60.0f); // Assuming 60 FPS
    for (int32 i = 0; i < NumIterations; ++i)
    {
        double StartTime = FPlatformTime::Seconds();
        WindSubsystem->Tick(1.0f / 60.0f);
        double EndTime = FPlatformTime::Seconds();
        TotalTime += (EndTime - StartTime) * 1000.0; // Convert to milliseconds

        CSV_CUSTOM_STAT(WindSystem, GridSize, Config.GridSize * Config.GridSize * Config.GridSize, ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(WindSystem, NumWindSources, Config.NumWindSources, ECsvCustomStatOp::Set);
    }

    // Clean up
    for (UWindGeneratorComponent* WindSource : WindSources)
    {
        WindSubsystem->UnregisterWindGenerator(WindSource);
        TestWorld->DestroyActor(WindSource->GetOwner());
    }
    TestWorld->DestroyActor(TestActor);

    return TotalTime / NumIterations;
}

bool FWindSystemStressTest::RunTest(const FString& Parameters)
{
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(WindSystemFullStressTest);
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(TestWorld);

    UWindSystemSettings* OriginalSettings = GetMutableDefault<UWindSystemSettings>();
    int32 OriginalGridSize = OriginalSettings->GridSize;
    float OriginalCellSize = OriginalSettings->CellSize;

    TArray<FTestConfiguration> TestConfigurations = {
        {64, 15.625f, 10, 5.0f},   // 1km�, 10 sources, 5 seconds
        {128, 7.8125f, 50, 5.0f},  // 1km�, 50 sources, 5 seconds
        {256, 3.90625f, 100, 5.0f},// 1km�, 100 sources, 5 seconds
        {512, 1.953125f, 200, 2.0f}// 1km�, 200 sources, 2 seconds (might be very intensive)
    };

    bool TestsPassed = true;

    for (const auto& Config : TestConfigurations)
    {
        CSV_SCOPED_TIMING_STAT_EXCLUSIVE(WindSystemConfigurationTest);
        float AverageTime = RunStressTest(TestWorld, Config);

        UE_LOG(LogTemp, Log, TEXT("Stress Test Results:"));
        UE_LOG(LogTemp, Log, TEXT("Grid Size: %d, Cell Size: %.6f, Wind Sources: %d"), Config.GridSize, Config.CellSize, Config.NumWindSources);
        UE_LOG(LogTemp, Log, TEXT("Simulation Duration: %.2f seconds"), Config.SimulationDuration);
        UE_LOG(LogTemp, Log, TEXT("Average Time per Tick: %.4f ms"), AverageTime);
        UE_LOG(LogTemp, Log, TEXT("Cells processed per second: %.2f million"),
            (Config.GridSize * Config.GridSize * Config.GridSize) / (AverageTime / 1000.0) / 1000000.0);

        // You can define different performance targets based on the configuration
        float PerformanceTarget = (Config.GridSize <= 256) ? 16.67f : 33.33f; // 60 FPS for smaller grids, 30 FPS for larger
        bool ConfigPassed = AverageTime < PerformanceTarget;
        TestsPassed &= ConfigPassed;
        CSV_CUSTOM_STAT(WindSystem, WindSystemStressAverageTickTime, AverageTime, ECsvCustomStatOp::Set);
        TestTrue(FString::Printf(TEXT("Configuration %dx%dx%d under %.2f ms"), Config.GridSize, Config.GridSize, Config.GridSize, PerformanceTarget), ConfigPassed);
    }

    // Restore original settings
    OriginalSettings->GridSize = OriginalGridSize;
    OriginalSettings->CellSize = OriginalCellSize;
    OriginalSettings->PostEditChange();

    // Clean up
    GEngine->DestroyWorldContext(TestWorld);
    TestWorld->DestroyWorld(false);

    return TestsPassed;
}

bool FWindSystemLargeScalePerformanceTest::RunTest(const FString& Parameters)
{
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(WindSystemFullPerformanceTest);
    // Create a test world
    UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    WorldContext.SetCurrentWorld(TestWorld);

    // Get the WindSystemSettings
    UWindSystemSettings* WindSettings = GetMutableDefault<UWindSystemSettings>();
    TestNotNull("Wind System Settings exist", WindSettings);

    if (!WindSettings)
    {
        return false;
    }

    // Store original settings
    const int32 OriginalGridSize = WindSettings->GridSize;
    const float OriginalCellSize = WindSettings->CellSize;

    // Set new test settings
    WindSettings->GridSize = 1024; // Example test size
    WindSettings->CellSize = 100.0f; // This makes the total volume 1km^3 =
    WindSettings->PostEditChange(); // Notify that we've changed the settings

    // Create the WindSimulationComponent
    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    UWindSimulationComponent* WindComponent = NewObject<UWindSimulationComponent>(TestActor);
    TestActor->AddOwnedComponent(WindComponent);
    WindComponent->RegisterComponent();

    // Initialize the wind system
    WindComponent->InitializeForTesting();

    // Get the actual grid size and cell size
    const int32 GridSize = WindSettings->GridSize;
    const float CellSize = WindSettings->CellSize;

    // Calculate the actual volume simulated
    const float SimulatedVolume = GridSize * CellSize;

    // Get the WindSubsystem
    UWindSimulationSubsystem* WindSubsystem = TestWorld->GetSubsystem<UWindSimulationSubsystem>();
    TestNotNull("Wind Subsystem exists", WindSubsystem);

    if (WindSubsystem)
    {
        // Add some wind sources to simulate a more realistic scenario
        const int32 NumWindSources = 10;
        TArray<UWindGeneratorComponent*> WindSources;
        for (int32 i = 0; i < NumWindSources; ++i)
        {
            FVector Location(
                FMath::RandRange(0.0f, SimulatedVolume),
                FMath::RandRange(0.0f, SimulatedVolume),
                FMath::RandRange(0.0f, SimulatedVolume)
            );
            UPointWindGeneratorComponent* WindSource = CreatePointWindGenerator(TestWorld, Location, 50.0f, 200.0f);
            WindSubsystem->RegisterWindGenerator(WindSource);
            WindSources.Add(WindSource);
        }

        // Warm-up run
        for (int32 i = 0; i < 10; ++i)
        {
            WindSubsystem->Tick(1.0f / 60.0f); // Simulate at 60 FPS
        }

        // Performance test
        const int32 NumIterations = 100;
        double TotalTime = 0.0;

        for (int32 i = 0; i < NumIterations; ++i)
        {
            double StartTime = FPlatformTime::Seconds();

            WindSubsystem->Tick(1.0f / 60.0f);

            double EndTime = FPlatformTime::Seconds();
            TotalTime += (EndTime - StartTime) * 1000.0; // Convert to milliseconds
        }

        double AverageTime = TotalTime / NumIterations;

        UE_LOG(LogTemp, Log, TEXT("Wind System Large Scale Performance Test Results:"));
        UE_LOG(LogTemp, Log, TEXT("Grid Size: %d x %d x %d"), GridSize, GridSize, GridSize);
        UE_LOG(LogTemp, Log, TEXT("Cell Size: %.2f meters"), CellSize/100.0f);
        UE_LOG(LogTemp, Log, TEXT("Simulated Volume: %.2f meters cubed"), SimulatedVolume/100.0f);
        UE_LOG(LogTemp, Log, TEXT("Number of Wind Sources: %d"), NumWindSources);
        UE_LOG(LogTemp, Log, TEXT("Average Time per Tick: %.4f ms"), AverageTime);

        // Check if the average time is under 2ms
        TestTrue("Average tick time is under 2ms", AverageTime < 2.0);
        CSV_CUSTOM_STAT(WindSystem, WindSystemPerfAverageTickTime, AverageTime, ECsvCustomStatOp::Set);
        // Additional performance metrics
        UE_LOG(LogTemp, Log, TEXT("Total cells: %d"), GridSize * GridSize * GridSize);
        UE_LOG(LogTemp, Log, TEXT("Cells processed per second: %.2f million"),
            (GridSize * GridSize * GridSize) / (AverageTime / 1000.0) / 1000000.0);

        // Clean up wind sources
        for (UWindGeneratorComponent* WindSource : WindSources)
        {
            WindSubsystem->UnregisterWindGenerator(WindSource);
            TestWorld->DestroyActor(WindSource->GetOwner());
        }
    }

    // Clean up
    TestWorld->DestroyActor(TestActor);
    GEngine->DestroyWorldContext(TestWorld);
    TestWorld->DestroyWorld(false);

    // Restore original settings
    WindSettings->GridSize = OriginalGridSize;
    WindSettings->CellSize = OriginalCellSize;
    WindSettings->PostEditChange(); // Notify that we've changed the settings back

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS