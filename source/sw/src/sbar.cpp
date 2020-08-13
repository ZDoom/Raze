//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"

#undef MAIN
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "pal.h"
#include "misc.h"
#include "player.h"
#include "v_2ddrawer.h"
#include "statusbar.h"

BEGIN_SW_NS


class DSWStatusBar : public DBaseStatusBar
{

    enum
    {
        PANEL_HEALTH_BOX_X = 20,
        PANEL_BOX_Y = (174-6),
        PANEL_HEALTH_XOFF = 2,
        PANEL_HEALTH_YOFF = 4,

    };

    enum
    {
        PANEL_FONT_G = 3636,
        PANEL_FONT_Y = 3646,
        PANEL_FONT_R = 3656,

        PANEL_SM_FONT_G = 3601,
        PANEL_SM_FONT_Y = 3613,
        PANEL_SM_FONT_R = 3625,

    };

    void DisplayPanelNumber(double xs, double ys, int number)
    {
        char buffer[32];
        char* ptr;
        double x;

        mysnprintf(buffer, 32, "%03d", number);

        for (ptr = buffer, x = xs; *ptr; ptr++)
        {
            if (!isdigit(*ptr))
            {
                continue;
            }
            int tex = PANEL_FONT_G + (*ptr - '0');
            DrawGraphic(tileGetTexture(tex), x, ys, DI_ITEM_LEFT_TOP, 1, -1, -1, 1, 1);
            x += tileWidth(tex) + 1;
        }
    }

    void DrawStatusbarHealth(int value)
    {
        double x = PANEL_HEALTH_BOX_X + PANEL_HEALTH_XOFF;
        double y = PANEL_BOX_Y + PANEL_HEALTH_YOFF;
        DisplayPanelNumber(x, y, value);
    }


    void DrawStatusBar()
    {
        auto pp = Player + screenpeek;
        USERp u = User[pp->PlayerSprite];
        BeginStatusBar(320, 200, tileHeight(STATUS_BAR));

        DrawGraphic(tileGetTexture(STATUS_BAR), 0, 200, DI_ITEM_LEFT_BOTTOM, 1, -1, -1, 1, 1);
        DrawStatusbarHealth(u->Health);
    }


    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------
public:
    void UpdateStatusBar(ClockTicks arg)
    {
        int nPalette = 0;

        if (gs.BorderNum <= BORDER_NONE) return;

        /*if (gs.BorderNum == BORDER_HUD)
        {
            DrawHUD2();
        }
        else*/ if (gs.BorderNum == BORDER_MINI_BAR)
        {
            //DrawHUD1(nPalette);
        }
        else
        {
            DrawStatusBar();
        }
    }

};


static void UpdateFrame(void)
{
	static const int kBackTile = 51;
    auto tex = tileGetTexture(kBackTile);

    twod->AddFlatFill(0, 0, xdim, windowxy1.y - 3, tex);
    twod->AddFlatFill(0, windowxy2.y + 4, xdim, ydim, tex);
    twod->AddFlatFill(0, windowxy1.y - 3, windowxy1.x - 3, windowxy2.y + 4, tex);
    twod->AddFlatFill(windowxy2.x + 4, windowxy1.y - 3, xdim, windowxy2.y + 4, tex);

    twod->AddFlatFill(windowxy1.x - 3, windowxy1.y - 3, windowxy1.x, windowxy2.y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.x, windowxy1.y - 3, windowxy2.x + 4, windowxy1.y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, windowxy2.x + 4, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.x - 3, windowxy2.y + 1, windowxy2.x + 1, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
}

void UpdateStatusBar(ClockTicks arg)
{
    DSWStatusBar sbar;

    if (gs.BorderNum >= BORDER_BAR)
    {
        UpdateFrame();
    }

    sbar.UpdateStatusBar(arg);
}



END_SW_NS
