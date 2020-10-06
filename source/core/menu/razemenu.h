#pragma once
#include "menu.h"
#include "gamestruct.h"
#include "c_cvars.h"

extern bool help_disabled;

void M_StartControlPanel (bool makeSound, bool scaleoverride = false);


extern FNewGameStartup NewGameStartupInfo;
void M_StartupEpisodeMenu(FNewGameStartup *gs);
void M_StartupSkillMenu(FNewGameStartup *gs);
void M_CreateGameMenus();
void SetDefaultMenuColors();

// The savegame manager contains too much code that is game specific. Parts are shareable but need more work first.

struct FSavegameManager
{
private:
	TArray<FSaveGameNode*> SaveGames;
	FSaveGameNode NewSaveNode;
	int LastSaved = -1;
	int LastAccessed = -1;
	FGameTexture *SavePic = nullptr;

public:
	int WindowSize = 0;
	FString SaveCommentString;
	FSaveGameNode *quickSaveSlot = nullptr;
	~FSavegameManager();

private:
	int InsertSaveNode(FSaveGameNode *node);
public:
	void NotifyNewSave(const FString &file, const FString &title, bool okForQuicksave, bool forceQuicksave);
	void ClearSaveGames();

	void ReadSaveStrings();
	void UnloadSaveData();

	int RemoveSaveSlot(int index);
	void LoadSavegame(int Selected);
	void DoSave(int Selected, const char *savegamestring);
	unsigned ExtractSaveData(int index);
	void ClearSaveStuff();
	bool DrawSavePic(int x, int y, int w, int h);
	void DrawSaveComment(FFont *font, int cr, int x, int y, int scalefactor);
	void SetFileInfo(int Selected);
	unsigned SavegameCount();
	FSaveGameNode *GetSavegame(int i);
	void InsertNewSaveNode();
	bool RemoveNewSaveNode();

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
