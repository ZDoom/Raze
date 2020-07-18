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

#include "ns.h"	// Must come before everything else!

#define game_c_

#include "duke3d.h"
#include "compat.h"
#include "baselayer.h"
#include "savegame.h"

#include "sbar.h"
#include "palette.h"
#include "gamecvars.h"
#include "gameconfigfile.h"
#include "printf.h"
#include "m_argv.h"
#include "filesystem.h"
#include "statistics.h"
#include "c_dispatch.h"
#include "mapinfo.h"
#include "v_video.h"
#include "glbackend/glbackend.h"
#include "st_start.h"
#include "i_interface.h"

BEGIN_DUKE_NS

int levelTextTime; // must be serialized
int rtsplaying;		// must be serialized
int otherp;			// MP only
bool sound445done; // this was local state inside a function, but this must be maintained globally and serialized

int16_t max_ammo_amount[MAX_WEAPONS];
int32_t spriteqamount = 64;

uint8_t shadedsector[MAXSECTORS];

int32_t cameradist = 0, cameraclock = 0;

int32_t g_Shareware = 0;

int32_t tempwallptr;
int32_t      actor_tog;

weaponhit hittype[MAXSPRITES];
ActorInfo actorinfo[MAXTILES];
player_struct ps[MAXPLAYERS];

int32_t PHEIGHT = PHEIGHT_DUKE;

int32_t lastvisinc;


END_DUKE_NS
