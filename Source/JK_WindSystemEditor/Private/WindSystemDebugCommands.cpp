#include "WindSystemDebugCommands.h"
#include "WindSystemDebugRenderer.h"
#include "WindSystemLog.h"
#include "Debug/DebugDrawService.h"

TAutoConsoleVariable<int32> FWindSystemDebugCommands::CVarShowWindSystemDebug(
	TEXT("ShowDebug.WindSystem"),
	0,
	TEXT("Toggles wind system debug visualization\n")
	TEXT("0: off\n")
	TEXT("1: on"),
	ECVF_Cheat);

FAutoConsoleCommandWithWorldAndArgs FWindSystemDebugCommands::ShowDebugWindSystemCommand(
	TEXT("ShowDebug.WindSystem.Toggle"),
	TEXT("Toggles wind system debug visualization"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FWindSystemDebugCommands::ShowDebugWindSystemCommandHandler)
);

void FWindSystemDebugCommands::Register()
{
	// Register with Debug Draw Service
	DebugWindDrawingDelegate = FDebugDrawDelegate::CreateStatic(&FWindSystemDebugRenderer::DrawDebug);
	DebugWindDrawingDelegateHandle = UDebugDrawService::Register(TEXT("Game"), DebugWindDrawingDelegate);
	{
		WINDSYSTEM_LOG_VERBOSE( TEXT("Registered WindSystem debug drawing"));
	}
}

void FWindSystemDebugCommands::Unregister()
{
	UDebugDrawService::Unregister(DebugWindDrawingDelegateHandle);
	WINDSYSTEM_LOG_VERBOSE(TEXT("Unregistered WindSystem debug drawing"));
}

bool FWindSystemDebugCommands::IsDebugEnabled()
{
	return CVarShowWindSystemDebug.GetValueOnGameThread() > 0;
}

void FWindSystemDebugCommands::ShowDebugWindSystemCommandHandler(const TArray<FString>& Args, UWorld* World)
{
	if (!World)
	{
		return;
	}

	// Toggle the debug state
	int32 CurrentValue = CVarShowWindSystemDebug.GetValueOnGameThread();
	CVarShowWindSystemDebug->Set(!CurrentValue, ECVF_SetByConsole);

	// Send feedback to console
	FString DebugStateStr = !CurrentValue ? TEXT("enabled") : TEXT("disabled");
	WINDSYSTEM_LOG_VERBOSE(TEXT("Wind System debug visualization %s"), *DebugStateStr);
    
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
			FString::Printf(TEXT("Wind System Debug %s"), *DebugStateStr));
	}
}