//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "cd.h"
#include "fx_man.h"
#include "sound.h"
#include "exhumed.h"
#include <stdio.h>
#include <stdlib.h>

BEGIN_PS_NS

extern short word_9AC30;

static char *pTrack = NULL;
int trackhandle = -1;
int nLastVolumeSet = 0;


int cd_check_device_present()
{
    return 1;
}

int initcdaudio()
{
    if (!cd_check_device_present())
    {
        word_9AC30 = 1;

        // return to text video mode
        initprintf("No MSCDEX driver installed!\n");
        exit(0);
    }

    return 1;
}

void setCDaudiovolume(int val)
{
    if (trackhandle > 0) {
        FX_SetPan(trackhandle, val, val, val);
    }
}

int playCDtrack(int nTrack)
{
    if (nTrack < 2) {
        return 0;
    }

    char filebuf[128];

    // prefer flac if available
    sprintf(filebuf, "exhumed%02d.flac", nTrack);
    int32_t hFile = kopen4load(filebuf, 0);
    if (hFile < 0)
    {
        // try ogg vorbis now
        sprintf(filebuf, "exhumed%02d.ogg", nTrack);
        hFile = kopen4load(filebuf, 0);
        if (hFile < 0) {
            return 0;
        }
    }

    int32_t nFileLen = kfilelength(hFile);

    pTrack = (char*)Xaligned_alloc(16, nFileLen);
    int nRead = kread(hFile, pTrack, nFileLen);

    kclose(hFile);

    trackhandle = FX_Play(pTrack, nRead, -1, 0, 0, 255, 255, 255, FX_MUSIC_PRIORITY, fix16_one, MUSIC_ID);
    if (trackhandle < 0)
    {
        if (pTrack)
        {
            Xaligned_free(pTrack);
            pTrack = NULL;
        }
        return 0;
    }

    setCDaudiovolume(gMusicVolume);

    nCDTrackLength = 1;

    return nCDTrackLength;
}

void StartfadeCDaudio()
{
    if (CDplaying()) {
        nLastVolumeSet = gMusicVolume;
    }
}

int StepFadeCDaudio()
{
    if (!CDplaying()) {
    return 0;
    }

    if (nLastVolumeSet <= 0) {
        return 0;
    }

    nLastVolumeSet -= 8;

    if (nLastVolumeSet <= 0) {
        nLastVolumeSet = 0;
    }

    setCDaudiovolume(nLastVolumeSet);

    if (nLastVolumeSet == 0) {
        StopCD();
    }

    return 1;
}

int CDplaying()
{
    if (trackhandle > 0 && pTrack) { // better way to do this?
    return 1;
    }
    else {
        return 0;
    }
}

void StopCD()
{
    if (trackhandle > 0) {
        FX_StopSound(trackhandle);
        trackhandle = -1;
    }

    nCDTrackLength = 0;

    if (pTrack)
    {
        Xaligned_free(pTrack);
        pTrack = NULL;
    }
}

END_PS_NS
