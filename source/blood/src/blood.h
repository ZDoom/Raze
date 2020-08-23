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
#include "misc.h"
#include "db.h"
#include "mapinfo.h"

BEGIN_BLD_NS

// Order is that of EDuke32 by necessity because it exposes the key binds to scripting  by index instead of by name.
enum GameFunction_t
{
	gamefunc_Move_Forward,
	gamefunc_Move_Backward,
	gamefunc_Turn_Left,
	gamefunc_Turn_Right,
	gamefunc_Strafe,
	gamefunc_Fire,
	gamefunc_Open,
	gamefunc_Run,
	gamefunc_Alt_Fire,	// Duke3D, Blood
	gamefunc_Jump,
	gamefunc_Crouch,
	gamefunc_Look_Up,
	gamefunc_Look_Down,
	gamefunc_Look_Left,
	gamefunc_Look_Right,
	gamefunc_Strafe_Left,
	gamefunc_Strafe_Right,
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_SendMessage,
	gamefunc_Map,
	gamefunc_Shrink_Screen,
	gamefunc_Enlarge_Screen,
	gamefunc_Show_Opponents_Weapon,
	gamefunc_Map_Follow_Mode,
	gamefunc_See_Coop_View,
	gamefunc_Mouse_Aiming,
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Third_Person_View,
	gamefunc_Toggle_Crouch,
	NUM_ACTIONS
};

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


extern short BloodVersion;
extern int gNetPlayers;
extern bool gRestartGame;
#define GAMEUPDATEAVGTIMENUMSAMPLES 100
extern double g_gameUpdateTime, g_gameUpdateAndDrawTime;
extern double g_gameUpdateAvgTime;
extern int blood_globalflags;

extern bool gSavingGame;
extern bool gQuitGame;
extern int gQuitRequest;

void QuitGame(void);
void PreloadCache(void);
void StartLevel(MapRecord *gameOptions);
void ProcessFrame(void);
void ScanINIFiles(void);
bool DemoRecordStatus(void);
bool VanillaMode(void);
int sndTryPlaySpecialMusic(int nMusic);
void sndPlaySpecialMusicOrNothing(int nMusic);

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Blood"; }
	void app_init() override;
	void RunGameFrame() override;
	bool GenerateSavePic() override;
	void FreeGameData() override;
	FString statFPS() override;
	FSavegameInfo GetSaveSig() override;
	void MenuOpened() override;
	void MenuClosed() override;
	bool CanSave() override;
	void StartGame(FNewGameStartup& gs) override;
	void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) override;
	void DrawMenuCaption(const DVector2& origin, const char* text) override;
	bool SaveGame(FSaveGameNode*) override;
	bool LoadGame(FSaveGameNode*) override;
	void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	void clearlocalinputstate() override;
	ReservedSpace GetReservedScreenSpace(int viewsize) override;

	GameStats getStats() override;
};

END_BLD_NS
