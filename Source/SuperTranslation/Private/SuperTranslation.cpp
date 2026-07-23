// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTranslation.h"

#include "ContentBrowserModule.h"
#include "EditorUtilityLibrary.h"
#include "HttpModule.h"
#include "SuperTranslationStyle.h"
#include "SuperTranslationCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/STranslationPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "IPythonScriptPlugin.h"
#include "HAL/FileManager.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/SuperTranslationSettings.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/SAssetRenamePanel.h"


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
	
	// 右键菜单的三次注册
	InitCBMenuExtension();
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

#pragma region ContextMenu

void FSuperTranslationModule::InitCBMenuExtension()
{
	// 加载 ContentBrowser 模块，并获取该模块的引用
	FContentBrowserModule& ContentBrowserModule = 
	FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	
	// 获取“内容浏览器路径区域右键菜单”的所有扩展委托
	TArray<FContentBrowserMenuExtender_SelectedPaths>& ContextBrowserModuleMenuExtenders = 
	ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	
	// CreateRaw可以同时完成创建和绑定， 第一次绑定
	ContextBrowserModuleMenuExtenders.Add(
		FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
			this, &FSuperTranslationModule::CustomCBMenuExtender));
	
	// 获取 Content Browser 中“选中资源后的右键菜单扩展器列表”
	TArray<FContentBrowserMenuExtender_SelectedAssets>& AssetMenuExtenders = 
		ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	AssetMenuExtenders.Add(
		FContentBrowserMenuExtender_SelectedAssets::CreateRaw(
	this, &FSuperTranslationModule::CustomSelectedAssetsCBMenuExtender));
	
	// 获取并注册 Content Browser 空白区域（当前路径）的右键菜单扩展。
	TArray<FContentBrowserMenuExtender_SelectedPaths>& MenuExtenders =
	ContentBrowserModule.GetAllAssetContextMenuExtenders();
	MenuExtenders.Add(
	FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
		this,
		&FSuperTranslationModule::CustomEmptyAreaMenuExtender
	));
}

TSharedRef<FExtender> FSuperTranslationModule::CustomCBMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());
	
	if (SelectedPaths.Num()>0)
	{
		MenuExtender->AddMenuExtension(FName("Delete"), 
			// EExtensionHook::After表示在Delete按钮之后添加
			EExtensionHook::After,
			//TSharedPtr<FUICommandList>()是一个空的， 这里可以写菜单的热键， 如果不想写， 可以留空
			TSharedPtr<FUICommandList>(),
			// 二次绑定
			FMenuExtensionDelegate::CreateRaw(this, &FSuperTranslationModule::AddCBMenuEntry));
	}
	return MenuExtender;
}

TSharedRef<FExtender> FSuperTranslationModule::CustomEmptyAreaMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());
	
	MenuExtender->AddMenuExtension(FName("Rename"), 
		// EExtensionHook::After表示在按钮之后添加
		EExtensionHook::After,
		//TSharedPtr<FUICommandList>()是一个空的， 这里可以写菜单的热键， 如果不想写， 可以留空
		TSharedPtr<FUICommandList>(),
		// 二次绑定
		FMenuExtensionDelegate::CreateRaw(this, &FSuperTranslationModule::AddAssetsCBMenuEntry));
	
	return MenuExtender;
}

TSharedRef<FExtender> FSuperTranslationModule::CustomSelectedAssetsCBMenuExtender(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());
	
	if (SelectedAssets.Num()>0)
	{
		MenuExtender->AddMenuExtension(FName("Rename"), 
			// EExtensionHook::After表示在按钮之后添加
			EExtensionHook::Before,
			//TSharedPtr<FUICommandList>()是一个空的， 这里可以写菜单的热键， 如果不想写， 可以留空
			TSharedPtr<FUICommandList>(),
			// 二次绑定
			FMenuExtensionDelegate::CreateRaw(this, &FSuperTranslationModule::AddSelectedAssetsCBMenuEntry));
	}

	return MenuExtender;
}

void FSuperTranslationModule::AddCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
	FText::FromString(TEXT("Delete Unused Asset 显示的标题文本")),
	FText::FromString(TEXT("Safely Delete all unused assets under folder 鼠标悬停时的提示")),
	// 这里可以设置Icon
	FSlateIcon(),
	FExecuteAction::CreateLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("点击了 Delete Unused Asset"));
		
		// 在这里写点击菜单后要执行的代码
	}));
}

void FSuperTranslationModule::AddSelectedAssetsCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
	FText::FromString(TEXT("Rename Asset")),
	FText::FromString(TEXT("Rename Asset")),
	// 这里可以设置Icon
	FSlateIcon(),
	FExecuteAction::CreateRaw(this, &FSuperTranslationModule::ShowRenameWidget));
}

void FSuperTranslationModule::AddAssetsCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
	FText::FromString(TEXT("Rename Asset")),
	FText::FromString(TEXT("Rename Asset")),
	// 这里可以设置Icon
	FSlateIcon(),
	FExecuteAction::CreateLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("点击了 测试菜单"));
				
		// 在这里写点击菜单后要执行的代码
	}));
}

#pragma endregion

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
	// Alternatives.Empty();
	//
	// // UE_LOG(LogTemp, Warning, TEXT("PluginTestButtonClicked"));
	// IPythonScriptPlugin::Get()->ExecPythonCommand(
	// TEXT("import deepseek_provider;deepseek_provider.DeepSeekProvider.test()"));
	//
	// FString JsonString;
	// if (!FFileHelper::LoadFileToString(JsonString, *JsonPath))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Json No NO NO NO %s"), *JsonPath);
	// 	return;
	// }
	//
	// TSharedPtr<FJsonObject> JsonObject;
	//
	// TSharedRef<TJsonReader<>> Reader =
	// 	TJsonReaderFactory<>::Create(JsonString);	
	//
	// if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Json Parse Failed Failed Failed Failed"));
	// 	return;
	// }
	//
	// TMap<FString, FString> JsonMap;
	// FString DeepSeekTranslations;
	// for (const auto& Pair : JsonObject->Values)
	// {
	// 	FString KeyStr = FString(Pair.Key);
	// 	
	// 	if (Pair.Value.IsValid() && FString(Pair.Key) == "alternatives" && Pair.Value->Type == EJson::Array)
	// 	{
	// 		const TArray<TSharedPtr<FJsonValue>>& ArrayVal = Pair.Value->AsArray();
	// 		for (const auto &Elem: ArrayVal)
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("咕咕嘎嘎 饿啊 : %s "), *Elem.Get()->AsString());
	// 			Alternatives.Add(Elem.Get()->AsString());
	// 		}
	// 		
	// 	}
	// 	
	// 	if (Pair.Value.IsValid() && FString(Pair.Key) == "translation")
	// 	{
	// 		DeepSeekTranslations = Pair.Value->AsString();
	// 		UE_LOG(LogTemp, Warning, TEXT("咕咕嘎嘎 饱了 : %s "), *DeepSeekTranslations);
	// 	}
	// }
	
	// const USuperTranslationSettings* Settings = GetDefault<USuperTranslationSettings>();
	// FSuperTranslationModule& SuperManagerModule = 
	// FModuleManager::LoadModuleChecked<FSuperTranslationModule>(TEXT("SuperTranslation"));
	
	// if (Settings->DeepSeekApiKey.IsEmpty()) return;
	//
	// FString AuthUrl = TEXT("https://api.deepseek.com/user/balance");
	//
	// TSharedRef<IHttpRequest, ESPMode::ThreadSafe> AuthRequest = FHttpModule::Get().CreateRequest();
	// AuthRequest->SetURL(AuthUrl);
	// AuthRequest->SetVerb(TEXT("GET"));
	// AuthRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
	// AuthRequest->SetHeader(
	// 	TEXT("Authorization"),
	// 	FString::Printf(TEXT("Bearer %s"), *Settings->DeepSeekApiKey));
	//
	// AuthRequest->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	// 	if (bWasSuccessful && Response.IsValid())
	// 	{
	// 		FString ResponseString = Response->GetContentAsString();
	// 		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	//         
	// 		TSharedPtr<FJsonObject> JsonObject;
	// 		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	// 		{
	// 			bool bAvailable = JsonObject->GetBoolField(TEXT("is_available"));
	//
	// 			const TArray<TSharedPtr<FJsonValue>>* BalanceInfos;
	// 			if (JsonObject->TryGetArrayField(TEXT("balance_infos"), BalanceInfos))
	// 			{
	// 				for (const TSharedPtr<FJsonValue>& Value : *BalanceInfos)
	// 				{
	// 					TSharedPtr<FJsonObject> Balance = Value->AsObject();
	//
	// 					FString Currency = Balance->GetStringField(TEXT("currency"));
	// 					FString Total = Balance->GetStringField(TEXT("total_balance"));
	//
	// 					UE_LOG(LogTemp, Warning, TEXT("%s : %s"), *Currency, *Total);
	// 				}
	// 			}
	// 		}
	// 	}
	// });
	//
	// AuthRequest->ProcessRequest();
	
	
	// FString AuthUrl = TEXT("https://api.deepseek.com/chat/completions");
	// TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	// Request->SetURL(AuthUrl);
	// // 这里必须是 POST
	// Request->SetVerb(TEXT("POST"));
	//
	// Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	// Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	// Request->SetHeader(
	// 	TEXT("Authorization"),
	// 	FString::Printf(TEXT("Bearer %s"), *Settings->DeepSeekApiKey)
	// );
	//
	// FString JsonBody = TEXT(R"(
	// 	{
	// 	  "messages": [
	// 	    {
	// 	      "content": "You are a professional Unreal Engine technical translator.Translate Unreal Engine related English text into Simplified Chinese.Rules:1. Use official Unreal Engine Simplified Chinese terminology when available.2. Keep Unreal class names and API names unchanged when appropriate.3. Provide the best translation.4. Provide alternative translations.5. Return only valid JSON.",
	// 	      "role": "system"
	// 	    },
	// 	    {
	// 	      "content": "The selected Static Mesh Component does not contain any valid collision data.",
	// 	      "role": "user"
	// 	    }
	// 	  ],
	// 	  "model": "deepseek-v4-pro",
	// 	  "thinking": {
	// 	    "type": "enabled"
	// 	  },
	// 	  "reasoning_effort": "high",
	// 	  "max_tokens": 4096,
	// 	  "response_format": {
	// 	    "type": "text"
	// 	  },
	// 	  "stop": null,
	// 	  "stream": false,
	// 	  "stream_options": null,
	// 	  "temperature": 1,
	// 	  "top_p": 1,
	// 	  "tools": null,
	// 	  "tool_choice": "none",
	// 	  "logprobs": false,
	// 	  "top_logprobs": null
	// 	}
	// 	)");
	//
	//
	// Request->SetContentAsString(JsonBody);
	//
	// // 绑定返回回调
	// Request->OnProcessRequestComplete().BindLambda(
	// 	[](FHttpRequestPtr HttpRequest,
	// 	   FHttpResponsePtr HttpResponse,
	// 	   bool bSucceeded)
	// 	{
	// 		if (!bSucceeded)
	// 		{
	// 			UE_LOG(LogTemp, Error, TEXT("DeepSeek request failed"));
	// 			return;
	// 		}
	//
	// 		if (!HttpResponse.IsValid())
	// 		{
	// 			UE_LOG(LogTemp, Error, TEXT("DeepSeek response is invalid"));
	// 			return;
	// 		}
	//
	// 		const int32 StatusCode = HttpResponse->GetResponseCode();
	// 		const FString ResponseContent =
	// 			HttpResponse->GetContentAsString();
	//
	// 		UE_LOG(
	// 			LogTemp,
	// 			Warning,
	// 			TEXT("Status Code: %d"),
	// 			StatusCode
	// 		);
	//
	// 		UE_LOG(
	// 			LogTemp,
	// 			Warning,
	// 			TEXT("Response: %s"),
	// 			*ResponseContent
	// 		);
	// 	}
	// );
	//
	// // 真正发送请求
	// const bool bStarted = Request->ProcessRequest();
	//
	// UE_LOG(
	// 	LogTemp,
	// 	Warning,
	// 	TEXT("Request started: %s"),
	// 	bStarted ? TEXT("true") : TEXT("false")
	// );
}


void FSuperTranslationModule::RegisterTranslationWidget()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
FName("SuperTranslationWidget"),
	FOnSpawnTab::CreateRaw(this, &FSuperTranslationModule::OnSpawnTranslationWidgetTab))
	.SetDisplayName(FText::FromString(TEXT("Query & Translate")))
	.SetIcon(
	FSlateIcon(
			FSuperTranslationStyle::GetStyleSetName(),
			TEXT("SuperTranslation.PluginAction")
		)
	);
}

TSharedRef<SDockTab> FSuperTranslationModule::OnSpawnTranslationWidgetTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
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

#pragma region RenameWidget

void FSuperTranslationModule::ShowRenameWidget()
{
	TSharedRef<SWindow> RenameWindow =
	SNew(SWindow)
	.Title(FText::FromString(TEXT("AI Asset Renamer")))
	.ClientSize(FVector2D(700.0f, 500.0f))
	[
		SNew(SAssetRenamePanel).AssetDataToStore(GetAllAssetDataUnderSelectedAsset())
	];
	FSlateApplication::Get().AddModalWindow(
	RenameWindow,
	nullptr,
	false
	);
	
	
	// FSlateApplication::Get().AddWindow(RenameWindow);
	
// 	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
// FName("SuperTranslationWidget"),
// 	FOnSpawnTab::CreateRaw(this, &FSuperTranslationModule::OnSpawnTranslationWidgetTab))
// 	.SetDisplayName(FText::FromString(TEXT("Query & Translate")));
}

TArray<TSharedPtr<FAssetData>> FSuperTranslationModule::GetAllAssetDataUnderSelectedAsset()
{
	TArray<TSharedPtr<FAssetData>> AvailableAssetData;
	TArray<FAssetData>  SelectedAssetData = UEditorUtilityLibrary::GetSelectedAssetData();
	for (const FAssetData& AssetData:SelectedAssetData)
	{
		UE_LOG(LogTemp, Warning, TEXT("sss  %s"), *AssetData.AssetName.ToString());
		AvailableAssetData.Add(MakeShared<FAssetData>(AssetData));
	}
	
	return AvailableAssetData;
}

#pragma endregion

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperTranslationModule, SuperTranslation)