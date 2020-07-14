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
#include "savegame.h"
#include "sbar.h"
#include "mapinfo.h"
#include "cheathandler.h"
#include "c_dispatch.h"

BEGIN_DUKE_NS

bool cheatGod(cheatseq_t*);
bool cheatClip(cheatseq_t*);
bool cheatWeapons(cheatseq_t* s);
bool cheatStuff(cheatseq_t* s);
bool cheatKeys(cheatseq_t* s);
bool cheatInventory(cheatseq_t* s);

static void dowarp(MapRecord *map)
{
    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    if (ps[myconnectindex].gm & MODE_GAME)
    {
        newgame(map, ud.m_player_skill);
        ps[myconnectindex].gm = MODE_RESTART;
    }
    else G_NewGame_EnterLevel(map, ud.m_player_skill);
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

    if (!fileSystem.Lookup(mapname, "MAP"))
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
        DefaultExtension(mapname, ".map");
        if (mapname[0] != '/') mapname.Insert(0, "/");
        map = SetupUserMap(mapname, !isRR() ? "dethtoll.mid" : nullptr);
    }
    if (numplayers > 1)
    {
        return CCMD_OK;
    }

    dowarp(map);
    return CCMD_OK;
}


static int ccmd_activatecheat(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return CCMD_SHOWHELP;

    PlaybackCheat(parm->parms[0]);
    return CCMD_OK;
}

static int ccmd_god(CCmdFuncPtr)
{
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME)
        cheatGod(nullptr);
    else
        Printf("god: Not in a single-player game.\n");

    return CCMD_OK;
}

static int ccmd_noclip(CCmdFuncPtr)
{
    if (numplayers == 1 && ps[myconnectindex].gm & MODE_GAME)
    {
        cheatClip(nullptr);
    }
    else
    {
        Printf("noclip: Not in a single-player game.\n");
    }

    return CCMD_OK;
}

static int ccmd_restartmap(CCmdFuncPtr)
{
    if (ps[myconnectindex].gm & MODE_GAME && ud.multimode == 1)
        ps[myconnectindex].gm = MODE_RESTART;

    return CCMD_OK;
}

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
            int i, j;
            for (j = 0; j < 2; j++) {
                for (i = 0; i < labelcnt; i++) {
                    if (
                        (j == 0 && !strcmp(label + (i * MAXLABELLEN), parm->parms[0])) ||
                        (j == 1 && !stricmp(label + (i * MAXLABELLEN), parm->parms[0]))
                        ) {
                        picnum = (unsigned short)labelcode[i];
                        break;
                    }
                }
                if (i < labelcnt) break;
            }
            if (i == labelcnt) {
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

// Strangely enough JFDuke does not have a 'give' CCMD, so this is based on the version in EDuke32.
static int ccmd_give(CCmdFuncPtr parm)
{
    if (numplayers != 1 || (ps[myconnectindex].gm & MODE_GAME) == 0 ||
            ps[myconnectindex].dead_flag != 0)
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return CCMD_OK;
    }

    if (parm->numparms != 1) return CCMD_SHOWHELP;

    cheatseq_t* cs = (cheatseq_t*)(intptr_t)1;
    if (!stricmp(parm->parms[0], "all"))
    {
        cheatStuff(cs);
    }
    else if (!stricmp(parm->parms[0], "health"))
    {
        sprite[ps[myconnectindex].i].extra = max_player_health<<1;
    }
    else if (!stricmp(parm->parms[0], "weapons"))
    {
        cheatWeapons(cs);
    }
    else if (!stricmp(parm->parms[0], "ammo"))
    {
        int maxw = VOLUMEONE ? SHRINKER_WEAPON : MAX_WEAPONS;
        for (int i = maxw; i >= PISTOL_WEAPON; i--)
            addammo(i, &ps[myconnectindex], max_ammo_amount[i]);
    }
    else if (!stricmp(parm->parms[0], "armor"))
    {
        ps[myconnectindex].shield_amount = 100;
    }
    else if (!stricmp(parm->parms[0], "keys"))
    {
        cheatKeys(cs);
    }
    else if (!stricmp(parm->parms[0], "inventory"))
    {
        cheatInventory(cs);
    }
    else return CCMD_SHOWHELP;
    return CCMD_OK;
}



int registerosdcommands(void)
{
	C_RegisterFunction("map","map <mapname>: warp to the given map, identified by its name", ccmd_map);
    C_RegisterFunction("levelwarp","levelwarp <e> <m>: warp to episode 'e' and map 'm'", ccmd_levelwarp);

    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", ccmd_give);
    C_RegisterFunction("god","god: toggles god mode", ccmd_god);
    C_RegisterFunction("activatecheat","activatecheat <string>: activates a cheat code", ccmd_activatecheat);

    C_RegisterFunction("noclip","noclip: toggles clipping mode", ccmd_noclip);
    C_RegisterFunction("restartmap", "restartmap: restarts the current map", ccmd_restartmap);

    C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",ccmd_spawn);

    return 0;
}

END_DUKE_NS
