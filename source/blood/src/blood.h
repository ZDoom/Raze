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
#include "gamestruct.h"
#include "mapinfo.h"

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

extern int gNetPlayers;
extern int blood_globalflags;

void QuitGame(void);
void PreloadCache(void);
void StartLevel(MapRecord *gameOptions);
void ProcessFrame(void);
void ScanINIFiles(void);
void EndLevel();

inline bool DemoRecordStatus(void)
{
    return false;
}

inline bool VanillaMode()
{
    return false;
}
void sndPlaySpecialMusicOrNothing(int nMusic);

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Blood"; }
	void app_init() override;
	bool GenerateSavePic() override;
	void FreeLevelData() override;
	void FreeGameData() override;
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
	ReservedSpace GetReservedScreenSpace(int viewsize) override;
	void UpdateSounds() override;
	void GetInput(InputPacket* packet, ControlInfo* const hidInput) override;
	void Ticker() override;
	void DrawBackground() override;
	void Startup() override;
	void Render() override;
	const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord *sng, int skill) override;
	void NextLevel(MapRecord* map, int skill) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	bool DrawAutomapPlayer(int x, int y, int z, int a) override;
	void SetTileProps(int til, int surf, int vox, int shade) override;
	fixed_t playerHorizMin() override { return IntToFixed(-79); }
	fixed_t playerHorizMax() override { return IntToFixed(219); }

	GameStats getStats() override;
};

END_BLD_NS
