// Copyright Crater Studios Assignment. All Rights Reserved.

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
				"LyraGame",
				"ModularGameplay",    
				"CommonGame",
				"ElementusInventory",
				"GameplayTags",
				"GameplayAbilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"NetCore",
				"DeveloperSettings",
				"CommonUI",
				"UMG"
			}
		);
	}
}