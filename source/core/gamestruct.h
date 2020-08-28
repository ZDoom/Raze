#pragma once

bool System_WantGuiCapture();	// During playing this tells us whether the game must be paused due to active GUI elememts.

#include <stdint.h>
#include "vectors.h"
#include "engineerrors.h"
#include "stats.h"

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

struct ReservedSpace
{
	int top;
	int statusbar;
};

enum EMenuSounds : int;

extern glcycle_t drawtime, actortime, thinktime, gameupdatetime;

struct GameInterface
{
	virtual const char* Name() { return "$"; }
	virtual ~GameInterface() {}
	virtual bool GenerateSavePic() { return false; }
	virtual void app_init() = 0;
	virtual void RunGameFrame() = 0;
	virtual void clearlocalinputstate() {}
	virtual void UpdateScreenSize() {}
	virtual void FreeGameData() {}
	virtual void PlayHudSound() {}
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
	virtual bool SaveGame(FSaveGameNode*) { return true; }
	virtual bool LoadGame(FSaveGameNode*) { return true; }
	virtual void SerializeGameState(FSerializer& arc) {}
	virtual bool CleanupForLoad() { return true; }
	virtual void DrawPlayerSprite(const DVector2& origin, bool onteam) {}
	virtual void QuitToTitle() {}
	virtual void SetAmbience(bool on) {}
	virtual FString GetCoordString() { return "'stat coord' not implemented"; }
	virtual bool CheatAllowed(bool printmsg) { return true; }
	virtual void ExitFromMenu() { throw CExitEvent(0); }
	virtual ReservedSpace GetReservedScreenSpace(int viewsize) { return { 0, 0 }; }
	virtual void ResetFollowPos(bool) {}
	virtual FString statFPS()
	{
		FString output;

		output.AppendFormat("Actor think time: %.3f ms\n", actortime.TimeMS());
		output.AppendFormat("Total think time: %.3f ms\n", thinktime.TimeMS());
		output.AppendFormat("Game Update: %.3f ms\n", gameupdatetime.TimeMS());
		output.AppendFormat("Draw time: %.3f ms\n", drawtime.TimeMS());

		return output;
	}


};

extern GameInterface* gi;


void ImGui_Begin_Frame();

