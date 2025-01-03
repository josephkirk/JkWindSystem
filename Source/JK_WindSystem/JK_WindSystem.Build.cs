// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JK_WindSystem : ModuleRules
{
	public JK_WindSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"JK_WindSystem/WindGenerators",
				"JK_WindSystem/Visualizers",
				"JK_WindSystem/WindZones"
				// ... add other private include paths required here ...
			}
		);


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"RHI",
				"Renderer"
            }
        );


        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"RHI",
				"Projects"
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
