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
#include "screenjob.h"
#include "common_game.h"
#include "inifile.h"
#include "mapinfo.h"

BEGIN_BLD_NS


enum EGameFlag
{
	GF_AdvanceLevel = 1,
};

struct GAMEOPTIONS {
	uint8_t nGameType;
	uint8_t nDifficulty;
	uint8_t nMonsterSettings;
	int uGameFlags;
	int uNetGameFlags;
	uint8_t nWeaponSettings;
	uint8_t nItemSettings;
	uint8_t nRespawnSettings;
	uint8_t nTeamSettings;
	int nMonsterRespawnTime;
	int nWeaponRespawnTime;
	int nItemRespawnTime;
	int nSpecialRespawnTime;
	int weaponsV10x;
	bool bFriendlyFire;
	bool bKeepKeysOnRespawn;
};

extern GAMEOPTIONS gSingleGameOptions;
extern GAMEOPTIONS gGameOptions;
extern int gSkill;
extern MapRecord* gNextLevel;
extern bool gGameStarted;

void levelLoadDefaults(void);
// arg: 0 is normal exit, 1 is secret level
void levelEndLevel(int arg);
void levelTryPlayMusic();

END_BLD_NS
