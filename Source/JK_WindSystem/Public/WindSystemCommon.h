#pragma once

#include "CoreMinimal.h"
#include "WindSystemLog.h"

inline bool IsVectorFinite(const FVector& Vector)
{
    return FMath::IsFinite(Vector.X) && FMath::IsFinite(Vector.Y) && FMath::IsFinite(Vector.Z);
}