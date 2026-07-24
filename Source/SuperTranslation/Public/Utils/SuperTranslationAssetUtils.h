// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * 
 */
class SUPERTRANSLATION_API SuperTranslationAssetUtils
{
public:
	SuperTranslationAssetUtils();
	~SuperTranslationAssetUtils();
	
public:
	static void FixUpRedirectors();
	
	// 检查目标资产是否被使用
	static bool CheckIsNameUsed(const FString& FolderPathToCheck, const FString& NameToCheck);
	static bool CheckIsNameUsed(const FString& FolderPathToCheck, const TSharedPtr<FAssetData>& AssetDataToCheck);
};
