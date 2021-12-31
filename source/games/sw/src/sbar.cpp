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

#include "game.h"
#include "names.h"
#include "names2.h"
#include "panel.h"
#include "pal.h"
#include "misc.h"
#include "player.h"
#include "v_2ddrawer.h"
#include "statusbar.h"
#include "network.h"
#include "v_draw.h"
#include "menus.h"
#include "automap.h"


BEGIN_SW_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void UpdateFrame(void)
{
	static const int kBackTile = 51;
    auto tex = tileGetTexture(kBackTile);

    twod->AddFlatFill(0, 0, xdim, windowxy1.Y - 3, tex);
    twod->AddFlatFill(0, windowxy2.Y + 4, xdim, ydim, tex);
    twod->AddFlatFill(0, windowxy1.Y - 3, windowxy1.X - 3, windowxy2.Y + 4, tex);
    twod->AddFlatFill(windowxy2.X + 4, windowxy1.Y - 3, xdim, windowxy2.Y + 4, tex);

    twod->AddFlatFill(windowxy1.X - 3, windowxy1.Y - 3, windowxy1.X, windowxy2.Y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.X, windowxy1.Y - 3, windowxy2.X + 4, windowxy1.Y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.X + 1, windowxy1.Y, windowxy2.X + 4, windowxy2.Y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.X - 3, windowxy2.Y + 1, windowxy2.X + 1, windowxy2.Y + 4, tex, 0, 1, 0xff2a2a2a);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void UpdateStatusBar()
{
    if (hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }

    SummaryInfo info{};
    info.kills = Player[screenpeek].Kills;
    info.maxkills = TotalKillable;
    info.secrets = Player[screenpeek].SecretsFound;
    info.maxsecrets = LevelSecrets;
    info.time = Scale(PlayClock, 1000, 120);

    ::UpdateStatusBar(&info);


    PLAYER* pp = &Player[screenpeek];
    if (pp->cookieTime > 0)
    {
        const int MESSAGE_LINE = 142;    // Used to be 164

        if (hud_textfont || !SmallFont2->CanPrint(pp->cookieQuote))
        {
            int x = 320 - SmallFont->StringWidth(pp->cookieQuote) / 2;
            DrawText(twod, SmallFont, CR_UNTRANSLATED, x, MESSAGE_LINE*2, pp->cookieQuote, DTA_FullscreenScale, FSMode_Fit640x400,
                DTA_Alpha, clamp(pp->cookieTime / 60., 0., 1.), TAG_DONE);
        }
        else
        {
            int x = 160 - SmallFont2->StringWidth(pp->cookieQuote) / 2;
            DrawText(twod, SmallFont2, CR_UNTRANSLATED, x, MESSAGE_LINE, pp->cookieQuote, DTA_FullscreenScale, FSMode_Fit320x200,
                DTA_Alpha, clamp(pp->cookieTime / 60., 0., 1.), TAG_DONE);

        }
    }
}




END_SW_NS
