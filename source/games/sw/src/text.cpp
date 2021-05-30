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

#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "game.h"
#include "pal.h"
#include "misc.h"
#include "menus.h"

#include "network.h"
#include "v_font.h"
#include "v_draw.h"

BEGIN_SW_NS

//==========================================================================
//
// Sets up the game fonts.
//
//==========================================================================

void InitFonts()
{
    GlyphSet fontdata;

    if (!V_GetFont("TileSmallFont"))
    {
        // Small font
        for (int i = 0; i < 95; i++)
        {
            auto tile = tileGetTexture(STARTALPHANUM + i);
            if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            {
                fontdata.Insert('!' + i, tile);
                tile->SetOffsetsNotForFont();
            }
        }
        new ::FFont("TileSmallFont", nullptr, nullptr, 0, 0, 0, -1, 4, false, false, false, &fontdata);
        fontdata.Clear();
    }

    SmallFont2 = V_GetFont("SmallFont2");
    if (!SmallFont2)
    {

        // Tiny font
        for (int i = 0; i < 95; i++)
        {
            auto tile = tileGetTexture(2930 + i);
            if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            {
                fontdata.Insert('!' + i, tile);
                tile->SetOffsetsNotForFont();
            }
        }
        SmallFont2 = new ::FFont("SmallFont2", nullptr, nullptr, 0, 0, 0, -1, 4, false, false, false, &fontdata);
        fontdata.Clear();
    }

    if (!V_GetFont("TileBigFont"))
    {

        const int FONT_LARGE_ALPHA = 3706;
        const int FONT_LARGE_DIGIT = 3732;

        // Big 
        for (int i = 0; i < 10; i++)
        {
            auto tile = tileGetTexture(FONT_LARGE_DIGIT + i);
            if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            {
                fontdata.Insert('0' + i, tile);
                tile->SetOffsetsNotForFont();
            }
        }
        for (int i = 0; i < 26; i++)
        {
            auto tile = tileGetTexture(FONT_LARGE_ALPHA + i);
            if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            {
                fontdata.Insert('A' + i, tile);
                tile->SetOffsetsNotForFont();
            }
        }

        auto BigFont = new ::FFont("TileBigFont", nullptr, nullptr, 0, 0, 0, -1, 10, false, false, false, &fontdata);
        BigFont->SetKerning(1);
    }

    BigFont = V_GetFont("BIGFONT");
    SmallFont = V_GetFont("SMALLFONT");

}

//---------------------------------------------------------------------------
//
// Notification messages. Native SW-style display should later be
// provided by the backend.
//
//---------------------------------------------------------------------------

void PutStringInfo(PLAYERp pp, const char *string)
{
    if (pp-Player == myconnectindex)
        Printf(PRINT_MEDIUM|PRINT_NOTIFY, "%s\n", string); // Put it in the console too
}

END_SW_NS
