#include "DirectionalWindVisualizer.h"
#include "SceneManagement.h"
#include "WindSourceComponent.h"

FPrimitiveSceneProxy* UDirectionalWindVisualizer::CreateSceneProxy()
{
    return new FDirectionalWindVisualizerSceneProxy(this);
}

FLinearColor UDirectionalWindVisualizer::GetColorForWindStrength(float Strength, float MaxStrength)
{
    float NormalizedStrength = FMath::Clamp(Strength / MaxStrength, 0.0f, 1.0f);
    
    // Interpolate between blue (weak) and red (strong)
    return FLinearColor::LerpUsingHSV(FLinearColor::Blue, FLinearColor::Red, NormalizedStrength);
}

FDirectionalWindVisualizerSceneProxy::FDirectionalWindVisualizerSceneProxy(const UDirectionalWindVisualizer* InComponent)
    : FWindSourceVisualizerSceneProxy(InComponent)
{
}

void FDirectionalWindVisualizerSceneProxy::GatherVisualizationData() const
{
    if (const UDirectionalWindGeneratorComponent* DirectionalWindSource = Cast<UDirectionalWindGeneratorComponent>(VisualizerComponent->GetWindSource()))
    {
        ComponentLocation = DirectionalWindSource->GetComponentLocation();
        Radius = DirectionalWindSource->Radius;
        Strength = DirectionalWindSource->Strength;
        Direction = DirectionalWindSource->GetForwardVector();
        ShapeType = DirectionalWindSource->ShapeType;
        ConeAngle = DirectionalWindSource->ConeAngle;
    }
}

void FDirectionalWindVisualizerSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    FWindSourceVisualizerSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);

    GatherVisualizationData();

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            const FSceneView* View = Views[ViewIndex];
            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

            FLinearColor WindColor = UDirectionalWindVisualizer::GetColorForWindStrength(Strength, MaxWindStrength);
            float LineThickness = FMath::Lerp(1.0f, 3.0f, Strength / MaxWindStrength);

            // Draw the main direction arrow
            PDI->DrawLine(ComponentLocation, ComponentLocation + Direction * Radius, WindColor, SDPG_World, LineThickness * 2);
            DrawArrowHead(PDI, ComponentLocation + Direction * Radius, Direction, 30.0f, WindColor, SDPG_World, LineThickness);

            if (ShapeType == EWindShapeType::Cylinder)
            {
                // Draw cylinder
                DrawWireCylinder(PDI, ComponentLocation+FVector(Radius,0,0), FVector::UpVector, FVector::RightVector, Direction, WindColor, Radius, Radius, 16, SDPG_World, LineThickness);

                // Draw wind lines
                int32 NumLines = 8;
                float LineLength = Radius * 0.8f;
                for (int32 i = 0; i < NumLines; ++i)
                {
                    float Angle = 2.0f * PI * i / NumLines;
                    FVector Offset = FVector::VectorPlaneProject(FVector::UpVector, Direction).GetSafeNormal().RotateAngleAxis(Angle, Direction) * Radius * 0.5f;
                    FVector Start = ComponentLocation + Offset;
                    FVector End = Start + Direction * LineLength;
                    PDI->DrawLine(Start, End, WindColor, SDPG_World, LineThickness);
                    DrawArrowHead(PDI, End, Direction, 15.0f, WindColor, SDPG_World, LineThickness);
                }
            }
            else if (ShapeType == EWindShapeType::Cone)
            {
                // Draw cone
                float ConeHalfAngle = FMath::DegreesToRadians(ConeAngle * 0.5f);
                float ConeHeight = Radius / FMath::Tan(ConeHalfAngle);
                FVector ConeEndPoint = ComponentLocation + Direction * ConeHeight;
                
                TArray<FVector> ConeVerts;
                DrawWireCone(PDI, ConeVerts, FTransform(Direction.ToOrientationQuat(), ComponentLocation), Radius, ConeHalfAngle, 16, WindColor, SDPG_World, LineThickness);

                // Draw wind lines within the cone
                int32 NumLines = 8;
                for (int32 i = 0; i < NumLines; ++i)
                {
                    float Angle = 2.0f * PI * i / NumLines;
                    FVector Offset = FVector::VectorPlaneProject(FVector::UpVector, Direction).GetSafeNormal().RotateAngleAxis(Angle, Direction) * Radius * 0.5f;
                    FVector Start = ComponentLocation + Offset * 0.2f; // Start closer to the cone's apex
                    FVector End = ConeEndPoint + Offset;
                    PDI->DrawLine(Start, End, WindColor, SDPG_World, LineThickness);
                    DrawArrowHead(PDI, End, (End - Start).GetSafeNormal(), 15.0f, WindColor, SDPG_World, LineThickness);
                }
            }
        }
    }
}

void FDirectionalWindVisualizerSceneProxy::DrawArrowHead(FPrimitiveDrawInterface* PDI, const FVector& TipLocation, const FVector& ArrowDirection, float Size, const FLinearColor& Color, ESceneDepthPriorityGroup DepthPriority, float Thickness) const
{
    FVector Right = FVector::CrossProduct(ArrowDirection, FVector::UpVector).GetSafeNormal();
    FVector Up = FVector::CrossProduct(Right, ArrowDirection);

    PDI->DrawLine(TipLocation, TipLocation - ArrowDirection * Size + Right * Size * 0.5f, Color, DepthPriority, Thickness);
    PDI->DrawLine(TipLocation, TipLocation - ArrowDirection * Size - Right * Size * 0.5f, Color, DepthPriority, Thickness);
    PDI->DrawLine(TipLocation, TipLocation - ArrowDirection * Size + Up * Size * 0.5f, Color, DepthPriority, Thickness);
    PDI->DrawLine(TipLocation, TipLocation - ArrowDirection * Size - Up * Size * 0.5f, Color, DepthPriority, Thickness);
}