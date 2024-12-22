#include "WindSystemTestCommon.h"
#include "WindDebugVisualizer.h"
#include "WindSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindSystemDebugVisualizationPerformanceTest, "JK_WindSystem.Performance.DebugVisualization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWindSystemDebugVisualizationPerformanceTest::RunTest(const FString& Parameters)
{
    // Create a test world
    UWorld* TestWorld = CreateTestWorld();
    if (!TestWorld)
    {
        AddError("Failed to create test world.");
        return false;
    }

    // Spawn a test actor
    AActor* TestActor = TestWorld->SpawnActor<AActor>();
    if (!TestActor)
    {
        AddError("Failed to spawn test actor.");
        DestroyTestWorld(TestWorld);
        return false;
    }

    // Create and initialize the WindDebugVisualizer component
    UWindDebugVisualizer* DebugVisualizer = NewObject<UWindDebugVisualizer>(TestActor);
    DebugVisualizer->RegisterComponent();
    TestActor->AddInstanceComponent(DebugVisualizer);

    // Set up the wind simulation subsystem
    UWindSimulationSubsystem* WindSubsystem = TestWorld->GetSubsystem<UWindSimulationSubsystem>();
    if (!WindSubsystem)
    {
        AddError("Failed to get WindSimulationSubsystem.");
        DestroyTestWorld(TestWorld);
        return false;
    }

    // Configure the debug visualizer
    DebugVisualizer->UpdateInterval = 0.0f; // Update every frame for the test
    DebugVisualizer->ArrowScale = 1.0f;
    DebugVisualizer->GridResolution = 10;
    DebugVisualizer->SetVisualizationExtent(FVector(1000.0f));

    const int32 TestDurationFrames = 100;
    const float DeltaTime = 1.0f / 60.0f; // Assume 60 FPS

    double TotalVisualizationTime = 0.0;

    for (int32 Frame = 0; Frame < TestDurationFrames; ++Frame)
    {
        // Simulate wind (you may need to adjust this based on your actual wind simulation)
        WindSubsystem->Tick(DeltaTime);

        // Measure debug visualization time
        double StartTime = FPlatformTime::Seconds();
        DebugVisualizer->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, nullptr);
        double EndTime = FPlatformTime::Seconds();
        double VisualizationTime = (EndTime - StartTime) * 1000.0; // Convert to milliseconds
        TotalVisualizationTime += VisualizationTime;

        // Log the time for each frame
        UE_LOG(LogTemp, Verbose, TEXT("Frame %d: Visualization Time: %.4f ms"), Frame, VisualizationTime);
    }

    double AverageVisualizationTime = TotalVisualizationTime / TestDurationFrames;
    UE_LOG(LogTemp, Log, TEXT("Average Visualization Time: %.4f ms"), AverageVisualizationTime);

    // Add your performance assertions here
    TestTrue("Average visualization time is under 16ms (60 FPS)", AverageVisualizationTime < 16.0);

    // Clean up
    DestroyTestWorld(TestWorld);

    return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS