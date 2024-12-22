#pragma once

#include "CoreMinimal.h"

JK_WINDSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogWindSystem, Log, All);

// Helper macros for logging
#define WINDSYSTEM_LOG(Verbosity, Format, ...) \
    UE_LOG(LogWindSystem, Verbosity, TEXT("%s: ") Format, *FString(__FUNCTION__), ##__VA_ARGS__)

#define WINDSYSTEM_LOG_WARNING(Format, ...) \
    WINDSYSTEM_LOG(Warning, Format, ##__VA_ARGS__)

#define WINDSYSTEM_LOG_ERROR(Format, ...) \
    WINDSYSTEM_LOG(Error, Format, ##__VA_ARGS__)

#if !UE_BUILD_SHIPPING
#define WINDSYSTEM_LOG_VERBOSE(Format, ...) \
    WINDSYSTEM_LOG(Verbose, Format, ##__VA_ARGS__)
#else
#define WINDSYSTEM_LOG_VERBOSE(Format, ...)
#endif