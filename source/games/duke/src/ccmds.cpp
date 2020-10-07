//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright 2020 Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "ns.h"

#include "duke3d.h"
#include "sbar.h"
#include "mapinfo.h"
#include "cheathandler.h"
#include "c_dispatch.h"
#include "gamestate.h"

BEGIN_DUKE_NS

int getlabelvalue(const char* text);

static int ccmd_spawn(CCmdFuncPtr parm)
{
	int x = 0, y = 0, z = 0;
	unsigned int cstat = 0, picnum = 0;
	unsigned int pal = 0;
	int ang = 0;
	int set = 0, idx;

#if 0 // fixme - route through the network and this limitation becomes irrelevant
	if (netgame || numplayers > 1 || !(ps[myconnectindex].gm & MODE_GAME)) {
		Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
		return CCMD_OK;
	}
#endif

	switch (parm->numparms) {
	case 7: // x,y,z
		x = atol(parm->parms[4]);
		y = atol(parm->parms[5]);
		z = atol(parm->parms[6]);
		set |= 8;
	case 4: // ang
		ang = atol(parm->parms[3]) & 2047; set |= 4;
	case 3: // cstat
		cstat = (unsigned short)atol(parm->parms[2]); set |= 2;
	case 2: // pal
		pal = (unsigned char)atol(parm->parms[1]); set |= 1;
	case 1: // tile number
		if (isdigit(parm->parms[0][0])) {
			picnum = (unsigned short)atol(parm->parms[0]);
		}
		else {
			picnum = getlabelvalue(parm->parms[0]);
			if (picnum < 0) {
				Printf("spawn: Invalid tile label given\n");
				return CCMD_OK;
			}
		}

		if (picnum >= MAXTILES) {
			Printf("spawn: Invalid tile number\n");
			return CCMD_OK;
		}
		break;
	default:
		return CCMD_SHOWHELP;
	}

	idx = fi.spawn(ps[myconnectindex].i, (short)picnum);
	if (set & 1) sprite[idx].pal = (char)pal;
	if (set & 2) sprite[idx].cstat = (short)cstat;
	if (set & 4) sprite[idx].ang = ang;
	if (set & 8) {
		if (setsprite(idx, x, y, z) < 0) {
			Printf("spawn: Sprite can't be spawned into null space\n");
			deletesprite(idx);
		}
	}

	return CCMD_OK;
}

static int osdcmd_warptocoords(CCmdFuncPtr parm)
{
	if (parm->numparms < 3 || parm->numparms > 5)
		return CCMD_SHOWHELP;

	player_struct* p = &ps[myconnectindex];

	p->oposx = p->posx = atoi(parm->parms[0]);
	p->oposy = p->posy = atoi(parm->parms[1]);
	p->oposz = p->posz = atoi(parm->parms[2]);

    if (parm->numparms >= 4)
    {
        p->oq16ang = p->q16ang = IntToFixed(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
        p->horizon.ohoriz = p->horizon.horiz = buildhoriz(atoi(parm->parms[4]));
    }

	return CCMD_OK;
}

static int osdcmd_third_person_view(CCmdFuncPtr parm)
{
	if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
	if (!isRRRA() || (!ps[myconnectindex].OnMotorcycle && !ps[myconnectindex].OnBoat))
	{
		if (ps[myconnectindex].over_shoulder_on)
			ps[myconnectindex].over_shoulder_on = 0;
		else
		{
			ps[myconnectindex].over_shoulder_on = 1;
			cameradist = 0;
			cameraclock = INT_MIN;
		}
		FTA(QUOTE_VIEW_MODE_OFF + ps[myconnectindex].over_shoulder_on, &ps[myconnectindex]);
	}
	return CCMD_OK;
}

static int osdcmd_coop_view(CCmdFuncPtr parm)
{
	if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
	if (ud.coop || ud.recstat == 2)
	{
		screenpeek = connectpoint2[screenpeek];
		if (screenpeek == -1) screenpeek = 0;
	}
	return CCMD_OK;
}

static int osdcmd_show_weapon(CCmdFuncPtr parm)
{
	if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
	if (ud.multimode > 1)
	{
		ud.showweapons = 1 - ud.showweapons;
		cl_showweapon = ud.showweapons;
		FTA(QUOTE_WEAPON_MODE_OFF - ud.showweapons, &ps[screenpeek]);
	}

	return CCMD_OK;
}


int registerosdcommands(void)
{
	C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",ccmd_spawn);
	C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);
	C_RegisterFunction("third_person_view", "Switch to third person view", osdcmd_third_person_view);
	C_RegisterFunction("coop_view", "Switch player to view from in coop", osdcmd_coop_view);
	C_RegisterFunction("show_weapon", "Show opponents' weapons", osdcmd_show_weapon);

	return 0;
}

END_DUKE_NS
