// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SAssetRenamePanel.h"

#include "EditorUtilityLibrary.h"
#include "AssetRegistry/AssetData.h"
#include "Components/VerticalBox.h"
#include "Widgets/Input/SButton.h"
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
			SNew(STextBlock)
				.Text(FText::FromString( AssetDataToDisplay.Get()->NewName ))
		]
	];
	
	return ListViewRowWidget;
}

TSharedRef<SListView<TSharedPtr<FAssetRenameItem>>> SAssetRenamePanel::ConstructAssetListView()
{
	ConstructedAssetListView = SNew(SListView<TSharedPtr<FAssetRenameItem>>)
		.ListItemsSource(&AssetDataAsStruct)
		.OnGenerateRow(this, &SAssetRenamePanel::OnGenerateRowForList);
	
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
		
		if (!PrefixFound||PrefixFound->IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to find Prefix for class %s"), *OldName);
			continue;
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
