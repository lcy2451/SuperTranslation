// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTranslation.h"
#include "SuperTranslationStyle.h"
#include "SuperTranslationCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/STranslationPanel.h"
#include "Widgets/Docking/SDockTab.h"

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
	UE_LOG(LogTemp, Warning, TEXT("PluginTestButtonClicked"));
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


#pragma endregion

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperTranslationModule, SuperTranslation)