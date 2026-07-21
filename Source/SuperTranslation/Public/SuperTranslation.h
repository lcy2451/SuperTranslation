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
	
	FString TempDir;
	
private:

	void RegisterMenus();
	void RegisterPythonScripts();
	
	//在Saved生成插件用的文件夹， 一个实例用一个
	void RegisterSavedFiles();
	
	//清理Saved里插件用的文件夹
	void UnregisterSavedFiles();

#pragma region TranslationWidget
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	void PluginTestButtonClicked();
	
	void RegisterTranslationWidget();
	TSharedRef<SDockTab> OnSpawnTranslationWidgetTab(const FSpawnTabArgs& SpawnTabArgs);
	
	// 生成 DeepSeek 暂存文件
	void RegisterDeepSeekJson();
	FString JsonPath;
	TArray<FString> Alternatives;
	
#pragma endregion

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
