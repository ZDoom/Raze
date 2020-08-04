//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors
Modified by Raze developers and contributors

This file was part of EDuke32.

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

#include "osdcmds.h"

#include "ns.h"

#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "mytypes.h"
#include "text.h"

#include "gamecontrol.h"
#include "gstrings.h"

#include "common.h"
#include "v_text.h"
#include "printf.h"

#include "cheats.h"
#include "demo.h"  // g_firstDemoFile[]
#include "menus.h"
#include "mapinfo.h"

BEGIN_SW_NS

char boardfilename[BMAX_PATH] = {0};

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat = { -1, 0, 0 };

static int osdcmd_map(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
    {
        return CCMD_SHOWHELP;
    }
    FString mapname = parm->parms[0];
    FString mapfilename = mapname;
    DefaultExtension(mapfilename, ".map");

    if (!fileSystem.FindFile(mapfilename))
    {
        Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", mapfilename.GetChars());
        return CCMD_OK;
    }
	
	// Check if the map is already defined.
    for (int i = 0; i < 32; i++)
    {
        if (mapList[i].labelName.CompareNoCase(mapname) == 0)
        {
			FStringf cheatcode("swtrek%02d", i);
			WarpCheat(Player, cheatcode);
			return CCMD_OK;
        }
    }
    return CCMD_OK;
}


static int osdcmd_activatecheat(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return CCMD_SHOWHELP;

    memset(MessageInputString, '\0', sizeof(MessageInputString));
    strcpy(MessageInputString, parm->parms[0]);
    CheatInput();

    return CCMD_OK;
}

static int osdcmd_god(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    GodCheat(Player, "swgod");

    return CCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    ClipCheat(Player, "swghost");

    return CCMD_OK;
}

int osdcmd_restartmap(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    RestartCheat(Player, "swstart");

    return CCMD_OK;
}

int osdcmd_levelwarp(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;

    char cheatcode[9] = "swtrek##";

    for (int i = 0; i < 2; i++)
        cheatcode[6+i] = parm->parms[0][i];

    WarpCheat(Player, cheatcode);
    return CCMD_OK;
}

#if 0
static int osdcmd_spawn(CCmdFuncPtr parm)
{
    int32_t picnum = 0;
    uint16_t cstat=0;
    char pal=0;
    int16_t ang=0;
    int16_t set=0, idx;
    vec3_t vect;

    if (numplayers > 1 || !(ps[myconnectindex].gm & MODE_GAME))
    {
        Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return CCMD_OK;
    }

    switch (parm->numparms)
    {
    case 7: // x,y,z
        vect.x = Batol(parm->parms[4]);
        vect.y = Batol(parm->parms[5]);
        vect.z = Batol(parm->parms[6]);
        set |= 8;
        fallthrough__;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047;
        set |= 4;
        fallthrough__;
    case 3: // cstat
        cstat = (uint16_t)Batol(parm->parms[2]);
        set |= 2;
        fallthrough__;
    case 2: // pal
        pal = (uint8_t)Batol(parm->parms[1]);
        set |= 1;
        fallthrough__;
    case 1: // tile number
        if (isdigit(parm->parms[0][0]))
        {
            picnum = Batol(parm->parms[0]);
        }
        else
        {
            int32_t i;
            int32_t j;

            for (j=0; j<2; j++)
            {
                for (i=0; i<g_labelCnt; i++)
                {
                    if ((j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0])))
                    {
                        picnum = labelcode[i];
                        break;
                    }
                }

                if (i < g_labelCnt)
                    break;
            }
            if (i==g_labelCnt)
            {
                Printf("spawn: Invalid tile label given\n");
                return CCMD_OK;
            }
        }

        if ((uint32_t)picnum >= MAXUSERTILES)
        {
            Printf("spawn: Invalid tile number\n");
            return CCMD_OK;
        }
        break;

    default:
        return CCMD_SHOWHELP;
    }

    idx = A_Spawn(ps[myconnectindex].i, picnum);
    if (set & 1) sprite[idx].pal = (uint8_t)pal;
    if (set & 2) sprite[idx].cstat = (int16_t)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8)
    {
        if (setsprite(idx, &vect) < 0)
        {
            Printf("spawn: Sprite can't be spawned into null space\n");
            A_DeleteSprite(idx);
        }
    }

    return CCMD_OK;
}
#endif

static int osdcmd_give(CCmdFuncPtr parm)
{
    int32_t i;

    if (parm->numparms != 1) return CCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        ItemCheat(Player, "swgimme");
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        HealCheat(Player, "swmedic");
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
	WeaponCheat(Player, "swguns");
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
	AmmoCheat(Player, "");
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        ArmorCheat(Player, ""); // this cheat did not exist before
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        KeysCheat(Player, "swkeys");
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        InventoryCheat(Player, ""); // this cheat did not exist before
        return CCMD_OK;
    }
    return CCMD_SHOWHELP;
}

static int osdcmd_warptocoords(CCmdFuncPtr parm)
{
    if (parm->numparms < 3 || parm->numparms > 5)
        return CCMD_SHOWHELP;

    Player->oposx = Player->posx = atoi(parm->parms[0]);
    Player->oposy = Player->posy = atoi(parm->parms[1]);
    Player->oposz = Player->posz = atoi(parm->parms[2]);

    if (parm->numparms == 4)
    {
		Player->oq16ang = Player->q16ang = Player->camq16ang = fix16_from_int(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
    	Player->oq16horiz = Player->q16horiz = Player->camq16horiz = fix16_from_int(atoi(parm->parms[4]));
    }

    return CCMD_OK;
}

int32_t registerosdcommands(void)
{
    C_RegisterFunction("map","map <mapfile>: loads the given map", osdcmd_map);
    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
//    C_RegisterFunction("activatecheat","activatecheat <string>: activates a classic cheat code", osdcmd_activatecheat);

    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    C_RegisterFunction("levelwarp", "levelwarp <num>: warp to level", osdcmd_levelwarp);

    C_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);

//    C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);

    return 0;
}

END_SW_NS
