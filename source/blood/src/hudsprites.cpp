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
#include "pragmas.h"
#include "mmulti.h"
#include "v_font.h"

#include "endgame.h"
#include "aistate.h"
#include "map2d.h"
#include "loadsave.h"
#include "sectorfx.h"
#include "choke.h"
#include "view.h"
#include "nnexts.h"
#include "zstring.h"
#include "menu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"
#include "glbackend/glbackend.h"

BEGIN_BLD_NS


static struct {
	short nTile;
	unsigned char nStat;
	unsigned char nPal;
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
	DrawTexture(&twodpsp, tileGetTexture(tile), x, y, DTA_FlipX, flipx, DTA_FlipY, flipy, DTA_TopLeft, true, DTA_FlipOffsets, true,
		DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ViewportX, gViewX0, DTA_ViewportY, gViewY0, DTA_Alpha, alpha,
		DTA_ViewportWidth, gViewX1 - gViewX0 + 1, DTA_ViewportHeight, gViewY1 - gViewY0 + 1, DTA_Pin, pin, DTA_ScaleX, scale, DTA_ScaleY, scale,
		DTA_TranslationIndex, TRANSLATION(Translation_Remap + basepal, 0), TAG_DONE);
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


void hudDraw(PLAYER *gView, int nSectnum, int defaultHoriz, int bobx, int boby, int zDelta, int basepal)
{
	if (gViewPos == 0)
	{
		if (cl_crosshair)
		{
			double crosshair_scale = cl_crosshairscale * .01;
			DrawTexture(twod, tileGetTexture(kCrosshairTile), 160, defaultHoriz,
				DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, crosshair_scale, DTA_ScaleY, crosshair_scale, DTA_CenterOffsetRel, true,
				DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1, TAG_DONE);
		}
		// This is dumb. Todo: Preserve full precision here!
		int cX = (bobx >> 8) + 160;
		int cY = (boby >> 8) + 220 + (zDelta >> 7);
		int nShade = sector[nSectnum].floorshade; 
		int nPalette = 0;
		if (sector[gView->pSprite->sectnum].extra > 0) {
			sectortype* pSector = &sector[gView->pSprite->sectnum];
			XSECTOR* pXSector = &xsector[pSector->extra];
			if (pXSector->color)
				nPalette = pSector->floorpal;
		}

		#ifdef NOONE_EXTENSIONS
		if (gView->sceneQav < 0) WeaponDraw(gView, nShade, cX, cY, nPalette, basepal);
			else if (gView->pXSprite->health > 0) playerQavSceneDraw(gView, nShade, cX, cY, nPalette, basepal);
		else {
			gView->sceneQav = gView->weaponQav = -1;
			gView->weaponTimer = gView->curWeapon = 0;
		}
		#else
			WeaponDraw(gView, nShade, cX, cY, nPalette, curbasepal);
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
		yxAspect = yxaspect;
		renderSetAspect(65536, 54613);
		r otatesprite(280 << 16, 35 << 16, 53248, 512, 4077, v10, v14, 512 + 6, gViewX0, gViewY0, gViewX1, gViewY1);
		r otatesprite(280 << 16, 35 << 16, 53248, 0, 1683, v10, 0, 512 + 35, gViewX0, gViewY0, gViewX1, gViewY1);
		renderSetAspect(viewingRange, yxAspect);
	}
#endif
}

END_BLD_NS
