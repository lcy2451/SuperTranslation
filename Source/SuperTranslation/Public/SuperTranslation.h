// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;
class FToolBarBuilder;
class FMenuBuilder;
struct FAssetData;


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

#pragma region ContextMenu

	// 初始化关卡视图右键菜单
	void InitCBMenuExtension();
	
	TSharedRef<FExtender> CustomCBMenuExtender(const TArray<FString>& SelectedPaths);
	TSharedRef<FExtender> CustomEmptyAreaMenuExtender(const TArray<FString>& SelectedPaths);
	TSharedRef<FExtender> CustomSelectedAssetsCBMenuExtender(const TArray<FAssetData>& SelectedAssets);
	
	void AddCBMenuEntry(FMenuBuilder& MenuBuilder);
	void AddSelectedAssetsCBMenuEntry(FMenuBuilder& MenuBuilder);
	void AddAssetsCBMenuEntry(FMenuBuilder& MenuBuilder);
	
#pragma endregion
	
#pragma region TranslationWidget
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	void PluginTestButtonClicked();
	
	void RegisterTranslationWidget();
	TSharedRef<SDockTab> OnSpawnTranslationWidgetTab(const FSpawnTabArgs& SpawnTabArgs);
	
	// 生成 DeepSeek 暂存文件
	void RegisterDeepSeekJson();
	FString JsonPath;
	// TArray<FString> Alternatives;
#pragma endregion
	
#pragma region RenameWidget
	void ShowRenameWidget();
	TArray<TSharedPtr<FAssetData>> GetAllAssetDataUnderSelectedAsset();
	
#pragma endregion
private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
