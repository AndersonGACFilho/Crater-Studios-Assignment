/**
 * @file ElementusItemData_LyraIntegration.h
 * @brief Data asset class that bridges Elementus inventory system with Lyra's item framework.
 * 
 * This file defines the data structure that allows Elementus items (data-driven storage)
 * to reference and integrate with Lyra inventory item definitions (fragment-based gameplay).
 * 
 * ## Purpose
 * Enables a single item to exist in both systems:
 * - Elementus: Handles storage, replication, and persistence
 * - Lyra: Provides gameplay fragments (abilities, equipment, stats)
 * 
 * ## Usage
 * Create a Blueprint or Data Asset derived from UElementusItemData_LyraIntegration,
 * then assign a Lyra Item Definition to enable fragment-based functionality.
 * 
 * @see UElementusItemData for base storage functionality
 * @see ULyraInventoryItemDefinition for fragment system
 * @author Anderson
 * @date 2025-11-28
 */

#pragma once

#include "CoreMinimal.h"
#include "Management/ElementusInventoryData.h"
#include "Templates/SubclassOf.h"
#include "ElementusItemData_LyraIntegration.generated.h"

// Forward declarations
class ULyraInventoryItemDefinition;
class ULyraInventoryItemFragment;

/**
 * @class UElementusItemData_LyraIntegration
 * @brief Elementus item data with Lyra inventory integration support.
 * 
 * This class extends the base Elementus item data to include a soft reference to
 * a Lyra Inventory Item Definition, implementing the Bridge pattern between two
 * inventory systems.
 * 
 * ## Key Features
 * - **Dual System Support**: Works in both Elementus and Lyra frameworks
 * - **Soft References**: Uses TSoftClassPtr to avoid hard asset dependencies
 * - **Fragment Access**: Provides helper methods to query Lyra fragments
 * - **Equipment Support**: Flags items as equippable with preferred slot hints
 * 
 * ## Example Setup
 * @code
 * // In your Data Asset or Blueprint:
 * LyraItemDefinition = "/Game/Items/Weapons/Sword/ItemDef_IronSword"
 * bCanBeEquipped = true
 * PreferredEquipmentSlot = 0  // Weapon slot
 * @endcode
 * 
 * ## Usage Pattern
 * @code
 * // Check if item has Lyra integration
 * if (ItemData->HasLyraDefinition())
 * {
 *     // Load the definition and access fragments
 *     TSubclassOf<ULyraInventoryItemDefinition> DefClass = ItemData->LoadLyraDefinitionSync();
 *     
 *     // Or find a specific fragment type
 *     const UCraterInventoryFragment_Equippable* EquipFragment = 
 *         ItemData->FindLyraFragment<UCraterInventoryFragment_Equippable>();
 * }
 * @endcode
 * 
 * ## Thread Safety
 * - Not thread-safe. Access from game thread only.
 * - LoadLyraDefinitionSync performs synchronous asset load (blocking)
 * 
 * @note This is a data-only class. Create instances via Data Assets or Blueprints.
 * @see UElementusItemData
 * @see ULyraInventoryItemDefinition
 * @see UCraterInventoryManagerComponent
 */
UCLASS(BlueprintType, Category = "Crater Inventory | Data")
class CRATERINVENTORY_API UElementusItemData_LyraIntegration : public UElementusItemData
{
	GENERATED_BODY()

public:
	UElementusItemData_LyraIntegration(const FObjectInitializer& ObjectInitializer);


	/**
	 * @brief Soft reference to a Lyra Inventory Item Definition.
	 * @details This allows the Elementus item to be associated with Lyra's 
	 * fragment-based item system without creating hard dependencies.
	 * The definition is loaded on-demand when the item is equipped.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crater Inventory | Lyra Integration",
		meta = (DisplayName = "Lyra Item Definition"))
	TSoftClassPtr<ULyraInventoryItemDefinition> LyraItemDefinition;

	/**
	 * @brief Whether this item can be equipped.
	 * @details When true, the CraterInventoryManagerComponent will allow this 
	 * item to be placed in equipment slots and will grant associated abilities.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crater Inventory | Lyra Integration")
	bool bCanBeEquipped = false;

	/**
	 * @brief The equipment slot type this item prefers.
	 * @details Used for auto-equip logic. -1 means any available slot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crater Inventory | Lyra Integration",
		meta = (EditCondition = "bCanBeEquipped", ClampMin = "-1"))
	int32 PreferredEquipmentSlot = -1;

	UFUNCTION(BlueprintPure, Category = "Crater Inventory | Lyra Integration")
	bool HasLyraDefinition() const { return !LyraItemDefinition.IsNull(); }

	// WARNING: Blocking call
	UFUNCTION(BlueprintCallable, Category = "Crater Inventory | Lyra Integration")
	TSubclassOf<ULyraInventoryItemDefinition> LoadLyraDefinitionSync() const;

	UFUNCTION(BlueprintPure, Category = "Crater Inventory | Lyra Integration")
	FText GetLyraDisplayName() const;

	UFUNCTION(BlueprintCallable, Category = "Crater Inventory | Lyra Integration",
		meta = (DeterminesOutputType = "FragmentClass"))
	const ULyraInventoryItemFragment* FindLyraFragment(TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const;

	template<typename TFragment>
	const TFragment* FindLyraFragment() const
	{
		return Cast<TFragment>(FindLyraFragment(TFragment::StaticClass()));
	}
};