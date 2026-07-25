// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"
#include "Interfaces/IHttpRequest.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsAsset.h"

class SCheckBox;

struct FAssetRenameItem
{
	TSharedPtr<FAssetData> AssetData;
	FString NewName;
	bool IsConflicting = false;
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
		{UMaterialInstanceConstant::StaticClass(), TEXT("MI_")},
		{UPhysicsAsset::StaticClass(), TEXT("PHYS_")},
		{UPhysicalMaterial::StaticClass(), TEXT("PM_")},
	};
	
	TArray<TSharedPtr<FAssetData>> ConflictingAssets;
	
	void RefreshAssetListViewState();
	void OnAssetNameCommitted(
		const FText& NewText,
		ETextCommit::Type CommitType,
		TSharedPtr<FAssetRenameItem> AssetItem);
	
	void UpdateRenameItem(
		const TSharedPtr<FAssetRenameItem>& AssetRenameItem, FString OldName="", bool Translate=false);
	
public:
	void GoogleTranslator(
	const TSharedPtr<FAssetRenameItem>& AssetRenameItem, const FString& TextToTranslate, const FString& TargetLang, const FString& SourceLang);
	
	void OnProcessRequestComplete(
		FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess, const TSharedPtr<FAssetRenameItem> Item);
	
	TSharedRef<SCheckBox> ConstructIsTranslate();
	TSharedPtr<SCheckBox> ConstructedIsTranslate;
	
};

