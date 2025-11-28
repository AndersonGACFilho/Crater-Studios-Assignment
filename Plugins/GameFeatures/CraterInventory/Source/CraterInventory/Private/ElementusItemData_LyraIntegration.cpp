/**
 * @file ElementusItemData_LyraIntegration.cpp
 * @brief Implementation of Lyra integration bridge for Elementus items.
 * 
 * This file implements the methods that connect Elementus item data to Lyra's
 * fragment system. The implementation focuses on safe asset loading and fragment
 * lookup with appropriate fallback behavior.
 * @see UElementusItemData_LyraIntegration
 * @author Andy
 * @date 2025-11-28
 */

#include "ElementusItemData_LyraIntegration.h"

#include "Inventory/LyraInventoryItemDefinition.h"

UElementusItemData_LyraIntegration::UElementusItemData_LyraIntegration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ItemType = EElementusItemType::Other;
}

TSubclassOf<ULyraInventoryItemDefinition> UElementusItemData_LyraIntegration::LoadLyraDefinitionSync() const
{
	if (LyraItemDefinition.IsNull())
		return nullptr;

	return LyraItemDefinition.LoadSynchronous();
}

FText UElementusItemData_LyraIntegration::GetLyraDisplayName() const
{
	TSubclassOf<ULyraInventoryItemDefinition> DefClass = LoadLyraDefinitionSync();
	if (!DefClass)
		return FText::FromName(ItemName);

	const ULyraInventoryItemDefinition* DefCDO =
		GetDefault<ULyraInventoryItemDefinition>(DefClass);
	if (!DefCDO)
		return FText::FromName(ItemName);

	if (DefCDO->DisplayName.IsEmpty())
		return FText::FromName(ItemName);

	return DefCDO->DisplayName;
}

const ULyraInventoryItemFragment* UElementusItemData_LyraIntegration::FindLyraFragment(
	TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const
{
	if (!FragmentClass)
		return nullptr;

	TSubclassOf<ULyraInventoryItemDefinition> DefClass = LoadLyraDefinitionSync();
	if (!DefClass)
		return nullptr;

	const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(DefClass);
	if (!DefCDO)
		return nullptr;

	return DefCDO->FindFragmentByClass(FragmentClass);
}