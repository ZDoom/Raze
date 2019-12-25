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
#include "sound.h"
#include "exhumed.h"
#include <stdio.h>
#include <stdlib.h>
#include "z_music.h"

BEGIN_PS_NS

extern short word_9AC30;

static char *pTrack = NULL;
static int trackhandle = -1;
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

    // try ogg vorbis now
    sprintf(filename, "exhumed%02d.ogg", nTrack);
    trackhandle = Mus_Play(nullptr, filename, true);
    return true;
}

void StartfadeCDaudio()
{
}

int StepFadeCDaudio()
{
    if (!CDplaying()) {
    return 0;
    }
    Mus_Stop();
    trackhandle = 0;
    return 1;
}

bool CDplaying()
{
    if (trackhandle <= 0) {
        return false;
    }
    else {
        return true;
    }
}

void StopCD()
{
    Mus_Stop();
}

void FadeSong()
{
}

END_PS_NS
