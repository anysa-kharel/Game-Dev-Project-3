using UnrealBuildTool;
public class ShootingEditorTarget : TargetRules
{
    public ShootingEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ShootingExpansion");
    }
}
