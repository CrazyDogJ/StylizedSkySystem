using UnrealBuildTool;

public class StylizedSkySystemEditor : ModuleRules
{
    public StylizedSkySystemEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "EditorStyle",
                "UnrealEd",
                "StylizedSkySystem", 
                "WorkspaceMenuStructure",
                "ToolMenus"
            }
        );
    }
}