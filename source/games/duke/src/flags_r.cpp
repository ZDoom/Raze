//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software, you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program, if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source, 1996 - Todd Replogle
Prepared for public release, 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_r.h"

BEGIN_DUKE_NS

void initactorflags_r()
{
	for (auto& fa : gs.actorinfo)
	{
		fa.falladjustz = 24;
	}
	if (isRRRA())
	{
		gs.actorinfo[RTILE_HULKBOAT].falladjustz = 12;
		gs.actorinfo[RTILE_MINIONBOAT].falladjustz = 3;
		gs.actorinfo[RTILE_CHEERBOAT].falladjustz = gs.actorinfo[RTILE_EMPTYBOAT].falladjustz = 6;
	}
	gs.actorinfo[RTILE_DRONE].falladjustz = 0;
	gs.weaponsandammosprites[0] = RTILE_CROSSBOWSPRITE;
	gs.weaponsandammosprites[1] = RTILE_RIFLEGUNSPRITE;
	gs.weaponsandammosprites[2] = RTILE_DEVISTATORAMMO;
	gs.weaponsandammosprites[3] = RTILE_RPGAMMO;
	gs.weaponsandammosprites[4] = RTILE_RPGAMMO;
	gs.weaponsandammosprites[5] = RTILE_COWPIE;
	gs.weaponsandammosprites[6] = RTILE_SHIELD;
	gs.weaponsandammosprites[7] = RTILE_FIRSTAID;
	gs.weaponsandammosprites[8] = RTILE_STEROIDS;
	gs.weaponsandammosprites[9] = RTILE_RPGAMMO;
	gs.weaponsandammosprites[10] = RTILE_RPGAMMO;
	gs.weaponsandammosprites[11] = RTILE_CROSSBOWSPRITE;
	gs.weaponsandammosprites[12] = RTILE_RPGAMMO;
	gs.weaponsandammosprites[13] = RTILE_TITSPRITE;
	gs.weaponsandammosprites[14] = RTILE_FREEZEAMMO;

	TILE_APLAYER = RTILE_APLAYER;
	TILE_DRONE = RTILE_DRONE;
	TILE_WATERBUBBLE = RTILE_WATERBUBBLE;
	TILE_BLOODPOOL = RTILE_BLOODPOOL;
	TILE_CROSSHAIR = RTILE_CROSSHAIR;

	gs.firstdebris = RTILE_SCRAP6;
	gs.gutsscale = 0.125;
}

END_DUKE_NS
