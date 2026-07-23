// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"

struct FAssetRenameItem
{
	TSharedPtr<FAssetData> AssetData;
	FString NewName;
};

/**
 * 
 */
class  SAssetRenamePanel: public SCompoundWidget  
{
	SLATE_BEGIN_ARGS(SAssetRenamePanel){}
	SLATE_ARGUMENT(TArray<TSharedPtr<FAssetData>>, AssetDataToStore);
	SLATE_END_ARGS()
	
public:
	void Construct(const FArguments& InArgs);
	
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FAssetRenameItem> AssetDataToDisplay, 
	const TSharedRef<STableViewBase>& OwnerTable);
	
	TSharedRef<SListView<TSharedPtr<FAssetRenameItem>>> ConstructAssetListView();
	TSharedPtr<SListView<TSharedPtr<FAssetRenameItem>>> ConstructedAssetListView;
	
	FReply OnRenameButtonClicked();
	FReply OnApplyRenameButtonClicked();
	
	TArray<TSharedPtr<FAssetRenameItem>> AssetDataAsStruct;
	
	TMap<UClass*, FString> PrefixMap = {
		{UBlueprint::StaticClass(), TEXT("BP_")},
		{UMaterial::StaticClass(), TEXT("M_")},
		{UMaterialInstanceConstant::StaticClass(), TEXT("MI_")}
	};
};
