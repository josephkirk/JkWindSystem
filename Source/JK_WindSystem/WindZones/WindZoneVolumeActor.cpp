#include "WindZoneVolumeActor.h"

AWindZoneVolumeActor::AWindZoneVolumeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    WindZoneComponent = CreateDefaultSubobject<UWindZoneVolumeComponent>(TEXT("WindZoneVolume"));
    RootComponent = WindZoneComponent;
}