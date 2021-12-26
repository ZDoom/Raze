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

static UsermapDirectory root;

void InsertMap(int lumpnum)
{
	FString filename = fileSystem.GetFileFullName(lumpnum);
	auto path = filename.Split("/");

	auto current = &root;
	for (unsigned i = 0; i < path.Size() - 1; i++)
	{
		unsigned place = current->subdirectories.FindEx([=](const UsermapDirectory& entry) { return entry.name.CompareNoCase(path[i]) == 0; });
		if (place == current->subdirectories.Size())
		{
			place = current->subdirectories.Reserve(1);
			current->subdirectories.Last().name = path[i];
		}
		current = &current->subdirectories[place];
	}
	current->entries.Reserve(1);
	current->entries.Last().displayname = path.Last();
	current->entries.Last().filename = fileSystem.GetFileFullName(lumpnum);
	current->entries.Last().container = fileSystem.GetResourceFileName(fileSystem.GetFileContainer(lumpnum));
	current->entries.Last().size = fileSystem.FileLength(lumpnum);
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

void SortEntries(UsermapDirectory& dir)
{
	qsort(dir.subdirectories.Data(), dir.subdirectories.Size(), sizeof(dir.subdirectories[0]), [](const void* a, const void* b) -> int
		{
			auto A = (UsermapDirectory*)a;
			auto B = (UsermapDirectory*)b;

			return A->name.CompareNoCase(B->name);
		});

	qsort(dir.entries.Data(), dir.entries.Size(), sizeof(dir.entries[0]), [](const void* a, const void* b) -> int
		{
			auto A = (UsermapEntry*)a;
			auto B = (UsermapEntry*)b;

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
	int numfiles = fileSystem.GetNumEntries();

	for (int i = 0; i < numfiles; i++)
	{
		auto fn1 = fileSystem.GetFileFullName(i);
		if (!fn1 || !*fn1) continue;
		auto map = strstr(fn1, ".map");
		if (!map || strcmp(map, ".map")) continue;
		if (!ValidateMap(i)) continue;
		InsertMap(i);
	}
	SortEntries(root);
}

CCMD(readusermaps)
{
	ReadUserMaps();
}