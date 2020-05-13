//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors
Copyright (C) 2020 Raze developers and contributors

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

#include "cheats.h"
#include "cmdline.h"
#include "demo.h"  // g_firstDemoFile[]
#include "duke3d.h"
#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "sbar.h"
#include "mapinfo.h"

BEGIN_DUKE_NS

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat = { -1, 0, 0 };

static int osdcmd_levelwarp(CCmdFuncPtr parm)
{
    if (parm->numparms != 2)
        return OSDCMD_SHOWHELP;
    int e = atoi(parm->parms[0]);
    int m = atoi(parm->parms[1]);
    if (e == 0 || m == 0)
    {
        Printf(TEXTCOLOR_RED "Invalid level!: E%sL%s\n", parm->parms[0], parm->parms[1]);
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = e - 1;
    m_level_number = m - 1;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        G_NewGame(ud.m_volume_number, m_level_number, ud.m_player_skill);
        g_player[myconnectindex].ps->gm = MODE_RESTART;
    }
    else G_NewGame_EnterLevel();

    return OSDCMD_OK;
}

static int osdcmd_map(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
    {
        return OSDCMD_SHOWHELP;
    }
    FString mapname = parm->parms[0];

    if (!fileSystem.Lookup(mapname, "MAP"))
    {
        Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", mapname.GetChars());
        return OSDCMD_OK;
    }
	
	// Check if the map is already defined.
    for (int i = 0; i < 512; i++)
    {
        if (mapList[i].labelName.CompareNoCase(mapname) == 0)
        {
           ud.m_volume_number = i / MAXLEVELS;
           m_level_number = i % MAXLEVELS;
           goto foundone;
        }
    }
	if (VOLUMEONE)
	{
		Printf(TEXTCOLOR_RED "Cannot use user maps in shareware.\n");
		return OSDCMD_OK;
	}
	// Treat as user map
    boardfilename[0] = '/';
    boardfilename[1] = 0;
    ud.m_volume_number = 0;
    m_level_number = 7;
	DefaultExtension(mapname, ".map");
    strcat(boardfilename, mapname);
foundone:
    if (numplayers > 1)
    {
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        G_NewGame(ud.m_volume_number, m_level_number, ud.m_player_skill);
        g_player[myconnectindex].ps->gm = MODE_RESTART;
    }
    else G_NewGame_EnterLevel();

    return OSDCMD_OK;
}


// demo <demonum or demofn> [<prof>]
//
// To profile a demo ("timedemo mode"), <prof> can be given in the range 0-8,
// which will start to replay it as fast as possible, rendering <prof> frames
// for each gametic.
//
// Notes:
//  * The demos should be recorded with demorec_diffs set to 0, so that the
//    game state updates are actually computed.
//  * Currently, the profiling can only be aborted on SDL 1.2 builds by
//    pressing any key.
//  * With <prof> greater than 1, interpolation should be calculated properly,
//    though this has not been verified by looking at the frames.
//  * When testing whether a change in the source has an effect on performance,
//    the variance of the run times MUST be taken into account (that is, the
//    replaying must be performed multiple times for the old and new versions,
//    etc.)
static int osdcmd_demo(CCmdFuncPtr parm)
{
    if (numplayers > 1)
    {
        Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        Printf("demo: Must not be in a game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1 && parm->numparms != 2)
        return OSDCMD_SHOWHELP;

    {
        int32_t prof = parm->numparms==2 ? Batoi(parm->parms[1]) : -1;

        Demo_SetFirst(parm->parms[0]);
        Demo_PlayFirst(clamp(prof, -1, 8)+1, 0);
    }

    return OSDCMD_OK;
}

static int osdcmd_activatecheat(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = Batoi(parm->parms[0]);
    else
        Printf("activatecheat: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_god(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CORNHOLIO;
    else
        Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CLIP;
    }
    else
    {
        Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

int osdcmd_restartmap(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (g_player[myconnectindex].ps->gm & MODE_GAME && ud.multimode == 1)
        g_player[myconnectindex].ps->gm = MODE_RESTART;

    return OSDCMD_OK;
}

static int osdcmd_spawn(CCmdFuncPtr parm)
{
    int32_t picnum = 0;
    uint16_t cstat=0;
    char pal=0;
    int16_t ang=0;
    int16_t set=0, idx;
    vec3_t vect;

    if (numplayers > 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
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
                for (i=0; i<labelcnt; i++)
                {
                    if ((j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0])))
                    {
                        picnum = labelcode[i];
                        break;
                    }
                }

                if (i < labelcnt)
                    break;
            }
            if (i==labelcnt)
            {
                Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if ((uint32_t)picnum >= MAXUSERTILES)
        {
            Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;

    default:
        return OSDCMD_SHOWHELP;
    }

    idx = A_Spawn(g_player[myconnectindex].ps->i, picnum);
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

    return OSDCMD_OK;
}


static int osdcmd_give(CCmdFuncPtr parm)
{
    int32_t i;

    if (numplayers != 1 || (g_player[myconnectindex].ps->gm & MODE_GAME) == 0 ||
            g_player[myconnectindex].ps->dead_flag != 0)
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_STUFF;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        sprite[g_player[myconnectindex].ps->i].extra = max_player_health<<1;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_WEAPONS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        for (i=MAX_WEAPONS-(VOLUMEONE?6:1)-1; i>=PISTOL_WEAPON; i--)
            P_AddAmmo(g_player[myconnectindex].ps,i, max_ammo_amount[i]);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        g_player[myconnectindex].ps->inv_amount[GET_SHIELD] = 100;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_KEYS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_INVENTORY;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

#if !defined NETCODE_DISABLE
static int osdcmd_disconnect(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    // NUKE-TODO:
    if (g_player[myconnectindex].ps->gm&MODE_MENU)
        g_netDisconnect = 1;
    return OSDCMD_OK;
}

static int osdcmd_connect(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    Net_Connect(parm->parms[0]);
    G_BackToMenu();
    return OSDCMD_OK;
}

static int osdcmd_password(CCmdFuncPtr parm)
{
    if (parm->numparms < 1)
    {
        Bmemset(g_netPassword, 0, sizeof(g_netPassword));
        return OSDCMD_OK;
    }
    Bstrncpy(g_netPassword, (parm->raw) + 9, sizeof(g_netPassword)-1);

    return OSDCMD_OK;
}

int osdcmd_listplayers(CCmdFuncPtr parm);
#endif

static int osdcmd_printtimes(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    char buf[32];
    int32_t maxlen = 0;
    int32_t haveac=0;

    for (int i=0; i<MAXTILES; i++)
        if (g_actorCalls[i])
        {
            if (!haveac)
            {
                haveac = 1;
                Printf("\nactor times: tile, total calls, total time [ms], {min,mean,max} time/call [us]\n");
            }

            buf[0] = 0;

            /*
            for (int ii=0; ii<labelcnt; ii++)
            {
                if (labelcode[ii] == i && labeltype[ii] & LABEL_ACTOR)
                {
                    Bstrcpy(buf, label+(ii<<6));
                    break;
                }
            }
            */

            if (!buf[0]) Bsprintf(buf, "%d", i);

            Printf("%17s, %8d, %9.3f, %9.3f, %9.3f, %9.3f,\n",
                buf, g_actorCalls[i], g_actorTotalMs[i],
                1000*g_actorMinMs[i],
                1000*g_actorTotalMs[i]/g_actorCalls[i],
                1000*g_actorMaxMs[i]);
        }

    return OSDCMD_OK;
}


int32_t registerosdcommands(void)
{

	C_RegisterFunction("map","map <mapname>: loads the given map", osdcmd_map);
    if (!VOLUMEONE)
    {
        C_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    }
    C_RegisterFunction("levelwarp","levelwarp <e> <m>: warp to episode 'e' and map 'm'", osdcmd_levelwarp);

#if !defined NETCODE_DISABLE
    C_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
    C_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);
#endif

    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    C_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

#ifdef DEBUGGINGAIDS
    C_RegisterFunction("inittimer","debug", osdcmd_inittimer);
#endif
#if !defined NETCODE_DISABLE
#if 0
    C_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
    C_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);
#endif

    C_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);
#endif
    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

#if !defined NETCODE_DISABLE
    C_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);
#endif

    C_RegisterFunction("printtimes", "printtimes: prints VM timing statistics", osdcmd_printtimes);

    C_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);

    C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    return 0;
}

END_DUKE_NS
