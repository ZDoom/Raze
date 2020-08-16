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

#include "ns.h"

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "mytypes.h"

#include "gamecontrol.h"
#include "gstrings.h"

#include "common.h"
#include "v_text.h"
#include "printf.h"

#include "misc.h"
#include "menus.h"
#include "mapinfo.h"
#include "jsector.h"
#include "network.h"
#include "gamestate.h"

BEGIN_SW_NS

static void levelwarp(MapRecord *maprec)
{
    if (CommEnabled)
        return;

    auto pp = &Player[myconnectindex];
    if (Skill >= 3)
    {
        PutStringInfo(pp, GStrings("TXTS_TOOSKILLFUL"));
        return;
    }

    if (TEST(pp->Flags, PF_DEAD))
        return;

    NextLevel = maprec;
    ExitLevel = TRUE;
    if (gamestate == GS_MENUSCREEN || gamestate == GS_FULLCONSOLE) NewGame = true;

    sprintf(ds, "%s %s", GStrings("TXT_ENTERING"), maprec->DisplayName());
    PutStringInfo(pp, ds);
}


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
    auto maprec = FindMapByName(mapname);
    if (maprec) levelwarp(maprec);
    else
    {
        maprec = SetupUserMap(mapfilename);
        if (maprec) levelwarp(maprec);
    }
    return CCMD_OK;
}

static int osdcmd_god(CCmdFuncPtr)
{
    C_DoCommand("activatecheat swgod");
    return CCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr)
{
    C_DoCommand("activatecheat swghost");
    return CCMD_OK;
}

int osdcmd_restartmap(CCmdFuncPtr)
{
    C_DoCommand("activatecheat swstart");
    return CCMD_OK;
}

int osdcmd_levelwarp(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;
    auto maprec = FindMapByLevelNum(atoi(parm->parms[0]));
    if (maprec) levelwarp(maprec);
    return CCMD_OK;
}


static int osdcmd_give(CCmdFuncPtr parm)
{
    int32_t i;

    if (parm->numparms != 1) return CCMD_SHOWHELP;

    if (!stricmp(parm->parms[0], "all"))
    {
        C_DoCommand("activatecheat swgimme");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "health"))
    {
        C_DoCommand("activatecheat swmedic");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "weapons"))
    {
        C_DoCommand("activatecheat swguns");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "ammo"))
    {
        C_DoCommand("activatecheat swammo");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "armor"))
    {
        C_DoCommand("activatecheat swarmor");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "keys"))
    {
        C_DoCommand("activatecheat swkeys");
        return CCMD_OK;
    }
    else if (!stricmp(parm->parms[0], "inventory"))
    {
        C_DoCommand("activatecheat switems");
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

    if (parm->numparms >= 4)
    {
		Player->oq16ang = Player->q16ang = Player->camq16ang = fix16_from_int(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
    	Player->oq16horiz = Player->q16horiz = Player->camq16horiz = fix16_from_int(atoi(parm->parms[4]));
    }

    return CCMD_OK;
}

static int osdcmd_bunny(CCmdFuncPtr parm)
{
    PLAYERp pp = Player + myconnectindex;

    if (CommEnabled)
        return CCMD_OK;

    pp->BunnyMode = !pp->BunnyMode;
    if (pp->BunnyMode)
        PutStringInfo(pp, "Bunny rockets enabled!");
    else
        PutStringInfo(pp, "Bunny rockets disabled!");
    return CCMD_OK;
}

static int osdcmd_mirror(CCmdFuncPtr parm)
{
    char base[80];
    int16_t op1 = 0;

    if (parm->numparms < 1)
    {
        return CCMD_SHOWHELP;
    }

    if (op1 < 0 || op1 > 9)
    {
        Printf("Mirror number is out of range!");
        return CCMD_OK;
    }

    Printf("camera is the ST1 sprite used as the view spot");
    Printf("camspite is the SpriteNum of the drawtotile tile in editart");
    Printf("camspic is the tile number of the drawtotile in editart");
    Printf("iscamera is whether or not this mirror is a camera type");
    Printf(" ");
    Printf("mirror[%d].mirrorwall = %d", op1, mirror[op1].mirrorwall);
    Printf("mirror[%d].mirrorsector = %d", op1, mirror[op1].mirrorsector);
    Printf("mirror[%d].camera = %d", op1, mirror[op1].camera);
    Printf("mirror[%d].camsprite = %d", op1, mirror[op1].camsprite);
    Printf("mirror[%d].campic = %d", op1, mirror[op1].campic);
    Printf("mirror[%d].iscamera = %d", op1, mirror[op1].ismagic);
    return CCMD_OK;
}


int32_t registerosdcommands(void)
{
    C_RegisterFunction("map","map <mapfile>: loads the given map", osdcmd_map);
    C_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    C_RegisterFunction("bunny", "bunny: toggles bunny rocket mode", osdcmd_bunny);
    C_RegisterFunction("mirror", "mirror [mirrornum]: print mirror debug info", osdcmd_mirror);

    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    C_RegisterFunction("levelwarp", "levelwarp <num>: warp to level", osdcmd_levelwarp);

    C_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);

//    C_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);

    return 0;
}

END_SW_NS
