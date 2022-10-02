#pragma once
#include "menu.h"
#include "gamestruct.h"
#include "c_cvars.h"
#include "savegamemanager.h"

extern bool help_disabled;

extern FNewGameStartup NewGameStartupInfo;
void M_StartupEpisodeMenu(FNewGameStartup *gs);
void M_StartupSkillMenu(FNewGameStartup *gs);
void SetDefaultMenuColors();
void BuildGameMenus();

class FSavegameManager : public FSavegameManagerBase
{
	void PerformSaveGame(const char *fn, const char *sgdesc) override;
	void PerformLoadGame(const char *fn, bool) override;
	FString ExtractSaveComment(FSerializer &arc) override;
	FString BuildSaveName(const char* prefix, int slot) override;
	void ReadSaveStrings() override;
};

extern FSavegameManager savegameManager;

enum EMenuSounds : int
{
	ActivateSound,
	CursorSound,
	AdvanceSound,
	BackSound,
	CloseSound,
	PageSound,
	ChangeSound,
	ChooseSound
};

EXTERN_CVAR(Bool, menu_sounds)
