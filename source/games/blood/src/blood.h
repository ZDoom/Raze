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

#include "build.h"
#include "gamestruct.h"
#include "mapinfo.h"
#include "d_net.h"

#include "common_game.h"
#include "fx.h"
#include "gameutil.h"
#include "db.h"

#include "actor.h"
#include "ai.h"
#include "aistate.h"
#include "aiunicult.h"
#include "callback.h"
#include "db.h"
#include "endgame.h"
#include "eventq.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "triggers.h"
#include "view.h"
#include "nnexts.h"
#include "player.h"
#include "misc.h"
#include "sectorfx.h"
#include "bloodactor.h"


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
void ProcessFrame(void);
void ScanINIFiles(void);
void EndLevel();

struct MIRROR
{
	int type;
	int link;
	int dx;
	int dy;
	int dz;
	int wallnum;
};

extern MIRROR mirror[16];
extern int mirrorcnt, mirrorsector, mirrorwall[4];


inline bool DemoRecordStatus(void)
{
    return false;
}

inline bool VanillaMode()
{
    return false;
}
void sndPlaySpecialMusicOrNothing(int nMusic);

struct GameInterface : public ::GameInterface
{
	const char* Name() override { return "Blood"; }
	void app_init() override;
	void SerializeGameState(FSerializer& arc) override;
	void loadPalette() override;
	void clearlocalinputstate() override;
	bool GenerateSavePic() override;
	void FreeLevelData() override;
	void FreeGameData() override;
	FSavegameInfo GetSaveSig() override;
	void MenuOpened() override;
	void MenuClosed() override;
	bool CanSave() override;
	FString GetCoordString() override;
	ReservedSpace GetReservedScreenSpace(int viewsize) override;
	void UpdateSounds() override;
	void GetInput(InputPacket* packet, ControlInfo* const hidInput) override;
	void Ticker() override;
	void DrawBackground() override;
	void Startup() override;
	void Render() override;
	const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord *sng, int skill, bool) override;
	void NextLevel(MapRecord* map, int skill) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	bool DrawAutomapPlayer(int x, int y, int z, int a, double const smoothratio) override;
	void SetTileProps(int til, int surf, int vox, int shade) override;
	fixed_t playerHorizMin() override { return IntToFixed(-180); }
	fixed_t playerHorizMax() override { return IntToFixed(120); }
	int playerKeyMove() override { return 1024; }
	void WarpToCoords(int x, int y, int z, int a, int h) override;
	void ToggleThirdPerson() override;
	void SwitchCoopView() override;
	void ToggleShowWeapon() override;
	int chaseCamX(binangle ang) override { return MulScale(-Cos(ang.asbuild()), 1280, 30); }
	int chaseCamY(binangle ang) override { return MulScale(-Sin(ang.asbuild()), 1280, 30); }
	int chaseCamZ(fixedhoriz horiz) override { return FixedToInt(MulScale(horiz.asq16(), 1280, 3)) - (16 << 8); }
	void processSprites(spritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio) override;
	void EnterPortal(spritetype* viewer, int type) override;
	void LeavePortal(spritetype* viewer, int type) override;
	void LoadGameTextures() override;
	int GetCurrentSkill() override;

	GameStats getStats() override;
};

END_BLD_NS
