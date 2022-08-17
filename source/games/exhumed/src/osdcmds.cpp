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

    nPlayer->pActor->set_int_pos({ x, y, z });
    nPlayer->pActor->backuppos();

    if (ang != INT_MIN)
    {
        nPlayer->angle.oang = nPlayer->angle.ang = DAngle::fromBuild(ang);
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

static int osdcmd_spawn(CCmdFuncPtr parm)
{
    if (parm->numparms != 1) return CCMD_SHOWHELP;
    auto c = parm->parms[0];
	auto sectp = initsectp;
	DVector3 initpos = { initx * inttoworld, inity * inttoworld, sectp->floorz };
    if (!stricmp(c, "anubis")) BuildAnubis(nullptr, initpos, sectp, inita, false);
    else if (!stricmp(c, "spider")) BuildSpider(nullptr, initpos, sectp, inita);
    else if (!stricmp(c, "mummy")) BuildMummy(nullptr, initpos, sectp, inita);
    else if (!stricmp(c, "fish")) BuildFish(nullptr, initpos.plusZ(PlayerList[nLocalPlayer].eyelevel * zinttoworld), sectp, inita);
    else if (!stricmp(c, "lion")) BuildLion(nullptr, initpos, sectp, inita);
    else if (!stricmp(c, "lava")) BuildLava(nullptr, initpos, sectp, inita, nNetPlayerCount);
    else if (!stricmp(c, "rex")) BuildRex(nullptr, initpos, sectp, inita, nNetPlayerCount);
    else if (!stricmp(c, "set")) BuildSet(nullptr, initpos, sectp, inita, nNetPlayerCount);
    else if (!stricmp(c, "queen")) BuildQueen(nullptr, initpos, sectp, inita, nNetPlayerCount);
    else if (!stricmp(c, "roach")) BuildRoach(0, nullptr, initpos, sectp, inita);
    else if (!stricmp(c, "roach2")) BuildRoach(1, nullptr, initpos, sectp, inita);
    else if (!stricmp(c, "wasp")) BuildWasp(nullptr, initpos.plusZ(-100), sectp, inita, false);
    else if (!stricmp(c, "scorp")) BuildScorp(nullptr, initpos, sectp, inita, nNetPlayerCount);
    else if (!stricmp(c, "rat")) BuildRat(nullptr, initpos, sectp, inita);
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
