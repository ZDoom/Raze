//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"

#include "build.h"
#include "baselayer.h"
#include "gamecvars.h"

#include "settings.h"
#include "mytypes.h"
#include "gamedefs.h"
#include "gamecontrol.h"
#include "sounds.h"
#include "config.h"
#include "common_game.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#include "baselayer.h"


BEGIN_SW_NS

//
// Comm variables
//

int32_t NumberPlayers,CommPort,PortSpeed,IrqNumber,UartAddress;

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/

int32_t CONFIG_ReadSetup(void)
{
    memcpy(&gs, &gs_defaults, sizeof(gs));
    return 0;
}

END_SW_NS
