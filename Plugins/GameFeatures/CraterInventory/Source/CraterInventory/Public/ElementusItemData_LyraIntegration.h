#pragma once

#include "CoreMinimal.h"
#include "Management/ElementusInventoryData.h"
#include "ElementusItemData_LyraIntegration.generated.h"
/**
 * @brief  Elementus Item Data class with Lyra Inventory integration.
 *
 * @description This class extends UElementusItemData to include a
 * soft reference to a Lyra Inventory Item Definition.
 * @see UElementusItemData
 */
UCLASS(BlueprintType, Category = "Crater Inventory | Data")
class CRATERINVENTORY_API UElementusItemData_LyraIntegration : public UElementusItemData
{
	GENERATED_BODY()

public:
	/**
	 * @brief Constructor
	 * @param ObjectInitializer The object initializer
	 */
	UElementusItemData_LyraIntegration(const FObjectInitializer& ObjectInitializer);


	// Soft reference to a Lyra Inventory Item Definition
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Crater Inventory | Lyra Integration"
	)
	TSoftClassPtr<class ULyraInventoryItemDefinition> LyraItemDefinition;
};
