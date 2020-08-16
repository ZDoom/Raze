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


#define kMaxMessages 32
#define kMaxEpisodes 7
#define kMaxLevels 16

#pragma pack(push, 1)

struct GAMEOPTIONS {
    unsigned char nGameType;
    unsigned char nDifficulty;
    int nEpisode;
    int nLevel;
    short nSaveGameSlot;
    int picEntry;
    char nMonsterSettings;
    int uGameFlags;
    int uNetGameFlags;
    char nWeaponSettings;
    char nItemSettings;
    char nRespawnSettings;
    char nTeamSettings;
    int nMonsterRespawnTime;
    int nWeaponRespawnTime;
    int nItemRespawnTime;
    int nSpecialRespawnTime;
    int weaponsV10x;
    bool bFriendlyFire;
    bool bKeepKeysOnRespawn;
};

#pragma pack(pop)

enum {
    MUS_INTRO = 0,
    MUS_LOADING = 1,
};

struct EPISODEINFO
{
    int nLevels;
    unsigned int bloodbath : 1;
    unsigned int cutALevel : 4;
    char cutsceneAName[BMAX_PATH];
    char cutsceneBName[BMAX_PATH];
    int at9028;
    int at902c;
    char cutsceneASound[BMAX_PATH];
    char cutsceneBSound[BMAX_PATH];
};

extern EPISODEINFO gEpisodeInfo[];
extern GAMEOPTIONS gSingleGameOptions;
extern GAMEOPTIONS gGameOptions;
extern int gSkill;
extern char BloodIniFile[];
extern char BloodIniPre[];
extern bool bINIOverride;
extern int gEpisodeCount;
extern int gNextLevel;
extern bool gGameStarted;
extern int gLevelTime;

void levelInitINI(const char *pzIni);
void levelOverrideINI(const char *pzIni);
void levelPlayIntroScene(int nEpisode, CompletionFunc completion);
void levelPlayEndScene(int nEpisode, CompletionFunc completion);
void levelSetupSecret(int nCount);
void levelTriggerSecret(int nSecret);
void CheckSectionAbend(const char *pzSection);
void CheckKeyAbend(const char *pzSection, const char *pzKey);
void levelSetupOptions(int nEpisode, int nLevel);
void levelLoadDefaults(void);
void levelAddUserMap(const char *pzMap);
// EndingA is normal ending, EndingB is secret level
void levelGetNextLevels(int nEpisode, int nLevel, int *pnEndingA, int *pnEndingB);
// arg: 0 is normal exit, 1 is secret level
void levelEndLevel(int arg);
void levelRestart(void);
bool levelTryPlayMusic();

END_BLD_NS
