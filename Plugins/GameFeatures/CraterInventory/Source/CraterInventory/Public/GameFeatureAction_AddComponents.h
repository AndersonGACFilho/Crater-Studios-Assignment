#pragma once

#include "CoreMinimal.h"
#include "GameFeatures/GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_AddComponents.generated.h"

struct FComponentRequestHandle;

/**
 * Configuration for a component to be added to specific actor types.
 */
USTRUCT(BlueprintType)
struct FGameFeatureComponentEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Components", meta = (AllowAbstract = "False"))
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, Category = "Components")
	TSoftClassPtr<UActorComponent> ComponentClass;

	UPROPERTY(EditAnywhere, Category = "Components")
	bool bClientComponent = true;

	UPROPERTY(EditAnywhere, Category = "Components")
	bool bServerComponent = true;
};

/**
 * GameFeatureAction that dynamically adds components to actors when the feature is active.
 * Uses GameFrameworkComponentManager for non-destructive component injection.
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Components"))
class UGameFeatureAction_AddComponents final : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Components", meta = (TitleProperty = "{ActorClass} -> {ComponentClass}"))
	TArray<FGameFeatureComponentEntry> ComponentList;

private:
	struct FPerContextData
	{
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
	};

	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;

	void Reset(FPerContextData& ActiveData);
};

