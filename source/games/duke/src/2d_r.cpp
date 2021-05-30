//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

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
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.

#include "ns.h"
#include "duke3d.h"
#include "names_r.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
#include "c_dispatch.h"
#include "gamestate.h"
#include "gamefuncs.h"

BEGIN_DUKE_NS


//==========================================================================
//
// Sets up the game fonts.
// This is a duplicate of the _d function but needed since the tile numbers differ.
//
//==========================================================================

void InitFonts_r()
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
		auto SmallFont = new ::FFont("TileSmallFont", nullptr, nullptr, 0, 0, 0, -1, 10, false, false, false, &fontdata);
		SmallFont->SetKerning(2);
		fontdata.Clear();
	}

	// Big font

	if (!V_GetFont("TileBigFont"))
	{
		// This font is VERY messy...
		fontdata.Insert('_', tileGetTexture(BIGALPHANUM - 11));
		fontdata.Insert('-', tileGetTexture(BIGALPHANUM - 11));
		for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(BIGALPHANUM - 10 + i));
		for (int i = 0; i < 26; i++) fontdata.Insert('A' + i, tileGetTexture(BIGALPHANUM + i));
		fontdata.Insert('.', tileGetTexture(BIGPERIOD));
		fontdata.Insert(',', tileGetTexture(BIGCOMMA));
		fontdata.Insert('!', tileGetTexture(BIGX));
		fontdata.Insert('?', tileGetTexture(BIGQ));
		fontdata.Insert(';', tileGetTexture(BIGSEMI));
		fontdata.Insert(':', tileGetTexture(BIGCOLIN));
		fontdata.Insert('\\', tileGetTexture(BIGALPHANUM + 68));
		fontdata.Insert('/', tileGetTexture(BIGALPHANUM + 68));
		fontdata.Insert('%', tileGetTexture(BIGALPHANUM + 69));
		fontdata.Insert('`', tileGetTexture(BIGAPPOS));
		fontdata.Insert('"', tileGetTexture(BIGAPPOS));
		fontdata.Insert('\'', tileGetTexture(BIGAPPOS));
		GlyphSet::Iterator it(fontdata);
		GlyphSet::Pair* pair;
		while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
		auto BigFont = new ::FFont("TileBigFont", nullptr, nullptr, 0, 0, 0, -1, 10, false, false, false, &fontdata);
		BigFont->SetKerning(6);
		fontdata.Clear();
	}

	SmallFont2 = V_GetFont("SmallFont2");
	if (!SmallFont2)
	{
		// Tiny font
		for (int i = 0; i < 95; i++)
		{
			auto tile = tileGetTexture(MINIFONT + i);
			if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
				fontdata.Insert('!' + i, tile);
		}
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		SmallFont2 = new ::FFont("SmallFont2", nullptr, nullptr, 0, 0, 0, -1, 6, false, false, false, &fontdata);
		SmallFont2->SetKerning(2);
		fontdata.Clear();
	}

	IndexFont = V_GetFont("IndexFont");
	if (!IndexFont)
	{
		// SBAR index font
		for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THREEBYFIVE + i));
		fontdata.Insert(':', tileGetTexture(THREEBYFIVE + 10));
		fontdata.Insert('/', tileGetTexture(THREEBYFIVE + 11));
		fontdata.Insert('%', tileGetTexture(MINIFONT + '%' - '!'));
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

		fontdata.Clear();
	}

	DigiFont = V_GetFont("DigiFont");
	if (!DigiFont)
	{
		// digital font
		for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(DIGITALNUM + i));
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);
	}

	BigFont = V_GetFont("BIGFONT");
	SmallFont = V_GetFont("SMALLFONT");

}


END_DUKE_NS
