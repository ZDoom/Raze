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

#include "c_dispatch.h"
#include "mapinfo.h"
#include "raze_music.h"
#include "filesystem.h"
#include "printf.h"
#include "raze_sound.h"

FString gSkillNames[MAXSKILLS];
int gDefaultVolume = 0, gDefaultSkill = 1;

GlobalCutscenes globalCutscenes;
TArray<ClusterDef> clusters;
TArray<VolumeRecord> volumes;
TArray<TPointer<MapRecord>> mapList;	// must be allocated as pointers because it can whack the currentlLevel pointer if this was a flat array.
MapRecord *currentLevel;	// level that is currently played.
MapRecord* lastLevel;		// Same here, for the last level.


CCMD(listmaps)
{
	for (auto& map : mapList)
	{
		int lump = fileSystem.FindFile(map->fileName);
		if (lump >= 0)
		{
			int rfnum = fileSystem.GetFileContainer(lump);
			Printf("%s - %s (%s)\n", map->LabelName(), map->DisplayName(), fileSystem.GetResourceFileName(rfnum));
		}
		else
		{
			Printf("%s - %s (defined but does not exist)\n", map->fileName.GetChars(), map->DisplayName());
		}
	}
}

CCMD(mapinfo)
{
	const char* mapname = nullptr;
	if (argv.argc() > 1) mapname = argv[1];

	if (!mapname)
	{
		for (auto& vol : volumes)
		{
			Printf("Volume %d\n\tName = '%s'\n\tstartmap = '%s'\n}\n", vol.index, vol.name.GetChars(), vol.startmap.GetChars());
		}
		for (auto& clus : clusters)
		{
			if (clus.intro.isdefined() || clus.outro.isdefined())
			{
				Printf("Cluster %d\n\tName = '%s'\n", clus.index, clus.name.GetChars());
				if (clus.intro.function.IsNotEmpty()) Printf("\tIntro function = %s\n", clus.intro.function.GetChars());
				if (clus.intro.video.IsNotEmpty()) Printf("\tIntro video = %s\n", clus.intro.video.GetChars());
				if (clus.outro.function.IsNotEmpty()) Printf("\tOutro function = %s\n", clus.outro.function.GetChars());
				if (clus.outro.video.IsNotEmpty()) Printf("\tOutro video = %s\n", clus.outro.video.GetChars());
				Printf("}\n");
			}
		}
	}
	for (auto& map : mapList)
	{
		if (mapname && map->labelName.CompareNoCase(mapname)) continue;
		int lump = fileSystem.FindFile(map->fileName);
		if (lump >= 0)
		{
			int rfnum = fileSystem.GetFileContainer(lump);
			Printf("%s - %s (%s)\n{\n", map->fileName.GetChars(), map->DisplayName(), fileSystem.GetResourceFileName(rfnum));
			Printf("\tlevel number = %d\n\tCluster = %d\n\tIndex = %d\n", map->levelNumber, map->cluster, map->mapindex);
			if (map->Author.IsNotEmpty()) Printf("\tAuthor = '%s'\n", map->Author.GetChars());
			if (map->NextMap.IsNotEmpty()) Printf("\tNext map = '%s'\n", map->NextMap.GetChars());
			if (map->NextSecret.IsNotEmpty()) Printf("\tNext secret map = '%s'\n", map->NextSecret.GetChars());
			if (map->music.IsNotEmpty()) Printf("\tMusic = '%s:%d'", map->music.GetChars(), map->musicorder);
			if (map->cdSongId > 0) Printf("\tCD track = %d\n", map->cdSongId);
			if (map->parTime) Printf("\tPar Time = %d\n", map->parTime);
			if (map->designerTime) Printf("\tPar Time = %d\n", map->designerTime);
			if (map->intro.function.IsNotEmpty()) Printf("\tIntro function = %s\n", map->intro.function.GetChars());
			if (map->intro.video.IsNotEmpty()) Printf("\tIntro video = %s\n", map->intro.video.GetChars());
			if (map->outro.function.IsNotEmpty()) Printf("\tOutro function = %s\n", map->outro.function.GetChars());
			if (map->outro.video.IsNotEmpty()) Printf("\tOutro video = %s\n", map->outro.video.GetChars());
			Printf("}\n");
		}
		else
		{
			Printf("%s - %s (defined but does not exist)\n", map->fileName.GetChars(), map->DisplayName());
		}
	}
}

MapRecord *FindMapByName(const char *nm)
{
	for (auto& map : mapList)
	{
		if (map->labelName.CompareNoCase(nm) == 0)
		{
			return map.Data();
		}
	}
	return nullptr;
}


MapRecord *FindMapByLevelNum(int num)
{
	for (auto& map : mapList)
	{
		if (map->levelNumber == num)
		{
			return map.Data();
		}
	}
	return nullptr;
}

VolumeRecord* FindVolume(int index)
{
	for (auto& vol : volumes)
	{
		if (vol.index == index) return &vol;
	}
	return nullptr;
}

ClusterDef* FindCluster(int index)
{
	for (auto& vol : clusters)
	{
		if (vol.index == index) return &vol;
	}
	return nullptr;
}

ClusterDef* AllocateCluster()
{
	return &clusters[clusters.Reserve(1)];
}

VolumeRecord* AllocateVolume()
{
	return &volumes[volumes.Reserve(1)];
}

MapRecord* FindMapByIndexOnly(int cluster, int num)
{
	for (auto& map : mapList)
	{
		if (map->mapindex == num && map->cluster == cluster) return map.Data();
	}
	return nullptr;
}

MapRecord* FindMapByIndex(int cluster, int num)
{
	auto map = FindMapByLevelNum(num);
	if (!map) map = FindMapByIndexOnly(cluster, num); // modern definitions take precedence.
	return map;
}

MapRecord* FindNextMap(MapRecord* thismap)
{
	MapRecord* next = nullptr;
	if (!thismap->NextMap.Compare("-")) return nullptr;	// '-' means to forcibly end the game here.
	if (thismap->NextMap.IsNotEmpty()) next = FindMapByName(thismap->NextMap);
	if (!next) next = FindMapByLevelNum(thismap->levelNumber + 1);
	return next;
}

MapRecord* FindNextSecretMap(MapRecord* thismap)
{
	MapRecord* next = nullptr;
	if (!thismap->NextSecret.Compare("-")) return nullptr;	// '-' means to forcibly end the game here.
	if (thismap->NextSecret.IsNotEmpty()) next = FindMapByName(thismap->NextSecret);
	return next? next : FindNextMap(thismap);
}


bool SetMusicForMap(const char* mapname, const char* music, bool namehack)
{
	static const char* specials[] = { "intro", "briefing", "loading" };
	for (int i = 0; i < 3; i++)
	{
		if (!stricmp(mapname, specials[i]))
		{
			if (specialmusic.Size() <= i) specialmusic.Resize(i + 1);
			specialmusic[i] = music;
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

		index = FindMapByIndexOnly(ep, lev);

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
	auto&p = mapList[mapList.Reserve(1)];
	p.Alloc();
	return p.Data();
}


MapRecord* SetupUserMap(const char* boardfilename, const char *defaultmusic)
{
	for (auto& map : mapList)
	{
		if (map->fileName.CompareNoCase(boardfilename) == 0)
		{
			return map.Data();
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
