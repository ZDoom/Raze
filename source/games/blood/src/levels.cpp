/*
 * Copyright (C) 2018, 2022 nukeykt
 * Copyright (C) 2020-2022 Christoph Oelckers
 *
 * This file is part of Raze
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include "ns.h"	// Must come before everything else!

#include <stdlib.h>
#include <string.h>

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

enum
{
	kMaxMessages = 32,
	kMaxEpisodes = 7,
	kMaxLevels = 16
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static IniFile* levelInitINI(const char* pzIni)
{
	if (!fileSystem.FileExists(pzIni))
		I_Error("%s: Ini file not found", pzIni);
	return new IniFile(pzIni);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void levelLoadMapInfo(IniFile* pIni, MapRecord* pLevelInfo, const char* pzSection, int epinum, int mapnum, int* nextmap, int* nextsecret)
{
	char buffer[16];
	pLevelInfo->SetName(pIni->GetKeyString(pzSection, "Title", pLevelInfo->labelName.GetChars()));
	pLevelInfo->Author = pIni->GetKeyString(pzSection, "Author", "");
	pLevelInfo->music = pIni->GetKeyString(pzSection, "Song", "");
	if (pLevelInfo->music.IsNotEmpty()) DefaultExtension(pLevelInfo->music, ".mid");
	pLevelInfo->cdSongId = pIni->GetKeyInt(pzSection, "Track", -1);
	*nextmap = pIni->GetKeyInt(pzSection, "EndingA", 0);
	*nextsecret = pIni->GetKeyInt(pzSection, "EndingB", 0);
	pLevelInfo->fog = pIni->GetKeyInt(pzSection, "Fog", -0);
	pLevelInfo->weather = pIni->GetKeyInt(pzSection, "Weather", -0);
	for (int i = 0; i < kMaxMessages; i++)
	{
		snprintf(buffer, 16, "Message%d", i + 1);
		auto msg = pIni->GetKeyString(pzSection, buffer, "");
		pLevelInfo->AddMessage(i, msg);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const char* DefFile(void)
{
	int found = -1;
	if (userConfig.DefaultCon.IsEmpty() || userConfig.DefaultCon.CompareNoCase("blood.ini") == 0)
	{
		int numlumps = fileSystem.GetFileCount();
		for (int i = numlumps - 1; i >= 0; i--)
		{
			int fileno = fileSystem.GetFileContainer(i);
			if (fileno != -1 && fileno <= fileSystem.GetMaxBaseNum()) continue;
			FString fn = fileSystem.GetFileName(i);
			FString ext = fn.Right(4);
			if (ext.CompareNoCase(".ini") == 0)
			{
				if (fileSystem.FindFile(fn.GetChars()) != i) continue;
				if (found == -1)
				{
					IniFile inif(fn.GetChars());
					for (int j = 1; j <= 6; j++)
					{
						FStringf key("Episode%d", j);
						if (inif.SectionExists(key.GetChars()))
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
	if (found >= 0) return fileSystem.GetFileName(found);
	// The command line parser stores this in the CON field.
	return userConfig.DefaultCon.IsNotEmpty() ? userConfig.DefaultCon.GetChars() : "blood.ini";
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static FString cleanPath(const char* pth)
{
	FString path = pth;
	FixPathSeperator(path);
	if (fileSystem.FileExists(path.GetChars())) return path;
	if (path.Len() > 3 && path[1] == ':' && isalpha((uint8_t)path[0]) && path[2] == '/')
	{
		path = path.Mid(3);
		if (fileSystem.FileExists(path.GetChars())) return path;
	}
	// optionally strip the first path component to account for poor logic of the DOS EXE.
	auto pos = path.IndexOf("/");
	if (pos >= 0)
	{
		auto npath = path.Mid(pos + 1);
		if (fileSystem.FileExists(npath.GetChars())) return npath;
	}
	return path;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void levelLoadDefaults(void)
{
	char buffer[64];
	char buffer2[16];

	int cutALevel = 0;

	auto fn = DefFile();
	std::unique_ptr<IniFile> pIni(levelInitINI(fn));
	int i;
	for (i = 1; i <= kMaxEpisodes; i++)
	{
		snprintf(buffer, 64, "Episode%d", i);
		if (!pIni->SectionExists(buffer))
			break;

		auto cluster = MustFindCluster(i);
		auto volume = MustFindVolume(i);
		CutsceneDef& csB = cluster->outro;
		FString ep_str = pIni->GetKeyString(buffer, "Title", buffer);
		ep_str.StripRight();
		cluster->name = volume->name = FStringTable::MakeMacro(ep_str.GetChars());
		if (i > 1) volume->flags |= VF_SHAREWARELOCK;
		if (pIni->GetKeyInt(buffer, "BloodBathOnly", 0)) volume->flags |= VF_HIDEFROMSP;

		csB.video = cleanPath(pIni->GetKeyString(buffer, "CutSceneB", ""));
		int soundint = pIni->GetKeyInt(buffer, "CutWavB", -1);
		if (soundint > 0) csB.soundID = soundint + 0x40000000;
		else csB.soundName = cleanPath(pIni->GetKeyString(buffer, "CutWavB", ""));

		cutALevel = pIni->GetKeyInt(buffer, "CutSceneALevel", 0);
		if (cutALevel < 1) cutALevel = 1;

		int nextmaps[kMaxLevels]{}, nextsecrets[kMaxLevels]{};
		for (int j = 1; j <= kMaxLevels; j++)
		{
			snprintf(buffer2, 16, "Map%d", j);
			if (!pIni->KeyExists(buffer, buffer2))
				break;

			auto pLevelInfo = AllocateMap();
			const char* pMap = pIni->GetKeyString(buffer, buffer2, NULL);

			if (!pMap || !pIni->SectionExists(pMap))
				I_Error("Section [%s] expected in %s", buffer2, fn);

			SetLevelNum(pLevelInfo, makelevelnum(i, j));
			pLevelInfo->cluster = i;
			pLevelInfo->labelName = pMap;
			if (j == 1) volume->startmap = pLevelInfo->labelName;
			pLevelInfo->fileName.Format("%s.map", pMap);
			levelLoadMapInfo(pIni.get(), pLevelInfo, pMap, i, j, &nextmaps[j - 1], &nextsecrets[j - 1]);
			if (j == cutALevel)
			{
				CutsceneDef& csA = pLevelInfo->intro;
				csA.video = cleanPath(pIni->GetKeyString(buffer, "CutSceneA", ""));
				int soundfileint = pIni->GetKeyInt(buffer, "CutWavA", -1);
				if (soundfileint > 0) csA.soundID = soundfileint + 0x40000000;
				else csA.soundName = cleanPath(pIni->GetKeyString(buffer, "CutWavA", ""));
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void levelEndLevel(int secret)
{
	gGameOptions.uGameFlags |= GF_AdvanceLevel;
	if (!secret) gNextLevel = FindNextMap(currentLevel);
	else gNextLevel = FindNextSecretMap(currentLevel);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void levelTryPlayMusic()
{
	FString buffer;
	if (mus_redbook && currentLevel->cdSongId > 0)
		buffer.Format("blood%02i.ogg", currentLevel->cdSongId);
	else
	{
		buffer = currentLevel->music;
		if (Mus_Play(buffer.GetChars(), true)) return;
		if (buffer.IsNotEmpty()) DefaultExtension(buffer, ".mid");
	}
	if (!Mus_Play(buffer.GetChars(), true))
	{
		Mus_Play("", true);
	}
}


END_BLD_NS
