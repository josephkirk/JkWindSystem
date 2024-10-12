#include "PointWindVisualizer.h"
#include "SceneManagement.h"

FPrimitiveSceneProxy* UPointWindVisualizer::CreateSceneProxy()
{
    return new FPointWindVisualizerSceneProxy(this);
}

FLinearColor UPointWindVisualizer::GetColorForWindStrength(float Strength, float MaxStrength)
{
    float NormalizedStrength = FMath::Clamp(Strength / MaxStrength, 0.0f, 1.0f);
    
    // Interpolate between blue (weak) and red (strong)
    return FLinearColor::LerpUsingHSV(FLinearColor::Blue, FLinearColor::Red, NormalizedStrength);
}

FPointWindVisualizerSceneProxy::FPointWindVisualizerSceneProxy(const UPointWindVisualizer* InComponent)
    : FWindSourceVisualizerSceneProxy(InComponent)
{
}

void FPointWindVisualizerSceneProxy::GatherVisualizationData() const
{
    if (const UPointWindGeneratorComponent* PointWindSource = Cast<UPointWindGeneratorComponent>(VisualizerComponent->GetWindSource()))
    {
        ComponentLocation = PointWindSource->GetComponentLocation();
        Radius = PointWindSource->Radius;
        Strength = PointWindSource->Strength;
    }
}

void FPointWindVisualizerSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    FWindSourceVisualizerSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);

    GatherVisualizationData();

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            const FSceneView* View = Views[ViewIndex];
            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

            // Draw sphere to represent the area of effect
            DrawWireSphere(PDI, ComponentLocation, FColor::Cyan, Radius, 32, SDPG_World, 1.0f);

            // Draw arrows pointing towards the center
            const int32 NumArrows = 16;
            FLinearColor WindColor = UPointWindVisualizer::GetColorForWindStrength(Strength, MaxWindStrength);
            float LineThickness = FMath::Lerp(1.0f, 3.0f, Strength / MaxWindStrength);

            for (int32 i = 0; i < NumArrows; ++i)
            {
                float Angle = 2.0f * PI * i / NumArrows;
                FVector Start = ComponentLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
                FVector End = ComponentLocation;
                PDI->DrawLine(Start, End, WindColor.ToFColor(true), SDPG_World, LineThickness);
                
                // Draw arrowhead
                FVector ArrowDirection = (End - Start).GetSafeNormal();
                FVector Right = FVector::CrossProduct(ArrowDirection, FVector::UpVector).GetSafeNormal();
                FVector Up = FVector::CrossProduct(Right, ArrowDirection);

                PDI->DrawLine(End, End - ArrowDirection * 20 + Right * 10, WindColor.ToFColor(true), SDPG_World, LineThickness);
                PDI->DrawLine(End, End - ArrowDirection * 20 - Right * 10, WindColor.ToFColor(true), SDPG_World, LineThickness);
                PDI->DrawLine(End, End - ArrowDirection * 20 + Up * 10, WindColor.ToFColor(true), SDPG_World, LineThickness);
                PDI->DrawLine(End, End - ArrowDirection * 20 - Up * 10, WindColor.ToFColor(true), SDPG_World, LineThickness);
            }
        }
    }
}