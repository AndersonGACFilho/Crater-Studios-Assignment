#include "CraterInventoryFragment_Equippable.h"

#include "CraterLogChannels.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "GameplayTagContainer.h"

void UCraterInventoryFragment_Equippable::OnInstanceCreated(ULyraInventoryItemInstance* Instance) const
{
	Super::OnInstanceCreated(Instance);

	if (!Instance)
	{
		return;
	}
	
	UE_LOG(LogCraterInventory_Data, Verbose, 
		TEXT("Equippable fragment initialized for instance. Grants %d abilities, %d effects"),
		AbilitiesToGrant.Num(), EffectsToApply.Num());
}

bool UCraterInventoryFragment_Equippable::CanBeEquippedBy(const FGameplayTagContainer& OwnerTags) const
{
	if (RequiredTagsToEquip.Num() > 0 && !OwnerTags.HasAll(RequiredTagsToEquip))
	{
		return false;
	}

	if (BlockingTagsToEquip.Num() > 0 && OwnerTags.HasAny(BlockingTagsToEquip))
	{
		return false;
	}

	return true;
}
