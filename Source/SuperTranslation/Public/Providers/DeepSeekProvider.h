// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"

class STranslationPanel;

/**
 * 
 */
class SUPERTRANSLATION_API DeepSeekProvider
{
public:
	DeepSeekProvider(TWeakPtr<STranslationPanel> TranslationPanelWeakPtr);
	DeepSeekProvider();
	~DeepSeekProvider();
	
public:
	void Translator(
		FString Text, FString TargetLang="Simplified Chinese", FString SourceLang="English",
		const FString &Model="deepseek-v4-flash");
	void SetTranslationPanel(TWeakPtr<STranslationPanel> TranslationPanelWeakPtr);
	
private:
	void OnProcessRequestComplete(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess);
	
	TWeakPtr<STranslationPanel> TranslationPanel;
	
};
