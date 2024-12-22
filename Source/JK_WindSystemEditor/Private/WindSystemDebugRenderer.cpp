#include "WindSystemDebugRenderer.h"
#include "WindSubsystem.h"
#include "WindSourceComponent.h"
#include "WindSystemActor.h"
#include "WindSystemComponent.h"

#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Debug/DebugDrawService.h"

void FWindSystemDebugRenderer::DrawDebug(UCanvas* Canvas, APlayerController* PC)
{
    if (!Canvas || !PC || !PC->GetWorld())
    {
        return;
    }

    UWindSimulationSubsystem* WindSystem = PC->GetWorld()->GetSubsystem<UWindSimulationSubsystem>();
    if (!WindSystem || !WindSystem->GetWindSystemActor())
    {
        DrawDebugString(Canvas, FDebugText("No Wind System Available", FLinearColor::Red));
        return;
    }

    DrawDebugHeader(Canvas, PC);
    DrawWindSystemInfo(Canvas, WindSystem);
    DrawWindSourcesList(Canvas, WindSystem);
    DrawWindVectorGrid(Canvas, WindSystem);
}

void FWindSystemDebugRenderer::DrawDebugHeader(UCanvas* Canvas, APlayerController* PC)
{
    float YPos = 50.0f;
    YPos += DrawDebugString(Canvas, FDebugText("WIND SYSTEM DEBUG", FLinearColor::Yellow, XIndent, YPos));
    YPos += DrawDebugString(Canvas, FDebugText("===============", FLinearColor::Yellow, XIndent, YPos));
}

void FWindSystemDebugRenderer::DrawWindSystemInfo(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem)
{
    float YPos = 90.0f;

    // Get player-centric info
    APawn* PlayerPawn = Cast<APawn>(WindSystem->GetWorld()->GetFirstPlayerController()->GetPawn());
    FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
    FVector WindAtPlayer = WindSystem->GetWindVelocityAtLocation(PlayerLocation);
    float WindSpeed = WindAtPlayer.Size();

    UWindSimulationComponent* WindComponent = WindSystem->GetWindSystemActor()->WindSimulationComponent;

    // System Info
    YPos += DrawDebugString(Canvas, FDebugText("System Information:", FLinearColor::Green, XIndent, YPos));
    YPos += DrawDebugString(Canvas, FDebugText(
        FString::Printf(TEXT("Grid: %dx%dx%d, Cell Size: %.1f, Frequency: %.1f Hz"),
            WindComponent->GetGridSize(),
            WindComponent->GetGridSize(),
            WindComponent->GetGridSize(),
            WindComponent->GetCellSize(),
            WindComponent->GetSimulationFrequency()),
        FLinearColor::White, XIndent + 10.0f, YPos));

    // Player Wind Info
    YPos += YPosIncrement;
    YPos += DrawDebugString(Canvas, FDebugText("Wind at Player:", FLinearColor::Green, XIndent, YPos));
    YPos += DrawDebugString(Canvas, FDebugText(
        FString::Printf(TEXT("Speed: %.1f, Direction: %s"),
            WindSpeed,
            *WindAtPlayer.GetSafeNormal().ToString()),
        GetWindSpeedColor(WindSpeed), XIndent + 10.0f, YPos));
}

void FWindSystemDebugRenderer::DrawWindSourcesList(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem)
{
    float YPos = 170.0f;
    int32 SourceCount = 0;

    YPos += DrawDebugString(Canvas, FDebugText("Wind Sources:", FLinearColor::Green, XIndent, YPos));

    // Iterate through all wind sources
    for (TObjectIterator<UWindGeneratorComponent> It; It; ++It)
    {
        if (UWindGeneratorComponent* WindSource = *It)
        {
            if (WindSource->GetWorld() == WindSystem->GetWorld())
            {
                YPos += DrawDebugString(Canvas, FDebugText(
                    GetWindSourceDebugString(WindSource),
                    FLinearColor::White, XIndent + 10.0f, YPos));
                
                DrawWindGeneratorDebug(Canvas, WindSource);
                SourceCount++;
            }
        }
    }

    if (SourceCount == 0)
    {
        YPos += DrawDebugString(Canvas, FDebugText("No wind sources found", FLinearColor::Red, XIndent + 10.0f, YPos));
    }
}

void FWindSystemDebugRenderer::DrawWindVectorGrid(UCanvas* Canvas, UWindSimulationSubsystem* WindSystem)
{
    APawn* PlayerPawn = Cast<APawn>(WindSystem->GetWorld()->GetFirstPlayerController()->GetPawn());
    if (!PlayerPawn)
    {
        return;
    }

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float CellSize = WindSystem->GetWindSystemActor()->WindSimulationComponent->GetCellSize();

    // Draw grid of wind vectors around player
    for (int32 x = -GridVisualizationSize; x <= GridVisualizationSize; x++)
    {
        for (int32 y = -GridVisualizationSize; y <= GridVisualizationSize; y++)
        {
            for (int32 z = -GridVisualizationSize; z <= GridVisualizationSize; z++)
            {
                FVector SamplePoint = PlayerLocation + FVector(x, y, z) * CellSize;
                FVector WindVelocity = WindSystem->GetWindVelocityAtLocation(SamplePoint);
                
                if (!WindVelocity.IsNearlyZero())
                {
                    DrawDebugWindVector(Canvas, SamplePoint, WindVelocity, GetWindSpeedColor(WindVelocity.Size()));
                }
            }
        }
    }
}

void FWindSystemDebugRenderer::DrawWindGeneratorDebug(UCanvas* Canvas, UWindGeneratorComponent* WindSource)
{
    // Draw influence radius
    FVector Location = WindSource->GetComponentLocation();
    float Radius = WindSource->Radius;

    if (auto* DirectionalWind = Cast<UDirectionalWindGeneratorComponent>(WindSource))
    {
        // Draw direction cone/cylinder
        FVector Direction = DirectionalWind->GetForwardVector();
        if (DirectionalWind->ShapeType == EWindShapeType::Cone)
        {
            // Draw cone visualization
            float ConeAngle = DirectionalWind->ConeAngle;
            // Add debug cone drawing here
        }
    }
    else if (auto* VortexWind = Cast<UVortexWindGeneratorComponent>(WindSource))
    {
        // Draw vortex visualization
        // Add vortex debug visualization here
    }
    else if (auto* SplineWind = Cast<USplineWindGeneratorComponent>(WindSource))
    {
        // Draw spline path
        if (SplineWind->WindSpline)
        {
            // Add spline debug visualization here
        }
    }
}

FString FWindSystemDebugRenderer::GetWindSourceDebugString(UWindGeneratorComponent* WindSource)
{
    FString TypeName;
    if (auto* Point = Cast<UPointWindGeneratorComponent>(WindSource))
    {
        TypeName = TEXT("Point");
    }
    else if (auto* Directional = Cast<UDirectionalWindGeneratorComponent>(WindSource))
    {
        TypeName = TEXT("Directional");
    }
    else if (auto* Vortex = Cast<UVortexWindGeneratorComponent>(WindSource))
    {
        TypeName = TEXT("Vortex");
    }
    else if (auto* Spline = Cast<USplineWindGeneratorComponent>(WindSource))
    {
        TypeName = TEXT("Spline");
    }
    
    return FString::Printf(TEXT("%s Wind (Str: %.1f, Rad: %.1f) @ %s"),
        *TypeName,
        WindSource->Strength,
        WindSource->Radius,
        *WindSource->GetComponentLocation().ToString());
}

void FWindSystemDebugRenderer::DrawDebugWindVector(UCanvas* Canvas, const FVector& Location, const FVector& WindVelocity, const FLinearColor& Color)
{
    APlayerController* PC = Canvas->GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    FVector2D ScreenLocation;
    if (PC->ProjectWorldLocationToScreen(Location, ScreenLocation))
    {
        FVector EndPoint = Location + WindVelocity * ArrowScale;
        FVector2D ScreenEndPoint;
        
        if (PC->ProjectWorldLocationToScreen(EndPoint, ScreenEndPoint))
        {
            // Draw arrow line
            Canvas->K2_DrawLine(ScreenLocation, ScreenEndPoint, 1.0f, Color);

            // Draw arrow head
            FVector2D ArrowDir = (ScreenEndPoint - ScreenLocation).GetSafeNormal();
            FVector2D PerpendicularDir(-ArrowDir.Y, ArrowDir.X);
            
            const float ArrowSize = 5.0f;
            FVector2D ArrowPoint1 = ScreenEndPoint - ArrowDir * ArrowSize + PerpendicularDir * ArrowSize;
            FVector2D ArrowPoint2 = ScreenEndPoint - ArrowDir * ArrowSize - PerpendicularDir * ArrowSize;
            
            Canvas->K2_DrawLine(ScreenEndPoint, ArrowPoint1, 1.0f, Color);
            Canvas->K2_DrawLine(ScreenEndPoint, ArrowPoint2, 1.0f, Color);
        }
    }
}

float FWindSystemDebugRenderer::DrawDebugString(UCanvas* Canvas, const FDebugText& DebugText)
{
    float XL, YL;
    Canvas->StrLen(GEngine->GetSmallFont(), DebugText.Text, XL, YL);

    float ScaledY = YL * DebugText.Scale;
    Canvas->SetDrawColor(DebugText.Color.ToFColor(true));
    Canvas->DrawText(GEngine->GetSmallFont(), DebugText.Text, DebugText.X, DebugText.Y, DebugText.Scale, DebugText.Scale);

    return ScaledY;
}

FLinearColor FWindSystemDebugRenderer::GetWindSpeedColor(float WindSpeed)
{
    float NormalizedSpeed = FMath::Clamp(WindSpeed / MaxWindSpeed, 0.0f, 1.0f);
    return FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, NormalizedSpeed);
}