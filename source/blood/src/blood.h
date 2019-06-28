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
extern INICHAIN const*pINISelected;

enum INPUT_MODE {
    INPUT_MODE_0 = 0,
    INPUT_MODE_1,
    INPUT_MODE_2,
    INPUT_MODE_3,
};

extern INPUT_MODE gInputMode;
extern char SetupFilename[BMAX_PATH];
extern int32_t gNoSetup;
extern short BloodVersion;
extern int gNetPlayers;
extern bool gRestartGame;
#define GAMEUPDATEAVGTIMENUMSAMPLES 100
extern double g_gameUpdateTime, g_gameUpdateAndDrawTime;
extern double g_gameUpdateAvgTime;
extern int blood_globalflags;
extern int gMusicPrevLoadedEpisode;
extern int gMusicPrevLoadedLevel;

extern int gSaveGameNum;
extern bool gPaused;
extern bool gSaveGameActive;
extern bool gSavingGame;
extern bool gQuitGame;
extern int gQuitRequest;
extern int gCacheMiss;

void QuitGame(void);
void PreloadCache(void);
void StartLevel(GAMEOPTIONS *gameOptions);
void ProcessFrame(void);
void ScanINIFiles(void);
bool LoadArtFile(const char *pzFile);
void LoadExtraArts(void);
bool DemoRecordStatus(void);
bool VanillaMode(void);
bool fileExistsRFF(int id, const char* ext);
int sndTryPlaySpecialMusic(int nMusic);
void sndPlaySpecialMusicOrNothing(int nMusic);