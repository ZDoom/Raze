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

#include "blood.h"
#include "inifile.h"
#include "razemenu.h"

BEGIN_BLD_NS

GAMEOPTIONS gGameOptions;

GAMEOPTIONS gSingleGameOptions = {
    0, 2, 0, 0, 0, 0, 0, 0, 2, 3600, 1800, 1800, 7200
};

EPISODEINFO gEpisodeInfo[kMaxEpisodes+1];

int gSkill = 2;
int gEpisodeCount;
int gNextLevel; // fixme: let this contain a full level number.

char BloodIniFile[BMAX_PATH] = "BLOOD.INI";
bool bINIOverride = false;
IniFile *BloodINI;


void levelInitINI(const char *pzIni)
{
	if (!fileSystem.FileExists(pzIni))
        I_Error("Initialization: %s does not exist", pzIni);
    BloodINI = new IniFile(pzIni);
    strncpy(BloodIniFile, pzIni, BMAX_PATH);
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
        I_Error("Section [%s] expected in BLOOD.INI", pzSection);
}

void CheckKeyAbend(const char *pzSection, const char *pzKey)
{
    assert(pzSection != NULL);

    if (!pzKey || !BloodINI->KeyExists(pzSection, pzKey))
        I_Error("Key %s expected in section [%s] of BLOOD.INI", pzKey, pzSection);
}


void levelLoadMapInfo(IniFile *pIni, MapRecord *pLevelInfo, const char *pzSection, int epinum, int mapnum)
{
    char buffer[16];
    pLevelInfo->SetName(pIni->GetKeyString(pzSection, "Title", pLevelInfo->labelName));
    pLevelInfo->author = pIni->GetKeyString(pzSection, "Author", "");
    pLevelInfo->music = pIni->GetKeyString(pzSection, "Song", ""); DefaultExtension(pLevelInfo->music, ".mid");
    pLevelInfo->cdSongId = pIni->GetKeyInt(pzSection, "Track", -1);
    pLevelInfo->nextLevel = pIni->GetKeyInt(pzSection, "EndingA", -1);
    pLevelInfo->nextSecret = pIni->GetKeyInt(pzSection, "EndingB", -1);
    pLevelInfo->fog = pIni->GetKeyInt(pzSection, "Fog", -0);
    pLevelInfo->weather = pIni->GetKeyInt(pzSection, "Weather", -0);
    for (int i = 0; i < kMaxMessages; i++)
    {
        sprintf(buffer, "Message%d", i+1);
		auto msg = pIni->GetKeyString(pzSection, buffer, "");
		pLevelInfo->AddMessage(i, msg);
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
        int j;
        for (j = 0; j < kMaxLevels; j++)
        {
            sprintf(buffer2, "Map%d", j+1);
            if (!BloodINI->KeyExists(buffer, buffer2))
                break;
            auto pLevelInfo = AllocateMap();
            const char *pMap = BloodINI->GetKeyString(buffer, buffer2, NULL);
            CheckSectionAbend(pMap);
			pLevelInfo->levelNumber = levelnum(i, j);
            pLevelInfo->labelName = pMap;
            pLevelInfo->fileName.Format("%s.map", pMap);
            levelLoadMapInfo(BloodINI, pLevelInfo, pMap, i, j);
        }
        pEpisodeInfo->nLevels = j;
    }
    gEpisodeCount = i;
}

void levelGetNextLevels(int *pnEndingA, int *pnEndingB)
{
    assert(pnEndingA != NULL && pnEndingB != NULL);
    int nEndingA = currentLevel->nextLevel;
    if (nEndingA >= 0)
        nEndingA--;
    int nEndingB = currentLevel->nextSecret;
    if (nEndingB >= 0)
        nEndingB--;
    *pnEndingA = nEndingA;
    *pnEndingB = nEndingB;
}

void levelEndLevel(int arg)
{
    int nEndingA, nEndingB;
    auto episode = volfromlevelnum(currentLevel->levelNumber);
    EPISODEINFO *pEpisodeInfo = &gEpisodeInfo[episode];
    gGameOptions.uGameFlags |= GF_AdvanceLevel;
    levelGetNextLevels(&nEndingA, &nEndingB);
    switch (arg)
    {
    case 0:
        if (nEndingA == -1)
        {
            if (pEpisodeInfo->cutsceneBName[0])
                gGameOptions.uGameFlags |= GF_PlayCutscene;
            gGameOptions.uGameFlags |= GF_EndGame;
        }
        else
            gNextLevel = nEndingA;
        break;
    case 1:
        if (nEndingB == -1)
        {
            if (episode + 1 < gEpisodeCount)
            {
                if (pEpisodeInfo->cutsceneBName[0])
                    gGameOptions.uGameFlags |= GF_PlayCutscene;
                gGameOptions.uGameFlags |= GF_EndGame;
            }
            else
            {
                gGameOptions.uGameFlags |= GF_AdvanceLevel;
            }
        }
        else
            gNextLevel = nEndingB;
        break;
    }
}

void levelTryPlayMusic()
{
    FString buffer;
    if (mus_redbook && currentLevel->cdSongId > 0)
        buffer.Format("blood%02i.ogg", currentLevel->cdSongId);
    else
    {
        buffer = currentLevel->music;
		if (Mus_Play(currentLevel->labelName, buffer, true)) return;
        DefaultExtension(buffer, ".mid");
    }
    if (!Mus_Play(currentLevel->labelName, buffer, true))
    {
        Mus_Play("", "", true);
    }
}


END_BLD_NS
