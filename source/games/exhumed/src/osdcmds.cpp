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
#include "exhumed.h"
#include "player.h"
#include "view.h"
#include "mapinfo.h"
#include "aistuff.h"
#include "input.h"
#include "cheathandler.h"
#include "gamestate.h"
#include "gamefuncs.h"

BEGIN_PS_NS

void GameInterface::WarpToCoords(int x, int y, int z, int ang, int horz)
{
    Player     *nPlayer = &PlayerList[nLocalPlayer];
    spritetype *pSprite = &nPlayer->Actor()->s(); 

    pSprite->ox = pSprite->x = x;
    pSprite->oy = pSprite->y = y;
    pSprite->oz = pSprite->z = z;

    if (ang != INT_MIN)
    {
        nPlayer->angle.oang = nPlayer->angle.ang = buildang(ang);
    }

    if (horz != INT_MIN)
    {
        nPlayer->horizon.ohoriz = nPlayer->horizon.horiz = buildhoriz(horz);
    }
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

    if (!stricmp(c, "anubis")) BuildAnubis(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, false);
    else if (!stricmp(c, "spider")) BuildSpider(nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "mummy")) BuildMummy(nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "fish")) BuildFish(nullptr, initx, inity, initz + PlayerList[nLocalPlayer].eyelevel, initsect, inita);
    else if (!stricmp(c, "lion")) BuildLion(nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "lava")) BuildLava(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "rex")) BuildRex(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "set")) BuildSet(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "queen")) BuildQueen(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "roach")) BuildRoach(0, nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "roach2")) BuildRoach(1, nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else if (!stricmp(c, "wasp")) BuildWasp(nullptr, initx, inity, sector[initsect].floorz - 25600, initsect, inita, false);
    else if (!stricmp(c, "scorp")) BuildScorp(nullptr, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
    else if (!stricmp(c, "rat")) BuildRat(nullptr, initx, inity, sector[initsect].floorz, initsect, inita);
    else Printf("Unknown creature type %s\n", c);
    return CCMD_OK;
}

void GameInterface::ToggleThirdPerson()
{
    if (gamestate != GS_LEVEL) return;
    if (!nFreeze)
    {
        bCamera = !bCamera;

        if (bCamera)
        {
            GrabPalette();
            cameradist = 0;
            cameraclock = INT_MIN;
        }
    }
}


int32_t registerosdcommands(void)
{
    //if (VOLUMEONE)
    C_RegisterFunction("doors", "opens/closes doors", osdcmd_doors);
    C_RegisterFunction("spawn","spawn <creaturetype>: spawns a creature",osdcmd_spawn);

    return 0;
}


END_PS_NS
