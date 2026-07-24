// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/SuperTranslationAssetUtils.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/ObjectRedirector.h"
#include "EditorAssetLibrary.h"


SuperTranslationAssetUtils::SuperTranslationAssetUtils()
{
}

SuperTranslationAssetUtils::~SuperTranslationAssetUtils()
{
}

void SuperTranslationAssetUtils::FixUpRedirectors()
{
	// 保存找到的所有重定向器
	TArray<UObjectRedirector*> RedirectorToFixArray;
	
	// 加载资产注册表模块
	const FAssetRegistryModule& AssetRegistryModule =
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"));
	
	// 设置资产搜索条件
	FARFilter Filter;
	
	// 搜索 /Game 下的所有子目录
	Filter.bRecursivePaths = true;
	
	// 搜索项目 Content 目录
	Filter.PackagePaths.Emplace("/Game");
	
	// 只搜索 UObjectRedirector 类型
	Filter.ClassPaths.Emplace(
		UObjectRedirector::StaticClass()->GetClassPathName()
		);
	
	// 保存搜索到的资产数据
	TArray<FAssetData> OutReferences;
	
	// 根据过滤条件查找资产
	AssetRegistryModule.Get().GetAssets(Filter, OutReferences);
	
	// 将 FAssetData 转换为 UObjectRedirector
	for (const FAssetData& AssetData:OutReferences)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(AssetData.GetAsset()))
		{
			RedirectorToFixArray.Add(RedirectorToFix);
		}
	}
	
	// 没有找到重定向器时直接结束
	if (RedirectorToFixArray.IsEmpty())
	{
		return;
	}
	
	// 加载 AssetTools 模块
	const FAssetToolsModule& AssetToolsModule = 
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	
	// 修复所有指向重定向器的资产引用
	AssetToolsModule.Get().FixupReferencers(
		RedirectorToFixArray, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
	
}

//检查目标资产是否被使用
bool SuperTranslationAssetUtils::CheckIsNameUsed(const FString& FolderPathToCheck, const FString& NameToCheck)
{
	TArray<FString> ExistingAssetsPaths = UEditorAssetLibrary::ListAssets(FolderPathToCheck, false);
	
	for (const FString& ExistingAssetPath: ExistingAssetsPaths)
	{
		const FString ExistingAssetName = FPaths::GetBaseFilename(ExistingAssetPath);
		if (ExistingAssetName.Equals(NameToCheck))
		{
			// FMessageDialog::Open(
			// EAppMsgType::Ok,
			// FText::FromString(MaterialNameToCheck + TEXT(" 已被资产使用"))
			// );
			return true;
		}
	}
	
	return false;
}

bool SuperTranslationAssetUtils::CheckIsNameUsed(
	const FString& FolderPathToCheck, const TSharedPtr<FAssetData>& AssetDataToCheck)
{
	const TArray<FString> ExistingAssetsPaths = UEditorAssetLibrary::ListAssets(FolderPathToCheck, false);
	
	for (const FString& ExistingAssetPath: ExistingAssetsPaths)
	{
		const FAssetData AssetData = UEditorAssetLibrary::FindAssetData(ExistingAssetPath);
		
		// UE_LOG(LogTemp, Warning, TEXT("骄傲u骄傲 %s:::::%s"), *ExistingAssetPath, *AssetDataToCheck->GetFullName())
		if (AssetData.IsValid() && AssetData.GetFullName() == AssetDataToCheck->GetFullName())
		{
			
			// UE_LOG(LogTemp, Warning, TEXT("咕咕嘎嘎、"));
			continue;
		}
		
		if (ExistingAssetPath.Equals(AssetDataToCheck->GetFullName()))
		{
			// FMessageDialog::Open(
			// EAppMsgType::Ok,
			// FText::FromString(MaterialNameToCheck + TEXT(" 已被资产使用"))
			// );
			return true;
		}
	}
	
	return false;
}
