// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Management/ElementusInventoryData.h"
#include "CraterInventoryManagerComponent.generated.h"

// Forward Declarations
class UElementusInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraterInventoryInitialized);

/**
 * @brief The authoritative controller of the inventory system.
 * @details It acts as a bridge between the PlayerController and the data storage (Elementus).
 * Utilized patterns:
 *  - Bridge Pattern: Decouples the abstraction (inventory operations) from its implementation (data storage).
 *  - Authority Pattern: Centralizes control of inventory operations to ensure consistency and integrity.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CRATERINVENTORY_API UCraterInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Constructor
	 * @param ObjectInitializer the object initializer
	 */
	UCraterInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	// Actor Component Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Static Methods
	/**
	 * @brief Finds the inventory manager component attached to the specified actor.
	 * 
	 * @param Actor The actor to search for the inventory manager component.
	 * @return The found inventory manager component, or nullptr if not found.
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	static UCraterInventoryManagerComponent* FindInventoryManager(AActor* Actor);

	// Transactional Logic

	/**
	 * @brief Request to add an item to the inventory.
	 * @details This function is executed on the server to ensure authoritative control over inventory modifications.
	 * 
	 * @param ItemId The ID of the item to be added.
	 * @param Quantity The quantity of the item to be added. Default is 1.
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_AddItem(const FPrimaryElementusItemId& ItemId, const int32 Quantity = 1);

	/**
	 * @brief Request to remove an item from the inventory.
	 * @details This function is executed on the server to ensure authoritative control over inventory modifications.
	 * 
	 * @param ItemId The ID of the item to be removed.
	 * @param Quantity The quantity of the item to be removed. Default is 1.
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_RemoveItem(const FPrimaryElementusItemId& ItemId, const int32 Quantity = 1);

	/**
	 * @brief Requests to swap item slots in the inventory.
	 * @details This function is executed on the server to ensure authoritative control over inventory modifications.
	 *
	 * @param FromSlotIndex The index of the slot to swap from.
	 * @param ToSlotIndex The index of the slot to swap to.
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_SwapItem(int32 FromSlotIndex, int32 ToSlotIndex);

	/**
	 * @brief Requests to discard an item from the inventory.
	 * @details This function is executed on the server to ensure authoritative control over inventory
	 * modifications.
	 *
	 * @param SlotIndex The index of the slot to discard the item from.
	 * @param Quantity The quantity of the item to be discarded. Default is 1.
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Crater|Inventory")
	void Server_DiscardItem(int32 SlotIndex, int32 Quantity = 1);

	// Inventory Accessors
	/**
	 * @brief Retrieves the current list of items in the inventory.
	 * @details This function is marked as BlueprintPure, indicating it does not modify the state
	 * and can be called in Blueprint without causing side effects.
	 * 
	 * @return The current list of items in the inventory.
	 */
	UFUNCTION(BlueprintPure, Category = "Crater|Inventory")
	TArray<FElementusItemInfo> GetInventoryItems() const;

protected:

	/**
	 * @brief Binds the inventory manager to the storage system.
	 * @details This function establishes the connection between the inventory manager and the underlying
	 * storage system, allowing for synchronization and updates.
	 */
	void BindToStorage();

	/**
	 * @brief Handles updates from the storage system.
	 * @details This function is called when the underlying storage system notifies of changes,
	 * allowing the inventory manager to synchronize its state accordingly.
	 */
	UFUNCTION()
	void HandleStorageUpdate();

public:
	/** Broadcasts when the component effectively links with the Storage system */
	UPROPERTY(BlueprintAssignable, Category = "Crater|Inventory")
	FOnCraterInventoryInitialized OnInventoryInitialized;

protected:
	/** The inventory component that holds the actual items */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Crater|Inventory")
	TObjectPtr<UElementusInventoryComponent> InventoryStorage;
};
