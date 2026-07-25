// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SAssetRenamePanel.h"

#include "EditorUtilityLibrary.h"
#include "HttpModule.h"
#include "AssetRegistry/AssetData.h"
#include "Components/VerticalBox.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/MessageDialog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Utils/SuperTranslationAssetUtils.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Views/SListView.h"

void SAssetRenamePanel::Construct(const FArguments& InArgs)
{
	
	for (const TSharedPtr<FAssetData>& AssetData: InArgs._AssetDataToStore)
	{
		FAssetRenameItem Item;
		Item.AssetData = AssetData;
		Item.NewName = "";
		
		AssetDataAsStruct.Add(MakeShared<FAssetRenameItem>(Item));
		
	}
	
	
	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		[
			ConstructAssetListView()
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Rename")))
				.OnClicked(this, &SAssetRenamePanel::OnRenameButtonClicked)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Apply")))
				.OnClicked(this, &SAssetRenamePanel::OnApplyRenameButtonClicked)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				ConstructIsTranslate()
				// .Text(FText::FromString(TEXT("Apply")))
				// .OnClicked(this, &SAssetRenamePanel::OnApplyRenameButtonClicked)
			]
		]
	];
}

TSharedRef<ITableRow> SAssetRenamePanel::OnGenerateRowForList(
	TSharedPtr<FAssetRenameItem> AssetDataToDisplay,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const TSharedPtr<FAssetData> AssetData = AssetDataToDisplay.Get()->AssetData;
	const FString DisplayAssetClassName = AssetData.Get()->AssetName.ToString();
	// AssetDataAsMap[DisplayAssetClassName] = "";
	
	TSharedRef<STableRow<TSharedPtr<FAssetData>>> ListViewRowWidget = 
	SNew(STableRow < TSharedPtr <FAssetData> >, OwnerTable)
		
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
				.Text(FText::FromString(DisplayAssetClassName))
		]

		+SHorizontalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([AssetDataToDisplay]()
			{
				return AssetDataToDisplay.IsValid() &&
					AssetDataToDisplay->IsConflicting
						? FLinearColor(0.5f, 0.05f, 0.05f, 0.35f)
						: FLinearColor::Transparent;
			})
			[
				SNew(SInlineEditableTextBlock)
				// .Text(FText::FromString( AssetDataToDisplay.Get()->NewName ))
				.Text_Lambda([AssetDataToDisplay]()
				{
					return FText::FromString(AssetDataToDisplay->NewName);
				})
				.OnTextCommitted(
					FOnTextCommitted::CreateSP(
						this,
						&SAssetRenamePanel::OnAssetNameCommitted,
						AssetDataToDisplay
					)
				)
			]
		]
		
	];
	
	return ListViewRowWidget;
}

TSharedRef<SListView<TSharedPtr<FAssetRenameItem>>> SAssetRenamePanel::ConstructAssetListView()
{
	ConstructedAssetListView = SNew(SListView<TSharedPtr<FAssetRenameItem>>)
		.ListItemsSource(&AssetDataAsStruct)
		.OnGenerateRow(this, &SAssetRenamePanel::OnGenerateRowForList)
		.HeaderRow
		(
			SNew(SHeaderRow)

			+ SHeaderRow::Column(TEXT("OldName"))
			.DefaultLabel(FText::FromString(TEXT("原名称")))
			.FillWidth(0.5f)

			+ SHeaderRow::Column(TEXT("NewName"))
			.DefaultLabel(FText::FromString(TEXT("新名称")))
			.FillWidth(0.5f)
		);
	
	return ConstructedAssetListView.ToSharedRef();
}

FReply SAssetRenamePanel::OnRenameButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("RenameButtonClicked"));
	
	ConflictingAssets.Empty();
	
	for (const TSharedPtr<FAssetRenameItem>& AssetRenameItem: AssetDataAsStruct)
	{
		UpdateRenameItem(AssetRenameItem, "", ConstructedIsTranslate->IsChecked());
	}
	
	ConstructedAssetListView->RebuildList();
	
	if (!ConflictingAssets.IsEmpty())
	{
		FMessageDialog::Open(  
		EAppMsgType::Ok,  
		FText::FromString(
			TEXT("Some calculated asset names already exist.\n")
			TEXT("部分计算出的资产名称已经存在，请修改后重试。")
		),
		FText::FromString(TEXT("Asset Name Conflict")));
	}
	
	return FReply::Handled();
}

FReply SAssetRenamePanel::OnApplyRenameButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnApplyRenameButtonClicked"));
	
	if (!ConflictingAssets.IsEmpty())
	{
		FMessageDialog::Open(  
		EAppMsgType::Ok,  
		FText::FromString(
			TEXT("Some calculated asset names already exist.\n")
			TEXT("部分计算出的资产名称已经存在，请修改后重试。")
		),
		FText::FromString(TEXT("Asset Name Conflict")));
		
		return FReply::Handled();
	}
	
	SuperTranslationAssetUtils::FixUpRedirectors();
	
	for (const TSharedPtr<FAssetRenameItem>& AssetRenameItem: AssetDataAsStruct)
	{
		const TSharedPtr<FAssetData> AssetData = AssetRenameItem.Get()->AssetData;
		FString NewName = AssetRenameItem.Get()->NewName;
		
		UE_LOG(LogTemp, Warning, TEXT("old Name %s  New Name %s"), *AssetRenameItem->AssetData->AssetName.ToString(), *NewName)
		if (NewName == AssetRenameItem->AssetData->AssetName.ToString())
		{
			UE_LOG(LogTemp, Warning, TEXT("跳过了 %s . 重命名"), *NewName)
			AssetRenameItem.Get()->NewName = "";
			continue;
		}
		UEditorUtilityLibrary::RenameAsset(AssetData.Get()->GetAsset(), NewName);
		
		AssetRenameItem.Get()->NewName = "";
	}
	
	// 关闭窗口
	const TSharedPtr<SWindow> ParentWindow =
		FSlateApplication::Get().FindWidgetWindow(AsShared());

	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}
	
	ConstructedAssetListView->RebuildList();
	
	return FReply::Handled();
}

void SAssetRenamePanel::RefreshAssetListViewState()
{
	ConflictingAssets.Empty();
	
	for (const TSharedPtr<FAssetRenameItem>& AssetRenameItem: AssetDataAsStruct)
	{
		const TSharedPtr<FAssetData> AssetData = AssetRenameItem.Get()->AssetData;
		
		FString OutSelectedPackagePath = AssetData->PackagePath.ToString();
		
		if (SuperTranslationAssetUtils::CheckIsNameUsed(OutSelectedPackagePath, AssetRenameItem.Get()->AssetData))
		{
			ConflictingAssets.AddUnique(AssetData);
			AssetRenameItem.Get()->IsConflicting = true;
		}
		else
		{
			AssetRenameItem.Get()->IsConflicting = false;
		}
		
	}
}

void SAssetRenamePanel::OnAssetNameCommitted(const FText& NewText, ETextCommit::Type CommitType,
	TSharedPtr<FAssetRenameItem> AssetItem)
{
	if (CommitType == ETextCommit::OnEnter ||
	CommitType == ETextCommit::OnUserMovedFocus)
	{
		AssetItem->NewName = NewText.ToString();
		// UE_LOG(LogTemp, Warning, TEXT("就哈哈哈%s"), *NewText.ToString());
		RefreshAssetListViewState();
	}
}

void SAssetRenamePanel::UpdateRenameItem(
	const TSharedPtr<FAssetRenameItem>& AssetRenameItem, FString OldName, bool Translate)
{
	ConstructedAssetListView->RequestListRefresh();
	
	const TSharedPtr<FAssetData> AssetData = AssetRenameItem.Get()->AssetData;
	const UClass* ClassName = AssetRenameItem.Get()->AssetData->GetClass();
	FString* PrefixFound = PrefixMap.Find(ClassName);
	
	if (OldName.IsEmpty())
	{
		OldName = AssetData->AssetName.ToString();
	};
	
	AssetRenameItem.Get()->NewName = "";
		
	if (!PrefixFound||PrefixFound->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find Prefix for class %s"), *OldName);
		return;
	}
		
	for (const TPair<UClass*, FString>& Pre: PrefixMap)
	{
		if (Pre.Key == ClassName)
		{
			continue;
		}
			
		if (OldName.StartsWith(Pre.Value, ESearchCase::CaseSensitive))
		{
			OldName = OldName.RightChop(Pre.Value.Len());
		}
			
	}
		
	if (OldName.StartsWith(*PrefixFound))
	{
		AssetRenameItem.Get()->NewName = OldName;
	}
	else
	{
		AssetRenameItem.Get()->NewName = *PrefixFound + AssetData->AssetName.ToString();
			
		FString OutSelectedPackagePath = AssetData->PackagePath.ToString();
		
		if (SuperTranslationAssetUtils::CheckIsNameUsed(OutSelectedPackagePath, AssetRenameItem.Get()->NewName))
		{
			ConflictingAssets.AddUnique(AssetData);
			AssetRenameItem.Get()->IsConflicting = true;
		}
		else
		{
			AssetRenameItem.Get()->IsConflicting = false;
		}
		
		ConstructedAssetListView->RebuildList();
	}
	
	const FString PinYiToChines = SuperTranslationAssetUtils::PreprocessPinyinInput(AssetRenameItem.Get()->NewName);
	if (Translate && !PinYiToChines.IsEmpty())
	{
		GoogleTranslator(AssetRenameItem, PinYiToChines, "en", "zh-CN");
		//因为 Http 翻译是异步的所以
	}
	// ConstructedAssetListView->RequestListRefresh();
}

void SAssetRenamePanel::GoogleTranslator(
	const TSharedPtr<FAssetRenameItem>& AssetRenameItem, const FString& TextToTranslate, const FString& TargetLang,
	const FString& SourceLang)
{
	// 格式化 Google 极简接口的 URL
	FString ApiUrl = FString::Printf(
		TEXT("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%s&tl=%s&dt=t&q=%s"),
		*SourceLang,
		*TargetLang,
		*FGenericPlatformHttp::UrlEncode(TextToTranslate) // 必须对中文进行 UrlEncode 编码
	);
	
	// 创建并发送 HTTP 请求
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiUrl);
	Request->SetVerb(TEXT("GET"));
	
	Request->OnProcessRequestComplete().BindSP(
				SharedThis(this),
				&SAssetRenamePanel::OnProcessRequestComplete,
				AssetRenameItem
			);
	
	Request->ProcessRequest();
}

void SAssetRenamePanel::OnProcessRequestComplete(
	FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess, const TSharedPtr<FAssetRenameItem> Item)
{

	if (bSuccess && Res.IsValid())
	{
		
		FString ResponseString = Res->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	        
		// 因为最外层是 [ 数组，而不是 { 对象
		TArray<TSharedPtr<FJsonValue>> OutermostArray;
		if (FJsonSerializer::Deserialize(Reader, OutermostArray) && OutermostArray.Num() > 0)
		{
			// 1. 获取第一层数组：[[["Hello", "你好", ...]]]
			TSharedPtr<FJsonValue> FirstLayer = OutermostArray[0];
			if (FirstLayer.IsValid() && FirstLayer->Type == EJson::Array)
			{
				const TArray<TSharedPtr<FJsonValue>>& SecondLayerArray = FirstLayer->AsArray();
				if (SecondLayerArray.Num() > 0)
				{
					// 2. 获取第二层数组：[["Hello", "你好", ...]]
					TSharedPtr<FJsonValue> SecondLayer = SecondLayerArray[0];
					if (SecondLayer.IsValid() && SecondLayer->Type == EJson::Array)
					{
						const TArray<TSharedPtr<FJsonValue>>& ThirdLayerArray = SecondLayer->AsArray();
						if (ThirdLayerArray.Num() > 0)
						{
							// 3. 获取第三层数组的第一个元素，即翻译后的字符串 "Hello"
							FString TranslatedText = ThirdLayerArray[0]->AsString();
							
							// 删掉可能出现的空格 
							TranslatedText.ReplaceInline(TEXT(" "), TEXT(""));
							UE_LOG(LogTemp, Warning, TEXT("【JSON 正规解析成功】 翻译结果为: %s"), *TranslatedText);
							// TranslationResults = TranslatedText;
							
							UpdateRenameItem(Item, TranslatedText, false);
							// Item->NewName = TranslatedText;
							ConstructedAssetListView->RebuildList();
						}
					}
				}
			}
		}
	}
}

TSharedRef<SCheckBox> SAssetRenamePanel::ConstructIsTranslate()
{
	ConstructedIsTranslate = SNew(SCheckBox).Type(ESlateCheckBoxType::CheckBox).IsChecked(ECheckBoxState::Checked)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Translate PinYi"))
				];
	return ConstructedIsTranslate.ToSharedRef();
}
