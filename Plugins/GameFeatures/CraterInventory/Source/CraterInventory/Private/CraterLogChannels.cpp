#include "CraterLogChannels.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// Define Categories
DEFINE_LOG_CATEGORY(LogCraterInventory);
DEFINE_LOG_CATEGORY(LogCraterInventory_Data);
DEFINE_LOG_CATEGORY(LogCraterInventory_UI);

FString CraterLogUtils::GetNetModeLabel(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;

	if (!World)
		return TEXT("[NoWorld]");

	if (World->IsNetMode(NM_Client))
		return TEXT("[Client]");

	if (World->IsNetMode(NM_DedicatedServer))
		return TEXT("[Server]");

	if (World->IsNetMode(NM_ListenServer))
		return TEXT("[ListenServer]");

	if (World->IsNetMode(NM_Standalone))
		return TEXT("[Standalone]");

	return TEXT("[Unknown]");
}