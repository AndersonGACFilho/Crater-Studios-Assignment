// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEElementusInventory

#pragma once

#include <CoreMinimal.h>
#include "PropertyEditorModule.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleInterface.h"

class FElementusInventoryEditorModule : public IModuleInterface 
{
protected:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs, FName TabId) const;

private:
	void RegisterMenus();
	FPropertyEditorModule* PropertyEditorModule = nullptr;
};
