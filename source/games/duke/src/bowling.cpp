//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_r.h"
#include "dukeactor.h"
#include "buildtiles.h"
#include "texturemanager.h"

BEGIN_DUKE_NS

//==========================================================================
//
//
//==========================================================================

void tileCopySection(FTextureID tilenum1, int sx1, int sy1, int xsiz, int ysiz, uint8_t* p2, int xsiz2, int ysiz2, int sx2, int sy2)
{
	auto tex = TexMan.GetGameTexture(tilenum1);
	int xsiz1 = tex->GetTexelWidth();
	int ysiz1 = tex->GetTexelHeight();
	if (xsiz1 > 0 && ysiz1 > 0)
	{
		auto p1 = GetRawPixels(tilenum1);
		if (!p1) return;

		int x1 = sx1;
		int x2 = sx2;
		for (int i = 0; i < xsiz; i++)
		{
			int y1 = sy1;
			int y2 = sy2;
			for (int j = 0; j < ysiz; j++)
			{
				if (x2 >= 0 && y2 >= 0 && x2 < xsiz2 && y2 < ysiz2)
				{
					auto src = p1[x1 * ysiz1 + y1];
					if (src != TRANSPARENT_INDEX)
						p2[x2 * ysiz2 + y2] = src;
				}

				y1++;
				y2++;
				if (y1 >= ysiz1) y1 = 0;
			}
			x1++;
			x2++;
			if (x1 >= xsiz1) x1 = 0;
		}
	}

}


void updatepindisplay(int tag, int pins)
{
	if (tag < 1 || tag > 4) return;

	static const char* lanepics[] = { "BOWLINGLANE1", "BOWLINGLANE2", "BOWLINGLANE3", "BOWLINGLANE4" };
	auto texidbg = TexMan.CheckForTexture("LANEPICBG", ETextureType::Any);
	auto texidlite = TexMan.CheckForTexture("LANEPICS", ETextureType::Any);
	auto texidwork = TexMan.CheckForTexture(lanepics[tag-1], ETextureType::Any);
	static const uint8_t pinx[] = { 64, 56, 72, 48, 64, 80, 40, 56, 72, 88 };
	static const uint8_t piny[] = { 48, 40, 40, 32, 32, 32, 24, 24, 24, 24 };

	auto pixels = GetWritablePixels(texidwork);
	if (pixels)
	{
		auto tex = TexMan.GetGameTexture(texidwork);
		tileCopySection(texidbg,  0, 0, 128, 64, pixels, tex->GetTexelWidth(), tex->GetTexelHeight(), 0, 0);
		for (int i = 0; i < 10; i++) if (pins & (1 << i))
			tileCopySection(texidlite, 0, 0, 8, 8, pixels, tex->GetTexelWidth(), tex->GetTexelHeight(), pinx[i] - 4, piny[i] - 10);
	}
}

void resetlanepics(void)
{
	if (!isRR()) return;
	for (int tag = 0; tag < 4; tag++)
	{
		int pic = tag + 1;
		if (pic == 0) continue;
		updatepindisplay(pic, 0xffff);
	}
}

END_DUKE_NS

