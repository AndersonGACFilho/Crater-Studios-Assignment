/**
 * @file CraterInventoryManagerComponent.cpp
 * @brief Implementation of the authoritative inventory management system for Crater Inventory.
 * 
 * This file implements the core inventory manager that bridges Elementus storage with Lyra's
 * item system while providing GAS (Gameplay Ability System) integration. The component handles
 * server-authoritative inventory operations, equipment management, and client replication.
 * 
 * @see UCraterInventoryManagerComponent
 * @author Andy
 * @date 2025-11-28
 */

#include "CraterInventoryManagerComponent.h"

#include "CraterLogChannels.h"
#include "ElementusItemData_LyraIntegration.h"
#include "CraterInventoryFragment_Equippable.h"
#include "Components/ElementusInventoryComponent.h"
#include "Management/ElementusInventoryFunctions.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"

UCraterInventoryManagerComponent::UCraterInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraterInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize equipment slots array
	EquippedItems.SetNum(MaxEquipmentSlots);

	// Initialize the Bridge to the storage component
	BindToStorage();
}

void UCraterInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		for (int32 i = 0; i < EquippedItems.Num(); ++i)
		{
			if (!EquippedItems[i].IsValid())
			{
				continue;
			}
			RevokeAbilitiesFromItem(i);
		}
	}

	if (InventoryStorage)
	{
		InventoryStorage->OnInventoryUpdate.RemoveDynamic(this, &ThisClass::HandleStorageUpdate);
	}

	Super::EndPlay(EndPlayReason);
}

void UCraterInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, EquippedItems, COND_OwnerOnly);
}

UCraterInventoryManagerComponent* UCraterInventoryManagerComponent::FindInventoryManager(AActor* Actor)
{
	if (!Actor)
		return nullptr;

	return Actor->FindComponentByClass<UCraterInventoryManagerComponent>();
}

void UCraterInventoryManagerComponent::BindToStorage()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	InventoryStorage = Owner->FindComponentByClass<UElementusInventoryComponent>();
	if (!InventoryStorage)
	{
		CRATER_FUNC_LOG(LogCraterInventory, Error, TEXT("Failed to find UElementusInventoryComponent on Owner! Inventory Logic will fail."));
		return;
	}

	CRATER_FUNC_LOG(LogCraterInventory, Display, TEXT("Successfully bridged to Storage: %s"), *InventoryStorage->GetName());

	InventoryStorage->OnInventoryUpdate.AddDynamic(this, &ThisClass::HandleStorageUpdate);

	OnInventoryInitialized.Broadcast();
}

void UCraterInventoryManagerComponent::Server_AddItem_Implementation(const FPrimaryElementusItemId& ItemId, int32 Quantity)
{
	if (!ValidateStorageReady())
		return;

	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Adding Item %s (x%d)"), *ItemId.ToString(), Quantity);
	
	TArray<FElementusItemInfo> ItemsToAdd;
	ItemsToAdd.Add(FElementusItemInfo(ItemId, Quantity));
	InventoryStorage->AddItems(ItemsToAdd);
}

void UCraterInventoryManagerComponent::Server_RemoveItem_Implementation(const FPrimaryElementusItemId& ItemId, int32 Quantity)
{
	if (!ValidateStorageReady())
		return;

	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Removing Item %s (x%d)"), *ItemId.ToString(), Quantity);

	TArray<FElementusItemInfo> ItemsToRemove;
	ItemsToRemove.Add(FElementusItemInfo(ItemId, Quantity));
	InventoryStorage->DiscardItems(ItemsToRemove);
}

void UCraterInventoryManagerComponent::Server_SwapItems_Implementation(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!ValidateStorageReady())
	{
		return;
	}

	if (FromSlotIndex == ToSlotIndex)
		return;

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	
	if (!Items.IsValidIndex(FromSlotIndex) || !Items.IsValidIndex(ToSlotIndex))
	{
		CRATER_LOG(LogCraterInventory, Warning, TEXT("Swap Failed: Invalid indices From=%d To=%d (ArraySize=%d)"), 
			FromSlotIndex, ToSlotIndex, Items.Num());
		return;
	}

	const FElementusItemInfo FromItem = Items[FromSlotIndex];
	const FElementusItemInfo ToItem = Items[ToSlotIndex];

	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Swapping items at slots %d (%s) <-> %d (%s)"), 
		FromSlotIndex, *FromItem.ItemId.ToString(), 
		ToSlotIndex, *ToItem.ItemId.ToString());

	TArray<FElementusItemInfo> ItemsToRemove;
	ItemsToRemove.Add(FromItem);
	ItemsToRemove.Add(ToItem);
	
	InventoryStorage->DiscardItems(ItemsToRemove);

	TArray<FElementusItemInfo> ItemsToAdd;
	ItemsToAdd.Add(ToItem);
	ItemsToAdd.Add(FromItem);
	InventoryStorage->AddItems(ItemsToAdd);

	UpdateEquippedIndicesAfterSwap(FromSlotIndex, ToSlotIndex);

	CRATER_LOG(LogCraterInventory, Verbose, TEXT("Swap complete. Equipped indices updated."));
}

void UCraterInventoryManagerComponent::Server_DiscardItem_Implementation(int32 ItemIndex, int32 Quantity)
{
	if (!ValidateStorageReady())
		return;

	if (!ValidateInventorySlot(ItemIndex))
		return;

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();

	for (const FCraterEquippedItemEntry& Entry : EquippedItems)
	{
		if (Entry.InventorySlotIndex == ItemIndex)
		{
			CRATER_LOG(LogCraterInventory, Warning, TEXT("Cannot discard equipped item at index %d. Unequip first."), ItemIndex);
			return;
		}
	}

	const FElementusItemInfo& ItemAtIndex = Items[ItemIndex];
	int32 RemoveQty = FMath::Min(Quantity, ItemAtIndex.Quantity);
	
	CRATER_LOG(LogCraterInventory, Log, TEXT("Transaction: Discarding %d of item %s at index %d"), 
		RemoveQty, *ItemAtIndex.ItemId.ToString(), ItemIndex);

	TArray<FElementusItemInfo> DiscardList;
	DiscardList.Add(FElementusItemInfo(ItemAtIndex.ItemId, RemoveQty));
	InventoryStorage->DiscardItems(DiscardList);
}

ECraterEquipResult UCraterInventoryManagerComponent::CanEquipItem(int32 SlotIndex, int32 EquipmentSlot) const
{
	if (!ValidateStorageReady())
		return ECraterEquipResult::StorageNotReady;

	if (!ValidateInventorySlot(SlotIndex))
		return ECraterEquipResult::InvalidSlot;

	if (IsItemEquipped(SlotIndex))
		return ECraterEquipResult::AlreadyEquipped;

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	const FElementusItemInfo& ItemInfo = Items[SlotIndex];

	const UElementusItemData_LyraIntegration* LyraData = GetLyraIntegrationData(ItemInfo.ItemId);
	if (!LyraData || !LyraData->bCanBeEquipped)
		return ECraterEquipResult::ItemNotEquippable;

	const UCraterInventoryFragment_Equippable* EquipFragment = GetEquippableFragment(ItemInfo.ItemId);

	int32 TargetSlot = EquipmentSlot;
	if (TargetSlot == AUTO_EQUIP_SLOT)
	{
		TargetSlot = FindAppropriateEquipmentSlot(EquipFragment, TargetSlot);
		if (TargetSlot == INDEX_NONE)
			return ECraterEquipResult::NoAvailableSlots;
	}

	if (TargetSlot < 0 || TargetSlot >= MaxEquipmentSlots)
		return ECraterEquipResult::InvalidSlot;

	if (EquipFragment && EquipFragment->RequiredEquipmentSlot >= 0)
	{
		if (EquipmentSlot >= 0 && EquipmentSlot != EquipFragment->RequiredEquipmentSlot)
			return ECraterEquipResult::SlotMismatch;
	}

	if (EquipFragment && !EquipFragment->CanBeEquippedBy(GetOwnerTags()))
		return ECraterEquipResult::TagRequirementsFailed;

	return ECraterEquipResult::Success;
}

bool UCraterInventoryManagerComponent::IsSlotItemEquippable(int32 SlotIndex) const
{
	if (!InventoryStorage)
		return false;

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	if (!Items.IsValidIndex(SlotIndex))
		return false;

	const UElementusItemData_LyraIntegration* LyraData = GetLyraIntegrationData(Items[SlotIndex].ItemId);
	return LyraData && LyraData->bCanBeEquipped;
}

FGameplayTagContainer UCraterInventoryManagerComponent::GetOwnerTags() const
{
	if (CachedASC.IsValid())
	{
		FGameplayTagContainer OwnerTags;
		CachedASC->GetOwnedGameplayTags(OwnerTags);
		return OwnerTags;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
		return FGameplayTagContainer();

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!ASC)
		return FGameplayTagContainer();

	FGameplayTagContainer OwnerTags;
	ASC->GetOwnedGameplayTags(OwnerTags);
	return OwnerTags;
}

void UCraterInventoryManagerComponent::Server_EquipItem_Implementation(int32 SlotIndex, int32 EquipmentSlot)
{
	ECraterEquipResult Result = CanEquipItem(SlotIndex, EquipmentSlot);
	if (Result != ECraterEquipResult::Success)
	{
		CRATER_LOG(LogCraterInventory, Warning, TEXT("Cannot equip item at slot %d: Result=%d"), SlotIndex, static_cast<int32>(Result));
		return;
	}

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	const FElementusItemInfo& ItemInfo = Items[SlotIndex];

	int32 TargetSlot = EquipmentSlot;
	if (TargetSlot == AUTO_EQUIP_SLOT)
	{
		const UCraterInventoryFragment_Equippable* EquipFragment = GetEquippableFragment(ItemInfo.ItemId);
		TargetSlot = FindAppropriateEquipmentSlot(EquipFragment, TargetSlot);
	}

	if (EquippedItems[TargetSlot].IsValid())
	{
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Unequipping existing item in slot %d"), TargetSlot);
		Server_UnequipItem_Implementation(TargetSlot);
	}

	FCraterEquippedItemEntry& Entry = EquippedItems[TargetSlot];
	Entry.InventorySlotIndex = SlotIndex;
	Entry.ItemInfo = ItemInfo;
	Entry.LyraInstance = CreateLyraInstanceFromItem(ItemInfo);

	GrantAbilitiesFromItem(TargetSlot, ItemInfo);

	CRATER_LOG(LogCraterInventory, Display, TEXT("Equipped %s to slot %d"), *ItemInfo.ItemId.ToString(), TargetSlot);

	OnItemEquipped.Broadcast(TargetSlot, ItemInfo);
}

void UCraterInventoryManagerComponent::Server_UnequipItem_Implementation(int32 EquipmentSlot)
{
	if (!IsValidEquipmentSlot(EquipmentSlot))
	{
		CRATER_LOG(LogCraterInventory, Warning, TEXT("Invalid equipment slot %d"), EquipmentSlot);
		return;
	}

	FCraterEquippedItemEntry& Entry = EquippedItems[EquipmentSlot];
	if (!Entry.IsValid())
	{
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Equipment slot %d is already empty"), EquipmentSlot);
		return;
	}

	FElementusItemInfo UnequippedItem = Entry.ItemInfo;
	
	RevokeAbilitiesFromItem(EquipmentSlot);

	Entry.Reset();

	CRATER_LOG(LogCraterInventory, Display, TEXT("Unequipped %s from slot %d"), *UnequippedItem.ItemId.ToString(), EquipmentSlot);

	OnItemUnequipped.Broadcast(EquipmentSlot, UnequippedItem);
}


TArray<FElementusItemInfo> UCraterInventoryManagerComponent::GetInventoryItems() const
{
	if (!InventoryStorage)
		return TArray<FElementusItemInfo>();

	return InventoryStorage->GetItemsArray();
}

bool UCraterInventoryManagerComponent::GetItemAtSlot(int32 SlotIndex, FElementusItemInfo& OutItemInfo) const
{
	if (!InventoryStorage)
		return false;

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	if (!Items.IsValidIndex(SlotIndex))
		return false;

	OutItemInfo = Items[SlotIndex];
	return true;
}

TArray<FCraterEquippedItemEntry> UCraterInventoryManagerComponent::GetEquippedItems() const
{
	return EquippedItems;
}

bool UCraterInventoryManagerComponent::GetEquippedItemAtSlot(int32 EquipmentSlot, FCraterEquippedItemEntry& OutEntry) const
{
	if (!IsValidEquipmentSlot(EquipmentSlot))
		return false;

	OutEntry = EquippedItems[EquipmentSlot];
	return OutEntry.IsValid();
}

bool UCraterInventoryManagerComponent::IsItemEquipped(int32 SlotIndex) const
{
	for (const FCraterEquippedItemEntry& Entry : EquippedItems)
	{
		if (Entry.IsValid() && Entry.InventorySlotIndex == SlotIndex)
			return true;
	}
	return false;
}

int32 UCraterInventoryManagerComponent::FindFirstAvailableEquipmentSlot() const
{
	for (int32 i = 0; i < EquippedItems.Num(); ++i)
	{
		if (!EquippedItems[i].IsValid())
			return i;
	}
	return INDEX_NONE;
}

int32 UCraterInventoryManagerComponent::FindAppropriateEquipmentSlot(const UCraterInventoryFragment_Equippable* EquipFragment, int32 PreferredSlot) const
{
	if (EquipFragment && EquipFragment->RequiredEquipmentSlot >= 0)
	{
		int32 RequiredSlot = EquipFragment->RequiredEquipmentSlot;
		if (RequiredSlot < MaxEquipmentSlots)
			return RequiredSlot;

		return INDEX_NONE;
	}

	if (PreferredSlot >= 0 && PreferredSlot < EquippedItems.Num())
	{
		if (!EquippedItems[PreferredSlot].IsValid())
			return PreferredSlot;
	}

	return INDEX_NONE;
}

ULyraInventoryItemInstance* UCraterInventoryManagerComponent::GetLyraInstanceAtSlot(int32 EquipmentSlot) const
{
	if (EquipmentSlot < 0 || EquipmentSlot >= EquippedItems.Num())
		return nullptr;

	return EquippedItems[EquipmentSlot].LyraInstance;
}

ULyraInventoryItemInstance* UCraterInventoryManagerComponent::CreateLyraInstanceFromItem(const FElementusItemInfo& ItemInfo)
{
	const UElementusItemData_LyraIntegration* LyraData = GetLyraIntegrationData(ItemInfo.ItemId);
	if (!LyraData)
		return nullptr;

	TSubclassOf<ULyraInventoryItemDefinition> DefClass = LyraData->LoadLyraDefinitionSync();
	if (!DefClass)
	{
		CRATER_LOG(LogCraterInventory, Warning, TEXT("Failed to load Lyra definition for %s"), *ItemInfo.ItemId.ToString());
		return nullptr;
	}

	// Create the instance
	ULyraInventoryItemInstance* Instance = NewObject<ULyraInventoryItemInstance>(GetOwner());
	Instance->SetItemDef(DefClass);

	// Initialize fragments
	const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(DefClass);
	if (DefCDO)
	{
		for (const ULyraInventoryItemFragment* Fragment : DefCDO->Fragments)
		{
			if (Fragment)
			{
				Fragment->OnInstanceCreated(Instance);
			}
		}
	}

	CRATER_LOG(LogCraterInventory, Verbose, TEXT("Created Lyra instance for %s"), *ItemInfo.ItemId.ToString());

	return Instance;
}

TSubclassOf<ULyraInventoryItemDefinition> UCraterInventoryManagerComponent::LoadLyraDefinitionForItem(const FPrimaryElementusItemId& ItemId) const
{
	const UElementusItemData_LyraIntegration* LyraData = GetLyraIntegrationData(ItemId);
	if (!LyraData)
		return nullptr;

	return LyraData->LoadLyraDefinitionSync();
}

const UElementusItemData_LyraIntegration* UCraterInventoryManagerComponent::GetLyraIntegrationData(const FPrimaryElementusItemId& ItemId) const
{
	const UElementusItemData* ItemData = UElementusInventoryFunctions::GetSingleItemDataById(ItemId, {});
	if (!ItemData)
		return nullptr;

	return Cast<UElementusItemData_LyraIntegration>(ItemData);
}

const UCraterInventoryFragment_Equippable* UCraterInventoryManagerComponent::GetEquippableFragment(const FPrimaryElementusItemId& ItemId) const
{
	const UElementusItemData_LyraIntegration* LyraData = GetLyraIntegrationData(ItemId);
	if (!LyraData)
		return nullptr;

	return LyraData->FindLyraFragment<UCraterInventoryFragment_Equippable>();
}


UAbilitySystemComponent* UCraterInventoryManagerComponent::GetOwnerASC()
{
	if (CachedASC.IsValid())
		return CachedASC.Get();

	AActor* Owner = GetOwner();
	if (!Owner)
		return nullptr;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (ASC)
	{
		CachedASC = ASC;
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Cached ASC reference for owner %s"), *GetNameSafe(Owner));
	}

	return ASC;
}

void UCraterInventoryManagerComponent::GrantAbilitiesFromItem(int32 EquipmentSlot, const FElementusItemInfo& ItemInfo)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("No ASC found on owner - skipping ability grant"));
		return;
	}

	FCraterEquippedItemEntry& Entry = EquippedItems[EquipmentSlot];
	
	const UCraterInventoryFragment_Equippable* EquipFragment = GetEquippableFragment(ItemInfo.ItemId);
	if (!EquipFragment)
	{
		CRATER_LOG(LogCraterInventory, Verbose, 
			TEXT("Item %s has no equippable fragment"), 
			*ItemInfo.ItemId.ToString());
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : EquipFragment->AbilitiesToGrant)
	{
		if (!AbilityClass)
			continue;

		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, GetOwner());
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		Entry.GrantedAbilityHandles.Add(Handle);
		
		CRATER_LOG(LogCraterInventory, Log, TEXT("Granted ability %s from item %s"), 
			*GetNameSafe(AbilityClass), *ItemInfo.ItemId.ToString());
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EquipFragment->EffectsToApply)
	{
		if (!EffectClass)
			continue;

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(GetOwner());
		
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1, ContextHandle);
		if (!SpecHandle.IsValid())
			continue;

		FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (EffectHandle.IsValid())
		{
			Entry.AppliedEffectHandles.Add(EffectHandle);
			
			CRATER_LOG(LogCraterInventory, Log, TEXT("Applied effect %s from item %s"), 
				*GetNameSafe(EffectClass), *ItemInfo.ItemId.ToString());
		}
	}

	// Apply equipped tags via loose gameplay tags (these are additive, not GE-based)
	if (EquipFragment->EquippedTags.Num() > 0)
	{
		ASC->AddLooseGameplayTags(EquipFragment->EquippedTags);
		Entry.GrantedTags = EquipFragment->EquippedTags;
		
		CRATER_LOG(LogCraterInventory, Log, TEXT("Applied %d equipped tags from item %s"), 
			EquipFragment->EquippedTags.Num(), *ItemInfo.ItemId.ToString());
	}

	CRATER_LOG(LogCraterInventory, Display, 
		TEXT("Granted %d abilities, %d effects, %d tags from item %s"),
		EquipFragment->AbilitiesToGrant.Num(), 
		EquipFragment->EffectsToApply.Num(),
		EquipFragment->EquippedTags.Num(),
		*ItemInfo.ItemId.ToString());
}

void UCraterInventoryManagerComponent::RevokeAbilitiesFromItem(int32 EquipmentSlot)
{
	if (EquipmentSlot < 0 || EquipmentSlot >= EquippedItems.Num())
		return;

	FCraterEquippedItemEntry& Entry = EquippedItems[EquipmentSlot];
	
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		Entry.GrantedAbilityHandles.Empty();
		Entry.AppliedEffectHandles.Empty();
		Entry.GrantedTags.Reset();
		return;
	}

	if (Entry.GrantedAbilityHandles.Num() > 0)
	{
		for (const FGameplayAbilitySpecHandle& Handle : Entry.GrantedAbilityHandles)
		{
			ASC->ClearAbility(Handle);
		}
		
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Revoked %d abilities from equipment slot %d"), 
			Entry.GrantedAbilityHandles.Num(), EquipmentSlot);
		
		Entry.GrantedAbilityHandles.Empty();
	}

	if (Entry.AppliedEffectHandles.Num() > 0)
	{
		for (const FActiveGameplayEffectHandle& Handle : Entry.AppliedEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
		
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Removed %d effects from equipment slot %d"), 
			Entry.AppliedEffectHandles.Num(), EquipmentSlot);
		
		Entry.AppliedEffectHandles.Empty();
	}

	if (Entry.GrantedTags.Num() > 0)
	{
		ASC->RemoveLooseGameplayTags(Entry.GrantedTags);
		
		CRATER_LOG(LogCraterInventory, Verbose, TEXT("Removed %d tags from equipment slot %d"), 
			Entry.GrantedTags.Num(), EquipmentSlot);
		
		Entry.GrantedTags.Reset();
	}
}


void UCraterInventoryManagerComponent::HandleStorageUpdate()
{
	if (bIsProcessingUpdate)
		return;

	bIsProcessingUpdate = true;

	CRATER_FUNC_LOG(LogCraterInventory, Verbose, TEXT("Storage Updated. Synchronizing state..."));

	ValidateEquippedItems();
	SyncLyraInstances();

	OnInventoryChanged.Broadcast();

	bIsProcessingUpdate = false;
}

void UCraterInventoryManagerComponent::ValidateEquippedItems()
{
	if (!InventoryStorage)
		return;

	const TArray<FElementusItemInfo>& CurrentItems = InventoryStorage->GetItemsArray();
	
	for (int32 i = 0; i < EquippedItems.Num(); ++i)
	{
		FCraterEquippedItemEntry& Entry = EquippedItems[i];
		if (!Entry.IsValid())
		{
			continue;
		}

		// Try to find the equipped item by ID (more reliable than slot index)
		bool bFoundItem = false;
		for (int32 j = 0; j < CurrentItems.Num(); ++j)
		{
			if (CurrentItems[j].ItemId == Entry.ItemInfo.ItemId)
			{
				// Update the slot index if it changed
				if (Entry.InventorySlotIndex != j)
				{
					CRATER_LOG(LogCraterInventory, Verbose, 
						TEXT("Equipped item %s moved from slot %d to %d"), 
						*Entry.ItemInfo.ItemId.ToString(), Entry.InventorySlotIndex, j);
					Entry.InventorySlotIndex = j;
				}
				Entry.ItemInfo = CurrentItems[j];
				bFoundItem = true;
				break;
			}
		}

		if (!bFoundItem)
		{
			CRATER_LOG(LogCraterInventory, Warning, 
				TEXT("Equipped item %s no longer in inventory - auto-unequipping"), 
				*Entry.ItemInfo.ItemId.ToString());
			RevokeAbilitiesFromItem(i);
			Entry.Reset();
		}
	}
}

void UCraterInventoryManagerComponent::SyncLyraInstances()
{
	for (FCraterEquippedItemEntry& Entry : EquippedItems)
	{
		if (!Entry.IsValid() || Entry.LyraInstance)
			continue;

		Entry.LyraInstance = CreateLyraInstanceFromItem(Entry.ItemInfo);
	}
}

void UCraterInventoryManagerComponent::UpdateEquippedIndicesAfterSwap(int32 FromIndex, int32 ToIndex)
{
	for (FCraterEquippedItemEntry& Entry : EquippedItems)
	{
		if (!Entry.IsValid())
			continue;

		if (Entry.InventorySlotIndex == FromIndex)
		{
			Entry.InventorySlotIndex = ToIndex;
			CRATER_LOG(LogCraterInventory, Verbose, TEXT("Updated equipped item index from %d to %d"), FromIndex, ToIndex);
		}
		else if (Entry.InventorySlotIndex == ToIndex)
		{
			Entry.InventorySlotIndex = FromIndex;
			CRATER_LOG(LogCraterInventory, Verbose, TEXT("Updated equipped item index from %d to %d"), ToIndex, FromIndex);
		}
	}
}

void UCraterInventoryManagerComponent::OnRep_EquippedItems()
{
	CRATER_FUNC_LOG(LogCraterInventory, Verbose, TEXT("Equipped items replicated"));

	OnInventoryChanged.Broadcast();
}

// ============================================================================
// Validation Helper Methods
// ============================================================================
// Internal validation utilities that implement the guard clause pattern.
// These methods encapsulate common validation logic to reduce code duplication
// and improve maintainability.
// ============================================================================

/**
 * Validates that the storage system is ready for operations.
 * 
 * Guard clause method that checks if the InventoryStorage component is
 * properly initialized and ready to handle operations.
 * 
 * @return True if storage is ready, false if not initialized
 * 
 * @note Logs warning if storage is not ready
 */
bool UCraterInventoryManagerComponent::ValidateStorageReady() const
{
	if (!InventoryStorage)
	{
		CRATER_FUNC_LOG(LogCraterInventory, Warning, TEXT("Storage not ready"));
		return false;
	}
	return true;
}

bool UCraterInventoryManagerComponent::ValidateInventorySlot(int32 SlotIndex) const
{
	if (!InventoryStorage)
	{
		return false;
	}

	const TArray<FElementusItemInfo>& Items = InventoryStorage->GetItemsArray();
	if (!Items.IsValidIndex(SlotIndex))
	{
		CRATER_LOG(LogCraterInventory, Warning, TEXT("Invalid inventory slot index: %d (size: %d)"), SlotIndex, Items.Num());
		return false;
	}
	return true;
}

bool UCraterInventoryManagerComponent::IsValidEquipmentSlot(int32 EquipmentSlot) const
{
	return EquipmentSlot >= 0 && EquipmentSlot < EquippedItems.Num();
}
