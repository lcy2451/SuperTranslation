// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// #include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SuperTranslationSettings.generated.h"

UENUM(BlueprintType)
enum class ETranslationEngine : uint8
{
	GoogleFree UMETA(DisplayName = "谷歌翻译 | Google Translate "),
	MicrosoftFree UMETA(DisplayName = "微软Edge 不推荐用 | Microsoft Edge (Recommended)"),
	DeepSeekApi UMETA(DisplayName = "DeepSeek | DeepSeek API"),
};

/**
 * 
 */
UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class SUPERTRANSLATION_API USuperTranslationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	
	USuperTranslationSettings();
	
	// 显示在 Project Settings -> Plugins
	virtual FName GetCategoryName() const override
	{
		return FName("Plugins");
	}
	
	//设置页面名称
	virtual FText GetSectionText() const override
	{
		return FText::FromString(TEXT("SuperTranslation"));
	}
	
	// 翻译引擎
	UPROPERTY(Config,
		EditAnywhere,
		meta = (DisplayName = "翻译引擎",
			Tooltip = "翻译引擎"))
	ETranslationEngine TranslationEngine;
};
