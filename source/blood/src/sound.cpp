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

BEGIN_BLD_NS

int32_t SoundToggle;
int32_t MusicToggle;
int32_t CDAudioToggle;
int32_t FXVolume;
int32_t MusicVolume;
int32_t CDVolume;
int32_t NumVoices;
int32_t NumChannels;
int32_t NumBits;
int32_t MixRate;
int32_t ReverseStereo;
int32_t MusicDevice;

Resource gSoundRes;

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
#define kMaxChannels 32

int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}

SAMPLE2D Channel[kMaxChannels];

SAMPLE2D * FindChannel(void)
{
    for (int i = kMaxChannels - 1; i >= 0; i--)
        if (Channel[i].at5 == 0) return &Channel[i];
    consoleSysMsg("No free channel available for sample");
    //ThrowError("No free channel available for sample");
    return NULL;
}

DICTNODE *hSong;
char *pSongPtr;
int nSongSize;
bool bWaveMusic;
int nWaveMusicHandle;

int sndPlaySong(const char *songName, bool bLoop)
{
    if (!MusicToggle)
        return 0;
    if (!songName || strlen(songName) == 0)
        return 1;

    auto fp = S_OpenAudio(songName, 0, 1);
    if (!fp.isOpen())
    {
        hSong = gSoundRes.Lookup(songName, "MID");
        if (!hSong)
        {
            OSD_Printf(OSD_ERROR "sndPlaySong(): error: can't open \"%s\" for playback!\n", songName);
            return 2;
        }
        int nNewSongSize = hSong->size;
        char *pNewSongPtr = (char *)Xaligned_alloc(16, nNewSongSize);
        gSoundRes.Load(hSong, pNewSongPtr);
        MUSIC_SetVolume(MusicVolume);
        int32_t retval = MUSIC_PlaySong(pNewSongPtr, nNewSongSize, bLoop);

        if (retval != MUSIC_Ok)
        {
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
        {
            FX_StopSound(nWaveMusicHandle);
            nWaveMusicHandle = -1;
        }

        bWaveMusic = false;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
        return 0;
    }

    int32_t nSongLen = fp.Tell();

    if (EDUKE32_PREDICT_FALSE(nSongLen < 4))
    {
        OSD_Printf(OSD_ERROR "sndPlaySong(): error: empty music file \"%s\"\n", songName);
        return 3;
    }

    char * pNewSongPtr = (char *)Xaligned_alloc(16, nSongLen);
    int nNewSongSize = fp.Read(pNewSongPtr, nSongLen);

    if (EDUKE32_PREDICT_FALSE(nNewSongSize != nSongLen))
    {
        OSD_Printf(OSD_ERROR "sndPlaySong(): error: read %d bytes from \"%s\", expected %d\n",
            nNewSongSize, songName, nSongLen);
        ALIGNED_FREE_AND_NULL(pNewSongPtr);
        return 4;
    }

    if (!Bmemcmp(pNewSongPtr, "MThd", 4))
    {
        int32_t retval = MUSIC_PlaySong(pNewSongPtr, nNewSongSize, bLoop);

        if (retval != MUSIC_Ok)
        {
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
        {
            FX_StopSound(nWaveMusicHandle);
            nWaveMusicHandle = -1;
        }

        bWaveMusic = false;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
    }
    else
    {
        int nNewWaveMusicHandle = FX_Play(pNewSongPtr, bLoop ? nNewSongSize : -1, 0, 0, 0, MusicVolume, MusicVolume, MusicVolume,
                                   FX_MUSIC_PRIORITY, 1.f, (intptr_t)&nWaveMusicHandle);

        if (nNewWaveMusicHandle <= FX_Ok)
        {
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
            FX_StopSound(nWaveMusicHandle);

        MUSIC_StopSong();

        nWaveMusicHandle = nNewWaveMusicHandle;
        bWaveMusic = true;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
    }

    return 0;
}

bool sndIsSongPlaying(void)
{
    //return MUSIC_SongPlaying();
    return false;
}

void sndFadeSong(int nTime)
{
    UNREFERENCED_PARAMETER(nTime);
    // NUKE-TODO:
    //if (MusicDevice == -1)
    //    return;
    //if (gEightyTwoFifty && sndMultiPlayer)
    //    return;
    //MUSIC_FadeVolume(0, nTime);
    if (bWaveMusic && nWaveMusicHandle >= 0)
    {
        FX_StopSound(nWaveMusicHandle);
        nWaveMusicHandle = -1;
        bWaveMusic = false;
    }
    MUSIC_SetVolume(0);
    MUSIC_StopSong();
}

void sndSetMusicVolume(int nVolume)
{
    MusicVolume = nVolume;
    if (bWaveMusic && nWaveMusicHandle >= 0)
        FX_SetPan(nWaveMusicHandle, nVolume, nVolume, nVolume);
    MUSIC_SetVolume(nVolume);
}

void sndSetFXVolume(int nVolume)
{
    FXVolume = nVolume;
    FX_SetVolume(nVolume);
}

void sndStopSong(void)
{
    if (bWaveMusic && nWaveMusicHandle >= 0)
    {
        FX_StopSound(nWaveMusicHandle);
        nWaveMusicHandle = -1;
        bWaveMusic = false;
    }

    MUSIC_StopSong();

    ALIGNED_FREE_AND_NULL(pSongPtr);
    nSongSize = 0;
}

void SoundCallback(intptr_t val)
{
    SAMPLE2D *pChannel = (SAMPLE2D*)val;
    pChannel->at0 = 0;
}

void sndKillSound(SAMPLE2D *pChannel);

void sndStartSample(const char *pzSound, int nVolume, int nChannel)
{
    if (!SoundToggle)
        return;
    if (!strlen(pzSound))
        return;
    dassert(nChannel >= -1 && nChannel < kMaxChannels);
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
    int nSize = pChannel->at5->size;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->at0 = FX_PlayRaw(pData, nSize, sndGetRate(1), 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop)
{
    if (!SoundToggle)
        return;
    dassert(nChannel >= -1 && nChannel < kMaxChannels);
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
    int nSize = pChannel->at5->size;
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
    if (!SoundToggle)
        return;
    dassert(nChannel >= -1 && nChannel < kMaxChannels);
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
    pChannel->at0 = FX_Play(pData, pChannel->at5->size, 0, -1, 0, nVolume, nVolume, nVolume, nVolume, 1.f, (intptr_t)&pChannel->at0);
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
    dassert(nChannel >= -1 && nChannel < kMaxChannels);
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
    char *pData = (char*)gSoundRes.Alloc(nLength);
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
    for (int i = 0; i < kMaxChannels; i++)
    {
        SAMPLE2D *pChannel = &Channel[i];
        if (pChannel->at0 > 0)
            sndKillSound(pChannel);
        if (pChannel->at5)
        {
            if (pChannel->at4 & 2)
            {
#if 0
                free(pChannel->at5);
#else
                gSoundRes.Free(pChannel->at5);
#endif
                pChannel->at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(pChannel->at5);
            }
            pChannel->at5 = 0;
        }
    }
}

void sndProcess(void)
{
    for (int i = 0; i < kMaxChannels; i++)
    {
        if (Channel[i].at0 <= 0 && Channel[i].at5)
        {
            if (Channel[i].at4 & 2)
            {
                gSoundRes.Free(Channel[i].at5);
                Channel[i].at4 &= ~2;
            }
            else
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
    nStatus = FX_Init(NumVoices, NumChannels, MixRate, initdata);
    if (nStatus != 0)
    {
        initprintf("InitSoundDevice: %s\n", FX_ErrorString(nStatus));
        return;
    }
    if (ReverseStereo == 1)
        FX_SetReverseStereo(!FX_GetReverseStereo());
    FX_SetVolume(FXVolume);
    FX_SetCallBack(SoundCallback);
}

void DeinitSoundDevice(void)
{
    int nStatus = FX_Shutdown();
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void InitMusicDevice(void)
{
    int nStatus = MUSIC_Init(MusicDevice, 0);
    if (nStatus != 0)
    {
        initprintf("InitMusicDevice: %s\n", MUSIC_ErrorString(nStatus));
        return;
    }
    MUSIC_SetVolume(MusicVolume);
}

void DeinitMusicDevice(void)
{
    FX_StopAllSounds();
    int nStatus = MUSIC_Shutdown();
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
}

bool sndActive = false;

void sndTerm(void)
{
    if (!sndActive)
        return;
    sndActive = false;
    sndStopSong();
    DeinitSoundDevice();
    DeinitMusicDevice();
}
extern char *pUserSoundRFF;
void sndInit(void)
{
    memset(Channel, 0, sizeof(Channel));
    pSongPtr = NULL;
    nSongSize = 0;
    bWaveMusic = false;
    nWaveMusicHandle = -1;
    InitSoundDevice();
    InitMusicDevice();
    //atexit(sndTerm);
    sndActive = true;
}

END_BLD_NS
