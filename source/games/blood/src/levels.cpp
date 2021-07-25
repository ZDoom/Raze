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

int gSkill = 2;
MapRecord* gNextLevel;

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



void levelLoadMapInfo(IniFile* pIni, MapRecord* pLevelInfo, const char* pzSection, int epinum, int mapnum, int* nextmap, int* nextsecret)
{
    char buffer[16];
    pLevelInfo->SetName(pIni->GetKeyString(pzSection, "Title", pLevelInfo->labelName));
    pLevelInfo->Author = pIni->GetKeyString(pzSection, "Author", "");
    pLevelInfo->music = pIni->GetKeyString(pzSection, "Song", ""); if (pLevelInfo->music.IsNotEmpty()) DefaultExtension(pLevelInfo->music, ".mid");
    pLevelInfo->cdSongId = pIni->GetKeyInt(pzSection, "Track", -1);
    *nextmap = pIni->GetKeyInt(pzSection, "EndingA", 0);
    *nextsecret = pIni->GetKeyInt(pzSection, "EndingB", 0);
    pLevelInfo->fog = pIni->GetKeyInt(pzSection, "Fog", -0);
    pLevelInfo->weather = pIni->GetKeyInt(pzSection, "Weather", -0);
    for (int i = 0; i < kMaxMessages; i++)
    {
        sprintf(buffer, "Message%d", i + 1);
        auto msg = pIni->GetKeyString(pzSection, buffer, "");
        pLevelInfo->AddMessage(i, msg);
    }
}

static const char* DefFile(void)
{
    int found = -1;
    if (userConfig.DefaultCon.IsEmpty() || userConfig.DefaultCon.CompareNoCase("blood.ini") == 0)
    {
        int numlumps = fileSystem.GetNumEntries();
        for (int i = numlumps - 1; i >= 0; i--)
        {
            int fileno = fileSystem.GetFileContainer(i);
            if (fileno != -1 && fileno <= fileSystem.GetMaxIwadNum()) break;
            FString fn = fileSystem.GetFileFullName(i, false);
            FString ext = fn.Right(4);
            if (ext.CompareNoCase(".ini") == 0)
            {
                if (fileSystem.CheckNumForFullName(fn) != i) continue;
                if (found == -1)
                {
                    IniFile inif(fn);
                    for (int j = 1; j <= 6; j++)
                    {
                        FStringf key("Episode%d", j);
                        if (inif.SectionExists(key))
                        {
                            found = i;
                            break;
                        }
                    }
                }
                else
                {
                    found = -1;
                    break;
                }
            }
        }
    }
    if (found >= 0) return fileSystem.GetFileFullName(found);
    // The command line parser stores this in the CON field.
    return userConfig.DefaultCon.IsNotEmpty() ? userConfig.DefaultCon.GetChars() : "blood.ini";
}

static FString cleanPath(const char* pth)
{
    FString path = pth;
    FixPathSeperator(path);
    if (fileSystem.FileExists(path)) return path;
    if (path.Len() > 3 && path[1] == ':' && isalpha(path[0]) && path[2] == '/')
    {
        path = path.Mid(3);
        if (fileSystem.FileExists(path)) return path;
    }
    // optionally strip the first path component to account for poor logic of the DOS EXE.
    auto pos = path.IndexOf("/");
    if (pos >= 0)
    {
        auto npath = path.Mid(pos + 1);
        if (fileSystem.FileExists(npath)) return npath;
    }
    return path;
}

void levelLoadDefaults(void)
{
    char buffer[64];
    char buffer2[16];

    int cutALevel = 0;

    levelInitINI(DefFile());
    int i;
    for (i = 1; i <= kMaxEpisodes; i++)
    {
        sprintf(buffer, "Episode%d", i);
        if (!BloodINI->SectionExists(buffer))
            break;
        auto cluster = MustFindCluster(i);
        auto volume = MustFindVolume(i);
        CutsceneDef &csB = cluster->outro;
        auto ep_str = BloodINI->GetKeyString(buffer, "Title", buffer);
		cluster->name = volume->name = ep_str;
        if (i > 1) volume->flags |= VF_SHAREWARELOCK;
        if (BloodINI->GetKeyInt(buffer, "BloodBathOnly", 0)) volume->flags |= VF_HIDEFROMSP;

        csB.video = cleanPath(BloodINI->GetKeyString(buffer, "CutSceneB", ""));
        int soundint = BloodINI->GetKeyInt(buffer, "CutWavB", -1);
        if (soundint > 0) csB.soundID = soundint + 0x40000000;
        else csB.soundName = cleanPath(BloodINI->GetKeyString(buffer, "CutWavB", ""));

        //pEpisodeInfo->bloodbath = BloodINI->GetKeyInt(buffer, "BloodBathOnly", 0);
        cutALevel = BloodINI->GetKeyInt(buffer, "CutSceneALevel", 0);
        if (cutALevel < 1) cutALevel = 1;

        int nextmaps[kMaxLevels]{}, nextsecrets[kMaxLevels]{};
        for (int j = 1; j <= kMaxLevels; j++)
        {
            sprintf(buffer2, "Map%d", j);
            if (!BloodINI->KeyExists(buffer, buffer2))
                break;
            auto pLevelInfo = AllocateMap();
            const char *pMap = BloodINI->GetKeyString(buffer, buffer2, NULL);
            CheckSectionAbend(pMap);
			SetLevelNum(pLevelInfo, makelevelnum(i, j));
            pLevelInfo->cluster = i;
            pLevelInfo->labelName = pMap;
			if (j == 1) volume->startmap = pLevelInfo->labelName;
            pLevelInfo->fileName.Format("%s.map", pMap);
            levelLoadMapInfo(BloodINI, pLevelInfo, pMap, i, j, &nextmaps[j - 1], &nextsecrets[j - 1]);
            if (j == cutALevel)
            {
                CutsceneDef& csA = pLevelInfo->intro;
                csA.video = cleanPath(BloodINI->GetKeyString(buffer, "CutSceneA", ""));
                int soundint = BloodINI->GetKeyInt(buffer, "CutWavA", -1);
                if (soundint > 0) csA.soundID = soundint + 0x40000000;
                else csA.soundName = cleanPath(BloodINI->GetKeyString(buffer, "CutWavA", ""));
            }
        }
        // Now resolve the level links
        for (int j = 1; j <= kMaxLevels; j++)
        {
            auto map = FindMapByIndexOnly(i, j);
            if (map)
            {
                if (nextmaps[j - 1] > 0)
                {
                    auto nmap = FindMapByIndexOnly(i, nextmaps[j - 1]);
                    if (nmap) map->NextMap = nmap->labelName;
                    else map->NextMap = "-";
                }
                else map->NextMap = "-";
                if (nextsecrets[j - 1] > 0)
                {
                    auto nmap = FindMapByIndexOnly(i, nextsecrets[j - 1]);
                    if (nmap) map->NextSecret = nmap->labelName;
                    else map->NextSecret = "-";
                }
                else map->NextSecret = "-";
            }
        }
    }
}

void levelEndLevel(int secret)
{
    gGameOptions.uGameFlags |= GF_AdvanceLevel;
    if (!secret) gNextLevel = FindNextMap(currentLevel);
    else gNextLevel = FindNextSecretMap(currentLevel);
}

void levelTryPlayMusic()
{
    FString buffer;
    if (mus_redbook && currentLevel->cdSongId > 0)
        buffer.Format("blood%02i.ogg", currentLevel->cdSongId);
    else
    {
        buffer = currentLevel->music;
		if (Mus_Play(buffer, true)) return;
        if (buffer.IsNotEmpty()) DefaultExtension(buffer, ".mid");
    }
    if (!Mus_Play(buffer, true))
    {
        Mus_Play("", true);
    }
}


END_BLD_NS
