//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "demo.h"
#include "screens.h"
#include "baselayer.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"

BEGIN_DUKE_NS

FFont* IndexFont;
FFont* DigiFont;

//---------------------------------------------------------------------------
//
// game specific command line args go here. 
//
//---------------------------------------------------------------------------

void checkcommandline()
{
	auto val = Args->CheckValue("-skill");
	if (!val) val = Args->CheckValue("-s");
	if (val)
	{
		ud.m_player_skill = ud.player_skill = clamp((int)strtol(val, nullptr, 0), 0, 5);
		if (ud.m_player_skill == 4) ud.m_respawn_monsters = ud.respawn_monsters = 1;
	}
	val = Args->CheckValue("-respawn");
	if (!val) val = Args->CheckValue("-t");
	if (val)
	{
		if (*val == '1') ud.m_respawn_monsters = 1;
		else if (*val == '2') ud.m_respawn_items = 1;
		else if (*val == '3') ud.m_respawn_inventory = 1;
		else
		{
			ud.m_respawn_monsters = 1;
			ud.m_respawn_items = 1;
			ud.m_respawn_inventory = 1;
		}
		Printf("Respawn on.\n");
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void genspriteremaps(void)
{
    int j;

	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
           return;

    j = lookups.loadTable(fr);

    if (j < 0)
    {
        if (j == -1)
            Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

        return;
    }

    uint8_t paldata[768];

    for (j=1; j<=5; j++)
    {
		if (fr.Read(paldata, 768) != 768)
			return;

        for (int k = 0; k < 768; k++) // Build uses 6 bit VGA palettes.
            paldata[k] = (paldata[k] << 2) | (paldata[k] >> 6);

        paletteSetColorTable(j, paldata, j == DREALMSPAL || j == ENDINGPAL, j < DREALMSPAL);
    }

    for (int i = 0; i < 256; i++)
    {
        // swap red and blue channels.
        paldata[i * 3] = GPalette.BaseColors[i].b;
        paldata[i * 3+1] = GPalette.BaseColors[i].g;
        paldata[i * 3+2] = GPalette.BaseColors[i].r;
    }
    paletteSetColorTable(DRUGPAL, paldata, false, false); // todo: implement this as a shader effect (swap R and B in postprocessing.)

    if (isRR())
    {
        uint8_t table[256];
        for (j = 0; j < 256; j++)
            table[j] = j;
        for (j = 0; j < 32; j++)
            table[j] = j + 32;

        lookups.makeTable(7, table, 0, 0, 0, 0);

        for (j = 0; j < 256; j++)
            table[j] = j;
        lookups.makeTable(30, table, 0, 0, 0, 0);
        lookups.makeTable(31, table, 0, 0, 0, 0);
        lookups.makeTable(32, table, 0, 0, 0, 0);
        lookups.makeTable(33, table, 0, 0, 0, 0);
        if (isRRRA())
            lookups.makeTable(105, table, 0, 0, 0, 0);

        int unk = 63;
        for (j = 64; j < 80; j++)
        {
            unk--;
            table[j] = unk;
            table[j + 16] = j - 24;
        }
        table[80] = 80;
        table[81] = 81;
        for (j = 0; j < 32; j++)
        {
            table[j] = j + 32;
        }
        lookups.makeTable(34, table, 0, 0, 0, 0);
        for (j = 0; j < 256; j++)
            table[j] = j;
        for (j = 0; j < 16; j++)
            table[j] = j + 129;
        for (j = 16; j < 32; j++)
            table[j] = j + 192;
        lookups.makeTable(35, table, 0, 0, 0, 0);
        if (isRRRA())
        {
            lookups.makeTable(50, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
            lookups.makeTable(51, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
            lookups.makeTable(54, lookups.getTable(8), 32 * 4, 32 * 4, 32 * 4, 0);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void FTA(int q, struct player_struct* p)
{
    if (hud_messages == 0 || q < 0 || !(p->gm & MODE_GAME))
        return;

    if (p->ftq != q)
    {
        if (q == 13) p->ftq = q;
        auto qu = quoteMgr.GetQuote(q);
        if (p == g_player[screenpeek].ps && qu[0] != '\0')
        {
            if (q >= 70 && q <= 72 && hud_messages == 2)
            {
                // Todo: redirect this to a centered message (these are "need a key" messages)
            }
            else
            {
                int printlevel = hud_messages == 1 ? PRINT_MEDIUM : PRINT_MEDIUM | PRINT_NOTIFY;
                Printf(printlevel, "%s\n", qu);
            }
        }
    }
}

//==========================================================================
//
// Draws the background
// todo: split up to have dedicated functions for both cases.
//
//==========================================================================

void drawbackground(void)
{
    if ((g_player[myconnectindex].ps->gm & MODE_GAME) == 0 && ud.recstat != 2)
    {
        twod->ClearScreen();
        auto tex = tileGetTexture(TILE_MENUSCREEN);
        PalEntry color = 0xff808080;
        if (!hud_bgstretch)
            DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_Color, color, TAG_DONE);
        else
            DrawTexture(twod, tex, 0, 0, DTA_VirtualWidth, twod->GetWidth(), DTA_VirtualHeight, twod->GetHeight(), DTA_KeepRatio, true, DTA_Color, color, TAG_DONE);
        return;
    }

    auto tex = tileGetTexture(isRRRA() ? /*TILE_RRTILE*/7629 : TILE_BIGHOLE);
    if (tex != nullptr && tex->isValid())
    {
        if (windowxy1.y > 0)
        {
            twod->AddFlatFill(0, 0, twod->GetWidth(), windowxy1.y, tex, false, 1);
        }
        if (windowxy2.y + 1 < twod->GetHeight())
        {
            twod->AddFlatFill(0, windowxy2.y + 1, twod->GetWidth(), twod->GetHeight(), tex, false, 1);
        }
        if (windowxy1.x > 0)
        {
            twod->AddFlatFill(0, windowxy1.y, windowxy1.x, windowxy2.y + 1, tex, false, 1);
        }
        if (windowxy2.x + 1 < twod->GetWidth())
        {
            twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, twod->GetWidth(), windowxy2.y + 1, tex, false, 1);
        }
        auto vb = tileGetTexture(TILE_VIEWBORDER);
        auto ve = tileGetTexture(TILE_VIEWBORDER + 1);
        int x1 = windowxy1.x - 4;
        int y1 = windowxy1.y - 4;
        int x2 = windowxy2.x + 5;
        int y2 = windowxy2.y + 5;
        twod->AddFlatFill(x1, y1, x2, y1 + 4, vb, 5);
        twod->AddFlatFill(x1, y2 - 4, x2, y2, vb, 6);
        twod->AddFlatFill(x1, y1, x1 + 4, y2, vb, 1);
        twod->AddFlatFill(x2 - 4, y1, x2, y2, vb, 3);
        twod->AddFlatFill(x1, y1, x1 + 4, y1 + 4, ve, 1);
        twod->AddFlatFill(x2 - 4, y1, x2, y1 + 4, ve, 3);
        twod->AddFlatFill(x1, y2 - 4, x1 + 4, y2, ve, 2);
        twod->AddFlatFill(x2 - 4, y2 - 4, x2, y2, ve, 4);
    }
    else
    {
        // If we got no frame just clear the screen.
        twod->ClearScreen();
    }
}

END_DUKE_NS

