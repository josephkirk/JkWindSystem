#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

class UWindSimulationSubsystem;
class UWindGeneratorComponent;

class FWindSystemDebugRenderer
{
public:
	static void DrawDebug(class UCanvas* Canvas, class APlayerController* PC);

private:

	struct FDebugText
	{
		FString Text;
		FLinearColor Color;
		float X;
		float Y;
		float Scale;

		FDebugText(const FString& InText, const FLinearColor& InColor = FLinearColor::White, float InX = 50.0f, float InY = 50.0f, float InScale = 1.0f)
			: Text(InText), Color(InColor), X(InX), Y(InY), Scale(InScale) {}
	};

	static void DrawDebugHeader(UCanvas* Canvas, APlayerController* PC);
	static void DrawWindSystemInfo(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem);
	static void DrawWindSourcesList(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem);
	static void DrawWindVectorGrid(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem);
	static void DrawWindGeneratorDebug(UCanvas* Canvas, UWindGeneratorComponent* WindSource);
	static float DrawDebugString(UCanvas* Canvas, const FDebugText& DebugText);

	static FString GetWindSourceDebugString(UWindGeneratorComponent* WindSource);
	static void DrawDebugWindVector(UCanvas* Canvas, const FVector& Location, const FVector& WindVelocity, const FLinearColor& Color);
    
	static FLinearColor GetWindSpeedColor(float WindSpeed);

	// Constants for visualization
	static constexpr float YPosIncrement = 15.0f;
	static constexpr float XIndent = 50.0f;
	static constexpr float MaxWindSpeed = 1000.0f; // For color normalization
	static constexpr int32 GridVisualizationSize = 5; // Number of cells to visualize in each direction
	static constexpr float ArrowScale = 0.5f;
};