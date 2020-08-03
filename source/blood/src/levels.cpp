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
#include "ns.h"	// Must come before everything else!

#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "common_game.h"

#include "blood.h"
#include "globals.h"
#include "endgame.h"
#include "inifile.h"
#include "levels.h"
#include "loadsave.h"
#include "messages.h"
#include "network.h"
#include "seq.h"
#include "sound.h"
#include "view.h"
#include "eventq.h"
#include "menu.h"

BEGIN_BLD_NS

GAMEOPTIONS gGameOptions;

GAMEOPTIONS gSingleGameOptions = {
    0, 2, 0, 0, "", 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 3600, 1800, 1800, 7200
};

EPISODEINFO gEpisodeInfo[kMaxEpisodes+1];

int gSkill = 2;
int gEpisodeCount;
int gNextLevel;

int gLevelTime;

char BloodIniFile[BMAX_PATH] = "BLOOD.INI";
char BloodIniPre[BMAX_PATH];
bool bINIOverride = false;
IniFile *BloodINI;


void levelInitINI(const char *pzIni)
{
	if (!fileSystem.FileExists(pzIni))
        ThrowError("Initialization: %s does not exist", pzIni);
    BloodINI = new IniFile(pzIni);
    Bstrncpy(BloodIniFile, pzIni, BMAX_PATH);
    Bstrncpy(BloodIniPre, pzIni, BMAX_PATH);
    ChangeExtension(BloodIniPre, "");
}


void levelOverrideINI(const char *pzIni)
{
    bINIOverride = true;
    strcpy(BloodIniFile, pzIni);
}

void levelClearSecrets(void)
{
    gSecretMgr.Clear();
}

void levelSetupSecret(int nCount)
{
    gSecretMgr.SetCount(nCount);
}

void levelTriggerSecret(int nSecret)
{
    gSecretMgr.Found(nSecret);
}

void CheckSectionAbend(const char *pzSection)
{
    if (!pzSection || !BloodINI->SectionExists(pzSection))
        ThrowError("Section [%s] expected in BLOOD.INI", pzSection);
}

void CheckKeyAbend(const char *pzSection, const char *pzKey)
{
    dassert(pzSection != NULL);

    if (!pzKey || !BloodINI->KeyExists(pzSection, pzKey))
        ThrowError("Key %s expected in section [%s] of BLOOD.INI", pzKey, pzSection);
}


void levelSetupOptions(int nEpisode, int nLevel)
{
    gGameOptions.nEpisode = nEpisode;
    gGameOptions.nLevel = nLevel;
    strcpy(gGameOptions.zLevelName, gEpisodeInfo[nEpisode].levels[nLevel].labelName);
    gGameOptions.uMapCRC = dbReadMapCRC(gGameOptions.zLevelName);
    gGameOptions.nTrackNumber = gEpisodeInfo[nEpisode].levels[nLevel].cdSongId;
}

void levelLoadMapInfo(IniFile *pIni, MapRecord *pLevelInfo, const char *pzSection, int epinum, int mapnum)
{
    char buffer[16];
    pLevelInfo->SetName(pIni->GetKeyString(pzSection, "Title", pLevelInfo->labelName));
    pLevelInfo->author = pIni->GetKeyString(pzSection, "Author", "");
    pLevelInfo->music = pIni->GetKeyString(pzSection, "Song", ""); DefaultExtension(pLevelInfo->music, ".mid");
    pLevelInfo->cdSongId = pIni->GetKeyInt(pzSection, "Track", -1);
    pLevelInfo->nextLevel = pIni->GetKeyInt(pzSection, "EndingA", -1); //if (pLevelInfo->nextLevel >= 0) pLevelInfo->nextLevel +epinum * kMaxLevels;
    pLevelInfo->nextSecret = pIni->GetKeyInt(pzSection, "EndingB", -1); //if (pLevelInfo->nextSecret >= 0) pLevelInfo->nextSecret + epinum * kMaxLevels;
    pLevelInfo->fog = pIni->GetKeyInt(pzSection, "Fog", -0);
    pLevelInfo->weather = pIni->GetKeyInt(pzSection, "Weather", -0);
    pLevelInfo->messageStart = 1024 + ((epinum * kMaxLevels) + mapnum) * kMaxMessages;
    for (int i = 0; i < kMaxMessages; i++)
    {
        sprintf(buffer, "Message%d", i+1);
        quoteMgr.InitializeQuote(pLevelInfo->messageStart + i, pIni->GetKeyString(pzSection, buffer, ""), true);
    }
}

static const char* DefFile(void)
{
    // The command line parser stores this in the CON field.
    return userConfig.DefaultCon.IsNotEmpty() ? userConfig.DefaultCon.GetChars() : "blood.ini";
}

void levelLoadDefaults(void)
{
    char buffer[64];
    char buffer2[16];
    levelInitINI(DefFile());
    memset(gEpisodeInfo, 0, sizeof(gEpisodeInfo));
    quoteMgr.InitializeQuote(MUS_INTRO, "PESTIS.MID");
    int i;
    for (i = 0; i < kMaxEpisodes; i++)
    {
        sprintf(buffer, "Episode%d", i+1);
        if (!BloodINI->SectionExists(buffer))
            break;
        EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[i];
		auto ep_str = BloodINI->GetKeyString(buffer, "Title", buffer);
		gVolumeNames[i] = ep_str; // only keep one table for the names. Todo: Consolidate this across games.
        strncpy(pEpisodeInfo->cutsceneAName, BloodINI->GetKeyString(buffer, "CutSceneA", ""), BMAX_PATH);
        pEpisodeInfo->at9028 = BloodINI->GetKeyInt(buffer, "CutWavA", -1);
        if (pEpisodeInfo->at9028 == 0)
            strncpy(pEpisodeInfo->cutsceneASound, BloodINI->GetKeyString(buffer, "CutWavA", ""), BMAX_PATH);
        else
            pEpisodeInfo->cutsceneASound[0] = 0;
        strncpy(pEpisodeInfo->cutsceneBName, BloodINI->GetKeyString(buffer, "CutSceneB", ""), BMAX_PATH);
        pEpisodeInfo->at902c = BloodINI->GetKeyInt(buffer, "CutWavB", -1);
        if (pEpisodeInfo->at902c == 0)
            strncpy(pEpisodeInfo->cutsceneBSound, BloodINI->GetKeyString(buffer, "CutWavB", ""), BMAX_PATH);
        else
            pEpisodeInfo->cutsceneBSound[0] = 0;

        pEpisodeInfo->bloodbath = BloodINI->GetKeyInt(buffer, "BloodBathOnly", 0);
        pEpisodeInfo->cutALevel = BloodINI->GetKeyInt(buffer, "CutSceneALevel", 0);
        if (pEpisodeInfo->cutALevel > 0)
            pEpisodeInfo->cutALevel--;
        pEpisodeInfo->levels = mapList + i * kMaxLevels;
        int j;
        for (j = 0; j < kMaxLevels; j++)
        {
            auto pLevelInfo = &pEpisodeInfo->levels[j];
            sprintf(buffer2, "Map%d", j+1);
            if (!BloodINI->KeyExists(buffer, buffer2))
                break;
            const char *pMap = BloodINI->GetKeyString(buffer, buffer2, NULL);
            CheckSectionAbend(pMap);
            pLevelInfo->labelName = pMap;
            pLevelInfo->fileName.Format("%s.map", pMap);
            levelLoadMapInfo(BloodINI, pLevelInfo, pMap, i, j);
        }
        pEpisodeInfo->nLevels = j;
    }
    gEpisodeCount = i;
}

void levelAddUserMap(const char *pzMap)
{
	// FIXME: Make this work with the reworked map system
    char buffer[BMAX_PATH];
    strncpy(buffer, pzMap, BMAX_PATH);
    ChangeExtension(buffer, ".DEF");

    IniFile UserINI(buffer);
    int nEpisode = ClipRange(UserINI.GetKeyInt(NULL, "Episode", 0), 0, 5);
    EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[nEpisode];
    int nLevel = ClipRange(UserINI.GetKeyInt(NULL, "Level", pEpisodeInfo->nLevels), 0, 15);
    if (nLevel >= pEpisodeInfo->nLevels)
    {
        if (pEpisodeInfo->nLevels == 0)
        {
            gEpisodeCount++;
			gVolumeNames[nEpisode].Format("Episode %d", nEpisode+1);
        }
        nLevel = pEpisodeInfo->nLevels++;
    }
    auto pLevelInfo = &pEpisodeInfo->levels[nLevel];
    ChangeExtension(buffer, "");
    pLevelInfo->name = buffer;
    levelLoadMapInfo(&UserINI, pLevelInfo, NULL, nEpisode, nLevel);
    gGameOptions.nEpisode = nEpisode;
    gGameOptions.nLevel = nLevel;
    gGameOptions.uMapCRC = dbReadMapCRC(pLevelInfo->name);
    strcpy(gGameOptions.zLevelName, pLevelInfo->name);
}

void levelGetNextLevels(int nEpisode, int nLevel, int *pnEndingA, int *pnEndingB)
{
    dassert(pnEndingA != NULL && pnEndingB != NULL);
    auto pLevelInfo = &gEpisodeInfo[nEpisode].levels[nLevel];
    int nEndingA = pLevelInfo->nextLevel;
    if (nEndingA >= 0)
        nEndingA--;
    int nEndingB = pLevelInfo->nextSecret;
    if (nEndingB >= 0)
        nEndingB--;
    *pnEndingA = nEndingA;
    *pnEndingB = nEndingB;
}

void levelEndLevel(int arg)
{
    int nEndingA, nEndingB;
    EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[gGameOptions.nEpisode];
    gGameOptions.uGameFlags |= 1;
    levelGetNextLevels(gGameOptions.nEpisode, gGameOptions.nLevel, &nEndingA, &nEndingB);
    switch (arg)
    {
    case 0:
        if (nEndingA == -1)
        {
            if (pEpisodeInfo->cutsceneBName[0])
                gGameOptions.uGameFlags |= 8;
            gGameOptions.nLevel = 0;
            gGameOptions.uGameFlags |= 2;
        }
        else
            gNextLevel = nEndingA;
        break;
    case 1:
        if (nEndingB == -1)
        {
            if (gGameOptions.nEpisode + 1 < gEpisodeCount)
            {
                if (pEpisodeInfo->cutsceneBName[0])
                    gGameOptions.uGameFlags |= 8;
                gGameOptions.nLevel = 0;
                gGameOptions.uGameFlags |= 2;
            }
            else
            {
                gGameOptions.nLevel = 0;
                gGameOptions.uGameFlags |= 1;
            }
        }
        else
            gNextLevel = nEndingB;
        break;
    }
}

void levelRestart(void)
{
    levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    gStartNewGame = true;
}

int levelGetMusicIdx(const char *str)
{
    int32_t lev, ep;
    signed char b1, b2;

    int numMatches = sscanf(str, "%c%d%c%d", &b1, &ep, &b2, &lev);

    if (numMatches != 4 || Btoupper(b1) != 'E' || Btoupper(b2) != 'L')
        return -1;

    if ((unsigned)--lev >= kMaxLevels || (unsigned)--ep >= kMaxEpisodes)
        return -2;

    return (ep * kMaxLevels) + lev;
}

bool levelTryPlayMusic(int nEpisode, int nLevel, bool bSetLevelSong)
{
    FString buffer;
    if (mus_redbook && gEpisodeInfo[nEpisode].levels[nLevel].cdSongId > 0)
        buffer.Format("blood%02i.ogg", gEpisodeInfo[nEpisode].levels[nLevel].cdSongId);
    else
    {
        buffer = gEpisodeInfo[nEpisode].levels[nLevel].music;
        DefaultExtension(buffer, ".mid");
    }
    return !!Mus_Play(gEpisodeInfo[nEpisode].levels[nLevel].labelName, buffer, true);
}

void levelTryPlayMusicOrNothing(int nEpisode, int nLevel)
{
    if (!levelTryPlayMusic(nEpisode, nLevel, true))
        Mus_Play("", "", true);
}

class LevelsLoadSave : public LoadSave
{
    virtual void Load(void);
    virtual void Save(void);
};


static LevelsLoadSave *myLoadSave;

void LevelsLoadSave::Load(void)
{
    Read(&gNextLevel, sizeof(gNextLevel));
    Read(&gGameOptions, sizeof(gGameOptions));
}

void LevelsLoadSave::Save(void)
{
    Write(&gNextLevel, sizeof(gNextLevel));
    Write(&gGameOptions, sizeof(gGameOptions));
}

void LevelsLoadSaveConstruct(void)
{
    myLoadSave = new LevelsLoadSave();
}

END_BLD_NS
