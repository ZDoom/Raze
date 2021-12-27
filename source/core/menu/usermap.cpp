/*
** usermap.cpp
** Management for the user maps menu
**
**---------------------------------------------------------------------------
** Copyright 2020-2021 Christoph Oelckers
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

#include "razemenu.h"
#include "version.h"
#include "m_png.h"
#include "filesystem.h"
#include "v_text.h"
#include "gstrings.h"
#include "serializer.h"
#include "vm.h"
#include "i_system.h"
#include "v_video.h"
#include "findfile.h"
#include "v_draw.h"
#include "usermap.h"
#include "gamecontrol.h"
#include "mapinfo.h"
#include "v_draw.h"

static FUsermapDirectory root;

void InsertMap(int lumpnum)
{
	FString filename = fileSystem.GetFileFullName(lumpnum);
	auto path = filename.Split("/");

	auto current = &root;
	for (unsigned i = 0; i < path.Size() - 1; i++)
	{
		unsigned place = current->subdirectories.FindEx([=](const FUsermapDirectory& entry) { return entry.dirname.CompareNoCase(path[i]) == 0; });
		if (place == current->subdirectories.Size())
		{
			place = current->subdirectories.Reserve(1);
			current->subdirectories.Last().dirname = path[i];
		}
		current = &current->subdirectories[place];
	}
	current->entries.Reserve(1);
	current->entries.Last().displayname = path.Last();
	current->entries.Last().filename = fileSystem.GetFileFullName(lumpnum);
	current->entries.Last().container = fileSystem.GetResourceFileName(fileSystem.GetFileContainer(lumpnum));
	current->entries.Last().size = fileSystem.FileLength(lumpnum);
	auto mapinfo = FindMapByName(StripExtension(path.Last()));
	if (mapinfo) current->entries.Last().info = mapinfo->name;
}

bool ValidateMap(int lumpnum)
{
	FString filename = fileSystem.GetFileFullName(lumpnum);

	if (fileSystem.FindFile(filename) != lumpnum) return false;
	auto fr = fileSystem.OpenFileReader(lumpnum);
	uint8_t check[4];
	fr.Read(&check, 4);
	if (!isBlood())
	{
		if (check[0] < 5 || check[0] > 9 || check[1] || check[2] || check[3]) return false;
	}
	else
	{
		if (memcmp(check, "BLM\x1a", 4)) return false;
	}
	return true;
}

void SortEntries(FUsermapDirectory& dir)
{
	qsort(dir.subdirectories.Data(), dir.subdirectories.Size(), sizeof(dir.subdirectories[0]), [](const void* a, const void* b) -> int
		{
			auto A = (FUsermapDirectory*)a;
			auto B = (FUsermapDirectory*)b;

			return A->dirname.CompareNoCase(B->dirname);
		});

	qsort(dir.entries.Data(), dir.entries.Size(), sizeof(dir.entries[0]), [](const void* a, const void* b) -> int
		{
			auto A = (FUsermapEntry*)a;
			auto B = (FUsermapEntry*)b;

			return A->displayname.CompareNoCase(B->displayname);
		});

	for (auto& subdir : dir.subdirectories)
	{
		subdir.parent = &dir;
		SortEntries(subdir);
	}
}

void ReadUserMaps()
{
	static bool didit = false;
	if (didit) return;
	didit = true;

	int numfiles = fileSystem.GetNumEntries();

	for (int i = 0; i < numfiles; i++)
	{
		auto fn1 = fileSystem.GetFileFullName(i);
		if (!fn1 || !*fn1) continue;
		FString lowfn = fn1;
		if (lowfn.Right(4).CompareNoCase(".map")) continue;
		if (!ValidateMap(i)) continue;
		InsertMap(i);
	}
	SortEntries(root);
}

void LoadMapPreview(FUsermapEntry* entry)
{
	if (entry->wallsread) return;
	entry->walls = loadMapWalls(entry->filename);
}

void UnloadMapPreviews(FUsermapDirectory* dir)
{
	for (auto& entry : dir->entries)
	{
		if (entry.walls.Size() > 0) entry.wallsread = false;
		entry.walls.Reset();
	}
	for (auto& sub : dir->subdirectories)
	{
		UnloadMapPreviews(&sub);
	}
}

DEFINE_FIELD(FUsermapEntry, filename);
DEFINE_FIELD(FUsermapEntry, container);
DEFINE_FIELD(FUsermapEntry, displayname);
DEFINE_FIELD(FUsermapEntry, info);
DEFINE_FIELD(FUsermapEntry, size);

DEFINE_FIELD(FUsermapDirectory, dirname);
DEFINE_FIELD(FUsermapDirectory, parent);


DEFINE_ACTION_FUNCTION(FUsermapDirectory, ReadData)
{
	ReadUserMaps();
	ACTION_RETURN_POINTER(&root);
}

DEFINE_ACTION_FUNCTION(FUsermapDirectory, GetNumEntries)
{
	PARAM_SELF_STRUCT_PROLOGUE(FUsermapDirectory);
	ACTION_RETURN_INT(self->entries.Size());
}

DEFINE_ACTION_FUNCTION(FUsermapDirectory, GetNumDirectories)
{
	PARAM_SELF_STRUCT_PROLOGUE(FUsermapDirectory);
	ACTION_RETURN_INT(self->subdirectories.Size());
}

DEFINE_ACTION_FUNCTION(FUsermapDirectory, GetEntry)
{
	PARAM_SELF_STRUCT_PROLOGUE(FUsermapDirectory);

	PARAM_UINT(num);
	if (num >= self->entries.Size()) ACTION_RETURN_POINTER(nullptr);
	ACTION_RETURN_POINTER(&self->entries[num]);
}

DEFINE_ACTION_FUNCTION(FUsermapDirectory, GetDirectory)
{
	PARAM_SELF_STRUCT_PROLOGUE(FUsermapDirectory);

	PARAM_UINT(num);
	if (num >= self->subdirectories.Size()) ACTION_RETURN_POINTER(nullptr);
	ACTION_RETURN_POINTER(&self->subdirectories[num]);
}

DEFINE_ACTION_FUNCTION(_UserMapMenu, DrawPreview)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(entry, FUsermapEntry);
	PARAM_INT(left);
	PARAM_INT(top);
	PARAM_INT(width);
	PARAM_INT(height);
	if (!entry) return 0;
	LoadMapPreview(entry);
	if (entry->walls.Size() == 0) return 0;
	int minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
	for (auto& wal : entry->walls)
	{
		if (wal.pos.X < minx) minx = wal.pos.X;
		if (wal.pos.X > maxx) maxx = wal.pos.X;
		if (wal.pos.Y < miny) miny = wal.pos.Y;
		if (wal.pos.Y > maxy) maxy = wal.pos.Y;
	}
	float scalex = float(width) / (maxx - minx);
	float scaley = float(height) / (maxy - miny);
	int centerx = (minx + maxx) >> 1;
	int centery = (miny + maxy) >> 1;
	int dcenterx = left + (width >> 1);
	int dcentery = top + (height >> 1);
	float scale = min(scalex, scaley);
	float drawleft = dcenterx - (centerx - minx) * scale;
	float drawtop = dcentery - (centery - miny) * scale;

	for (auto& wal : entry->walls)
	{
		if (wal.nextwall < 0) continue;
		auto point2 = &entry->walls[wal.point2];
		twod->AddLine(dcenterx + (wal.pos.X - centerx) * scale, dcentery + (wal.pos.Y - centery) * scale,
			dcenterx + (point2->pos.X - centerx) * scale, dcentery + (point2->pos.Y - centery) * scale,
			-1, -1, INT_MAX, INT_MAX, 0xff808080);
	}
	for (auto& wal : entry->walls)
	{
		if (wal.nextwall >= 0) continue;
		auto point2 = &entry->walls[wal.point2];
		twod->AddLine(dcenterx + (wal.pos.X - centerx) * scale, dcentery + (wal.pos.Y - centery) * scale,
			dcenterx + (point2->pos.X - centerx) * scale, dcentery + (point2->pos.Y - centery) * scale,
			-1, -1, INT_MAX, INT_MAX, 0xffffffff);
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(_UsermapMenu, StartMap)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(entry, FUsermapEntry);

	if (DMenu::InMenu == 0)
	{
		ThrowAbortException(X_OTHER, "Attempt to start user map outside of menu code");
	}
	NewGameStartupInfo.Episode = -1;
	NewGameStartupInfo.Level = -1;
	NewGameStartupInfo.Map = SetupUserMap(entry->filename, g_gameType & GAMEFLAG_DUKE ? "dethtoll.mid" : nullptr);
	M_SetMenu(NAME_Skillmenu, INT_MAX);
	return 0;
}
