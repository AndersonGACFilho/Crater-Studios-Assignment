// Copyright Crater Studios. All Rights Reserved.

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

	// Safety check
	if (!World)
	{
		return TEXT("[NoWorld]");
	}

	// Determine Net Mode
	if (World->IsNetMode(NM_Client))
	{
		return TEXT("[Client]");
	}

	// Server modes
	if (World->IsNetMode(NM_DedicatedServer))
	{
		return TEXT("[Server]");
	}

	// Listen Server mode
	if (World->IsNetMode(NM_ListenServer))
	{
		return TEXT("[ListenServer]");
	}

	// Standalone mode
	if (World->IsNetMode(NM_Standalone))
	{
		return TEXT("[Standalone]");
	}

	// Unknown mode
	return TEXT("[Unknown]");
}