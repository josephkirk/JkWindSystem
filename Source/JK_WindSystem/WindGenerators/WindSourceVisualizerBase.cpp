#include "WindSourceVisualizerBase.h"
#include "SceneManagement.h"

UWindSourceVisualizerBase::UWindSourceVisualizerBase()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    bTickInEditor = true;
    bUseEditorCompositing = true;
}

void UWindSourceVisualizerBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (WindSource)
    {
        UpdateVisualization();
    }
}

FPrimitiveSceneProxy* UWindSourceVisualizerBase::CreateSceneProxy()
{
    // This should be overridden in derived classes
    return nullptr;
}

FBoxSphereBounds UWindSourceVisualizerBase::CalcBounds(const FTransform& LocalToWorld) const
{
    if (WindSource)
    {
        return FBoxSphereBounds(WindSource->GetComponentLocation(), FVector(WindSource->Radius), WindSource->Radius);
    }
    return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
}

void UWindSourceVisualizerBase::SetWindSourceComponent(UWindGeneratorComponent* InWindSource)
{
    WindSource = InWindSource;
    UpdateVisualization();
}

void UWindSourceVisualizerBase::UpdateVisualization()
{
    MarkRenderStateDirty();
}

FWindSourceVisualizerSceneProxy::FWindSourceVisualizerSceneProxy(const UWindSourceVisualizerBase* InComponent)
    : FPrimitiveSceneProxy(InComponent)
    , VisualizerComponent(InComponent)
{
    bWillEverBeLit = false;
}

void FWindSourceVisualizerSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    GatherVisualizationData();

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            const FSceneView* View = Views[ViewIndex];
            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

            // Draw base shape (e.g., sphere for point wind, cylinder for directional wind)
            DrawWireSphere(PDI, ComponentLocation, FColor::Cyan, Radius, 32, SDPG_World, 1.0f);

            // Draw strength indicator
            FVector StrengthVector = FVector::UpVector * Strength * 0.01f; // Scale strength for visualization
            PDI->DrawLine(ComponentLocation, ComponentLocation + StrengthVector, FColor::Yellow, SDPG_World, 2.0f);
        }
    }
}

FPrimitiveViewRelevance FWindSourceVisualizerSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bDynamicRelevance = true;
    Result.bShadowRelevance = false;
    Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
    return Result;
}

SIZE_T FWindSourceVisualizerSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}