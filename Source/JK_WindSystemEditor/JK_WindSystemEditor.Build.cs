using UnrealBuildTool;

public class JK_WindSystemEditor : ModuleRules
{
    public JK_WindSystemEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.AddRange(
			new string[] {
				"JK_WindSystem/WindGenerators",
				"JK_WindSystem/Visualizers",
                "JK_WindSystemEditor/Widgets"
				// ... add other private include paths required here ...
			}
        );
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "JK_WindSystem",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "ToolMenus",
                "EditorSubsystem",
                "LevelEditor",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "InputCore"
            }
        );
    }
}