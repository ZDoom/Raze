//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020 Raze developers and contributors

This file was part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "build.h"
#include "baselayer.h"
#include "osd.h"
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"
#include "config.h"
#include "blood.h"
#include "demo.h"
#include "gamemenu.h"
#include "globals.h"
#include "levels.h"
#include "messages.h"
#include "network.h"
#include "osdcmds.h"
#include "screen.h"
#include "sound.h"
#include "sfx.h"
#include "view.h"
#include "mapinfo.h"

BEGIN_BLD_NS

void LevelWarp(int nEpisode, int nLevel);

static int osdcmd_map(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    char filename[BMAX_PATH];

    strcpy(filename, parm->parms[0]);
    ChangeExtension(filename, "");

    if (!fileSystem.Lookup(filename, "MAP"))
    {
        Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }

    for (int i = 0; i < 512; i++)
    {
        if (mapList[i].labelName.CompareNoCase(filename) == 0)
        {
            int e = i / kMaxLevels;
            int m = i % kMaxLevels;
            LevelWarp(e, m);
            return OSDCMD_OK;
        }
    }
    // Map has not been defined. Treat as user map.

    if (gDemo.at1)
        gDemo.StopPlayback();

    levelAddUserMap(filename);

    if (numplayers > 1)
    {
        gPacketStartGame.episodeId = gGameOptions.nEpisode;
        gPacketStartGame.levelId = gGameOptions.nLevel;
        netBroadcastNewGame();
        gStartNewGame = 1;
        return OSDCMD_OK;
    }
    levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);

    return OSDCMD_OK;
}

static int osdcmd_demo(CCmdFuncPtr parm)
{
    if (numplayers > 1)
    {
        Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    //if (g_player[myconnectindex].ps->gm & MODE_GAME)
    //{
    //    Printf("demo: Must not be in a game.\n");
    //    return OSDCMD_OK;
    //}

    if (parm->numparms != 1/* && parm->numparms != 2*/)
        return OSDCMD_SHOWHELP;

    gDemo.SetupPlayback(parm->parms[0]);
    gGameStarted = 0;
    gDemo.Playback();

    return OSDCMD_OK;
}


static int osdcmd_give(CCmdFuncPtr parm)
{
    if (numplayers != 1 || !gGameStarted || gMe->pXSprite->health == 0)
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        SetWeapons(true);
        SetAmmo(true);
        SetToys(true);
        SetArmor(true);
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        SetWeapons(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        SetAmmo(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        SetArmor(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        SetToys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

static int osdcmd_god(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && gGameStarted)
    {
        SetGodMode(!gMe->godMode);
        gCheatMgr.m_bPlayerCheated = true;
    }
    else
        Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && gGameStarted)
    {
        SetClipMode(!gNoClip);
        gCheatMgr.m_bPlayerCheated = true;
    }
    else
    {
        Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_activatecheat(CCmdFuncPtr parm)
{
    FString CheatEntry;
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    CheatEntry = (char*)(parm->parms[0]);
    CheatEntry.ToUpper();

    if (gCheatMgr.Check((char*)(CheatEntry.GetChars())))
	    return OSDCMD_OK;
    else
    {
        Printf("Unrecognized cheat!: %s\n", parm->parms[0]);
        return OSDCMD_OK;
    }
}

static int osdcmd_levelwarp(CCmdFuncPtr parm)
{
    if (parm->numparms != 2)
        return OSDCMD_SHOWHELP;
    int e = atoi(parm->parms[0]);
    int m = atoi(parm->parms[1]);
    if (e == 0 || m == 0)
    {
        Printf(TEXTCOLOR_RED "Invalid level!: E%sM%s\n", parm->parms[0], parm->parms[1]);
        return OSDCMD_OK;
    }
    LevelWarp(e - 1, m - 1);
    return OSDCMD_OK;
}

int32_t registerosdcommands(void)
{
    C_RegisterFunction("map","map <mapname>: loads the given map", osdcmd_map);
    C_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);

    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    C_RegisterFunction("activatecheat","activatecheat <string>: activates a classic cheat code", osdcmd_activatecheat);

    C_RegisterFunction("levelwarp","levelwarp <e> <m>: warp to episode 'e' and map 'm'", osdcmd_levelwarp);

    return 0;
}

END_BLD_NS
