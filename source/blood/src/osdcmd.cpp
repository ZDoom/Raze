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
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"
#include "blood.h"
#include "globals.h"
#include "levels.h"
#include "messages.h"
#include "sound.h"
#include "view.h"
#include "mapinfo.h"
#include "gamestate.h"

BEGIN_BLD_NS

static int osdcmd_warptocoords(CCmdFuncPtr parm)
{
    if (parm->numparms < 3 || parm->numparms > 5)
        return CCMD_SHOWHELP;

    PLAYER *pPlayer = &gPlayer[myconnectindex];

    pPlayer->pSprite->x = gView->pSprite->x = atoi(parm->parms[0]);
    pPlayer->pSprite->y = gView->pSprite->y = atoi(parm->parms[1]);
    pPlayer->zView      = gView->zView      = atoi(parm->parms[2]);

    if (parm->numparms >= 4)
    {
        pPlayer->q16ang = gView->q16ang = IntToFixed(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
        pPlayer->q16horiz = gView->q16horiz = IntToFixed(atoi(parm->parms[4]));
    }

    viewBackupView(pPlayer->nPlayer);

    return CCMD_OK;
}

static int osdcmd_third_person_view(CCmdFuncPtr parm)
{
    if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
    if (gViewPos > VIEWPOS_0)
        gViewPos = VIEWPOS_0;
    else
        gViewPos = VIEWPOS_1;
    return CCMD_OK;
}

static int osdcmd_coop_view(CCmdFuncPtr parm)
{
    if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
    if (gGameOptions.nGameType == 1)
    {
        gViewIndex = connectpoint2[gViewIndex];
        if (gViewIndex == -1)
            gViewIndex = connecthead;
        gView = &gPlayer[gViewIndex];
    }
    else if (gGameOptions.nGameType == 3)
    {
        int oldViewIndex = gViewIndex;
        do
        {
            gViewIndex = connectpoint2[gViewIndex];
            if (gViewIndex == -1)
                gViewIndex = connecthead;
            if (oldViewIndex == gViewIndex || gMe->teamId == gPlayer[gViewIndex].teamId)
                break;
        } while (oldViewIndex != gViewIndex);
        gView = &gPlayer[gViewIndex];
    }
    return CCMD_OK;
}

static int osdcmd_show_weapon(CCmdFuncPtr parm)
{
    if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
    cl_showweapon = (cl_showweapon + 1) & 3;
    return CCMD_OK;
}



int32_t registerosdcommands(void)
{
    C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);
    C_RegisterFunction("third_person_view", "Switch to third person view", osdcmd_third_person_view);
    C_RegisterFunction("coop_view", "Switch player to view from in coop", osdcmd_coop_view);
    C_RegisterFunction("show_weapon", "Show opponents' weapons", osdcmd_show_weapon);

    return 0;
}

END_BLD_NS
