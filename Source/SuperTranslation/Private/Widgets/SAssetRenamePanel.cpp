// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SAssetRenamePanel.h"

#include "EditorUtilityLibrary.h"
#include "AssetRegistry/AssetData.h"
#include "Components/VerticalBox.h"
#include "Utils/SuperTranslationAssetUtils.h"
#include "Widgets/Input/SButton.h"
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
			SNew(SInlineEditableTextBlock)
				// .Text(FText::FromString( AssetDataToDisplay.Get()->NewName ))
			.Text_Lambda([AssetDataToDisplay]()
			{
				return FText::FromString(AssetDataToDisplay->NewName);
			})
			.OnTextCommitted_Lambda(
				[AssetDataToDisplay](
					const FText& NewText,
					ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter ||
						CommitType == ETextCommit::OnUserMovedFocus)
					{
						AssetDataToDisplay->NewName = NewText.ToString();
					}
				})
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
	
	for (const TSharedPtr<FAssetRenameItem>& AssetRenameItem: AssetDataAsStruct)
	{
		const TSharedPtr<FAssetData> AssetData = AssetRenameItem.Get()->AssetData;
		const UClass* ClassName = AssetRenameItem.Get()->AssetData->GetClass();
		FString* PrefixFound = PrefixMap.Find(ClassName);
		FString OldName = AssetData->AssetName.ToString();
		AssetRenameItem.Get()->NewName = "";
		
		if (!PrefixFound||PrefixFound->IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to find Prefix for class %s"), *OldName);
			continue;
		}
		
		for (const TPair<UClass*, FString>& Pre: PrefixMap)
		{
			if (Pre.Key == ClassName)
			{
				continue;
			}
			
			if (OldName.StartsWith(Pre.Value, ESearchCase::CaseSensitive))
			{
				AssetRenameItem.Get()->NewName = OldName.RightChop(Pre.Value.Len());
			}
			
		}
		
		if (OldName.StartsWith(*PrefixFound))
		{
			AssetRenameItem.Get()->NewName = OldName;
		}
		else
		{
			AssetRenameItem.Get()->NewName = *PrefixFound + AssetData->AssetName.ToString();
		}
	}
	ConstructedAssetListView->RebuildList();
	return FReply::Handled();
}

FReply SAssetRenamePanel::OnApplyRenameButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnApplyRenameButtonClicked"));
	
	SuperTranslationAssetUtils::FixUpRedirectors();
	
	for (const TSharedPtr<FAssetRenameItem>& AssetRenameItem: AssetDataAsStruct)
	{
		const TSharedPtr<FAssetData> AssetData = AssetRenameItem.Get()->AssetData;
		FString NewName = AssetRenameItem.Get()->NewName;
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
