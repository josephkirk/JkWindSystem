using UnrealBuildTool;

public class JK_WindSystemTests : ModuleRules
{
    public JK_WindSystemTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
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
                "Engine",
                "FunctionalTesting",
                "AutomationController"
            }
        );

        // Add this line to include the HeadMountedDisplay module
        if (Target.bBuildDevelopmentTools)
        {
            PrivateDependencyModuleNames.Add("HeadMountedDisplay");
        }
    }
}