//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "build.h"
#include "v_font.h"

#include "blood.h"

#include "zstring.h"
#include "razemenu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "gamehud.h"

BEGIN_BLD_NS


static struct {
	short nTile;
	uint8_t nStat;
	uint8_t nPal;
	int nScale;
	short nX, nY;
} burnTable[9] = {
	 { 2101, 2, 0, 118784, 10, 220 },
	 { 2101, 2, 0, 110592, 40, 220 },
	 { 2101, 2, 0, 81920, 85, 220 },
	 { 2101, 2, 0, 69632, 120, 220 },
	 { 2101, 2, 0, 61440, 160, 220 },
	 { 2101, 2, 0, 73728, 200, 220 },
	 { 2101, 2, 0, 77824, 235, 220 },
	 { 2101, 2, 0, 110592, 275, 220 },
	 { 2101, 2, 0, 122880, 310, 220 }
};


static void drawElement(int x, int y, int tile, double scale = 1, int flipx = 0, int flipy = 0, int pin = 0, int basepal = 0, double alpha = 1)
{
	int flags = RS_TOPLEFT;
	if (flipx) flags |= RS_XFLIPHUD;
	if (flipy) flags |= RS_YFLIPHUD;
	if (pin == -1) flags |= RS_ALIGN_L;
	else if (pin == 1) flags |= RS_ALIGN_R;
	hud_drawsprite(x, y, int(scale*65536), 0, tile, 0, basepal, flags, alpha);
}


static void viewBurnTime(int gScale)
{
	if (!gScale) return;

	for (int i = 0; i < 9; i++)
	{
		int nTile = burnTable[i].nTile + qanimateoffs(burnTable[i].nTile, 32768 + i);
		int nScale = burnTable[i].nScale;
		if (gScale < 600)
		{
			nScale = scale(nScale, gScale, 600);
		}
		drawElement(burnTable[i].nX, burnTable[i].nY, nTile, nScale / 65536.);
	}
}


void hudDraw(PLAYER *gView, int nSectnum, double bobx, double boby, double zDelta, int basepal, double smoothratio)
{
	double look_anghalf = gView->angle.look_anghalf(smoothratio);

	DrawCrosshair(kCrosshairTile, gView->pXSprite->health >> 4, -look_anghalf, 0, 2);

	if (gViewPos == 0)
	{
		double looking_arc = gView->angle.looking_arc(smoothratio);

		double cX = 160 - look_anghalf;
		double cY = 220 + looking_arc;
		if (cl_weaponsway)
		{
			if (cl_hudinterpolation)
			{
				cX += (bobx / 256.);
				cY += (boby / 256.) + (zDelta / 128.);
			}
			else
			{
				cX += (int(bobx) >> 8);
				cY += (int(boby) >> 8) + (int(zDelta) >> 7);
			}
		}
		else
		{
			cY += (-2048. / 128.);
		}
		int nShade = sector[nSectnum].floorshade; 
		int nPalette = 0;
		if (sector[gView->pSprite->sectnum].extra > 0) {
			sectortype* pSector = &sector[gView->pSprite->sectnum];
			XSECTOR* pXSector = &xsector[pSector->extra];
			if (pXSector->color)
				nPalette = pSector->floorpal;
		}

		#ifdef NOONE_EXTENSIONS
		if (gView->sceneQav < 0) WeaponDraw(gView, nShade, cX, cY, nPalette, smoothratio);
			else if (gView->pXSprite->health > 0) playerQavSceneDraw(gView, nShade, cX, cY, nPalette, smoothratio);
		else {
			gView->sceneQav = gView->weaponQav = -1;
			gView->weaponTimer = gView->curWeapon = 0;
		}
		#else
			WeaponDraw(gView, nShade, cX, cY, nPalette, smoothratio);
		#endif
	}
	if (gViewPos == 0 && gView->pXSprite->burnTime > 60)
	{
		viewBurnTime(gView->pXSprite->burnTime);
	}
	if (packItemActive(gView, 1))
	{
		drawElement(0, 0, 2344, 1, 0, 0, -1);
		drawElement(320, 0, 2344, 1, 1, 0, 1);
		drawElement(0, 200, 2344, 1, 0, 1, -1);
		drawElement(320, 200, 2344, 1, 1, 1, 1);
		if (gDetail >= 4)
		{
			drawElement(15, 3, 2346, 1, 0, 0, -1, 0, 0.2);
			drawElement(212, 77, 2347, 1, 0, 0, 1, 0, 0.2);
		}
	}
	if (powerupCheck(gView, kPwUpAsbestArmor) > 0)
	{
		drawElement(0, 237, 2358, 1, 0, 1, -1);
		drawElement(320, 237, 2358, 1, 1, 1, 1);
	}

#if 0 // This currently does not work. May have to be redone as a hardware effect.
	if (v4 && gNetPlayers > 1)
	{
		DoLensEffect();
		viewingRange = viewingrange;
		r otatesprite(IntToFixed(280), IntToFixed(35), 53248, 512, 4077, v10, v14, 512 + 6, gViewX0, gViewY0, gViewX1, gViewY1);
		r otatesprite(IntToFixed(280), IntToFixed(35), 53248, 0, 1683, v10, 0, 512 + 35, gViewX0, gViewY0, gViewX1, gViewY1);
	}
#endif
}

END_BLD_NS
