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

#include "asound.h"
#include "blood.h"
#include "config.h"
#include "credits.h"
#include "endgame.h"
#include "inifile.h"
#include "levels.h"
#include "loadsave.h"
#include "messages.h"
#include "network.h"
#include "screen.h"
#include "seq.h"
#include "sound.h"
#include "sfx.h"
#include "view.h"
#include "eventq.h"

BEGIN_BLD_NS

GAMEOPTIONS gGameOptions;

GAMEOPTIONS gSingleGameOptions = {
    0, 2, 0, 0, "", "", 2, "", "", 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 3600, 1800, 1800, 7200
};

EPISODEINFO gEpisodeInfo[kMaxEpisodes+1];

int gSkill = 2;
int gEpisodeCount;
int gNextLevel;
bool gGameStarted;

int gLevelTime;

char BloodIniFile[BMAX_PATH] = "BLOOD.INI";
char BloodIniPre[BMAX_PATH];
bool bINIOverride = false;
IniFile *BloodINI;


void levelInitINI(const char *pzIni)
{
	if (!testkopen(pzIni, 0))
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

void levelPlayIntroScene(int nEpisode)
{
    gGameOptions.uGameFlags &= ~4;
    sndStopSong();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
    credPlaySmk(pEpisode->at8f08, pEpisode->at9030, pEpisode->at9028);
    scrSetDac();
    viewResizeView(gViewSize);
    credReset();
    scrSetDac();
}

void levelPlayEndScene(int nEpisode)
{
    gGameOptions.uGameFlags &= ~8;
    sndStopSong();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
    credPlaySmk(pEpisode->at8f98, pEpisode->at90c0, pEpisode->at902c);
    scrSetDac();
    viewResizeView(gViewSize);
    credReset();
    scrSetDac();
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

LEVELINFO * levelGetInfoPtr(int nEpisode, int nLevel)
{
    dassert(nEpisode >= 0 && nEpisode < gEpisodeCount);
    EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[nEpisode];
    dassert(nLevel >= 0 && nLevel < pEpisodeInfo->nLevels);
    return &pEpisodeInfo->at28[nLevel];
}

char * levelGetFilename(int nEpisode, int nLevel)
{
    dassert(nEpisode >= 0 && nEpisode < gEpisodeCount);
    EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[nEpisode];
    dassert(nLevel >= 0 && nLevel < pEpisodeInfo->nLevels);
    return pEpisodeInfo->at28[nLevel].at0;
}

char * levelGetMessage(int nMessage)
{
    int nEpisode = gGameOptions.nEpisode;
    int nLevel = gGameOptions.nLevel;
    dassert(nMessage < kMaxMessages);
    char *pMessage = gEpisodeInfo[nEpisode].at28[nLevel].atec[nMessage];
    if (*pMessage == 0)
        return NULL;
    return pMessage;
}

char * levelGetTitle(void)
{
    int nEpisode = gGameOptions.nEpisode;
    int nLevel = gGameOptions.nLevel;
    char *pTitle = gEpisodeInfo[nEpisode].at28[nLevel].at90;
    if (*pTitle == 0)
        return NULL;
    return pTitle;
}

char * levelGetAuthor(void)
{
    int nEpisode = gGameOptions.nEpisode;
    int nLevel = gGameOptions.nLevel;
    char *pAuthor = gEpisodeInfo[nEpisode].at28[nLevel].atb0;
    if (*pAuthor == 0)
        return NULL;
    return pAuthor;
}

void levelSetupOptions(int nEpisode, int nLevel)
{
    gGameOptions.nEpisode = nEpisode;
    gGameOptions.nLevel = nLevel;
    strcpy(gGameOptions.zLevelName, gEpisodeInfo[nEpisode].at28[nLevel].at0);
    gGameOptions.uMapCRC = dbReadMapCRC(gGameOptions.zLevelName);
    // strcpy(gGameOptions.zLevelSong, gEpisodeInfo[nEpisode].at28[nLevel].atd0);
    gGameOptions.nTrackNumber = gEpisodeInfo[nEpisode].at28[nLevel].ate0;
}

void levelLoadMapInfo(IniFile *pIni, LEVELINFO *pLevelInfo, const char *pzSection)
{
    char buffer[16];
    strncpy(pLevelInfo->at90, pIni->GetKeyString(pzSection, "Title", pLevelInfo->at0), 31);
    strncpy(pLevelInfo->atb0, pIni->GetKeyString(pzSection, "Author", ""), 31);
    strncpy(pLevelInfo->atd0, pIni->GetKeyString(pzSection, "Song", ""), BMAX_PATH);
    pLevelInfo->ate0 = pIni->GetKeyInt(pzSection, "Track", -1);
    pLevelInfo->ate4 = pIni->GetKeyInt(pzSection, "EndingA", -1);
    pLevelInfo->ate8 = pIni->GetKeyInt(pzSection, "EndingB", -1);
    pLevelInfo->at8ec = pIni->GetKeyInt(pzSection, "Fog", -0);
    pLevelInfo->at8ed = pIni->GetKeyInt(pzSection, "Weather", -0);
    for (int i = 0; i < kMaxMessages; i++)
    {
        sprintf(buffer, "Message%d", i+1);
        strncpy(pLevelInfo->atec[i], pIni->GetKeyString(pzSection, buffer, ""), 63);
    }
}

extern void MenuSetupEpisodeInfo(void);

void levelLoadDefaults(void)
{
    char buffer[64];
    char buffer2[16];
    levelInitINI(G_ConFile());	// This doubles for the INI in the global code.
    memset(gEpisodeInfo, 0, sizeof(gEpisodeInfo));
    strncpy(gEpisodeInfo[MUS_INTRO/kMaxLevels].at28[MUS_INTRO%kMaxLevels].atd0, "PESTIS", BMAX_PATH);
    int i;
    for (i = 0; i < kMaxEpisodes; i++)
    {
        sprintf(buffer, "Episode%d", i+1);
        if (!BloodINI->SectionExists(buffer))
            break;
        EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[i];
        strncpy(pEpisodeInfo->at0, BloodINI->GetKeyString(buffer, "Title", buffer), 31);
        strncpy(pEpisodeInfo->at8f08, BloodINI->GetKeyString(buffer, "CutSceneA", ""), BMAX_PATH);
        pEpisodeInfo->at9028 = BloodINI->GetKeyInt(buffer, "CutWavA", -1);
        if (pEpisodeInfo->at9028 == 0)
            strncpy(pEpisodeInfo->at9030, BloodINI->GetKeyString(buffer, "CutWavA", ""), BMAX_PATH);
        else
            pEpisodeInfo->at9030[0] = 0;
        strncpy(pEpisodeInfo->at8f98, BloodINI->GetKeyString(buffer, "CutSceneB", ""), BMAX_PATH);
        pEpisodeInfo->at902c = BloodINI->GetKeyInt(buffer, "CutWavB", -1);
        if (pEpisodeInfo->at902c == 0)
            strncpy(pEpisodeInfo->at90c0, BloodINI->GetKeyString(buffer, "CutWavB", ""), BMAX_PATH);
        else
            pEpisodeInfo->at90c0[0] = 0;

        pEpisodeInfo->bloodbath = BloodINI->GetKeyInt(buffer, "BloodBathOnly", 0);
        pEpisodeInfo->cutALevel = BloodINI->GetKeyInt(buffer, "CutSceneALevel", 0);
        if (pEpisodeInfo->cutALevel > 0)
            pEpisodeInfo->cutALevel--;
        int j;
        for (j = 0; j < kMaxLevels; j++)
        {
            LEVELINFO *pLevelInfo = &pEpisodeInfo->at28[j];
            sprintf(buffer2, "Map%d", j+1);
            if (!BloodINI->KeyExists(buffer, buffer2))
                break;
            const char *pMap = BloodINI->GetKeyString(buffer, buffer2, NULL);
            CheckSectionAbend(pMap);
            strncpy(pLevelInfo->at0, pMap, BMAX_PATH);
            levelLoadMapInfo(BloodINI, pLevelInfo, pMap);
        }
        pEpisodeInfo->nLevels = j;
    }
    gEpisodeCount = i;
    MenuSetupEpisodeInfo();
}

void levelAddUserMap(const char *pzMap)
{
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
            sprintf(pEpisodeInfo->at0, "Episode %d", nEpisode);
        }
        nLevel = pEpisodeInfo->nLevels++;
    }
    LEVELINFO *pLevelInfo = &pEpisodeInfo->at28[nLevel];
    ChangeExtension(buffer, "");
    strncpy(pLevelInfo->at0, buffer, BMAX_PATH);
    levelLoadMapInfo(&UserINI, pLevelInfo, NULL);
    gGameOptions.nEpisode = nEpisode;
    gGameOptions.nLevel = nLevel;
    gGameOptions.uMapCRC = dbReadMapCRC(pLevelInfo->at0);
    strcpy(gGameOptions.zLevelName, pLevelInfo->at0);
    MenuSetupEpisodeInfo();
}

void levelGetNextLevels(int nEpisode, int nLevel, int *pnEndingA, int *pnEndingB)
{
    dassert(pnEndingA != NULL && pnEndingB != NULL);
    LEVELINFO *pLevelInfo = &gEpisodeInfo[nEpisode].at28[nLevel];
    int nEndingA = pLevelInfo->ate4;
    if (nEndingA >= 0)
        nEndingA--;
    int nEndingB = pLevelInfo->ate8;
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
            if (pEpisodeInfo->at8f98[0])
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
                if (pEpisodeInfo->at8f98[0])
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

// By NoOne: this function can be called via sending numbered command to TX kGDXChannelEndLevel
// This allows to set custom next level instead of taking it from INI file.
void levelEndLevelCustom(int nLevel) {

    gGameOptions.uGameFlags |= 1;

    if (nLevel >= 16  || nLevel < 0)
    {

        gGameOptions.uGameFlags |= 2;
        gGameOptions.nLevel = 0;
        return;
    }

    gNextLevel = nLevel;
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
    char buffer[BMAX_PATH];
    if (mus_redbook && gEpisodeInfo[nEpisode].at28[nLevel].ate0 > 0)
        snprintf(buffer, BMAX_PATH, "blood%02i.ogg", gEpisodeInfo[nEpisode].at28[nLevel].ate0);
    else
        strncpy(buffer, gEpisodeInfo[nEpisode].at28[nLevel].atd0, BMAX_PATH);
    bool bReturn = !!sndPlaySong(gEpisodeInfo[nEpisode].at28[nLevel].atd0, buffer, true);
    if (bReturn || bSetLevelSong)
        strncpy(gGameOptions.zLevelSong, buffer, BMAX_PATH);
	else *gGameOptions.zLevelSong = 0;
    return bReturn;
}

void levelTryPlayMusicOrNothing(int nEpisode, int nLevel)
{
    if (levelTryPlayMusic(nEpisode, nLevel, true))
        sndStopSong();
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
    Read(&gGameStarted, sizeof(gGameStarted));
}

void LevelsLoadSave::Save(void)
{
    Write(&gNextLevel, sizeof(gNextLevel));
    Write(&gGameOptions, sizeof(gGameOptions));
    Write(&gGameStarted, sizeof(gGameStarted));
}

void LevelsLoadSaveConstruct(void)
{
    myLoadSave = new LevelsLoadSave();
}

END_BLD_NS
