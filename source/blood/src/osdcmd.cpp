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
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"
#include "blood.h"
#include "globals.h"
#include "levels.h"
#include "messages.h"
#include "network.h"
#include "sound.h"
#include "view.h"
#include "mapinfo.h"
#include "gamestate.h"

BEGIN_BLD_NS

void LevelWarp(int nEpisode, int nLevel);

static int osdcmd_map(CCmdFuncPtr parm)
{
    if (parm->numparms != 1)
        return CCMD_SHOWHELP;

    FString mapname = parm->parms[0];
    FString mapfilename = mapname;
    DefaultExtension(mapfilename, ".map");

    if (!fileSystem.FindFile(mapfilename))
    {
        Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", mapfilename.GetChars());
        return CCMD_OK;
    }

    for (int i = 0; i < 512; i++)
    {
        if (mapList[i].labelName.CompareNoCase(mapname) == 0)
        {
            int e = volfromlevelnum(mapList[i].levelNumber);
            int m = mapfromlevelnum(mapList[i].levelNumber);
            LevelWarp(e, m);
            return CCMD_OK;
        }
    }
    // Map has not been defined. Treat as user map.

    levelAddUserMap(mapname);

    if (numplayers > 1)
    {
        gPacketStartGame.episodeId = gGameOptions.nEpisode;
        gPacketStartGame.levelId = gGameOptions.nLevel;
        netBroadcastNewGame();
        gStartNewGame = 1;
        return CCMD_OK;
    }
    levelSetupOptions(gGameOptions.nEpisode, gGameOptions.nLevel);
    StartLevel(&gGameOptions);
    viewResizeView(gViewSize);

    return CCMD_OK;
}


static int osdcmd_give(CCmdFuncPtr parm)
{
    if (numplayers != 1 || gamestate != GS_LEVEL|| gMe->pXSprite->health == 0)
    {
        Printf("give: Cannot give while dead or not in a single-player game.\n");
        return CCMD_OK;
    }

    if (parm->numparms != 1) return CCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        SetWeapons(true);
        SetAmmo(true);
        SetToys(true);
        SetArmor(true);
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        actHealDude(gMe->pXSprite, 200, 200);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        SetWeapons(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        SetAmmo(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        SetArmor(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        SetKeys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        SetToys(true);
        gCheatMgr.m_bPlayerCheated = true;
        return CCMD_OK;
    }
    return CCMD_SHOWHELP;
}

static int osdcmd_god(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && gamestate == GS_LEVEL)
    {
        SetGodMode(!gMe->godMode);
        gCheatMgr.m_bPlayerCheated = true;
    }
    else
        Printf("god: Not in a single-player game.\n");

    return CCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && gamestate == GS_LEVEL)
    {
        SetClipMode(!gNoClip);
        gCheatMgr.m_bPlayerCheated = true;
    }
    else
    {
        Printf("noclip: Not in a single-player game.\n");
    }

    return CCMD_OK;
}

static int osdcmd_activatecheat(CCmdFuncPtr parm)
{
    FString CheatEntry;
    if (parm->numparms != 1)
        return CCMD_SHOWHELP;

    CheatEntry = (char*)(parm->parms[0]);
    CheatEntry.ToUpper();

    if (gCheatMgr.Check((char*)(CheatEntry.GetChars())))
	    return CCMD_OK;
    else
    {
        Printf("Unrecognized cheat!: %s\n", parm->parms[0]);
        return CCMD_OK;
    }
}

static int osdcmd_levelwarp(CCmdFuncPtr parm)
{
    if (parm->numparms != 2)
        return CCMD_SHOWHELP;
    int e = atoi(parm->parms[0]);
    int m = atoi(parm->parms[1]);
    if (e == 0 || m == 0)
    {
        Printf(TEXTCOLOR_RED "Invalid level!: E%sM%s\n", parm->parms[0], parm->parms[1]);
        return CCMD_OK;
    }
    LevelWarp(e - 1, m - 1);
    return CCMD_OK;
}

int32_t registerosdcommands(void)
{
    C_RegisterFunction("map","map <mapname>: loads the given map", osdcmd_map);

    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    C_RegisterFunction("activatecheat","activatecheat <string>: activates a classic cheat code", osdcmd_activatecheat);

    C_RegisterFunction("levelwarp","levelwarp <e> <m>: warp to episode 'e' and map 'm'", osdcmd_levelwarp);

    return 0;
}

END_BLD_NS
