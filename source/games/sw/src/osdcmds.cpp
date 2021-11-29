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
#include "gamefuncs.h"
#include "player.h"

BEGIN_SW_NS

void GameInterface::WarpToCoords(int x, int y, int z, int ang, int horz)
{
    Player->oposx = Player->posx = x;
    Player->oposy = Player->posy = y;
    Player->oposz = Player->posz = z;

    if (ang != INT_MIN)
    {
		Player->angle.oang = Player->angle.ang = buildang(ang);
    }

    if (horz != INT_MIN)
    {
    	Player->horizon.ohoriz = Player->horizon.horiz = buildhoriz(horz);
    }
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
    Printf("camspite is the sprite number of the drawtotile tile in editart");
    Printf("camspic is the tile number of the drawtotile in editart");
    Printf("iscamera is whether or not this mirror is a camera type");
    Printf(" ");
    Printf("mirror[%d].mirrorwall = %d", op1, mirror[op1].mirrorwall);
    Printf("mirror[%d].mirrorsector = %d", op1, mirror[op1].mirrorsector);
    Printf("mirror[%d].camera = %d", op1, mirror[op1].cameraActor->GetIndex());
    Printf("mirror[%d].camsprite = %d", op1, mirror[op1].camspriteActor->GetIndex());
    Printf("mirror[%d].campic = %d", op1, mirror[op1].campic);
    Printf("mirror[%d].iscamera = %d", op1, mirror[op1].ismagic);
    return CCMD_OK;
}

void GameInterface::ToggleThirdPerson()
{
    if (gamestate != GS_LEVEL) return;
    auto pp = &Player[myconnectindex];
    if (TEST(pp->Flags, PF_VIEW_FROM_OUTSIDE))
    {
        RESET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
    }
    else
    {
        SET(pp->Flags, PF_VIEW_FROM_OUTSIDE);
        cameradist = 0;
    }
}

void GameInterface::SwitchCoopView()
{
    if (gamestate != GS_LEVEL) return;
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
}

int32_t registerosdcommands(void)
{
    C_RegisterFunction("mirror_debug", "mirror [mirrornum]: print mirror debug info", osdcmd_mirror);
    return 0;
}

END_SW_NS
