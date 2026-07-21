// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

/**
 * 
 */

class FMenuBuilder;
class SThrobber;
class SMultiLineEditableTextBox;

class STranslationPanel: public SCompoundWidget  
{  
	SLATE_BEGIN_ARGS(STranslationPanel){}  
  
	SLATE_ARGUMENT(TArray<TSharedPtr<FAssetData>>, AssetDataToStore)
   SLATE_END_ARGS()  
public:  
	void Construct(const FArguments& InArgs);
    
	TMap<FString, FText> LanguageToLanguageMap = {
		{"auto", FText::FromString(TEXT("检测语言"))},
		{"zh", FText::FromString(TEXT("简体中文"))},
		{"en", FText::FromString(TEXT("英语"))}
	};
	
	TSharedRef<SWidget> BuildFirstToolbar();
	TSharedRef<SWidget> BuildSecondToolbar();
	
	void MakeFirstMenuEntries(FMenuBuilder& MenuBuilder);
	void MakeSecondMenuEntries(FMenuBuilder& MenuBuilder);
	
	// 存储当前的文本内容（在其他地方直接修改这个变量即可）
	static FText FirstMenuLabel;
	static FText SecondMenuLabel;
	
	// 提供给 Slate 动态获取文本的方法
	FText GetFirstMenuLabel();
	FText GetSecondMenuLabel();
	
	void OnCheckFirstLanguageClicked(const FString TargetLanguage);
	void OnCheckSecondLanguageClicked(const FString TargetLanguage);
	FReply OnSwitchLanguageButtonClicked();
	
	TSharedRef<SMultiLineEditableTextBox> ConstructFirstMultiLineEditableTextBox();
	TSharedPtr<SMultiLineEditableTextBox> ConstructedFirstMultiLineEditableTextBox;
	
	TSharedRef<SMultiLineEditableTextBox> ConstructSecondMultiLineEditableTextBox();
	TSharedPtr<SMultiLineEditableTextBox> ConstructedSecondMultiLineEditableTextBox;
	
	void OnTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnTextChanged(const FText& NewText);
	
	TSharedRef<SThrobber> ConstructSThrobber();
	TSharedPtr<SThrobber> ConstructedSThrobber;
	
	static FText FirstMenuLanguage;
	static FText SecondMenuLanguage;
	
	void GoogleTranslator(
	const FString& TextToTranslate, const FString& TargetLang, const FString& SourceLang);
	
	void BingTranslator(
	const FString& TextToTranslate, const FString& TargetLang, const FString& SourceLang);
	
	void DeepSeekTranslator(
	const FString& TextToTranslate, const FString& TargetLang, const FString& SourceLang);
	
	void OnProcessRequestComplete(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess);
	void OnProcessBingRequestComplete(
		FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnProcessBingTransRequestComplete(
	FHttpRequestPtr TransReq, FHttpResponsePtr TransResp, bool bTransSuccess);
	
	FString LastInput;
	FString LastResult;
	
	TSharedRef<SListView<TSharedPtr<FString>>> ConstructAlternativesListView();
	TSharedPtr<SListView<TSharedPtr<FString>>> ConstructedAlternativesListView;
	TArray<TSharedPtr<FString>> DisplayedAlternatives;
	
	TSharedRef<ITableRow> OnGenerateAlternativesRowForList(TSharedPtr<FString> AlternativesToDisplay,
														const TSharedRef<STableViewBase>& OwnerTable);
	
	void SaveDeepSeekApiToFile();
	
	// 生成 DeepSeek 暂存文件
	void RegisterDeepSeekJson();
	FString JsonPath;
	TArray<FString> Alternatives;
};
