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

#include "controls.h"
#include "levels.h"

#define kInputBufferSize 1024

#pragma pack(push, 1)

struct GAMEOPTIONSLEGACY {
    char nGameType;
    char nDifficulty;
    int nEpisode;
    int nLevel;
    char zLevelName[144];
    char zLevelSong[144];
    int nTrackNumber; //at12a;
    char szSaveGameName[16];
    char szUserGameName[16];
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
};

struct DEMOHEADER
{
    int signature;
    short nVersion;
    int nBuild;
    int nInputCount;
    int nNetPlayers;
    short nMyConnectIndex;
    short nConnectHead;
    short connectPoints[8];
};

#pragma pack(pop)

struct DEMOCHAIN
{
    DEMOCHAIN *pNext;
    char zName[BMAX_PATH];
};

class CDemo {
public:
    CDemo();
    ~CDemo();
    bool Create(const char *);
    void Write(GINPUT *);
    void Close(void);
    bool SetupPlayback(const char *);
    void ProcessKeys(void);
    void Playback(void);
    void StopPlayback(void);
    void LoadDemoInfo(void);
    void NextDemo(void);
    void FlushInput(int nCount);
    void ReadInput(int nCount);
    bool at0; // record
    bool at1; // playback
    bool m_bLegacy;
    char at2;
    int at3;
    int hPFile;
    FILE *hRFile;
    int atb;
    DEMOHEADER atf;
    GAMEOPTIONS m_gameOptions;
    GINPUT at1aa[kInputBufferSize];
    const char **pzDemoFile;
    DEMOCHAIN *pFirstDemo;
    DEMOCHAIN *pCurrentDemo;
    int at59ef;
};

extern CDemo gDemo;
