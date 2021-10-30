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
#include "blood.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "gamecontrol.h"

BEGIN_BLD_NS

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


void ByteSwapSFX(SFX* pSFX)
{
#if B_BIG_ENDIAN == 1
    pSFX->relVol = LittleLong(pSFX->relVol);
    pSFX->pitch = LittleLong(pSFX->pitch);
    pSFX->pitchRange = LittleLong(pSFX->pitchRange);
    pSFX->format = LittleLong(pSFX->format);
    pSFX->loopStart = LittleLong(pSFX->loopStart);
#endif
}

//==========================================================================
//
// S_AddBloodSFX
//
// Registers a new sound with the name "<lumpname>.sfx"
// Actual sound data is searched for in the ns_bloodraw namespace.
//
//==========================================================================

static void S_AddBloodSFX(int lumpnum)
{
    auto sfxlump = fileSystem.ReadFile(lumpnum);
    SFX* sfx = (SFX*)sfxlump.GetMem();
    ByteSwapSFX(sfx);
    FStringf rawname("%s.raw", sfx->rawName);
    auto rawlump = fileSystem.FindFile(rawname);
    int sfxnum;

    if (rawlump != -1)
    {
        auto& S_sfx = soundEngine->GetSounds();
        sfxnum = soundEngine->AddSoundLump(sfx->rawName, rawlump, 0, fileSystem.GetResourceId(lumpnum), 6);
        if (sfx->format < 5 || sfx->format > 12)
        {	// [0..4] + invalid formats
            S_sfx[sfxnum].RawRate = 11025;
        }
        else if (sfx->format < 9)
        {	// [5..8]
            S_sfx[sfxnum].RawRate = 22050;
        }
        else
        {	// [9..12]
            S_sfx[sfxnum].RawRate = 44100;
        }
        S_sfx[sfxnum].bLoadRAW = true;
        S_sfx[sfxnum].LoopStart = LittleLong(sfx->loopStart);
        //S_sfx[sfxnum].Volume = sfx->relVol / 255.f; This cannot be done because this volume setting is optional.
        S_sfx[sfxnum].UserData.Resize(3);
        int* udata = (int*)S_sfx[sfxnum].UserData.Data();
        udata[0] = sfx->pitch;
        udata[1] = sfx->pitchRange;
        udata[2] = sfx->relVol;   
    }
}

void sndInit(void)
{
    soundEngine = new BloodSoundEngine;
    soundEngine->AddSoundLump("", 0, 0, -1, 6); // add a dummy entry at index #0
    for (int i = fileSystem.GetNumEntries() - 1; i >= 0; i--)
    {
        auto type = fileSystem.GetResourceType(i);
        if (!stricmp(type, "SFX"))
        {
            if (soundEngine->FindSoundByResID(fileSystem.GetResourceId(i)) == 0)
                S_AddBloodSFX(i);
        }
        else if (!stricmp(type, "WAV") || !stricmp(type, "OGG") || !stricmp(type, "FLAC") || !stricmp(type, "VOC"))
        {
            if (fileSystem.GetFileNamespace(i) != ns_music)
                soundEngine->AddSoundLump(fileSystem.GetFileFullName(i), i, 0, fileSystem.GetResourceId(i)| 0x40000000, 6); // mark the resource ID as special.
        }
    }
    soundEngine->HashSounds();
}




int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}



bool sndCheckPlaying(unsigned int nSound)
{
    auto snd = soundEngine->FindSoundByResID(nSound);
    return snd > 0 ? soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, snd) : false;
}

void sndStopSample(unsigned int nSound)
{
    auto snd = soundEngine->FindSoundByResID(nSound);

    if (snd > 0)
    {
        soundEngine->StopSoundID(snd);
    }
}

void sndStartSample(const char *pzSound, int nVolume, int nChannel)
{
    if (!SoundEnabled())
        return;
    if (!strlen(pzSound))
        return;
    auto snd = soundEngine->FindSound(pzSound);
    if (snd > 0)
    {
        soundEngine->StartSound(SOURCE_None, nullptr, nullptr, nChannel + 1, 0, snd, nVolume / 255.f, ATTN_NONE);
    }
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop, EChanFlags chanflags)
{
    if (!SoundEnabled())
        return;
    if (nChannel >= 7) nChannel = -1;
    auto snd = soundEngine->FindSoundByResID(nSound);
    if (snd > 0)
    {
        if (nVolume < 0)
        {
            auto udata = soundEngine->GetUserData(snd);
            if (udata) nVolume = min(Scale(udata[2], 255, 100), 255);
            else nVolume = 255;
        }
        if (bLoop) chanflags |= CHANF_LOOP;
        soundEngine->StopActorSounds(SOURCE_None, nullptr, nChannel + 1, nChannel + 1);
        soundEngine->StartSound(SOURCE_None, nullptr, nullptr, (nChannel + 1), chanflags, snd, nVolume / 255.f, ATTN_NONE);
    }
}

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
    return sndStartSample(nSound | 0x40000000, nVolume, nChannel);
}

void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel)
{
    FString name = pzFile;
    FixPathSeperator(name);
    return sndStartSample(name, nVolume, nChannel);
}

void sndKillAllSounds(void)
{
    soundEngine->StopSound(CHAN_AUTO);
}



END_BLD_NS
