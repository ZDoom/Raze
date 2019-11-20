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

#include "compat.h"
#include "baselayer.h"
#include "cd.h"
#include <stdio.h>
#include <stdlib.h>

extern short word_9AC30;

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

}

int playCDtrack(int nTrack)
{
    return 1;
}

void StartfadeCDaudio()
{

}

int StepFadeCDaudio()
{
    return 0;
}

int CDplaying()
{
    return 1;
}

void StopCD()
{

}