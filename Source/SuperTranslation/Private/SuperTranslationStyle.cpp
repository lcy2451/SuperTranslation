// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTranslationStyle.h"
#include "SuperTranslation.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FSuperTranslationStyle::StyleInstance = nullptr;

void FSuperTranslationStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSuperTranslationStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FSuperTranslationStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SuperTranslationStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FSuperTranslationStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("SuperTranslationStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("SuperTranslation")->GetBaseDir() / TEXT("Resources"));

	Style->Set("SuperTranslation.PluginAction", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	return Style;
}

void FSuperTranslationStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FSuperTranslationStyle::Get()
{
	return *StyleInstance;
}
