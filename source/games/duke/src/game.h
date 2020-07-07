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

#ifndef ONLY_USERDEFS
#include "premap.h" // XXX
#endif

#include "fix16.h"
#include "gamedef.h"
#include "net.h"
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

extern char boardfilename[BMAX_PATH];

extern int32_t g_Shareware;
extern int32_t cameraclock;
extern int32_t cameradist;
extern int32_t g_doQuickSave;
extern int32_t tempwallptr;

void G_BackToMenu(void);

void G_HandleLocalKeys(void);
void G_UpdatePlayerFromMenu(void);


void G_InitTimer(int32_t ticspersec);

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


extern int startrts(int lumpNum, int localPlayer);

extern void G_MaybeAllocPlayer(int32_t pnum);


static inline void G_NewGame_EnterLevel(MapRecord *map, int skill)
{
    newgame(map, skill);

    if (enterlevel(map, MODE_GAME))
        G_BackToMenu();
}

extern void G_InitMultiPsky(int CLOUDYOCEAN__DYN, int MOONSKY1__DYN, int BIGORBIT1__DYN, int LA__DYN);
extern void G_SetupGlobalPsky(void);

//////////

extern void genspriteremaps(void);

extern int32_t      actor_tog;
extern int32_t      otherp;


extern ActorInfo   actorinfo[MAXTILES];
extern weaponhit      hittype[MAXSPRITES];

#endif

END_DUKE_NS
