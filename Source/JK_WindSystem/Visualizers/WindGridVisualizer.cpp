#include "WindGridVisualizer.h"
#include "SceneManagement.h"
#include "PrimitiveSceneProxy.h"

class FWindGridSceneProxy final : public FPrimitiveSceneProxy
{
public:
    SIZE_T GetTypeHash() const override
    {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }

    FWindGridSceneProxy(const UWindGridVisualizer* InComponent)
        : FPrimitiveSceneProxy(InComponent)
        , GridColor(InComponent->GridColor)
        , LineThickness(InComponent->LineThickness)
        , GridSize(InComponent->GetWindSettings()->GridSize)
        , CellSize(InComponent->GetWindSettings()->CellSize)
    {
        bWillEverBeLit = false;
    }

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
    {
        QUICK_SCOPE_CYCLE_COUNTER(STAT_WindGridSceneProxy_GetDynamicMeshElements);

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
        {
            if (VisibilityMap & (1 << ViewIndex))
            {
                const FSceneView* View = Views[ViewIndex];
                FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

                const float GridExtent = GridSize * CellSize;
                const float HalfCellSize = CellSize * 0.5f;
                const FVector Offset(-HalfCellSize, -HalfCellSize, -HalfCellSize);

                // Draw grid lines
                for (int32 i = 0; i <= GridSize; ++i)
                {
                    float LinePos = i * CellSize;

                    // Draw lines along X-axis
                    PDI->DrawLine(
                        FVector(LinePos, 0, 0) + Offset,
                        FVector(LinePos, GridExtent, 0) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(LinePos, 0, GridExtent) + Offset,
                        FVector(LinePos, GridExtent, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(LinePos, 0, 0) + Offset,
                        FVector(LinePos, 0, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(LinePos, GridExtent, 0) + Offset,
                        FVector(LinePos, GridExtent, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);

                    // Draw lines along Y-axis
                    PDI->DrawLine(
                        FVector(0, LinePos, 0) + Offset,
                        FVector(GridExtent, LinePos, 0) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(0, LinePos, GridExtent) + Offset,
                        FVector(GridExtent, LinePos, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(0, LinePos, 0) + Offset,
                        FVector(0, LinePos, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(GridExtent, LinePos, 0) + Offset,
                        FVector(GridExtent, LinePos, GridExtent) + Offset,
                        GridColor, SDPG_World, LineThickness);

                    // Draw lines along Z-axis
                    PDI->DrawLine(
                        FVector(0, 0, LinePos) + Offset,
                        FVector(GridExtent, 0, LinePos) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(0, GridExtent, LinePos) + Offset,
                        FVector(GridExtent, GridExtent, LinePos) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(0, 0, LinePos) + Offset,
                        FVector(0, GridExtent, LinePos) + Offset,
                        GridColor, SDPG_World, LineThickness);
                    PDI->DrawLine(
                        FVector(GridExtent, 0, LinePos) + Offset,
                        FVector(GridExtent, GridExtent, LinePos) + Offset,
                        GridColor, SDPG_World, LineThickness);
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
    FLinearColor GridColor;
    float LineThickness;
    int32 GridSize;
    float CellSize;
};

UWindGridVisualizer::UWindGridVisualizer()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

FPrimitiveSceneProxy* UWindGridVisualizer::CreateSceneProxy()
{
    return new FWindGridSceneProxy(this);
}

FBoxSphereBounds UWindGridVisualizer::CalcBounds(const FTransform& LocalToWorld) const
{
    const UWindSystemSettings* WindSettings = GetWindSettings();
    float Extent = WindSettings->GridSize * WindSettings->CellSize * 0.5f;
    return FBoxSphereBounds(FBox(-FVector(Extent), FVector(Extent))).TransformBy(LocalToWorld);
}

const UWindSystemSettings* UWindGridVisualizer::GetWindSettings() const
{
    return GetDefault<UWindSystemSettings>();
}