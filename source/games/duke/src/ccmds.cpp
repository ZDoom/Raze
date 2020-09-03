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

static void dowarp(MapRecord *map)
{
	ud.m_monsters_off = ud.monsters_off = 0;

	ud.m_respawn_items = 0;
	ud.m_respawn_inventory = 0;

	ud.multimode = 1;

	if (ps[myconnectindex].gm & MODE_GAME)
	{
		donewgame(map, ud.m_player_skill);
		ps[myconnectindex].gm = MODE_RESTART;
	}
	else startnewgame(map, ud.m_player_skill);
}

static int ccmd_levelwarp(CCmdFuncPtr parm)
{
	if (parm->numparms != 2)
		return CCMD_SHOWHELP;
	int e = atoi(parm->parms[0]);
	int m = atoi(parm->parms[1]);
	if (e == 0 || m == 0)
	{
		Printf(TEXTCOLOR_RED "Invalid level!: E%sL%s\n", parm->parms[0], parm->parms[1]);
		return CCMD_OK;
	}
	auto map = FindMapByLevelNum(levelnum(e - 1, m - 1));
	if (!map)
	{
		Printf(TEXTCOLOR_RED "Level not found!: E%sL%s\n", parm->parms[0], parm->parms[1]);
		return CCMD_OK;
	}
	dowarp(map);

	return CCMD_OK;
}

static int ccmd_map(CCmdFuncPtr parm)
{
	if (parm->numparms != 1)
	{
		return CCMD_SHOWHELP;
	}
	FString mapname = parm->parms[0];
	FString mapfilename = mapname;
	DefaultExtension(mapfilename, ".map");

	if (!fileSystem.FileExists(mapfilename))
	{
		Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", mapname.GetChars());
		return CCMD_OK;
	}
	// Check if the map is already defined.
	auto map = FindMapByName(mapname);
	if (map == nullptr)
	{
		// got a user map
		if (VOLUMEONE)
		{
			Printf(TEXTCOLOR_RED "Cannot use user maps in shareware.\n");
			return CCMD_OK;
		}
		if (mapfilename[0] != '/') mapfilename.Insert(0, "/");
		map = SetupUserMap(mapfilename, !isRR() ? "dethtoll.mid" : nullptr);
	}
	if (numplayers > 1)
	{
		return CCMD_OK;
	}

	dowarp(map);
	return CCMD_OK;
}

static int ccmd_restartmap(CCmdFuncPtr)
{
	if (ps[myconnectindex].gm & MODE_GAME && ud.multimode == 1)
		ps[myconnectindex].gm = MODE_RESTART;

	return CCMD_OK;
}

int getlabelvalue(const char* text);

static int ccmd_spawn(CCmdFuncPtr parm)
{
	int x = 0, y = 0, z = 0;
	unsigned int cstat = 0, picnum = 0;
	unsigned int pal = 0;
	int ang = 0;
	int set = 0, idx;

	if (numplayers > 1 || !(ps[myconnectindex].gm & MODE_GAME)) {
		Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
		return CCMD_OK;
	}

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
        p->oq16horiz = p->q16horiz = IntToFixed(atoi(parm->parms[4]));
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
	C_RegisterFunction("map","map <mapname>: warp to the given map, identified by its name", ccmd_map);
	C_RegisterFunction("levelwarp","levelwarp <e> <m>: warp to episode 'e' and map 'm'", ccmd_levelwarp);
	C_RegisterFunction("restartmap", "restartmap: restarts the current map", ccmd_restartmap);
	C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",ccmd_spawn);
	C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);
	C_RegisterFunction("third_person_view", "Switch to third person view", osdcmd_third_person_view);
	C_RegisterFunction("coop_view", "Switch player to view from in coop", osdcmd_coop_view);
	C_RegisterFunction("show_weapon", "Show opponents' weapons", osdcmd_show_weapon);

	return 0;
}

END_DUKE_NS
