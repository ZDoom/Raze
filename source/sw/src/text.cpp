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
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    fontdata.Clear();

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
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    fontdata.Clear();

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

    BigFont = new ::FFont("BigFont", nullptr, "defbigfont", 0, 0, 0, -1, 10, false, false, false, &fontdata);
    BigFont->SetKerning(1);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void MNU_DrawStringLarge(int x, int y, const char* string, int shade, int align)
{
    if (align > -1)
    {
        int w = BigFont->StringWidth(string);
        if (align == 0) x -= w / 2;
        else x -= w;
    }

    DrawText(twod, BigFont, CR_UNDEFINED, x, y, string, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, 
        DTA_Color, shadeToLight(shade), TAG_DONE);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void MNU_DrawString(int x, int y, const char* string, int shade, int pal, int align)
{
    if (align > -1)
    {
        int w = SmallFont->StringWidth(string);
        if (align == 0) x -= w / 2;
        else x -= w;
    }
    DrawText(twod, SmallFont, CR_UNDEFINED, x, y, string, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
        DTA_Color, shadeToLight(shade), DTA_TranslationIndex, TRANSLATION(Translation_Remap, pal), TAG_DONE);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void MNU_DrawSmallString(int x, int y, const char* string, int shade, int pal, int align, double alpha)
{
    if (align > -1)
    {
        int w = SmallFont2->StringWidth(string);
        if (align == 0) x -= w / 2;
        else x -= w;
    }
    DrawText(twod, SmallFont2, CR_UNDEFINED, x, y, string, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
        DTA_Color, shadeToLight(shade), DTA_TranslationIndex, TRANSLATION(Translation_Remap, pal), DTA_Alpha, alpha, TAG_DONE);

}

//---------------------------------------------------------------------------
//
// Notification messages. Native SW-style display should later be
// provided by the backend.
//
//---------------------------------------------------------------------------

void PutStringInfo(PLAYERp pp, const char *string)
{
    if (pp-Player == myconnectindex && hud_messages)
        Printf(PRINT_MEDIUM|PRINT_NOTIFY, "%s\n", string); // Put it in the console too
}


#if 0 // kept as a reminder to reimplement a 'native' looking display option in the backend
void PutStringInfoLine(PLAYERp pp, const char *string)
{
    short GlobInfoStringTime = TEXT_INFO_TIME;

    short x,y;
    short w,h;

    if (pp-Player != myconnectindex)
        return;

    x = 160;
    y = TEXT_INFO_LINE(0);

    PutStringTimer(pp, x, y, string, GlobInfoStringTime);

}
#endif

END_SW_NS
