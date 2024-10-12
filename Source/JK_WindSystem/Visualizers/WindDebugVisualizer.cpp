#include "WindDebugVisualizer.h"
#include "WindSubsystem.h"
#include "SceneManagement.h"
#include "PrimitiveSceneProxy.h"

class FWindDebugSceneProxy final : public FPrimitiveSceneProxy
{
public:
    SIZE_T GetTypeHash() const override
    {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }

    FWindDebugSceneProxy(const UWindDebugVisualizer* InComponent)
        : FPrimitiveSceneProxy(InComponent)
        , DebugArrows(InComponent->DebugArrows)
    {
        bWillEverBeLit = false;
    }

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
    {
        QUICK_SCOPE_CYCLE_COUNTER(STAT_WindDebugSceneProxy_GetDynamicMeshElements);

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
        {
            if (VisibilityMap & (1 << ViewIndex))
            {
                const FSceneView* View = Views[ViewIndex];
                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

                for (const FWindDebugArrow& Arrow : DebugArrows)
                {
                    PDI->DrawLine(Arrow.Start, Arrow.End, Arrow.Color, SDPG_World, 2.0f);
                    
                    // Draw arrowhead
                    FVector ArrowDirection = (Arrow.End - Arrow.Start).GetSafeNormal();
                    FVector Right = FVector::CrossProduct(ArrowDirection, FVector::UpVector).GetSafeNormal();
                    FVector Up = FVector::CrossProduct(Right, ArrowDirection);

                    PDI->DrawLine(Arrow.End, Arrow.End - ArrowDirection * 20 + Right * 10, Arrow.Color, SDPG_World, 2.0f);
                    PDI->DrawLine(Arrow.End, Arrow.End - ArrowDirection * 20 - Right * 10, Arrow.Color, SDPG_World, 2.0f);
                    PDI->DrawLine(Arrow.End, Arrow.End - ArrowDirection * 20 + Up * 10, Arrow.Color, SDPG_World, 2.0f);
                    PDI->DrawLine(Arrow.End, Arrow.End - ArrowDirection * 20 - Up * 10, Arrow.Color, SDPG_World, 2.0f);
                }
            }
        }
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View);
        Result.bDynamicRelevance = true;
        Result.bShadowRelevance = false;
        Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
        return Result;
    }

    virtual uint32 GetMemoryFootprint(void) const override { return sizeof(*this) + GetAllocatedSize(); }
    uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
    TArray<FWindDebugArrow> DebugArrows;
};

UWindDebugVisualizer::UWindDebugVisualizer()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    bWantsInitializeComponent = true;
    VisualizationExtent = FVector(1000.0f);
}

void UWindDebugVisualizer::SetVisualizationExtent(const FVector& Extent)
{
    VisualizationExtent = Extent;
    MarkRenderStateDirty();
}

void UWindDebugVisualizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        UpdateDebugArrows();
        TimeSinceLastUpdate = 0.0f;
    }
}

FPrimitiveSceneProxy* UWindDebugVisualizer::CreateSceneProxy()
{
    return new FWindDebugSceneProxy(this);
}

FBoxSphereBounds UWindDebugVisualizer::CalcBounds(const FTransform& LocalToWorld) const
{
    FBox BoundingBox(ForceInit);
    for (const FWindDebugArrow& Arrow : DebugArrows)
    {
        BoundingBox += Arrow.Start;
        BoundingBox += Arrow.End;
    }
    return FBoxSphereBounds(BoundingBox);
}

void UWindDebugVisualizer::UpdateDebugArrows()
{
    UWindSimulationSubsystem* WindSubsystem = GetWorld()->GetSubsystem<UWindSimulationSubsystem>();
    if (!WindSubsystem)
    {
        return;
    }

    DebugArrows.Reset();

    FVector Origin = GetComponentLocation();
    FVector CellSize = VisualizationExtent / GridResolution;

    for (int32 x = 0; x < GridResolution; ++x)
    {
        for (int32 y = 0; y < GridResolution; ++y)
        {
            for (int32 z = 0; z < GridResolution; ++z)
            {
                FVector CellCenter = Origin + FVector(
                    x * CellSize.X - VisualizationExtent.X * 0.5f,
                    y * CellSize.Y - VisualizationExtent.Y * 0.5f,
                    z * CellSize.Z - VisualizationExtent.Z * 0.5f
                ) + CellSize * 0.5f;

                FVector WindVelocity = WindSubsystem->GetWindVelocityAtLocation(CellCenter);

                if (!WindVelocity.IsNearlyZero())
                {
                    FWindDebugArrow Arrow;
                    Arrow.Start = CellCenter;
                    Arrow.End = CellCenter + WindVelocity * ArrowScale;
                    Arrow.Color = FColor::MakeRedToGreenColorFromScalar(WindVelocity.Size() / 100.0f);
                    DebugArrows.Add(Arrow);
                }
            }
        }
    }

    MarkRenderStateDirty();
}
