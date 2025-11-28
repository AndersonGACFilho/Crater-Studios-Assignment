/**
 * @file CraterInventoryManagerComponent.h
 * @brief Authoritative inventory management system integrating Elementus storage with Lyra's item framework.
 * 
 * This component serves as the central controller for player inventory operations, providing:
 * - Server-authoritative inventory transactions (add, remove, swap, discard)
 * - Equipment system with GAS (Gameplay Ability System) integration
 * - Efficient client replication using owner-only conditions
 * - Bridge between Elementus storage and Lyra item definitions
 * 
 * ## Architecture Patterns
 * - **Bridge Pattern**: Decouples inventory operations from storage implementation
 * - **Facade Pattern**: Simplifies interaction between Elementus and Lyra systems
 * - **Observer Pattern**: Reacts to storage updates for synchronization
 * - **Authority Pattern**: Centralizes control to prevent cheating
 * 
 * ## Network Model
 * - Server: Authoritative, executes all modifications
 * - Client: Receives replicated state, sends RPC requests
 * - Replication: Owner-only for bandwidth efficiency
 * 
 * @see UElementusInventoryComponent for underlying storage
 * @see ULyraInventoryItemDefinition for item definitions
 * @author Anderson
 * @date 2025-11-28
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Management/ElementusInventoryData.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "CraterInventoryManagerComponent.generated.h"

// Forward Declarations
class UElementusInventoryComponent;
class ULyraInventoryItemDefinition;
class ULyraInventoryItemInstance;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UCraterInventoryFragment_Equippable;
class UElementusItemData_LyraIntegration;
struct FGameplayAbilitySpecHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraterInventoryInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraterInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraterItemEquipped, int32, SlotIndex, const FElementusItemInfo&, ItemInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraterItemUnequipped, int32, SlotIndex, const FElementusItemInfo&, ItemInfo);

/**
 * @enum ECraterEquipResult
 * @brief Result codes for equipment validation operations.
 * 
 * These codes provide specific feedback about why an equip operation
 * succeeded or failed, enabling UI to display appropriate error messages.
 */
UENUM(BlueprintType)
enum class ECraterEquipResult : uint8
{
	/** Operation completed successfully */
	Success,
	
	/** The provided equipment slot index is invalid */
	InvalidSlot,
	
	/** The item does not have equippable configuration */
	ItemNotEquippable,
	
	/** No equipment slots are available for this item */
	NoAvailableSlots,
	
	/** The item requires a specific slot that doesn't match the provided slot */
	SlotMismatch,
	
	/** The owner doesn't meet the tag requirements to equip this item */
	TagRequirementsFailed,
	
	/** The storage system is not ready (not initialized) */
	StorageNotReady,
	
	/** The item is already equipped in another slot */
	AlreadyEquipped
};

/**
 * @struct FCraterEquippedItemEntry
 * @brief Represents a single equipped item with its associated runtime data.
 * 
 * This structure maintains the complete state of an equipped item including:
 * - Reference to the inventory slot (for lookups and validation)
 * - Cached item data (for quick access without storage lookups)
 * - Lyra item instance (for fragment-based functionality)
 * - Granted GAS resources (abilities, effects, tags) for cleanup
 * 
 * ## Lifecycle
 * 1. Created empty when equipment slots are initialized
 * 2. Populated when an item is equipped (via Server_EquipItem)
 * 3. Grants abilities/effects to the owner's ASC
 * 4. Reset when item is unequipped (via Server_UnequipItem)
 * 
 * ## Network Replication
 * This struct is replicated to the owning client only (COND_OwnerOnly)
 * for bandwidth efficiency.
 * 
 * @see UCraterInventoryManagerComponent::Server_EquipItem
 * @see UCraterInventoryManagerComponent::GrantAbilitiesFromItem
 */
USTRUCT(BlueprintType)
struct FCraterEquippedItemEntry
{
	GENERATED_BODY()

	FCraterEquippedItemEntry() = default;

	/** 
	 * The inventory slot index where this item resides.
	 * Used to maintain consistency when storage updates occur.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Crater|Inventory")
	int32 InventorySlotIndex = INDEX_NONE;

	/** 
	 * Cached item information for quick access.
	 * Synchronized with storage on updates to avoid constant lookups.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Crater|Inventory")
	FElementusItemInfo ItemInfo;

	/** 
	 * The Lyra item instance created for this equipped item.
	 * Provides access to fragments and Lyra-specific functionality.
	 * May be nullptr if the item has no Lyra definition.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Crater|Inventory")
	TObjectPtr<ULyraInventoryItemInstance> LyraInstance = nullptr;

	/** 
	 * Gameplay ability spec handles granted by this item.
	 * Stored for revocation when the item is unequipped.
	 */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	/** 
	 * Active gameplay effect handles applied by this item.
	 * Stored for removal when the item is unequipped.
	 */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;

	/** 
	 * Gameplay tags granted by this equipment.
	 * Stored for removal when the item is unequipped.
	 */
	UPROPERTY()
	FGameplayTagContainer GrantedTags;

	/**
	 * Checks if this entry contains a valid equipped item.
	 * @return True if the entry has a valid inventory slot index
	 */
	bool IsValid() const { return InventorySlotIndex != INDEX_NONE; }
	
	/**
	 * Resets this entry to its empty state.
	 * Clears all data and handles, preparing for reuse.
	 */
	void Reset()
	{
		InventorySlotIndex = INDEX_NONE;
		ItemInfo = FElementusItemInfo();
		LyraInstance = nullptr;
		GrantedAbilityHandles.Empty();
		AppliedEffectHandles.Empty();
		GrantedTags.Reset();
	}
};

/**
 * @class UCraterInventoryManagerComponent
 * @brief The authoritative controller of the inventory system.
 * 
 * Acts as a bridge between the PlayerController and the data storage (Elementus),
 * while integrating with Lyra's item definition and fragment system.
 * 
 * ## Key Responsibilities
 * - **Transaction Management**: All inventory modifications go through server RPCs
 * - **Equipment System**: Manages equipped items and their GAS integration
 * - **Storage Bridge**: Connects to Elementus for persistence and replication
 * - **State Synchronization**: Keeps equipped items in sync with storage updates
 * 
 * ## Usage Example
 * @code
 * // Find the inventory manager on a player pawn
 * UCraterInventoryManagerComponent* Inventory = 
 *     UCraterInventoryManagerComponent::FindInventoryManager(PlayerPawn);
 * 
 * // Add an item (server RPC)
 * FPrimaryElementusItemId ItemId = FPrimaryElementusItemId(TEXT("Weapon.Sword"));
 * Inventory->Server_AddItem(ItemId, 1);
 * 
 * // Equip the item from slot 0 to equipment slot 0
 * Inventory->Server_EquipItem(0, 0);
 * 
 * // Check if item can be equipped (validation)
 * ECraterEquipResult Result = Inventory->CanEquipItem(0, 0);
 * if (Result == ECraterEquipResult::Success) {
 *     // Proceed with equipping
 * }
 * @endcode
 * 
 * ## Threading & Network
 * - **Thread Safety**: Not thread-safe. Must be called from game thread.
 * - **Authority**: All mutations require server authority (ROLE_Authority)
 * - **Replication**: Equipped items replicate to owning client only
 * 
 * ## Design Patterns
 * - **Bridge Pattern**: Decouples inventory operations from storage implementation
 * - **Facade Pattern**: Provides unified interface to Elementus storage and Lyra items
 * - **Observer Pattern**: Reacts to storage changes to sync Lyra instances and GAS
 * - **Authority Pattern**: Centralizes control to ensure consistency and security
 * 
 * @note Elementus manages item storage order internally. This component tracks items by
 *       ItemId rather than array position to maintain equipped item consistency across
 *       storage updates.
 * 
 * @see UElementusInventoryComponent
 * @see ULyraInventoryItemDefinition
 * @see UCraterInventoryFragment_Equippable
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CRATERINVENTORY_API UCraterInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraterInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	/** 
	 * Special value indicating automatic equipment slot selection.
	 * Pass this to Server_EquipItem to let the system choose the best slot.
	 */
	static constexpr int32 AUTO_EQUIP_SLOT = -1;

	// ~Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// ~End UActorComponent Interface

	/**
	 * @brief Finds the inventory manager component attached to the specified actor.
	 * @param Actor The actor to search for the inventory manager component.
	 * @return The found inventory manager component, or nullptr if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	static UCraterInventoryManagerComponent* FindInventoryManager(AActor* Actor);

	/**
	 * Requests to add items to the inventory.
	 * 
	 * This is a server RPC that adds a specified quantity of an item to the player's
	 * inventory. The operation is validated and executed on the server, then changes
	 * are replicated to the owning client through the Elementus storage system.
	 * 
	 * ## Behavior
	 * - Server-authoritative: Only executes on ROLE_Authority
	 * - If the item already exists, increases the stack
	 * - If the inventory is full, behavior depends on Elementus configuration
	 * 
	 * ## Example
	 * @code
	 * FPrimaryElementusItemId SwordId(TEXT("Weapon.IronSword"));
	 * InventoryManager->Server_AddItem(SwordId, 1);
	 * @endcode
	 * 
	 * @param ItemId The unique identifier of the item to add
	 * @param Quantity The number of items to add (must be positive, default: 1)
	 * 
	 * @note Requires valid storage component. Operation logged for debugging.
	 * @see Server_RemoveItem, Server_DiscardItem
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_AddItem(const FPrimaryElementusItemId& ItemId, const int32 Quantity = 1);

	/**
	 * Requests to remove items from the inventory.
	 * 
	 * Removes a specified quantity of an item by its ID. If multiple stacks exist,
	 * Elementus handles which stacks to remove from. If the quantity exceeds available
	 * amount, all instances of the item are removed.
	 * 
	 * ## Behavior
	 * - Server-authoritative RPC
	 * - Will not remove equipped items (unequip first)
	 * - Logs warning if attempting to remove more than available
	 * 
	 * @param ItemId The unique identifier of the item to remove
	 * @param Quantity The number of items to remove (must be positive, default: 1)
	 * 
	 * @note Equipped items must be unequipped before removal
	 * @see Server_AddItem, Server_DiscardItem
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_RemoveItem(const FPrimaryElementusItemId& ItemId, const int32 Quantity = 1);

	/**
	 * Requests to discard items from a specific inventory slot.
	 * 
	 * Unlike Server_RemoveItem which operates by ItemId, this method targets a
	 * specific slot index. Useful for UI-driven operations where the player clicks
	 * a slot to discard its contents.
	 * 
	 * ## Safety Checks
	 * - Validates slot index is within bounds
	 * - Prevents discarding equipped items (must unequip first)
	 * - Clamps quantity to available amount in the slot
	 * 
	 * ## Example
	 * @code
	 * // Discard half the stack at slot 5
	 * int32 SlotIndex = 5;
	 * int32 HalfStack = ItemsArray[SlotIndex].Quantity / 2;
	 * InventoryManager->Server_DiscardItem(SlotIndex, HalfStack);
	 * @endcode
	 * 
	 * @param SlotIndex The inventory array index to discard from (0-based)
	 * @param Quantity The number of items to discard (default: 1)
	 * 
	 * @note Fails with warning if item at slot is currently equipped
	 * @see Server_RemoveItem, Server_UnequipItem
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_DiscardItem(int32 SlotIndex, int32 Quantity = 1);

	/**
	 * Requests to swap two items between inventory slots.
	 * 
	 * Exchanges the positions of items in two inventory slots. The swap is implemented
	 * using remove-and-add since Elementus manages internal slot ordering. If either
	 * item is equipped, the equipment slot index is automatically updated.
	 * 
	 * ## Algorithm
	 * 1. Validates both slot indices
	 * 2. Captures item data from both slots
	 * 3. Removes both items from storage
	 * 4. Re-adds items in swapped positions
	 * 5. Updates equipped item indices if needed
	 * 
	 * @param FromSlotIndex The source slot index (0-based)
	 * @param ToSlotIndex The destination slot index (0-based)
	 * 
	 * @note No operation if indices are identical. Updates equipment tracking automatically.
	 * @see UpdateEquippedIndicesAfterSwap
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_SwapItems(int32 FromSlotIndex, int32 ToSlotIndex);


	/**
	 * Requests to equip an item from inventory.
	 * 
	 * Moves an item from the player's inventory into an equipment slot, granting
	 * any associated abilities, effects, and tags to the owning actor's ASC.
	 * 
	 * ## Process
	 * 1. Validates item can be equipped (via CanEquipItem)
	 * 2. Determines target equipment slot (auto-select if AUTO_EQUIP_SLOT)
	 * 3. Unequips any existing item in the target slot
	 * 4. Creates Lyra item instance for fragment access
	 * 5. Grants abilities, effects, and tags from equippable fragment
	 * 6. Broadcasts OnItemEquipped event
	 * 
	 * ## Equipment Slot Selection
	 * - Pass a specific slot index (0-based) to force a particular slot
	 * - Pass AUTO_EQUIP_SLOT (-1) to let the system choose automatically
	 * - System respects fragment-defined required slots when auto-selecting
	 * 
	 * ## Example
	 * @code
	 * // Equip item from inventory slot 0 into equipment slot 0
	 * InventoryManager->Server_EquipItem(0, 0);
	 * 
	 * // Auto-select best equipment slot
	 * InventoryManager->Server_EquipItem(2, AUTO_EQUIP_SLOT);
	 * @endcode
	 * 
	 * @param SlotIndex The inventory slot index containing the item to equip (0-based)
	 * @param EquipmentSlot Target equipment slot (0-based), or AUTO_EQUIP_SLOT for automatic
	 * 
	 * @note Automatically unequips items in the target slot. Fails if validation fails.
	 * @see Server_UnequipItem, CanEquipItem, GrantAbilitiesFromItem
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_EquipItem(int32 SlotIndex, int32 EquipmentSlot = -1);

	/**
	 * Requests to unequip an item from an equipment slot.
	 * 
	 * Removes an equipped item, returning it to inventory state and revoking
	 * all granted abilities, effects, and tags. The item remains in inventory
	 * but is no longer actively equipped.
	 * 
	 * ## Cleanup Process
	 * 1. Validates equipment slot has an item
	 * 2. Revokes all abilities granted by the item
	 * 3. Removes all active effects applied by the item
	 * 4. Removes all tags granted by the item
	 * 5. Clears the Lyra item instance
	 * 6. Broadcasts OnItemUnequipped event
	 * 
	 * @param EquipmentSlot The equipment slot to unequip (0-based)
	 * 
	 * @note No-op if slot is already empty. Logs verbose message.
	 * @see Server_EquipItem, RevokeAbilitiesFromItem
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_UnequipItem(int32 EquipmentSlot);

	/**
	 * Validates whether an item can be equipped.
	 * 
	 * Performs comprehensive validation before allowing an equip operation,
	 * checking storage state, item properties, slot availability, and tag requirements.
	 * 
	 * ## Validation Checks (in order)
	 * 1. Storage system is ready
	 * 2. Inventory slot index is valid
	 * 3. Item is not already equipped
	 * 4. Item has Lyra integration data
	 * 5. Item is marked as equippable
	 * 6. Equipment slot is available or can be determined
	 * 7. Slot matches any required slot restrictions
	 * 8. Owner meets tag requirements
	 * 9. Owner doesn't have blocking tags
	 * 
	 * ## Usage
	 * Call this before Server_EquipItem to check if the operation will succeed,
	 * enabling UI to show appropriate feedback to the player.
	 * 
	 * @param SlotIndex The inventory slot containing the item (0-based)
	 * @param EquipmentSlot Target equipment slot, or AUTO_EQUIP_SLOT for auto-select
	 * @return Result code indicating success or the specific failure reason
	 * 
	 * @see ECraterEquipResult for all possible result codes
	 * @see Server_EquipItem
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	ECraterEquipResult CanEquipItem(int32 SlotIndex, int32 EquipmentSlot = -1) const;

	/**
	 * Quick check if an item has equippable configuration.
	 * 
	 * Lightweight validation that only checks if the item at the given slot
	 * has Lyra integration data and is flagged as equippable. Does not perform
	 * full validation like CanEquipItem().
	 * 
	 * ## Use Cases
	 * - UI icons to show which items can be equipped
	 * - Quick filters for equipment-only views
	 * - Performance-sensitive checks (avoids full validation)
	 * 
	 * @param SlotIndex The inventory slot to check (0-based)
	 * @return True if the item has equippable data, false otherwise
	 * 
	 * @note This does NOT check tag requirements or slot availability
	 * @see CanEquipItem for comprehensive validation
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	bool IsSlotItemEquippable(int32 SlotIndex) const;

	/**
	 * @brief Validates that the storage system is ready for operations.
	 * @return True if storage is ready, false otherwise.
	 */
	bool ValidateStorageReady() const;

	/**
	 * @brief Validates that an inventory slot index is valid.
	 * @param SlotIndex The slot index to validate.
	 * @return True if the slot index is valid.
	 */
	bool ValidateInventorySlot(int32 SlotIndex) const;

	/**
	 * @brief Validates that an equipment slot is valid.
	 * @param EquipmentSlot The equipment slot to validate.
	 * @return True if the equipment slot is valid.
	 */
	bool IsValidEquipmentSlot(int32 EquipmentSlot) const;

	/**
	 * Retrieves all items currently in the inventory.
	 * 
	 * Returns a copy of the complete inventory array from the underlying Elementus
	 * storage component. Use this for displaying full inventory UI or performing
	 * searches across all items.
	 * 
	 * ## Performance Note
	 * This creates a copy of the array. For frequent access or iteration,
	 * consider caching the result if the inventory hasn't changed.
	 * 
	 * @return Array of all inventory items. Empty array if storage not ready.
	 * 
	 * @see GetItemAtSlot for single-item access
	 * @see OnInventoryChanged for change notifications
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	TArray<FElementusItemInfo> GetInventoryItems() const;

	/**
	 * Retrieves an item from a specific inventory slot.
	 * 
	 * Fetches information about the item at the given slot index. Prefer this
	 * over GetInventoryItems() when you only need data about a single slot.
	 * 
	 * @param SlotIndex The slot index to query (0-based)
	 * @param OutItemInfo [out] Populated with item data if slot is valid
	 * @return True if slot is valid and contains an item, false otherwise
	 * 
	 * @note Returns false if slot is out of bounds or storage not ready
	 * @see GetInventoryItems
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	bool GetItemAtSlot(int32 SlotIndex, FElementusItemInfo& OutItemInfo) const;

	/**
	 * Retrieves all currently equipped items.
	 * 
	 * Returns a copy of the equipped items array, where each index corresponds
	 * to an equipment slot. Empty slots will have invalid entries (IsValid() == false).
	 * 
	 * ## Example
	 * @code
	 * TArray<FCraterEquippedItemEntry> Equipped = Inventory->GetEquippedItems();
	 * for (int32 i = 0; i < Equipped.Num(); ++i)
	 * {
	 *     if (Equipped[i].IsValid())
	 *     {
	 *         UE_LOG(LogTemp, Log, TEXT("Slot %d: %s"), i, *Equipped[i].ItemInfo.ItemId.ToString());
	 *     }
	 * }
	 * @endcode
	 * 
	 * @return Array of equipped item entries (size equals MaxEquipmentSlots)
	 * 
	 * @see GetEquippedItemAtSlot, GetMaxEquipmentSlots
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	TArray<FCraterEquippedItemEntry> GetEquippedItems() const;

	/**
	 * Retrieves the equipped item at a specific equipment slot.
	 * 
	 * @param EquipmentSlot The equipment slot to query (0-based)
	 * @param OutEntry [out] Populated with equipped item data if valid
	 * @return True if slot contains an equipped item, false if empty or invalid
	 * 
	 * @see GetEquippedItems, IsItemEquipped
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	bool GetEquippedItemAtSlot(int32 EquipmentSlot, FCraterEquippedItemEntry& OutEntry) const;

	/**
	 * Checks if an inventory slot's item is currently equipped.
	 * 
	 * Searches through all equipment slots to determine if the item at the
	 * given inventory slot is currently equipped anywhere.
	 * 
	 * @param SlotIndex The inventory slot to check (0-based)
	 * @return True if the item is equipped in any equipment slot
	 * 
	 * @note Useful for UI to grey out or mark equipped items
	 * @see GetEquippedItemAtSlot
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	bool IsItemEquipped(int32 SlotIndex) const;

	/**
	 * Gets the maximum number of equipment slots.
	 * 
	 * This value is configured in the component's defaults and determines
	 * how many items can be equipped simultaneously.
	 * 
	 * @return The maximum equipment slot count (configured in editor)
	 * 
	 * @see MaxEquipmentSlots property
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	int32 GetMaxEquipmentSlots() const { return MaxEquipmentSlots; }

	/**
	 * Checks if the storage system is ready for operations.
	 * 
	 * The storage system is ready after successful binding in BeginPlay.
	 * All inventory operations require ready storage.
	 * 
	 * @return True if InventoryStorage component is bound and valid
	 * 
	 * @note Listen to OnInventoryInitialized for notification when ready
	 * @see BindToStorage, OnInventoryInitialized
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	bool IsStorageReady() const { return InventoryStorage != nullptr; }

	/**
	 * Gets the Lyra item instance for an equipped item.
	 * 
	 * Lyra item instances provide access to the fragment system and item-specific
	 * functionality. This method retrieves the instance associated with an equipped
	 * item, if one exists.
	 * 
	 * ## Use Cases
	 * - Accessing item fragments for additional data
	 * - Calling fragment-specific methods
	 * - Integrating with Lyra's inventory UI widgets
	 * 
	 * @param EquipmentSlot The equipment slot to query (0-based)
	 * @return The Lyra instance if equipped and has definition, nullptr otherwise
	 * 
	 * @note Not all items have Lyra definitions. Check for nullptr.
	 * @see CreateLyraInstanceFromItem
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory|Lyra")
	ULyraInventoryItemInstance* GetLyraInstanceAtSlot(int32 EquipmentSlot) const;

	/**
	 * Creates a Lyra item instance from an Elementus item.
	 * 
	 * Bridges Elementus item data to Lyra's fragment-based item system by
	 * creating a ULyraInventoryItemInstance with the associated definition.
	 * The instance is created as a newobject and persists until destroyed.
	 * 
	 * ## Process
	 * 1. Retrieves Lyra integration data from the item
	 * 2. Loads the Lyra item definition synchronously
	 * 3. Creates new instance from definition
	 * 4. Initializes instance (calls OnInstanceCreated on fragments)
	 * 
	 * @param ItemInfo The Elementus item to convert to Lyra instance
	 * @return Created instance if item has Lyra definition, nullptr otherwise
	 * 
	 * @warning Performs synchronous asset load. Consider async version for production.
	 * @see GetLyraInstanceAtSlot
	 */
	UFUNCTION(BlueprintCallable, Category = "Crater|Inventory|Lyra")
	ULyraInventoryItemInstance* CreateLyraInstanceFromItem(const FElementusItemInfo& ItemInfo);

protected:
	/** Binds the inventory manager to the storage system. */
	void BindToStorage();

	/** Handles updates from the storage system. */
	UFUNCTION()
	void HandleStorageUpdate();

	/**
	 * 
	 * @param FromIndex 
	 * @param ToIndex 
	 */
	void UpdateEquippedIndicesAfterSwap(int32 FromIndex, int32 ToIndex);

	/** Finds the first available equipment slot. Returns INDEX_NONE if none available. */
	int32 FindFirstAvailableEquipmentSlot() const;

	/** 
	 * @brief Finds an appropriate equipment slot considering fragment requirements.
	 * @param EquipFragment The equippable fragment to check requirements (can be null).
	 * @param PreferredSlot The preferred slot (-1 for auto).
	 * @return A valid equipment slot index or INDEX_NONE.
	 */
	int32 FindAppropriateEquipmentSlot(const UCraterInventoryFragment_Equippable* EquipFragment, int32 PreferredSlot) const;

	/** Validates that all equipped items still exist in the inventory. */
	void ValidateEquippedItems();

	/** Synchronizes Lyra item instances with equipped items. */
	void SyncLyraInstances();

	/** Grants abilities, applies effects, and adds tags from an equipped item. */
	void GrantAbilitiesFromItem(int32 EquipmentSlot, const FElementusItemInfo& ItemInfo);

	/** Revokes abilities, removes effects, and clears tags that were granted from an item. */
	void RevokeAbilitiesFromItem(int32 EquipmentSlot);

	/** Gets the owner's current gameplay tags for validation. */
	FGameplayTagContainer GetOwnerTags() const;

	/** 
	 * Gets the Ability System Component from the owner with lazy caching.
	 * @note Non-const to allow updating the cached reference.
	 */
	UAbilitySystemComponent* GetOwnerASC();

	/** Loads the Lyra item definition from an Elementus item data. */
	TSubclassOf<ULyraInventoryItemDefinition> LoadLyraDefinitionForItem(const FPrimaryElementusItemId& ItemId) const;

	/** Gets the Lyra integration data for an item, if it exists. */
	const UElementusItemData_LyraIntegration* GetLyraIntegrationData(const FPrimaryElementusItemId& ItemId) const;

	/** Gets the equippable fragment for an item, if it exists. */
	const UCraterInventoryFragment_Equippable* GetEquippableFragment(const FPrimaryElementusItemId& ItemId) const;

public:
public:
	/**
	 * Broadcasts when the inventory system finishes initialization.
	 * 
	 * This event fires after the component successfully binds to the Elementus
	 * storage system during BeginPlay. UI and other systems should wait for
	 * this event before attempting inventory operations.
	 * 
	 * ## Timing
	 * - Fires once during BeginPlay after storage binding succeeds
	 * - Fires on both server and client
	 * - Will not fire if storage component is missing (error logged instead)
	 * 
	 * ## Example Usage
	 * @code
	 * void UMyInventoryWidget::NativeConstruct()
	 * {
	 *     Super::NativeConstruct();
	 *     
	 *     if (UCraterInventoryManagerComponent* Inventory = GetInventoryManager())
	 *     {
	 *         Inventory->OnInventoryInitialized.AddDynamic(this, &UMyInventoryWidget::HandleInventoryReady);
	 *     }
	 * }
	 * 
	 * void UMyInventoryWidget::HandleInventoryReady()
	 * {
	 *     RefreshInventoryDisplay();
	 * }
	 * @endcode
	 * 
	 * @see BindToStorage
	 */
	UPROPERTY(BlueprintAssignable, Category = "Crater|Inventory")
	FOnCraterInventoryInitialized OnInventoryInitialized;

	/**
	 * Broadcasts when the inventory contents change.
	 * 
	 * This event fires whenever items are added, removed, swapped, or when
	 * equipped items are replicated to the client. Use this to refresh UI
	 * displays or trigger gameplay reactions to inventory changes.
	 * 
	 * ## Trigger Conditions
	 * - Items added via Server_AddItem
	 * - Items removed via Server_RemoveItem or Server_DiscardItem
	 * - Items swapped via Server_SwapItems
	 * - Equipped items replicated (OnRep_EquippedItems)
	 * - Storage update processed (HandleStorageUpdate)
	 * 
	 * ## Example Usage
	 * @code
	 * Inventory->OnInventoryChanged.AddDynamic(this, &UMyWidget::RefreshDisplay);
	 * @endcode
	 * 
	 * @note May fire multiple times for a single user action (e.g., swap triggers storage update)
	 * @see OnItemEquipped, OnItemUnequipped for equipment-specific events
	 */
	UPROPERTY(BlueprintAssignable, Category = "Crater|Inventory")
	FOnCraterInventoryChanged OnInventoryChanged;

	/**
	 * Broadcasts when an item is successfully equipped.
	 * 
	 * Fires after an item is equipped and abilities/effects have been granted.
	 * Provides the equipment slot index and item information for UI updates
	 * or gameplay reactions.
	 * 
	 * ## Parameters
	 * - **SlotIndex**: The equipment slot where the item was equipped (0-based)
	 * - **ItemInfo**: Complete information about the equipped item
	 * 
	 * ## Example Usage
	 * @code
	 * void AMyCharacter::OnItemEquippedHandler(int32 Slot, const FElementusItemInfo& Item)
	 * {
	 *     UE_LOG(LogTemp, Log, TEXT("Equipped %s to slot %d"), *Item.ItemId.ToString(), Slot);
	 *     
	 *     // Update character visuals
	 *     if (Slot == 0) // Weapon slot
	 *     {
	 *         AttachWeaponMesh(Item.ItemId);
	 *     }
	 * }
	 * @endcode
	 * 
	 * @see Server_EquipItem, OnItemUnequipped
	 */
	UPROPERTY(BlueprintAssignable, Category = "Crater|Inventory")
	FOnCraterItemEquipped OnItemEquipped;

	/**
	 * Broadcasts when an item is successfully unequipped.
	 * 
	 * Fires after an item is unequipped and abilities/effects have been revoked.
	 * Use this to update visuals, UI, or gameplay state when equipment changes.
	 * 
	 * ## Parameters
	 * - **SlotIndex**: The equipment slot that was emptied (0-based)
	 * - **ItemInfo**: Information about the item that was unequipped
	 * 
	 * ## Example Usage
	 * @code
	 * void AMyCharacter::OnItemUnequippedHandler(int32 Slot, const FElementusItemInfo& Item)
	 * {
	 *     if (Slot == 0) // Weapon slot
	 *     {
	 *         DetachWeaponMesh();
	 *     }
	 * }
	 * @endcode
	 * 
	 * @see Server_UnequipItem, OnItemEquipped
	 */
	UPROPERTY(BlueprintAssignable, Category = "Crater|Inventory")
	FOnCraterItemUnequipped OnItemUnequipped;

protected:
	/**
	 * Reference to the Elementus storage component.
	 * 
	 * This component handles the actual item data storage, persistence,
	 * and replication. Set during BindToStorage() in BeginPlay.
	 * 
	 * @note Transient - not saved. Re-acquired on component initialization.
	 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Crater|Inventory")
	TObjectPtr<UElementusInventoryComponent> InventoryStorage;

	/**
	 * Array of currently equipped items indexed by equipment slot.
	 * 
	 * Each index represents an equipment slot (0 to MaxEquipmentSlots-1).
	 * Empty slots have entries with InventorySlotIndex == INDEX_NONE.
	 * 
	 * ## Replication
	 * Replicated to owning client only (COND_OwnerOnly) for bandwidth efficiency.
	 * Uses OnRep_EquippedItems for client notification.
	 * 
	 * @note Array size is fixed to MaxEquipmentSlots on initialization
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, VisibleAnywhere, BlueprintReadOnly, Category = "Crater|Inventory")
	TArray<FCraterEquippedItemEntry> EquippedItems;

	/**
	 * Maximum number of equipment slots available.
	 * 
	 * Determines how many items can be equipped simultaneously.
	 * Set this in the component defaults or Blueprint class defaults.
	 * 
	 * ## Constraints
	 * - Min: 1 slot
	 * - Max: 10 slots (editor UI clamping)
	 * - Default: 3 slots
	 * 
	 * @note Changing this at runtime has no effect. Set before initialization.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crater|Inventory", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxEquipmentSlots = 3;

	/**
	 * Cached reference to the owner's Ability System Component.
	 * 
	 * Lazily initialized on first access to handle cases where ASC isn't
	 * available at BeginPlay (common in Lyra with late ASC initialization).
	 * 
	 * Uses weak pointer to avoid preventing ASC garbage collection.
	 * 
	 * @see GetOwnerASC
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

private:
	UFUNCTION()
	void OnRep_EquippedItems();

	/** Flag to prevent recursive update handling */
	bool bIsProcessingUpdate = false;
};