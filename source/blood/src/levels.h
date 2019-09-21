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
#include "common_game.h"
#include "inifile.h"

#define kMaxMessages 32
#define kMaxEpisodes 7
#define kMaxLevels 16

#pragma pack(push, 1)

struct GAMEOPTIONS {
    char nGameType;
    char nDifficulty;
    int nEpisode;
    int nLevel;
    char zLevelName[BMAX_PATH];
    char zLevelSong[BMAX_PATH];
    int nTrackNumber; //at12a;
    char szSaveGameName[BMAX_PATH];
    char szUserGameName[BMAX_PATH];
    short nSaveGameSlot;
    int picEntry;
    unsigned int uMapCRC;
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
    MUS_FIRST_SPECIAL = kMaxEpisodes*kMaxLevels,

    MUS_INTRO = MUS_FIRST_SPECIAL,
    MUS_LOADING = MUS_FIRST_SPECIAL + 1,
};

struct LEVELINFO
{
    char at0[BMAX_PATH]; // Filename
    char at90[32]; // Title
    char atb0[32]; // Author
    char atd0[BMAX_PATH]; // Song;
    int ate0; // SongId
    int ate4; // EndingA
    int ate8; // EndingB
    char atec[kMaxMessages][64]; // Messages
    char at8ec; // Fog
    char at8ed; // Weather
}; // 0x8ee bytes

struct EPISODEINFO
{
    char at0[32];
    int nLevels;
    unsigned int bloodbath : 1;
    unsigned int cutALevel : 4;
    LEVELINFO at28[kMaxLevels];
    char at8f08[BMAX_PATH];
    char at8f98[BMAX_PATH];
    int at9028;
    int at902c;
    char at9030[BMAX_PATH];
    char at90c0[BMAX_PATH];
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
void levelPlayIntroScene(int nEpisode);
void levelPlayEndScene(int nEpisode);
void levelClearSecrets(void);
void levelSetupSecret(int nCount);
void levelTriggerSecret(int nSecret);
void CheckSectionAbend(const char *pzSection);
void CheckKeyAbend(const char *pzSection, const char *pzKey);
LEVELINFO * levelGetInfoPtr(int nEpisode, int nLevel);
char * levelGetFilename(int nEpisode, int nLevel);
char * levelGetMessage(int nMessage);
char * levelGetTitle(void);
char * levelGetAuthor(void);
void levelSetupOptions(int nEpisode, int nLevel);
void levelLoadMapInfo(IniFile *pIni, LEVELINFO *pLevelInfo, const char *pzSection);
void levelLoadDefaults(void);
void levelAddUserMap(const char *pzMap);
// EndingA is normal ending, EndingB is secret level
void levelGetNextLevels(int nEpisode, int nLevel, int *pnEndingA, int *pnEndingB);
// arg: 0 is normal exit, 1 is secret level
void levelEndLevel(int arg);

// By NoOne: custom level selection via numbered command which sent to TX ID 6.
void levelEndLevelCustom(int nLevel);

void levelRestart(void);
int levelGetMusicIdx(const char *str);
bool levelTryPlayMusic(int nEpisode, int nlevel, bool bSetLevelSong = false);
void levelTryPlayMusicOrNothing(int nEpisode, int nLevel);
