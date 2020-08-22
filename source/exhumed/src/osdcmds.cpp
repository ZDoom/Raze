//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
#include "ns.h"
#include "compat.h"
#include "build.h"
#include "common.h"
#include "exhumed.h"
#include "player.h"
#include "view.h"
#include "mapinfo.h"
#include "aistuff.h"

BEGIN_PS_NS


static int osdcmd_god(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (!nNetPlayerCount && !bInDemo)
    {
        DoPassword(3);
    }
    else
        Printf("god: Not in a single-player game.\n");

    return CCMD_OK;
}

static int osdcmd_noclip(CCmdFuncPtr UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (!nNetPlayerCount && !bInDemo)
    {
        DoPassword(6);
    }
    else
    {
        Printf("noclip: Not in a single-player game.\n");
    }

    return CCMD_OK;
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

    if (!fileSystem.FileExists(mapfilename))
    {
        Printf(TEXTCOLOR_RED "map: file \"%s\" not found.\n", mapfilename.GetChars());
        return CCMD_OK;
    }
	
	// Check if the map is already defined.
    for (int i = 0; i <= ISDEMOVER? 4 : 32; i++)
    {
        if (mapList[i].labelName.CompareNoCase(mapname) == 0)
        {
			levelnew = i;
			levelnum = i;
			return CCMD_OK;
        }
    }
    return CCMD_OK;
}

static int osdcmd_changelevel(CCmdFuncPtr parm)
{
    char* p;

    if (parm->numparms != 1) return CCMD_SHOWHELP;

    int nLevel = strtol(parm->parms[0], &p, 10);
    if (p[0]) return CCMD_SHOWHELP;

    if (nLevel < 0) return CCMD_SHOWHELP;

    int nMaxLevels;

    if (!ISDEMOVER) {
        nMaxLevels = 32;
    }
    else {
        nMaxLevels = 4;
    }

    if (nLevel > nMaxLevels)
    {
        Printf("changelevel: invalid level number\n");
        return CCMD_SHOWHELP;
    }

    levelnew = nLevel;
    levelnum = nLevel;

    return CCMD_OK;
}

static int osdcmd_warptocoords(CCmdFuncPtr parm)
{
    if (parm->numparms < 3 || parm->numparms > 5)
        return CCMD_SHOWHELP;

    Player     *nPlayer = &PlayerList[nLocalPlayer];
    spritetype *pSprite = &sprite[nPlayer->nSprite]; 

    nPlayer->opos.x = pSprite->x = atoi(parm->parms[0]);
    nPlayer->opos.y = pSprite->y = atoi(parm->parms[1]);
    nPlayer->opos.z = pSprite->z = atoi(parm->parms[2]);

    if (parm->numparms >= 4)
    {
        nPlayer->q16angle = fix16_from_int(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
        nPlayer->q16horiz = fix16_from_int(atoi(parm->parms[4]));
    }

    return CCMD_OK;
}

static int osdcmd_exitmap(CCmdFuncPtr parm)
{
    EndLevel = true;
    return CCMD_OK;
}

static int osdcmd_doors(CCmdFuncPtr parm)
{
    for (int i = 0; i < kMaxChannels; i++)
    {
        // CHECKME - does this toggle?
        if (sRunChannels[i].c == 0) {
            runlist_ChangeChannel(i, 1);
        }
        else {
            runlist_ChangeChannel(i, 0);
        }
    }
    return CCMD_OK;
}

extern int initx;
extern int inity;
extern int initz;
extern short inita;
extern short initsect;

static int osdcmd_spawn(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;
    auto c = parm->parms[0];

    if (!stricmp(c, "anubis")) BuildAnubis(-1, initx, inity, sector[initsect].floorz, initsect, inita, false);
    else if (!stricmp(c, "spider")) BuildSpider(-1, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "mummy")) BuildMummy(-1, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "fish")) BuildFish(-1, initx, inity, initz + eyelevel[nLocalPlayer], initsect, inita);
    else if (!stricmp(c, "lion")) BuildLion(-1, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "lava")) BuildLava(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "rex")) BuildRex(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "set")) BuildSet(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "queen")) BuildQueen(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "roach")) BuildRoach(0, -1, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "roach2")) BuildRoach(1, -1, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "wasp")) BuildWasp(-1, initx, inity, sector[initsect].floorz - 25600, initsect, inita);
    else if (!stricmp(c, "scorp")) BuildScorp(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "rat")) BuildRat(-1, initx, inity, sector[initsect].floorz, initsect, inita);
    else Printf("Unknown creature type %s\n", c);
    return CCMD_OK;
}



int32_t registerosdcommands(void)
{
    //if (VOLUMEONE)
    C_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    C_RegisterFunction("map","map <mapname>: loads the given map", osdcmd_map);
    C_RegisterFunction("exitmap", "exits current map", osdcmd_exitmap);
    C_RegisterFunction("doors", "opens/closes doors", osdcmd_doors);
    C_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    C_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);
    C_RegisterFunction("spawn","spawn <creaturetype>: spawns a creature",osdcmd_spawn);
    C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);

    return 0;
}


END_PS_NS
