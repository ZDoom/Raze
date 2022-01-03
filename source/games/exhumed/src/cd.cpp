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
#include "engine.h"
#include "sound.h"
#include "exhumed.h"
#include <stdio.h>
#include <stdlib.h>
#include "raze_music.h"

BEGIN_PS_NS


int nLastVolumeSet = 0;

/* TODO

Currently playing music must keep playing on return to map screen or exit from training level

*/

bool playCDtrack(int nTrack, bool bLoop)
{
    if (nTrack < 2) {
        return false;
    }

    StopCD();

    char filename[128];

    // try ogg vorbis now from root directory.
    sprintf(filename, "exhumed%02d.ogg", nTrack);
    if (!Mus_Play(filename, bLoop))
    {
        // try ogg vorbis now from GOG MUSIC subdirectory.
        sprintf(filename, "track%02d.ogg", nTrack);
        Mus_Play(filename, bLoop);
    }
    return true;
}

int StepFadeCDaudio()
{
    if (!CDplaying()) {
    return 0;
    }
    Mus_Stop();
    return 1;
}

bool CDplaying()
{
    return Mus_IsPlaying();
}

void StopCD()
{
    Mus_Stop();
}

void FadeSong()
{
}

int fadecdaudio()
{
    Mus_Stop();
    return 1;
}


END_PS_NS
