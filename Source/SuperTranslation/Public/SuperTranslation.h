// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;
class FToolBarBuilder;
class FMenuBuilder;

class FSuperTranslationModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	

	
private:

	void RegisterMenus();

#pragma region TranslationWidget
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	void PluginTestButtonClicked();
	
	void RegisterTranslationWidget();
	TSharedRef<SDockTab> OnSpawnTranslationWidgetTab(const FSpawnTabArgs& SpawnTabArgs);
	
#pragma endregion

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
