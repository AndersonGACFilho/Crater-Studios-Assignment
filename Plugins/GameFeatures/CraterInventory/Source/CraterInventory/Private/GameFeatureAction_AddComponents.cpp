#include "GameFeatureAction_AddComponents.h"
#include "CraterLogChannels.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddComponents)

#define LOCTEXT_NAMESPACE "CraterGameFeatures"

void UGameFeatureAction_AddComponents::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(FGameFeatureStateChangeContext(Context));

	if (!ensureAlways(ActiveData.ComponentRequests.IsEmpty()))
	{
		Reset(ActiveData);
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddComponents::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	
	FPerContextData* ActiveData = ContextData.Find(FGameFeatureStateChangeContext(Context));
	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameFeatureAction_AddComponents::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const FGameFeatureComponentEntry& Entry : ComponentList)
	{
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(
				LOCTEXT("ComponentEntryHasNullActor", "Null ActorClass at index {0} in ComponentList"),
				FText::AsNumber(EntryIndex)));
		}

		if (Entry.ComponentClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(
				LOCTEXT("ComponentEntryHasNullComponent", "Null ComponentClass at index {0} in ComponentList"),
				FText::AsNumber(EntryIndex)));
		}

		if (!Entry.bClientComponent && !Entry.bServerComponent)
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(
				LOCTEXT("ComponentEntryHasNoTarget", "Entry at index {0} has both bClientComponent and bServerComponent set to false"),
				FText::AsNumber(EntryIndex)));
		}

		++EntryIndex;
	}

	return Result;
}
#endif

void UGameFeatureAction_AddComponents::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{
		if (UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			const ENetMode NetMode = World->GetNetMode();
			const bool bIsServer = (NetMode != NM_Client);
			const bool bIsClient = (NetMode != NM_DedicatedServer);

			for (const FGameFeatureComponentEntry& Entry : ComponentList)
			{
				if (Entry.ActorClass.IsNull() || Entry.ComponentClass.IsNull())
				{
					continue;
				}

				const bool bShouldAddRequest = (bIsServer && Entry.bServerComponent) || (bIsClient && Entry.bClientComponent);
				if (!bShouldAddRequest)
				{
					continue;
				}

				TSharedPtr<FComponentRequestHandle> RequestHandle = ComponentManager->AddComponentRequest(Entry.ActorClass, Entry.ComponentClass);
				ActiveData.ComponentRequests.Add(RequestHandle);

				UE_LOG(LogCraterInventory, Display, 
					TEXT("GameFeature registered component request: %s -> %s"),
					*Entry.ActorClass.ToString(), *Entry.ComponentClass.ToString());
			}
		}
		else
		{
			UE_LOG(LogCraterInventory, Error,
				TEXT("GameFrameworkComponentManager not available for GameFeature component requests"));
		}
	}
}

void UGameFeatureAction_AddComponents::Reset(FPerContextData& ActiveData)
{
	ActiveData.ComponentRequests.Empty();
}

#undef LOCTEXT_NAMESPACE

