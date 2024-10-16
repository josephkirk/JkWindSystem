using UnrealBuildTool;

public class JK_WindSystemTests : ModuleRules
{
    public JK_WindSystemTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.AddRange(
			new string[] {
				"JK_WindSystem/WindGenerators",
				"JK_WindSystem/Visualizers"
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
                "InputCore",
                "JK_WindSystem"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "UnrealEd",
                "FunctionalTesting",
                "AutomationController"
            }
        );

        // Add this line to include the HeadMountedDisplay module
            PrivateDependencyModuleNames.Add("HeadMountedDisplay");
        
        // Uncomment this if you need to use UMG in your tests
        // PrivateDependencyModuleNames.Add("UMG");

        // Uncomment this if you need to use the new Chaos physics system
        // PrivateDependencyModuleNames.Add("Chaos");
    }
}