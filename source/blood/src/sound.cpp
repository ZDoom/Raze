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

#include "build.h"
#include "compat.h"
#include "music.h"
#include "fx_man.h"
#include "common_game.h"
#include "config.h"
#include "levels.h"
#include "resource.h"
#include "sound.h"
#include "renderlayer.h"
#include "al_midi.h"
#include "openaudio.h"
#include "z_music.h"

BEGIN_BLD_NS

Resource& gSoundRes = fileSystem;

int soundRates[13] = {
    11025,
    11025,
    11025,
    11025,
    11025,
    22050,
    22050,
    22050,
    22050,
    44100,
    44100,
    44100,
    44100,
};
#define kChannelMax 32

int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}

SAMPLE2D Channel[kChannelMax];

SAMPLE2D * FindChannel(void)
{
    for (int i = kChannelMax - 1; i >= 0; i--)
        if (Channel[i].at5 == 0) return &Channel[i];
    consoleSysMsg("No free channel available for sample");
    //ThrowError("No free channel available for sample");
    return NULL;
}

int sndPlaySong(const char *mapname, const char* songName, bool bLoop)
{
	return Mus_Play(mapname, songName, bLoop);
}

bool sndIsSongPlaying(void)
{
	// Not used
	return false;
}

void sndFadeSong(int nTime)
{
	// not implemented
}

void sndStopSong(void)
{
	Mus_Stop();
}

void sndSetFXVolume(int nVolume)
{
	snd_fxvolume = nVolume;
	FX_SetVolume(nVolume);
}


void SoundCallback(intptr_t val)
{
    SAMPLE2D *pChannel = (SAMPLE2D*)val;
    pChannel->at0 = 0;
}

void sndKillSound(SAMPLE2D *pChannel);

void sndStartSample(const char *pzSound, int nVolume, int nChannel)
{
    if (!SoundEnabled())
        return;
    if (!strlen(pzSound))
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->at0 > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pzSound, "RAW");
    if (!pChannel->at5)
        return;
    int nSize = pChannel->at5->Size();
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->at0 = FX_PlayRaw(pData, nSize, sndGetRate(1), 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop)
{
    if (!SoundEnabled())
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    DICTNODE *hSfx = gSoundRes.Lookup(nSound, "SFX");
    if (!hSfx)
        return;
    SFX *pEffect = (SFX*)gSoundRes.Lock(hSfx);
    dassert(pEffect != NULL);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->at0 > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!pChannel->at5)
        return;
    if (nVolume < 0)
        nVolume = pEffect->relVol;
    int nSize = pChannel->at5->Size();
    int nLoopEnd = nSize - 1;
    if (nLoopEnd < 0)
        nLoopEnd = 0;
    if (nSize <= 0)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    if (nChannel < 0)
        bLoop = false;
    if (bLoop)
    {
        pChannel->at0 = FX_PlayLoopedRaw(pData, nSize, pData + pEffect->loopStart, pData + nLoopEnd, sndGetRate(pEffect->format),
            0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
        pChannel->at4 |= 1;
    }
    else
    {
        pChannel->at0 = FX_PlayRaw(pData, nSize, sndGetRate(pEffect->format), 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
        pChannel->at4 &= ~1;
    }
}

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
    if (!SoundEnabled())
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->at0 > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(nSound, "WAV");
    if (!pChannel->at5)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->at0 = FX_Play(pData, pChannel->at5->Size(), 0, -1, 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
}

void sndKillSound(SAMPLE2D *pChannel)
{
    if (pChannel->at4 & 1)
    {
        FX_EndLooping(pChannel->at0);
        pChannel->at4 &= ~1;
    }
    FX_StopSound(pChannel->at0);
}

void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel)
{
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->at0 > 0)
        sndKillSound(pChannel);
    auto hFile = kopenFileReader(pzFile, 0);
    if (!hFile.isOpen())
        return;
    int nLength = hFile.GetLength();
	char* pData = nullptr;
	cacheAllocateBlock((intptr_t*)pData, nLength, nullptr);	// use this obsolete call to indicate that some work is needed here!
    if (!pData)
    {
        return;
    }
	hFile.Read(pData, nLength);
    pChannel->at5 = (DICTNODE*)pData;
    pChannel->at4 |= 2;
    pChannel->at0 = FX_Play(pData, nLength, 0, -1, 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
}

void sndKillAllSounds(void)
{
    for (int i = 0; i < kChannelMax; i++)
    {
        SAMPLE2D *pChannel = &Channel[i];
        if (pChannel->at0 > 0)
            sndKillSound(pChannel);
        if (pChannel->at5)
        {
            if (pChannel->at4 & 2)
            {
                pChannel->at4 &= ~2;
            }
            else // This 'else' needs to be removed once the file system is up (when cacheAllocateBlock gets replaced.)
            {
                gSoundRes.Unlock(pChannel->at5);
            }
            pChannel->at5 = 0;
        }
    }
}

void sndProcess(void)
{
    for (int i = 0; i < kChannelMax; i++)
    {
        if (Channel[i].at0 <= 0 && Channel[i].at5)
        {
            if (Channel[i].at4 & 2)
            {
                Channel[i].at4 &= ~2;
            }
            else // This 'else' needs to be removed once the file system is up (when cacheAllocateBlock gets replaced.)
            {
                gSoundRes.Unlock(Channel[i].at5);
            }
            Channel[i].at5 = 0;
        }
    }
}

void InitSoundDevice(void)
{
#ifdef MIXERTYPEWIN
    void *initdata = (void *)win_gethwnd(); // used for DirectSound
#else
    void *initdata = NULL;
#endif
    int nStatus;
    nStatus = FX_Init(snd_numvoices, snd_numchannels, snd_mixrate, initdata);
    if (nStatus != 0)
    {
        initprintf("InitSoundDevice: %s\n", FX_ErrorString(nStatus));
        return;
    }
	snd_reversestereo.Callback();
	snd_fxvolume.Callback();
    FX_SetCallBack(SoundCallback);
}

void DeinitSoundDevice(void)
{
    int nStatus = FX_Shutdown();
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}


bool sndActive = false;

void sndTerm(void)
{
    if (!sndActive)
        return;
    sndActive = false;
    sndStopSong();
    DeinitSoundDevice();
    //DeinitMusicDevice();
}
extern char *pUserSoundRFF;
void sndInit(void)
{
    memset(Channel, 0, sizeof(Channel));
#if 0
    pSongPtr = NULL;
    nSongSize = 0;
    bWaveMusic = false;
    nWaveMusicHandle = -1;
#endif
    InitSoundDevice();
    //InitMusicDevice();
    //atexit(sndTerm);
    sndActive = true;
}

END_BLD_NS
