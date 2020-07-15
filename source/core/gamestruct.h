#pragma once

bool System_WantGuiCapture();	// During playing this tells us whether the game must be paused due to active GUI elememts.

#include <stdint.h>
#include "vectors.h"

struct GameStats
{
	int kill, tkill;
	int secret, tsecret;
	int timesecnd;
	int frags;
};

struct FNewGameStartup
{
	int Episode;
	int Level;
	int Skill;
	int CustomLevel1;
	int CustomLevel2;
};

struct FSavegameInfo
{
	const char *savesig;
	int minsavever;
	int currentsavever;
};

struct FSaveGameNode
{
	FString SaveTitle;
	FString Filename;
	bool bOldVersion = false;
	bool bMissingWads = false;
	bool bNoDelete = false;
	bool bIsExt = false;

	bool isValid() const
	{
		return Filename.IsNotEmpty() && !bOldVersion && !bMissingWads;
	}
};


enum EMenuSounds : int;

struct GameInterface
{
	virtual const char* Name() { return "$"; }
	virtual ~GameInterface() {}
	virtual bool GenerateSavePic() { return false; }
	virtual int app_main() = 0;
	virtual void UpdateScreenSize() {}
	virtual void FreeGameData() {}
	virtual bool validate_hud(int) { return true; }
	virtual void set_hud_layout(int size) = 0;
	virtual void set_hud_scale(int size) {}
	virtual bool automapActive() { return false; }
	virtual void PlayHudSound() {}
	virtual FString statFPS() { return "FPS display not available"; }
	virtual GameStats getStats() { return {}; }
	virtual void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) {}
	virtual void MainMenuOpened() {}
	virtual void MenuOpened() {}
	virtual void MenuClosed() {}
	virtual void MenuSound(EMenuSounds snd) {}
	virtual bool CanSave() { return true; }
	virtual void CustomMenuSelection(int menu, int item) {}
	virtual void StartGame(FNewGameStartup& gs) {}
	virtual FSavegameInfo GetSaveSig() { return { "", 0, 0}; }
	virtual bool DrawSpecialScreen(const DVector2 &origin, int tilenum) { return false; }
	virtual void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool withbg = true);
	virtual double SmallFontScale() { return 1; }
	virtual void DrawMenuCaption(const DVector2& origin, const char* text) {}
	virtual bool SaveGame(FSaveGameNode*) { return false; }
	virtual bool LoadGame(FSaveGameNode*) { return false; }
	virtual bool CleanupForLoad() { return true; }
	virtual void DrawPlayerSprite(const DVector2& origin, bool onteam) {}
	virtual void QuitToTitle() {}
	virtual void SetAmbience(bool on) {}
	virtual FString GetCoordString() { return "'stat coord' not implemented"; }
	virtual bool CheatAllowed(bool printmsg) { return true; }

};

extern GameInterface* gi;
extern double g_beforeSwapTime;


void ImGui_Begin_Frame();

