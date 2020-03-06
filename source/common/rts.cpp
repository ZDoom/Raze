/*
** rts.cpp
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
**
*/ 

#include <stdint.h>
#include "zstring.h"
#include "tarray.h"
#include "filesystem/filesystem.h"
#include "rts.h"
#include "m_swap.h"
#include "s_soundinternal.h"

const char* G_DefaultRtsFile(void);

struct WadInfo
{
    char identification[4];
    int32_t numlumps;
    int32_t infotableofs;
};

struct FileLump
{
    int32_t position;
    int32_t size;
    char name[8];
}; 

struct LumpInfoInternal
{
	int32_t position, size, sid;
};

//=============
// STATICS
//=============

static FString RTSName;
static TArray<uint8_t> RTSFile;
static TArray<LumpInfoInternal> LumpInfo;
static bool checked;


void RTS_Init(const char *filename)
{
	// Don't do anything before actually requesting some data from it.
	// In most cases this is never ever used, so just remember the file name for later.
	// This also simplifies initialization a lot because everything can just call this function and the last one wins.
	RTSName = filename;
}

bool RTS_IsInitialized()
{
	if (LumpInfo.Size() > 0) return true;
	if (RTSName.IsEmpty() && !checked)
	{
		RTSName = G_DefaultRtsFile();
		checked = true;
	}
	if (RTSName.IsEmpty()) return false;
	auto fr = fileSystem.OpenFileReader(RTSName, 0);
	RTSName = "";	// don't try ever again.
	if (!fr.isOpen()) return false;
	RTSFile = fr.Read();
	if (RTSFile.Size() >= 28)	// minimum size for one entry
	{
		WadInfo *wi = (WadInfo*)RTSFile.Data();
		if (!memcmp(wi->identification, "IWAD", 4))
		{
			auto numlumps = LittleLong(wi->numlumps);
			if (numlumps >= 1)
			{
				FileLump *li = (FileLump*)(RTSFile.Data() + LittleLong(wi->infotableofs));
				LumpInfo.Resize(numlumps);
				for(unsigned i = 0; i < numlumps; i++, li++)
				{
					LumpInfo[i] = { LittleLong(li->position), LittleLong(li->size), -1 };
					if (unsigned(LumpInfo[i].position + LumpInfo[i].size) >= RTSFile.Size())
					{
						LumpInfo[i].size = 0;	// points to invalid data
					}
				}
				return true;
			}
		}
	}
	RTSFile.Reset();
	LumpInfo.Reset();

	// For the benefit of the sound system we have to link the RTS content into the file system.
	// Unfortunately the file cannot be added directly because the internal names are meaningless.
	int i = 0;
	for (auto& li : LumpInfo)
	{
		if (li.size > 0)
		{
			FStringf rts("rts%02d", i);
			int lump = fileSystem.AddFromBuffer(rts, "rts", (char*)RTSFile.Data() + li.position, li.size, -1, 0);
			li.sid = soundEngine->AddSoundLump(rts, lump, 0, -1);
		}
	}
	return false;
}

int RTS_SoundLength(int lump)
{
	if (!RTS_IsInitialized()) return 0;
    lump++;
    if ((unsigned)lump >= LumpInfo.Size()) return 0;
    return LumpInfo[lump].size;
}

void *RTS_GetSound(int lump)
{
	if (!RTS_IsInitialized()) return nullptr;
    lump++;
    if ((unsigned)lump >= LumpInfo.Size()) return nullptr;
	if(LumpInfo[lump].size <= 0) return nullptr;
	return RTSFile.Data() + LumpInfo[lump].position;
}

int RTS_GetSoundID(int lump)
{
	if (!RTS_IsInitialized()) return -1;
	lump++;
	if ((unsigned)lump >= LumpInfo.Size()) return -1;
	if (LumpInfo[lump].size <= 0) return -1;
	return LumpInfo[lump].sid;
}
