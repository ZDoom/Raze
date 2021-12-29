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
#pragma once

#include "misc.h"
#include "raze_music.h"
#include "s_soundinternal.h"

BEGIN_BLD_NS

struct SFX
{
	int relVol;
	int pitch;
	int pitchRange;
	int format;
	int loopStart;
	char rawName[9];
};

int sndGetRate(int format);
bool sndCheckPlaying(unsigned int nSound);
void sndStopSample(unsigned int nSound);
void sndStartSample(const char* pzSound, int nVolume, int nChannel = -1);
void sndStartSample(unsigned int nSound, int nVolume, int nChannel = -1, bool bLoop = false, EChanFlags soundflags = CHANF_NONE);
void sndStartWavID(unsigned int nSound, int nVolume, int nChannel = -1);
void sndStartWavDisk(const char* pzFile, int nVolume, int nChannel = -1);
void sndKillAllSounds(void);
void sndProcess(void);
void sndTerm(void);
void sndInit(void);

void sfxPlay3DSound(int x, int y, int z, int soundId, sectortype* pSector);
void sfxPlay3DSound(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0);
void sfxPlay3DSoundCP(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0, int pitch = 0, int volume = 0);
void sfxKill3DSound(DBloodActor* pSprite, int a2 = -1, int a3 = -1);
void sfxKillAllSounds(void);
void sfxSetReverb(bool toggle);
void sfxSetReverb2(bool toggle);

void ambProcess(void);
void ambKillAll(void);
void ambInit(void);

enum EPlayFlags
{
	FX_GlobalChannel = 1,
	FX_SoundMatch = 2,
	FX_ChannelMatch = 4,
};


END_BLD_NS
