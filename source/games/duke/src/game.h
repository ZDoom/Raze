//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#include "fix16.h"
#include "mmulti.h"
#include "palette.h"
#include "cmdlib.h"
#include "screenjob.h"
#include "constants.h"
#include "types.h"

BEGIN_DUKE_NS


extern user_defs ud;
extern int rtsplaying;

#ifndef ONLY_USERDEFS

extern int32_t g_Shareware;
extern int32_t cameraclock;
extern int32_t cameradist;
extern int32_t tempwallptr;

enum
{
    TFLAG_WALLSWITCH = 1
};
// for now just flags not related to actors, may get more info later.
struct TileInfo
{
    int flags;
};
extern TileInfo tileinfo[MAXTILES];


extern int32_t      actor_tog;
extern int32_t      otherp;


extern ActorInfo   actorinfo[MAXTILES];
extern weaponhit      hittype[MAXSPRITES];
extern bool sound445done;

#endif


extern intptr_t apScriptGameEvent[];

extern TArray<int> ScriptCode;


#include "concmd.h"

END_DUKE_NS
