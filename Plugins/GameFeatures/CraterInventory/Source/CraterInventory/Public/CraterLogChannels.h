// Copyright Crater Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// General logic bridge events (e.g., Inventory Manager, Component Logic)
CRATERINVENTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogCraterInventory, Log, All);

// Data Adapter events (e.g., Asset Loading, Database Transformations)
CRATERINVENTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogCraterInventory_Data, Log, All);

// User Interface events (e.g., Widget Construction, Updates)
CRATERINVENTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogCraterInventory_UI, Log, All);

namespace CraterLogUtils
{
    /**
     * @brief Retrieves a string label representing the network mode (Server/Client) of the provided World Context Object.
     * @details This function checks the network mode of the world associated with the given UObject
     * and returns a corresponding label:
     * - "[Server]" for dedicated servers
     * - "[Client]" for clients
     * - "[ListenServer]" for listen servers
     * - "[Standalone]" for standalone mode
     * - "[NoWorld]" if the world context is invalid
     * - "[Unknown]" for any other unrecognized modes
     *
     * @param WorldContextObject The UObject used to determine the world context.
     * @return A FString label indicating the network mode.
    */
    CRATERINVENTORY_API FString GetNetModeLabel(const UObject* WorldContextObject);
}

/**
 * CRATER_LOG
 * Prints a formatted log message prepended with the Network Context (Server/Client).
 * * Usage: CRATER_LOG(LogCraterInventory, Display, TEXT("Equipped Item: %s"), *ItemName);
 * Output: [Server] Equipped Item: Rifle
 *
 * @param CategoryName The log category to use.
 * @param Verbosity The verbosity level of the log message.
 * @param Format The format string for the log message.
 * @param ... Additional arguments for the format string.
 */
#define CRATER_LOG(CategoryName, Verbosity, Format, ...) \
    { \
        const FString NetLabel = CraterLogUtils::GetNetModeLabel(this); \
        UE_LOG(CategoryName, Verbosity, TEXT("%s %s"), *NetLabel, *FString::Printf(Format, ##__VA_ARGS__)); \
    }

/**
 * CRATER_FUNC_LOG
 * Prints Network Context + Class Name + Function Name + Message.
 * Useful for tracing execution flow across the bridge.
 * * Usage: CRATER_FUNC_LOG(LogCraterInventory, Display, TEXT("Processing..."));
 * Output: [Client] UCraterInventoryManager::BeginPlay: Processing...
 *
 * @param CategoryName The log category to use.
 * @param Verbosity The verbosity level of the log message.
 * @param Format The format string for the log message.
 * @param ... Additional arguments for the format string.
 */
#define CRATER_FUNC_LOG(CategoryName, Verbosity, Format, ...) \
    { \
        const FString NetLabel = CraterLogUtils::GetNetModeLabel(this); \
        UE_LOG(CategoryName, Verbosity, TEXT("%s %s: %s"), *NetLabel, *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
    }