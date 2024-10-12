#include "WindGridVisualizer.h"
#include "WindSubsystem.h"
#include "WindSystemActor.h"
#include "SceneManagement.h"
#include "PrimitiveSceneProxy.h"

UWindGridVisualizer::UWindGridVisualizer()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    TimeSinceLastUpdate = 0.0f;
}

void UWindGridVisualizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        UpdateGridData();
        TimeSinceLastUpdate = 0.0f;
    }
}

FPrimitiveSceneProxy* UWindGridVisualizer::CreateSceneProxy()
{
    return new FWindGridSceneProxy(this);
}

FBoxSphereBounds UWindGridVisualizer::CalcBounds(const FTransform& LocalToWorld) const
{
    FBox BoundingBox(ForceInit);
    for (const FVector& Point : GridPoints)
    {
        BoundingBox += Point;
    }
    return FBoxSphereBounds(BoundingBox);
}

void UWindGridVisualizer::UpdateGridData()
{
    UWindSimulationSubsystem* WindSubsystem = GetWorld()->GetSubsystem<UWindSimulationSubsystem>();
    if (!WindSubsystem || !WindSubsystem->GetWindSystemActor())
    {
        return;
    }

    UWindSimulationComponent* WindComponent = WindSubsystem->GetWindSystemActor()->WindSimulationComponent;
    if (!WindComponent)
    {
        return;
    }

    int32 GridSize = WindComponent->GetGridSize();
    float CellSize = WindComponent->GetCellSize();

    GridPoints.Reset(GridSize * GridSize * GridSize);
    WindVelocities.Reset(GridSize * GridSize * GridSize);

    for (int32 x = 0; x < GridSize; ++x)
    {
        for (int32 y = 0; y < GridSize; ++y)
        {
            for (int32 z = 0; z < GridSize; ++z)
            {
                FVector WorldLocation = WindComponent->GetComponentLocation() + FVector(x, y, z) * CellSize;
                GridPoints.Add(WorldLocation);
                WindVelocities.Add(WindSubsystem->GetWindVelocityAtLocation(WorldLocation));
            }
        }
    }

    MarkRenderStateDirty();
}

FWindGridSceneProxy::FWindGridSceneProxy(const UWindGridVisualizer* InComponent)
    : FPrimitiveSceneProxy(InComponent)
    , GridPoints(InComponent->GridPoints)
    , WindVelocities(InComponent->WindVelocities)
    , ArrowScale(InComponent->ArrowScale)
{
}

void FWindGridSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_WindGridSceneProxy_GetDynamicMeshElements);

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            const FSceneView* View = Views[ViewIndex];
            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

            for (int32 i = 0; i < GridPoints.Num(); ++i)
            {
                const FVector& Start = GridPoints[i];
                FVector End = Start + WindVelocities[i] * ArrowScale;
                FColor Color = FColor::MakeRedToGreenColorFromScalar(WindVelocities[i].Size() / 100.0f);

                PDI->DrawLine(Start, End, Color, SDPG_World, 1.0f);
                
                // Draw arrowhead
                FVector ArrowDirection = (End - Start).GetSafeNormal();
                FVector Right = FVector::CrossProduct(ArrowDirection, FVector::UpVector).GetSafeNormal();
                FVector Up = FVector::CrossProduct(Right, ArrowDirection);

                PDI->DrawLine(End, End - ArrowDirection * 10 + Right * 5, Color, SDPG_World, 1.0f);
                PDI->DrawLine(End, End - ArrowDirection * 10 - Right * 5, Color, SDPG_World, 1.0f);
                PDI->DrawLine(End, End - ArrowDirection * 10 + Up * 5, Color, SDPG_World, 1.0f);
                PDI->DrawLine(End, End - ArrowDirection * 10 - Up * 5, Color, SDPG_World, 1.0f);
            }
        }
    }
}

FPrimitiveViewRelevance FWindGridSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bDynamicRelevance = true;
    Result.bShadowRelevance = false;
    Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
    return Result;
}

uint32 FWindGridSceneProxy::GetMemoryFootprint() const
{
    return sizeof(*this) + GridPoints.GetAllocatedSize() + WindVelocities.GetAllocatedSize();
}

SIZE_T FWindGridSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}