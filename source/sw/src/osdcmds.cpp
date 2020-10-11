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

#include "v_text.h"
#include "printf.h"

#include "misc.h"
#include "menus.h"
#include "mapinfo.h"
#include "jsector.h"
#include "network.h"
#include "gamestate.h"
#include "player.h"

BEGIN_SW_NS

static int osdcmd_warptocoords(CCmdFuncPtr parm)
{
    if (parm->numparms < 3 || parm->numparms > 5)
        return CCMD_SHOWHELP;

    Player->oposx = Player->posx = atoi(parm->parms[0]);
    Player->oposy = Player->posy = atoi(parm->parms[1]);
    Player->oposz = Player->posz = atoi(parm->parms[2]);

    if (parm->numparms >= 4)
    {
		Player->angle.oang = Player->angle.ang = buildang(atoi(parm->parms[3]));
    }

    if (parm->numparms == 5)
    {
    	Player->horizon.ohoriz = Player->horizon.horiz = buildhoriz(atoi(parm->parms[4]));
    }

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

static int osdcmd_third_person_view(CCmdFuncPtr parm)
{
    if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
    auto pp = &Player[myconnectindex];
    if (inputState.ShiftPressed())
    {
        if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
            pp->view_outside_dang = NORM_ANGLE(pp->view_outside_dang + 256);
    }
    else
    {
        if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
        {
            RESET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
        }
        else
        {
            SET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
            pp->camera_dist = 0;
        }
    }
    return CCMD_OK;
}

static int osdcmd_coop_view(CCmdFuncPtr parm)
{
    if (gamestate != GS_LEVEL || System_WantGuiCapture()) return CCMD_OK;
    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
    {
        screenpeek = connectpoint2[screenpeek];

        if (screenpeek < 0)
            screenpeek = connecthead;

        if (screenpeek == myconnectindex)
        {
            // JBF: figure out what's going on here
            auto pp = &Player[myconnectindex];
            DoPlayerDivePalette(pp);  // Check Dive again
            DoPlayerNightVisionPalette(pp);  // Check Night Vision again
        }
        else
        {
            PLAYERp tp = Player + screenpeek;
            DoPlayerDivePalette(tp);
            DoPlayerNightVisionPalette(tp);
        }
    }
    return CCMD_OK;
}

static int osdcmd_noop(CCmdFuncPtr parm)
{
	// this is for silencing key bindings only.
	return CCMD_OK;
}

int32_t registerosdcommands(void)
{
    C_RegisterFunction("mirror_debug", "mirror [mirrornum]: print mirror debug info", osdcmd_mirror);
    C_RegisterFunction("warptocoords","warptocoords [x] [y] [z] [ang] (optional) [horiz] (optional): warps the player to the specified coordinates",osdcmd_warptocoords);
    C_RegisterFunction("third_person_view", "Switch to third person view", osdcmd_third_person_view);
    C_RegisterFunction("coop_view", "Switch player to view from in coop", osdcmd_coop_view);
	C_RegisterFunction("show_weapon", "Show opponents' weapons", osdcmd_noop);

    return 0;
}

END_SW_NS
