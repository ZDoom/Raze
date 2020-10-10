/*
** mapinfo.cpp
**
** Map record management
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/ 

#include "mapinfo.h"
#include "raze_music.h"
#include "filesystem.h"
#include "printf.h"

FString gSkillNames[MAXSKILLS];
FString gVolumeNames[MAXVOLUMES];
FString gVolumeSubtitles[MAXVOLUMES];
int32_t gVolumeFlags[MAXVOLUMES];
int gDefaultVolume = 0, gDefaultSkill = 1;

MapRecord mapList[512];		// Due to how this gets used it needs to be static. EDuke defines 7 episode plus one spare episode with 64 potential levels each and relies on the static array which is freely accessible by scripts.
MapRecord *currentLevel;	// level that is currently played. (The real level, not what script hacks modfifying the current level index can pretend.)
MapRecord* lastLevel;		// Same here, for the last level.
unsigned int numUsedSlots;


MapRecord *FindMapByName(const char *nm)
{
	for (unsigned i = 0; i < numUsedSlots; i++)
	{
		auto &map = mapList[i];
		if (map.labelName.CompareNoCase(nm) == 0)
		{
			return &map;
		}
	}
	return nullptr;
}


MapRecord *FindMapByLevelNum(int num)
{
	for (unsigned i = 0; i < numUsedSlots; i++)
	{
		auto &map = mapList[i];
		if (map.levelNumber == num)
		{
			return &map;
		}
	}
	return nullptr;
}

MapRecord *FindNextMap(MapRecord *thismap)
{
	if (thismap->nextLevel != -1) return &mapList[thismap->nextLevel];
	return FindMapByLevelNum(thismap->levelNumber+1);
}

bool SetMusicForMap(const char* mapname, const char* music, bool namehack)
{
	static const char* specials[] = { "intro", "briefing", "loading" };
	for (int i = 0; i < 3; i++)
	{
		if (!stricmp(mapname, specials[i]))
		{
			// todo: store this properly.
			return true;
		}
	}

	auto index = FindMapByName(mapname);

	// This is for the DEFS parser's MUSIC command which never bothered to check for the real map name.
	if (index == nullptr && namehack)
	{
		int lev, ep;
		signed char b1, b2;

		int numMatches = sscanf(mapname, "%c%d%c%d", &b1, &ep, &b2, &lev);

		if (numMatches != 4 || toupper(b1) != 'E' || toupper(b2) != 'L')
			return false;

		index = FindMapByLevelNum(ep*100 + lev);

	}
	if (index != nullptr)
	{
		index->music = music;
		return true;
	}
	return false;
}

MapRecord *AllocateMap()
{
	return &mapList[numUsedSlots++];
}


MapRecord* SetupUserMap(const char* boardfilename, const char *defaultmusic)
{
	for (unsigned i = 0; i < numUsedSlots; i++)
	{
		auto &map = mapList[i];
		if (map.fileName.CompareNoCase(boardfilename) == 0)
		{
			return &map;
		}
	}

	if (!fileSystem.FileExists(boardfilename))
	{
		Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", boardfilename);
		return nullptr;
	}

	auto map = AllocateMap();
	map->name = "";
	map->SetFileName(boardfilename);
	map->flags = MI_USERMAP|MI_FORCEEOG;
	map->music = G_SetupFilenameBasedMusic(boardfilename, defaultmusic);
	return map;
}
