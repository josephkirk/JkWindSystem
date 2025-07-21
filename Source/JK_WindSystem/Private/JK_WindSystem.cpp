// Copyright Epic Games, Inc. All Rights Reserved.

#include "JK_WindSystem.h"
#include "Interfaces/IPluginManager.h"
#define LOCTEXT_NAMESPACE "FJK_WindSystemModule"

void FJK_WindSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin("JK_WindSystem")->GetBaseDir(), "Shaders/Private");
	AddShaderSourceDirectoryMapping("/JK_WindSystem", PluginShaderDir);
}

void FJK_WindSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJK_WindSystemModule, JK_WindSystem)