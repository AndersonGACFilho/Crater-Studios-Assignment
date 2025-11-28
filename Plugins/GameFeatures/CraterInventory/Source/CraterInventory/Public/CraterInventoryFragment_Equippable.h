#pragma once

#include "CoreMinimal.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "GameplayAbilitySpec.h"
#include "CraterInventoryFragment_Equippable.generated.h"

class UGameplayAbility;
class UGameplayEffect;
struct FGameplayTag;

/**
 * @brief Fragment that defines equippable item behavior for Crater Inventory.
 * @details Attach this fragment to a ULyraInventoryItemDefinition to define:
 * - Abilities granted when the item is equipped
 * - Gameplay Effects applied when equipped
 * - Equipment slot restrictions
 * 
 * This fragment is read by UCraterInventoryManagerComponent when an item is equipped
 * to grant the appropriate abilities and effects to the owner's AbilitySystemComponent.
 */
UCLASS(DisplayName = "Crater Equippable")
class CRATERINVENTORY_API UCraterInventoryFragment_Equippable : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// ~Begin ULyraInventoryItemFragment Interface
	virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const override;
	// ~End ULyraInventoryItemFragment Interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "-1"))
	int32 RequiredEquipmentSlot = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FGameplayTagContainer EquippedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FGameplayTagContainer RequiredTagsToEquip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FGameplayTagContainer BlockingTagsToEquip;

	UFUNCTION(BlueprintPure, Category = "Crater Inventory | Equipment")
	bool CanBeEquippedBy(const FGameplayTagContainer& OwnerTags) const;

	UFUNCTION(BlueprintPure, Category = "Crater Inventory | Equipment")
	int32 GetNumAbilities() const { return AbilitiesToGrant.Num(); }

	UFUNCTION(BlueprintPure, Category = "Crater Inventory | Equipment")
	int32 GetNumEffects() const { return EffectsToApply.Num(); }
};
