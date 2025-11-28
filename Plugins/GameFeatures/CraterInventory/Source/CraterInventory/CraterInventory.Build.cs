using UnrealBuildTool;

public class CraterInventory : ModuleRules
{
	public CraterInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",
				"ElementusInventory",
				"GameplayTags",
				"GameplayAbilities",
				"LyraGame"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"DeveloperSettings",
				"CommonUI",
				"UMG"
			}
		);
	}
}