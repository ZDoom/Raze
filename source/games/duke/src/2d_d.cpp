//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.

#include "ns.h"
#include "duke3d.h"
#include "names_d.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
#include "buildtiles.h"
#include "mapinfo.h"
#include "c_dispatch.h"
#include "gamestate.h"
#include "gamefuncs.h"

BEGIN_DUKE_NS

//==========================================================================
//
// Sets up the game fonts.
//
//==========================================================================

void InitFonts_d()
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
				if (i >= 'a' && i <= 'z' && tileEqualTo(i, i - 32)) continue;
				fontdata.Insert('!' + i, tile);
				tile->SetOffsetsNotForFont();
			}
		}
		new ::FFont("TileSmallFont", nullptr, nullptr, 0, 0, 0, -1, 5, false, false, false, &fontdata);
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
		// The texture offsets in this font are useless for font printing. This should only apply to these glyphs, not for international extensions, though.
		GlyphSet::Iterator it(fontdata);
		GlyphSet::Pair* pair;
		while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
		new ::FFont("TileBigFont", nullptr, nullptr, 0, 0, 0, -1, 5, false, false, false, &fontdata);
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
			{
				if (i >= 'a' && i <= 'z' && tileEqualTo(i, i - 32)) continue;
				fontdata.Insert('!' + i, tile);
				tile->SetOffsetsNotForFont();
			}
		}
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		SmallFont2 = new ::FFont("SmallFont2", nullptr, nullptr, 0, 0, 0, -1, 3, false, false, false, &fontdata);
		SmallFont2->SetKerning(1);
		fontdata.Clear();
	}

	IndexFont = V_GetFont("IndexFont");
	if (!IndexFont)
	{
		// SBAR index font
		for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THREEBYFIVE0 + i));
		fontdata.Insert(':', tileGetTexture(THREEBYFIVE0 + 10));
		fontdata.Insert('/', tileGetTexture(THREEBYFIVE0 + 11));
		fontdata.Insert('%', tileGetTexture(MINIFONT + '%' - '!'));
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		GlyphSet::Iterator iti(fontdata);
		GlyphSet::Pair* pair;
		while (iti.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
		IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);
		fontdata.Clear();
	}

	DigiFont = V_GetFont("DigiFont");
	if (!DigiFont)
	{
		// digital font
		for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(DIGITALNUM + i));
		fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
		GlyphSet::Iterator itd(fontdata);
		GlyphSet::Pair* pair;
		while (itd.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
		DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);
	}

	// Todo: virtualize this;
	BigFont = V_GetFont("BIGFONT");
	SmallFont = V_GetFont("SMALLFONT");
}


END_DUKE_NS
