// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperTranslationCommands.h"

#define LOCTEXT_NAMESPACE "FSuperTranslationModule"

void FSuperTranslationCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "SuperTranslation", "Execute SuperTranslation action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
