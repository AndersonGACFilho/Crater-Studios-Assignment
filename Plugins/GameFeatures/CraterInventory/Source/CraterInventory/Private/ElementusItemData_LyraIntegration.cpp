#include "ElementusItemData_LyraIntegration.h"

UElementusItemData_LyraIntegration::UElementusItemData_LyraIntegration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to 'Other' or 'Special' to indicate this relies on external Lyra Definitions 
	// for its primary gameplay tags/behavior.
	ItemType = EElementusItemType::Other;
}