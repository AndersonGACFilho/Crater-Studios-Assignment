// Fill out your copyright notice in the Description page of Project Settings.


#include "CraterInventoryManagerComponent.h"

#include "CraterLogChannels.h"
#include "Components/ElementusInventoryComponent.h"

UCraterInventoryManagerComponent::UCraterInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraterInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the Bridge to the storage component
	BindToStorage();
}

void UCraterInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryStorage)
	{
		InventoryStorage->OnInventoryUpdate.RemoveDynamic(this, &ThisClass::HandleStorageUpdate);
	}

	Super::EndPlay(EndPlayReason);
}

UCraterInventoryManagerComponent* UCraterInventoryManagerComponent::FindInventoryManager(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UCraterInventoryManagerComponent>() : nullptr;
}

void UCraterInventoryManagerComponent::BindToStorage()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Locate the Elementus Component (The "Database" Storage)
	// We expect this to be injected via GameFeatureAction_AddComponents alongside this manager
	InventoryStorage = Owner->FindComponentByClass<UElementusInventoryComponent>();

	if (InventoryStorage)
	{
		CRATER_FUNC_LOG(LogCraterInventory, Display, TEXT("Successfully bridged to Storage: %s"), *InventoryStorage->GetName());

		// Bind to storage updates to trigger GAS logic later
		InventoryStorage->OnInventoryUpdate.AddDynamic(this, &ThisClass::HandleStorageUpdate);

		// Notify Listeners (UI) that we are ready
		if (OnInventoryInitialized.IsBound())
		{
			OnInventoryInitialized.Broadcast();
		}
	}
	else
	{
		CRATER_FUNC_LOG(LogCraterInventory, Error, TEXT("Failed to find UElementusInventoryComponent on Owner! Inventory Logic will fail."));
	}
}

void UCraterInventoryManagerComponent::Server_AddItem_Implementation(const FPrimaryElementusItemId& ItemId, int32 Quantity)
{
	if (!InventoryStorage)
	{
		CRATER_FUNC_LOG(LogCraterInventory, Warning, TEXT("Storage not ready. Cannot Add Item."));
		return;
	}

	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Adding Item %s (x%d)"), *ItemId.ToString(), Quantity);
	
	// Create the info struct required by Elementus
	FElementusItemInfo ItemInfo(ItemId, Quantity);
	
	// Execute on Storage (Elementus handles the replication dirty marking)
	TArray<FElementusItemInfo> ItemsToAdd;
	ItemsToAdd.Add(ItemInfo);
	InventoryStorage->AddItems(ItemsToAdd);
}

void UCraterInventoryManagerComponent::Server_RemoveItem_Implementation(const FPrimaryElementusItemId& ItemId, int32 Quantity)
{
	if (!InventoryStorage) return;

	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Removing Item %s (x%d)"), *ItemId.ToString(), Quantity);

	FElementusItemInfo ItemInfo(ItemId, Quantity);
	TArray<FElementusItemInfo> ItemsToRemove;
	ItemsToRemove.Add(ItemInfo);
	
	InventoryStorage->DiscardItems(ItemsToRemove);
}

void UCraterInventoryManagerComponent::Server_SwapItem_Implementation(int32 FromIndex, int32 ToIndex)
{
	// Elementus currently handles sorting via 'SortInventory', but manual swapping 
	// would require direct manipulation of the array if permitted.
	// For this assignment, we ensure the request reaches the server. 
	// Since Elementus doesn't expose a direct "Swap" index function easily without modifying source,
	// we log this intent. In a full implementation, we would extend Elementus or manipulate the TArray if accessible.
	
	CRATER_LOG(LogCraterInventory, Warning, TEXT("Swap Requested: %d -> %d. Note: Elementus native swap requires extension."), FromIndex, ToIndex);
}

void UCraterInventoryManagerComponent::Server_DiscardItem_Implementation(int32 ItemIndex, int32 Quantity)
{
	if (!InventoryStorage) return;

	// We need to fetch the item at the specific index to know what to remove
	if (InventoryStorage->GetItemsArray().IsValidIndex(ItemIndex))
	{
		FElementusItemInfo ItemAtIndex = InventoryStorage->GetItemsArray()[ItemIndex];
		
		// Clamp quantity
		int32 RemoveQty = FMath::Min(Quantity, ItemAtIndex.Quantity);
		
		FElementusItemInfo ItemToRemove(ItemAtIndex.ItemId, RemoveQty);
		TArray<FElementusItemInfo> DiscardList;
		DiscardList.Add(ItemToRemove);

		InventoryStorage->DiscardItems(DiscardList);
	}
}

TArray<FElementusItemInfo> UCraterInventoryManagerComponent::GetInventoryItems() const
{
	if (InventoryStorage)
	{
		return InventoryStorage->GetItemsArray();
	}
	
	// Fallback empty array
	static const TArray<FElementusItemInfo> EmptyInventory;
	return EmptyInventory;
}

void UCraterInventoryManagerComponent::HandleStorageUpdate()
{
	// This is where the Observer Pattern triggers.
	// When Data changes (Elementus), we will check for Equipped items and Grant Abilities (GAS).
	CRATER_FUNC_LOG(LogCraterInventory, Verbose, TEXT("Storage Updated. Refreshing Gameplay Cues..."));
}

