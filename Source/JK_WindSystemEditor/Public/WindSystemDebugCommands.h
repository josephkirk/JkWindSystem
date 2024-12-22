#pragma once

#include "CoreMinimal.h"
#include "Debug/DebugDrawService.h"

class FWindSystemDebugCommands
{
public:
	void Register();
	void Unregister();
	static bool IsDebugEnabled();
protected:
	FDebugDrawDelegate DebugWindDrawingDelegate;
	FDelegateHandle DebugWindDrawingDelegateHandle;
private:
	
	static TAutoConsoleVariable<int32> CVarShowWindSystemDebug;
	static FAutoConsoleCommandWithWorldAndArgs ShowDebugWindSystemCommand;

	static void ShowDebugWindSystemCommandHandler(const TArray<FString>& Args, UWorld* World);
};
