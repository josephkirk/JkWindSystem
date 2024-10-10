#include "WindSystemSettings.h"

UWindSystemSettings::UWindSystemSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MaxAllowedWindVelocity = 1000000.0f;
}