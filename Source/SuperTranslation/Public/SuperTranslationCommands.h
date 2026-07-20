// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "SuperTranslationStyle.h"

class FSuperTranslationCommands : public TCommands<FSuperTranslationCommands>
{
public:

	FSuperTranslationCommands()
		: TCommands<FSuperTranslationCommands>(TEXT("SuperTranslation"), NSLOCTEXT("Contexts", "SuperTranslation", "SuperTranslation Plugin"), NAME_None, FSuperTranslationStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
	TSharedPtr< FUICommandInfo > TestAction;
};
