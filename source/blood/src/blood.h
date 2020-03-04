//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once

#include "levels.h"
#include "resource.h"
#include "db.h"

BEGIN_BLD_NS

struct INIDESCRIPTION {
    const char *pzName;
    const char *pzFilename;
    const char **pzArts;
    int nArts;
};

struct INICHAIN {
    INICHAIN *pNext;
    char zName[BMAX_PATH];
    INIDESCRIPTION *pDescription;
};

extern INICHAIN *pINIChain;


enum INPUT_MODE {
    kInputGame = 0,
    kInputMessage,
    kInputEndGame,
};

extern INPUT_MODE gInputMode;
extern short BloodVersion;
extern int gNetPlayers;
extern bool gRestartGame;
#define GAMEUPDATEAVGTIMENUMSAMPLES 100
extern double g_gameUpdateTime, g_gameUpdateAndDrawTime;
extern double g_gameUpdateAvgTime;
extern int blood_globalflags;

extern bool gPaused;
extern bool gSavingGame;
extern bool gQuitGame;
extern int gQuitRequest;

void QuitGame(void);
void PreloadCache(void);
void StartLevel(GAMEOPTIONS *gameOptions);
void ProcessFrame(void);
void ScanINIFiles(void);
bool DemoRecordStatus(void);
bool VanillaMode(void);
bool fileExistsRFF(int id, const char* ext);
int sndTryPlaySpecialMusic(int nMusic);
void sndPlaySpecialMusicOrNothing(int nMusic);

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Blood"; }
	void faketimerhandler() override;
	int app_main() override;
	void UpdateScreenSize() override;
	bool GenerateSavePic() override;
	void FreeGameData() override;
	void set_hud_layout(int size) override;
	void set_hud_scale(int size) override;
	FString statFPS() override;
	FSavegameInfo GetSaveSig() override;
	void MenuOpened() override;
	void MenuClosed() override;
	bool CanSave() override;
	void StartGame(FGameStartup& gs) override;
	void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) override;
	void DrawMenuCaption(const DVector2& origin, const char* text) override;
	bool SaveGame(FSaveGameNode*) override;
	bool LoadGame(FSaveGameNode*) override;
	void DoPrintMessage(int prio, const char*) override;
	void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	int GetStringTile(int font, const char* t, int f) override;

	GameStats getStats() override;
};

END_BLD_NS
