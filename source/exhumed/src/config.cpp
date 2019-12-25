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
#include "compat.h"
#include "renderlayer.h"
#include "build.h"
#include "exhumed.h"
#include "typedefs.h"
#include "view.h"

#include "config.h"

#include <string>
//#include <io.h>
#include <stdio.h>
#include <time.h>

BEGIN_PS_NS



ud_setup_t gSetup;

int lMouseSens = 32;
unsigned int dword_1B82E0 = 0;

int32_t FXVolume;
int32_t MusicVolume;
int32_t MidiPort;
int32_t NumBits;
int32_t ReverseStereo;
int32_t MusicDevice;
int32_t FXDevice;
int32_t ControllerType;

int32_t setupread;
// TODO: implement precaching toggle
int32_t useprecache;

int CONFIG_ReadSetup(void)
{

    // currently settings.cfg is only read after the startup window launches the game,
    // and rereading binds might be fickle so just enable this
    
    NumBits = 16;
    return 0;
}

END_PS_NS