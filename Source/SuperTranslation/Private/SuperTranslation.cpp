// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTranslation.h"
#include "SuperTranslationStyle.h"
#include "SuperTranslationCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/STranslationPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "IPythonScriptPlugin.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"


static const FName SuperTranslationTabName("SuperTranslation");

#define LOCTEXT_NAMESPACE "FSuperTranslationModule"

void FSuperTranslationModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FSuperTranslationStyle::Initialize();
	FSuperTranslationStyle::ReloadTextures();

	FSuperTranslationCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSuperTranslationCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FSuperTranslationModule::PluginButtonClicked),
		FCanExecuteAction());
	
	PluginCommands->MapAction(
	FSuperTranslationCommands::Get().TestAction,
	FExecuteAction::CreateRaw(this, &FSuperTranslationModule::PluginTestButtonClicked),
	FCanExecuteAction());

	RegisterTranslationWidget();
	
	// 在Saved生成插件用的文件夹， 一个实例用一个
	RegisterSavedFiles();
	
	RegisterDeepSeekJson();
	
	// RegistPythonScripts();
	if (IPythonScriptPlugin::Get())
	{
		IPythonScriptPlugin::Get()
		->OnPythonInitialized()
		.AddRaw(this, &FSuperTranslationModule::RegisterPythonScripts);
	}
	
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSuperTranslationModule::RegisterMenus));
}

void FSuperTranslationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSuperTranslationStyle::Shutdown();

	FSuperTranslationCommands::Unregister();
	
	UnregisterSavedFiles();
}

void FSuperTranslationModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FSuperTranslationCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(FSuperTranslationCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
				
				FToolMenuEntry& EntryTest = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(FSuperTranslationCommands::Get().TestAction));
				EntryTest.SetCommandList(PluginCommands);
			}
		}
	}
}

void FSuperTranslationModule::RegisterPythonScripts()
{
	const FString 
	PythonLib = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(
		TEXT("SuperTranslation"))->GetBaseDir() / "Resources" / "DefaultPythonSource" / "Lib");
	const FString PythonPath = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(
	TEXT("SuperTranslation"))->GetBaseDir() / "Resources" / "DefaultPythonSource" / "Python");
	
	const FString PythonCommand = FString::Printf(
	TEXT("import sys; sys.path.insert(0, r'%s');sys.path.insert(0, r'%s')"),
	*PythonLib,
	*PythonPath
	);
	
	FString Cmd = FString::Printf(
	TEXT("import os; os.environ['SUPER_TRANSLATION_TEMP']=r'%s'"),
	*TempDir
	);

	IPythonScriptPlugin::Get()->ExecPythonCommand(*Cmd);
	
	if (!IPythonScriptPlugin::Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("Python No NO NO NO"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Python Yes Yes Yes"));
	}
	IPythonScriptPlugin::Get()->ExecPythonCommand(ToCStr(PythonCommand));
}


void FSuperTranslationModule::RegisterSavedFiles()
{
	TempDir = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectSavedDir() / TEXT("SuperTranslation") / FGuid::NewGuid().ToString());
	
	IFileManager::Get().MakeDirectory(*TempDir, true);
}

void FSuperTranslationModule::UnregisterSavedFiles()
{
	if (IFileManager::Get().DirectoryExists(*TempDir))
	{
		IFileManager::Get().DeleteDirectory(*TempDir);
	}
}

#pragma region TranslationWidget

void FSuperTranslationModule::PluginButtonClicked()
{
	// Put your "OnButtonClicked" stuff here
	// FText DialogText = FText::Format(
	// 						LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
	// 						FText::FromString(TEXT("FSuperTranslationModule::PluginButtonClicked()")),
	// 						FText::FromString(TEXT("SuperTranslation.cpp"))
	// );
	// FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	
	FGlobalTabmanager::Get()->TryInvokeTab(FName("SuperTranslationWidget"));
}

void FSuperTranslationModule::PluginTestButtonClicked()
{
	Alternatives.Empty();
	
	// UE_LOG(LogTemp, Warning, TEXT("PluginTestButtonClicked"));
	IPythonScriptPlugin::Get()->ExecPythonCommand(
	TEXT("import deepseek_provider;deepseek_provider.DeepSeekProvider.test()"));
	
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Json No NO NO NO %s"), *JsonPath);
		return;
	}
	
	TSharedPtr<FJsonObject> JsonObject;
	
	TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(JsonString);	
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Json Parse Failed Failed Failed Failed"));
		return;
	}
	
	TMap<FString, FString> JsonMap;
	FString DeepSeekTranslations;
	for (const auto& Pair : JsonObject->Values)
	{
		FString KeyStr = FString(Pair.Key);
		
		if (Pair.Value.IsValid() && FString(Pair.Key) == "alternatives" && Pair.Value->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& ArrayVal = Pair.Value->AsArray();
			for (const auto &Elem: ArrayVal)
			{
				UE_LOG(LogTemp, Warning, TEXT("咕咕嘎嘎 饿啊 : %s "), *Elem.Get()->AsString());
				Alternatives.Add(Elem.Get()->AsString());
			}
			
		}
		
		if (Pair.Value.IsValid() && FString(Pair.Key) == "translation")
		{
			DeepSeekTranslations = Pair.Value->AsString();
			UE_LOG(LogTemp, Warning, TEXT("咕咕嘎嘎 饱了 : %s "), *DeepSeekTranslations);
		}
	}
	
}


void FSuperTranslationModule::RegisterTranslationWidget()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
FName("SuperTranslationWidget"),
	FOnSpawnTab::CreateRaw(this, &FSuperTranslationModule::OnSpawnTranslationWidgetTab))
	.SetDisplayName(FText::FromString(TEXT("Query & Translate")));
}

TSharedRef<SDockTab> FSuperTranslationModule::OnSpawnTranslationWidgetTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
	[
		SNew(STranslationPanel)
	];
}

void FSuperTranslationModule::RegisterDeepSeekJson()
{
	JsonPath = TempDir / "DeepSeek.json";
	FFileHelper::SaveStringToFile(
	TEXT("{}"),
	*JsonPath
	);
}

#pragma endregion

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperTranslationModule, SuperTranslation)