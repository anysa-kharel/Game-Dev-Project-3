using UnrealBuildTool;
public class ShootingTarget : TargetRules
{
    public ShootingTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("ShootingExpansion");
    }
}
