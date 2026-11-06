using UnrealBuildTool;
public class ShootingExpansion : ModuleRules
{
    public ShootingExpansion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "UMG", "Slate", "SlateCore" });
    }
}
