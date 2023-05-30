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


static const struct {
	int nScale;
	int16_t nX, nY;
} burnTable[9] = {
	 { 118784, 10, 220 },
	 { 110592, 40, 220 },
	 { 81920, 85, 220 },
	 { 69632, 120, 220 },
	 { 61440, 160, 220 },
	 { 73728, 200, 220 },
	 { 77824, 235, 220 },
	 { 110592, 275, 220 },
	 { 122880, 310, 220 }
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void drawElement(int x, int y, FTextureID tile, double scale = 1, int flipx = 0, int flipy = 0, int pin = 0, int basepal = 0, double alpha = 1)
{
	int flags = RS_TOPLEFT;
	if (flipx) flags |= RS_XFLIPHUD;
	if (flipy) flags |= RS_YFLIPHUD;
	if (pin == -1) flags |= RS_ALIGN_L;
	else if (pin == 1) flags |= RS_ALIGN_R;
	hud_drawsprite(x, y, scale, 0, tile, 0, basepal, flags, alpha);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void viewBurnTime(int gScale)
{
	if (!gScale) return;

	for (int i = 0; i < 9; i++)
	{
		int nScale = burnTable[i].nScale;
		if (gScale < 600)
		{
			nScale = Scale(nScale, gScale, 600);
		}
		hud_drawsprite(burnTable[i].nX, burnTable[i].nY, nScale / 65536., 0, aTexIds[kTexTORCHEFFECT], 0, 0, RS_STRETCH, 1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void hudDraw(PLAYER* pPlayer, sectortype* pSector, double bobx, double boby, double zDelta, DAngle angle, int basepal, double interpfrac)
{
	if (gViewPos == 0)
	{
		// Nullify incoming roll angle for now as it doesn't draw weapons made up of parts correctly.
		angle = nullAngle;

		auto cXY = DVector2(160, 220) + pPlayer->Angles.getWeaponOffsets(interpfrac).first;

		if (cl_weaponsway)
		{
			if (cl_hudinterpolation)
			{
				cXY.X += bobx;
				cXY.Y += boby + (zDelta * 2.);
			}
			else
			{
				cXY.X += int(bobx);
				cXY.Y += int(boby) + int(zDelta * 2);
			}
		}
		else
		{
			cXY.Y += (-2048. / 128.);
		}
		int nShade = pSector? pSector->floorshade : 0;
		int nPalette = 0;
		if (pPlayer->actor->sector()->hasX()) {
			sectortype* pViewSect = pPlayer->actor->sector();
			XSECTOR* pXSector = &pViewSect->xs();
			if (pXSector->color)
				nPalette = pViewSect->floorpal;
		}

		#ifdef NOONE_EXTENSIONS
		if (pPlayer->sceneQav < 0) WeaponDraw(pPlayer, nShade, cXY.X, cXY.Y, nPalette, angle);
			else if (pPlayer->actor->xspr.health > 0) playerQavSceneDraw(pPlayer, nShade, cXY.X, cXY.Y, nPalette, angle);
		else {
			pPlayer->sceneQav = pPlayer->weaponQav = kQAVNone;
			pPlayer->qavTimer = pPlayer->weaponTimer = pPlayer->curWeapon = 0;
		}
		#else
			WeaponDraw(pPlayer, nShade, cX, cY, nPalette);
		#endif
	}
	if (gViewPos == 0 && pPlayer->actor->xspr.burnTime > 60)
	{
		viewBurnTime(pPlayer->actor->xspr.burnTime);
	}
	if (packItemActive(pPlayer, 1))
	{
		drawElement(0, 0, aTexIds[kTexDIVEHUD], 1, 0, 0, -1);
		drawElement(320, 0, aTexIds[kTexDIVEHUD], 1, 1, 0, 1);
		drawElement(0, 200, aTexIds[kTexDIVEHUD], 1, 0, 1, -1);
		drawElement(320, 200, aTexIds[kTexDIVEHUD], 1, 1, 1, 1);
		if (gDetail >= 4)
		{
			drawElement(15, 3, aTexIds[kTexDIVEDETAIL1], 1, 0, 0, -1, 0, 0.2);
			drawElement(212, 77, aTexIds[kTexDIVEDETAIL2], 1, 0, 0, 1, 0, 0.2);
		}
	}
	if (powerupCheck(pPlayer, kPwUpAsbestArmor) > 0)
	{
		drawElement(0, 237, aTexIds[kTexASBESTHUD], 1, 0, 1, -1);
		drawElement(320, 237, aTexIds[kTexASBESTHUD], 1, 1, 1, 1);
	}

	int zn = int(((pPlayer->zWeapon - pPlayer->zView - 12) * 2.) + 220);
	PLAYER* pPSprite = &gPlayer[pPlayer->actor->spr.type - kDudePlayer1];
	if (pPlayer->actor->IsPlayerActor() && pPSprite->hand == 1)
	{
		gChoke.animateChoke(160, zn, interpfrac);
	}
}

END_BLD_NS
