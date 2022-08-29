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
#include "v_font.h"
#include "hw_voxels.h"
#include "gamefuncs.h"

BEGIN_BLD_NS

static DAngle gCameraAng;
int dword_172CE0[16][3];

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void RotateYZ(int*, int* pY, int* pZ, int ang)
{
	int oY, oZ, angSin, angCos;
	oY = *pY;
	oZ = *pZ;
	angSin = Sin(ang);
	angCos = Cos(ang);
	*pY = dmulscale30r(oY, angCos, oZ, -angSin);
	*pZ = dmulscale30r(oY, angSin, oZ, angCos);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void RotateXZ(int* pX, int*, int* pZ, int ang)
{
	int oX, oZ, angSin, angCos;
	oX = *pX;
	oZ = *pZ;
	angSin = Sin(ang);
	angCos = Cos(ang);
	*pX = dmulscale30r(oX, angCos, oZ, -angSin);
	*pZ = dmulscale30r(oX, angSin, oZ, angCos);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

tspritetype* viewInsertTSprite(tspriteArray& tsprites, sectortype* pSector, int nStatnum, tspritetype const* const parentTSprite)
{
	tspritetype* pTSprite = tsprites.newTSprite();
	memset(pTSprite, 0, sizeof(tspritetype));
	pTSprite->cstat = CSTAT_SPRITE_YCENTER;
	pTSprite->xrepeat = 64;
	pTSprite->yrepeat = 64;
	pTSprite->ownerActor = nullptr;
	pTSprite->type = -int(tsprites.Size() - 1);
	pTSprite->statnum = nStatnum;
	pTSprite->sectp = pSector;

	DVector3 pos = { 0,0,0 };
	if (parentTSprite)
	{
		pos = parentTSprite->pos;
		pTSprite->ownerActor = parentTSprite->ownerActor;
		pTSprite->angle = parentTSprite->angle;
	}
	pos.X += gCameraAng.Cos() * 2;
	pos.Y += gCameraAng.Sin() * 2;
	pTSprite->pos = pos;
	return pTSprite;
}

static const int effectDetail[kViewEffectMax] = {
	4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};


struct WEAPONICON {
	int16_t nTile;
	uint8_t zOffset;
};

static const WEAPONICON gWeaponIcon[] = {
	{ -1, 0 },
	{ -1, 0 }, // 1: pitchfork
	{ 524, 6 }, // 2: flare gun
	{ 559, 6 }, // 3: shotgun
	{ 558, 8 }, // 4: tommy gun
	{ 526, 6 }, // 5: napalm launcher
	{ 589, 11 }, // 6: dynamite
	{ 618, 11 }, // 7: spray can
	{ 539, 6 }, // 8: tesla gun
	{ 800, 0 }, // 9: life leech
	{ 525, 11 }, // 10: voodoo doll
	{ 811, 11 }, // 11: proxy bomb
	{ 810, 11 }, // 12: remote bomb
	{ -1, 0 },
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static tspritetype* viewAddEffect(tspriteArray& tsprites, int nTSprite, VIEW_EFFECT nViewEffect)
{
	assert(nViewEffect >= 0 && nViewEffect < kViewEffectMax);
	auto pTSprite = tsprites.get(nTSprite);
	auto owneractor = static_cast<DBloodActor*>(pTSprite->ownerActor);
	if (gDetail < effectDetail[nViewEffect]) return NULL;
	switch (nViewEffect)
	{
	case kViewEffectSpotProgress: {
		int perc = (100 * owneractor->xspr.data3) / kMaxPatrolSpotValue;
		int width = (94 * owneractor->xspr.data3) / kMaxPatrolSpotValue;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);

		auto pNSprite2 = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite2)
			break;

		pNSprite2->picnum = 2203;

		pNSprite2->xrepeat = width;
		pNSprite2->yrepeat = 20;
		pNSprite2->pal = 10;
		if (perc >= 75) pNSprite2->pal = 0;
		else if (perc >= 50) pNSprite2->pal = 6;

		pNSprite2->set_int_z(top - 2048);
		pNSprite2->shade = -128;
		break;
	}
	case kViewEffectAtom:
		for (int i = 0; i < 16; i++)
		{
			auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
			if (!pNSprite)
				break;

			int ang = (PlayClock * 2048) / 120;
			int nRand1 = dword_172CE0[i][0];
			int nRand2 = dword_172CE0[i][1];
			int nRand3 = dword_172CE0[i][2];
			ang += nRand3;
			int x = MulScale(512, Cos(ang), 30);
			int y = MulScale(512, Sin(ang), 30);
			int z = 0;
			RotateYZ(&x, &y, &z, nRand1);
			RotateXZ(&x, &y, &z, nRand2);
			pNSprite->set_int_pos({ pTSprite->int_pos().X + x, pTSprite->int_pos().Y + y, pTSprite->int_pos().Z + (z << 4) });
			pNSprite->picnum = 1720;
			pNSprite->shade = -128;
		}
		break;
	case kViewEffectFlag:
	case kViewEffectBigFlag:
	{
		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pal = 0;
		pNSprite->set_int_z(top);
		if (nViewEffect == kViewEffectFlag)
			pNSprite->xrepeat = pNSprite->yrepeat = 24;
		else
			pNSprite->xrepeat = pNSprite->yrepeat = 64;
		pNSprite->picnum = 3558;
		return pNSprite;
	}
	case kViewEffectTesla:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->pos.Z = pTSprite->pos.Z;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->shade = -128;
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = pTSprite->yrepeat;
		pNSprite->picnum = 2135;
		break;
	}
	case kViewEffectShoot:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pal = 0;
		pNSprite->xrepeat = pNSprite->yrepeat = 64;
		pNSprite->picnum = 2605;
		return pNSprite;
	}
	case kViewEffectReflectiveBall:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = 26;
		pNSprite->pal = 0;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->xrepeat = pNSprite->yrepeat = 64;
		pNSprite->picnum = 2089;
		break;
	}
	case kViewEffectPhase:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->shade = 26;
		pNSprite->pal = 0;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->xrepeat = pNSprite->yrepeat = 24;
		pNSprite->picnum = 626;
		pNSprite->set_int_z(top);
		break;
	}
	case kViewEffectTrail:
	{
		int nAng = pTSprite->int_ang();
		if (pTSprite->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)
		{
			nAng = (nAng + 512) & 2047;
		}
		else
		{
			nAng = (nAng + 1024) & 2047;
		}
		for (int i = 0; i < 5; i++)
		{
			auto pSector = pTSprite->sectp;
			auto pNSprite = viewInsertTSprite(tsprites, pSector, 32767, NULL);
			if (!pNSprite)
				break;

			int nLen = 128 + (i << 7);
			int x = MulScale(nLen, Cos(nAng), 30);
			int y = MulScale(nLen, Sin(nAng), 30);
			pNSprite->set_int_pos({ pTSprite->int_pos().X + x, pTSprite->int_pos().Y + y, pTSprite->int_pos().Z });
			assert(pSector);
			auto pSector2 = pSector;
			updatesectorz(pNSprite->pos, &pSector2);
			if (pSector2) pSector = pSector2;
			pNSprite->sectp = pSector;
			pNSprite->ownerActor = pTSprite->ownerActor;
			pNSprite->picnum = pTSprite->picnum;
			pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
			if (i < 2)
				pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
			pNSprite->shade = ClipLow(pTSprite->shade - 16, -128);
			pNSprite->xrepeat = pTSprite->xrepeat;
			pNSprite->yrepeat = pTSprite->yrepeat;
			pNSprite->picnum = pTSprite->picnum;
		}
		break;
	}
	case kViewEffectFlame:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pos.Z = pTSprite->pos.Z;
		pNSprite->picnum = 908;
		pNSprite->statnum = kStatDecoration;
		pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum) * pTSprite->xrepeat) / 64;
		break;
	}
	case kViewEffectSmokeHigh:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->set_int_z(top);
		if (IsDudeSprite(pTSprite))
			pNSprite->picnum = 672;
		else
			pNSprite->picnum = 754;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->shade = 8;
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = pTSprite->yrepeat;
		break;
	}
	case kViewEffectSmokeLow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->set_int_z(bottom);
		if (pTSprite->type >= kDudeBase && pTSprite->type < kDudeMax)
			pNSprite->picnum = 672;
		else
			pNSprite->picnum = 754;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->shade = 8;
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = pTSprite->yrepeat;
		break;
	}
	case kViewEffectTorchHigh:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->set_int_z(top);
		pNSprite->picnum = 2101;
		pNSprite->shade = -128;
		pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum) * pTSprite->xrepeat) / 32;
		break;
	}
	case kViewEffectTorchLow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		int top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->set_int_z(bottom);
		pNSprite->picnum = 2101;
		pNSprite->shade = -128;
		pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum) * pTSprite->xrepeat) / 32;
		break;
	}
	case kViewEffectShadow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;
		pNSprite->set_int_z(getflorzofslopeptr(pTSprite->sectp, pNSprite->int_pos().X, pNSprite->int_pos().Y));
		if ((pNSprite->sectp->floorpicnum >= 4080) && (pNSprite->sectp->floorpicnum <= 4095) && !VanillaMode()) // if floor has ror, find actual floor
		{
			int cX = pNSprite->int_pos().X, cY = pNSprite->int_pos().Y, cZ = pNSprite->int_pos().Z, cZrel = pNSprite->int_pos().Z;
			auto cSect = pNSprite->sectp;
			for (int i = 0; i < 16; i++) // scan through max stacked sectors
			{
				if (!CheckLink(&cX, &cY, &cZ, &cSect)) // if no more floors underneath, abort
					break;
				const int newFloorZ = getflorzofslopeptr(cSect, cX, cZ);
				cZrel += newFloorZ - cZ; // get height difference for next sector's ceiling/floor, and add to relative height for shadow
				if ((cSect->floorpicnum < 4080) || (cSect->floorpicnum > 4095)) // if current sector is not open air, use as floor for shadow casting, otherwise continue to next sector
					break;
				cZ = newFloorZ;
			}
			pNSprite->sectp = cSect;
			pNSprite->set_int_z(cZrel);
		}
		pNSprite->shade = 127;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = pTSprite->yrepeat>>2;
		pNSprite->picnum = pTSprite->picnum;
		if (!VanillaMode() && (pTSprite->type == kThingDroppedLifeLeech)) // fix shadow for thrown lifeleech
			pNSprite->picnum = 800;
		pNSprite->pal = 5;
		int height = tileHeight(pNSprite->picnum);
		int center = height / 2 + tileTopOffset(pNSprite->picnum);
		pNSprite->add_int_z(-(pNSprite->yrepeat << 2) * (height - center));
		break;
	}
	case kViewEffectFlareHalo:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pal = 2;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->pos.Z = pTSprite->pos.Z;
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = pTSprite->yrepeat;
		pNSprite->picnum = 2427;
		break;
	}
	case kViewEffectCeilGlow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		sectortype* pSector = pTSprite->sectp;
		pNSprite->pos = { pTSprite->pos.X, pTSprite->pos.Y, pSector->ceilingz };

		pNSprite->picnum = 624;
		pNSprite->shade = ((pTSprite->int_pos().Z - pSector->int_ceilingz()) >> 8) - 64;
		pNSprite->pal = 2;
		pNSprite->xrepeat = pNSprite->yrepeat = 64;
		pNSprite->cstat |= CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_YFLIP | CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->angle = pTSprite->angle;
		pNSprite->ownerActor = pTSprite->ownerActor;
		break;
	}
	case kViewEffectFloorGlow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		sectortype* pSector = pTSprite->sectp;
		pNSprite->pos = { pTSprite->pos.X, pTSprite->pos.Y, pSector->floorz };
		pNSprite->picnum = 624;
		uint8_t nShade = (uint8_t)clamp(pSector->floorz - pTSprite->pos.Z, 0., 255.);
		pNSprite->shade = nShade - 32;
		pNSprite->pal = 2;
		pNSprite->xrepeat = pNSprite->yrepeat = nShade;
		pNSprite->cstat |= CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->angle = pTSprite->angle;
		pNSprite->ownerActor = pTSprite->ownerActor;
		break;
	}
	case kViewEffectSpear:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->pos.Z = pTSprite->pos.Z;
		if (gDetail > 1)
			pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
		pNSprite->shade = ClipLow(pTSprite->shade - 32, -128);
		pNSprite->xrepeat = pTSprite->xrepeat;
		pNSprite->yrepeat = 64;
		pNSprite->picnum = 775;
		break;
	}
	case kViewEffectShowWeapon:
	{
		assert(pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8);
		PLAYER* pPlayer = &gPlayer[pTSprite->type - kDudePlayer1];
		WEAPONICON weaponIcon = gWeaponIcon[pPlayer->curWeapon];
		auto& nTile = weaponIcon.nTile;
		if (nTile < 0) break;
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->pos = pTSprite->pos.plusZ(-32 - weaponIcon.zOffset);
		pNSprite->picnum = nTile;
		pNSprite->shade = pTSprite->shade;
		pNSprite->xrepeat = 32;
		pNSprite->yrepeat = 32;
		auto& nVoxel = voxelIndex[nTile];
		if (cl_showweapon == 2 && r_voxels && nVoxel != -1)
		{
			pNSprite->set_int_ang((gView->actor->int_ang() + 512) & 2047); // always face viewer
			pNSprite->cstat |= CSTAT_SPRITE_ALIGNMENT_SLAB;
			pNSprite->cstat &= ~CSTAT_SPRITE_YFLIP;
			pNSprite->picnum = nVoxel;
			if (pPlayer->curWeapon == kWeapLifeLeech) // position lifeleech behind player
			{
				pNSprite->add_int_x(MulScale(128, Cos(gView->actor->int_ang()), 30));
				pNSprite->add_int_y(MulScale(128, Sin(gView->actor->int_ang()), 30));
			}
			if ((pPlayer->curWeapon == kWeapLifeLeech) || (pPlayer->curWeapon == kWeapVoodooDoll))  // make lifeleech/voodoo doll always face viewer like sprite
				pNSprite->set_int_ang((pNSprite->int_ang() + 512) & 2047); // offset angle 90 degrees
		}
		break;
	}
	}
	return NULL;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void viewApplyDefaultPal(tspritetype* pTSprite, sectortype const* pSector)
{
	XSECTOR const* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;
	if (pXSector && pXSector->color && (VanillaMode() || pSector->floorpal != 0))
	{
		copyfloorpal(pTSprite, pSector);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void viewProcessSprites(tspriteArray& tsprites, int32_t cX, int32_t cY, int32_t cZ, DAngle cA, int32_t smoothratio)
{
	int nViewSprites = tsprites.Size();
	// shift before interpolating to increase precision.
	int myclock = (PlayClock << 3) + MulScale(4 << 3, smoothratio, 16);
	gCameraAng = cA;
	for (int nTSprite = int(tsprites.Size()) - 1; nTSprite >= 0; nTSprite--)
	{
		tspritetype* pTSprite = tsprites.get(nTSprite);
		auto owneractor = static_cast<DBloodActor*>(pTSprite->ownerActor);
		if (owneractor->spr.detail > gDetail)
		{
			pTSprite->xrepeat = 0;
			continue;
		}
		int nTile = pTSprite->picnum;
		if (nTile < 0 || nTile >= kMaxTiles)
		{
			pTSprite->xrepeat = 0;
			continue;
		}
		// skip picnum 0 on face sprites. picnum 0 is a simple wall texture in Blood, 
		// but there are maps that use 0 on some operator sprites that may show up in potals as a result.
		// Since the wall texture is perfectly fine for wall and floor sprites, these will be allowed to pass.
		if (nTile == 0 && (pTSprite->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FACING)
		{
			pTSprite->xrepeat = 0;
			continue;
		}

		if (cl_interpolate && owneractor->interpolated && !(pTSprite->flags & 512))
		{
			pTSprite->pos = owneractor->interpolatedvec3(gInterpolate / 65536.);
			pTSprite->angle = owneractor->interpolatedangle(gInterpolate / 65536.);
		}
		int nAnim = 0;
		switch (picanm[nTile].extra & 7) {
		case 0:
			if (!owneractor->hasX()) break;
			switch (pTSprite->type) {
#ifdef NOONE_EXTENSIONS
			case kModernCondition:
			case kModernConditionFalse:
				if (!gModernMap) break;
				[[fallthrough]];
#endif
			case kSwitchToggle:
			case kSwitchOneWay:
				if (owneractor->xspr.state) nAnim = 1;
				break;
			case kSwitchCombo:
				nAnim = owneractor->xspr.data1;
				break;
			}
			break;
		case 1:
		{
			if (tilehasmodelorvoxel(pTSprite->picnum, pTSprite->pal) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}
			int dX = cX - pTSprite->int_pos().X;
			int dY = cY - pTSprite->int_pos().Y;
			RotateVector(&dX, &dY, 128 - pTSprite->int_ang());
			nAnim = GetOctant(dX, dY);
			if (nAnim <= 4)
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
			}
			else
			{
				nAnim = 8 - nAnim;
				pTSprite->cstat |= CSTAT_SPRITE_XFLIP;
			}
			break;
		}
		case 2:
		{
			if (tilehasmodelorvoxel(pTSprite->picnum, pTSprite->pal) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}
			int dX = cX - pTSprite->int_pos().X;
			int dY = cY - pTSprite->int_pos().Y;
			RotateVector(&dX, &dY, 128 - pTSprite->int_ang());
			nAnim = GetOctant(dX, dY);
			break;
		}
		case 3:
		{
			if (owneractor->hasX())
			{
				if (owneractor->hit.florhit.type == kHitNone)
					nAnim = 1;
			}
			else
			{
				int top, bottom;
				GetSpriteExtents(pTSprite, &top, &bottom);
				if (getflorzofslopeptr(pTSprite->sectp, pTSprite->int_pos().X, pTSprite->int_pos().Y) > bottom)
					nAnim = 1;
			}
			break;
		}
		case 6:
		case 7:
		{
			if (hw_models && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
				break;

			// Can be overridden by def script
			if (r_voxels && tiletovox[pTSprite->picnum] == -1 && voxelIndex[pTSprite->picnum] != -1 && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				if ((pTSprite->flags & kHitagRespawn) == 0)
				{
					pTSprite->cstat |= CSTAT_SPRITE_ALIGNMENT_SLAB;
					pTSprite->cstat &= ~(CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
					pTSprite->yoffset += tileTopOffset(pTSprite->picnum);
					pTSprite->picnum = voxelIndex[pTSprite->picnum];
					if ((picanm[nTile].extra & 7) == 7)
					{
						pTSprite->set_int_ang( myclock & 2047);
					}
				}
			}
			break;
		}
		}
		while (nAnim > 0)
		{
			pTSprite->picnum += picanm[pTSprite->picnum].num + 1;
			nAnim--;
		}

		if ((pTSprite->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB && r_voxels && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
		{
			int const nRootTile = pTSprite->picnum;
			int nAnimTile = pTSprite->picnum + tileAnimateOfs(pTSprite->picnum, (pTSprite->ownerActor->GetIndex() & 16383));

#if 0
			if (tiletovox[nAnimTile] != -1)
			{
				pTSprite->yoffset += tileTopOffset(nAnimTile);
				pTSprite->xoffset += tileLeftOffset(nAnimTile);
			}
#endif

			int const nVoxel = tiletovox[pTSprite->picnum];

			if (nVoxel != -1 && (picanm[nRootTile].extra & 7) == 7)
				pTSprite->clipdist |= TSPR_MDLROTATE; // per-sprite rotation setting.
		}

		if ((pTSprite->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB && hw_models && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
		{
			int const nRootTile = pTSprite->picnum;
			int nAnimTile = pTSprite->picnum + tileAnimateOfs(pTSprite->picnum, (pTSprite->ownerActor->GetIndex() & 16383));

			if (tile2model[Ptile2tile(nAnimTile, pTSprite->pal)].modelid >= 0 &&
				tile2model[Ptile2tile(nAnimTile, pTSprite->pal)].framenum >= 0)
			{
				pTSprite->yoffset += tileTopOffset(nAnimTile);
				pTSprite->xoffset += tileLeftOffset(nAnimTile);

				if ((picanm[nRootTile].extra & 7) == 7)
					pTSprite->clipdist |= TSPR_MDLROTATE; // per-sprite rotation setting.
			}
		}

		sectortype* pSector = pTSprite->sectp;
		XSECTOR const* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;
		int nShade = pTSprite->shade;

		if ((pSector->ceilingstat & CSTAT_SECTOR_SKY) && (pSector->floorstat & CSTAT_SECTOR_NO_CEILINGSHADE) == 0)
		{
			nShade += tileShade[pSector->ceilingpicnum] + pSector->ceilingshade;
		}
		else
		{
			nShade += tileShade[pSector->floorpicnum] + pSector->floorshade;
		}
		nShade += tileShade[pTSprite->picnum];
		pTSprite->shade = ClipRange(nShade, -128, 127);
		if ((pTSprite->flags & kHitagRespawn) && pTSprite->ownerActor->spr.intowner == 3 && owneractor->hasX())    // Where does this 3 come from? Nothing sets it.
		{
			pTSprite->xrepeat = 48;
			pTSprite->yrepeat = 48;
			pTSprite->shade = -128;
			pTSprite->picnum = 2272 + 2 * owneractor->xspr.respawnPending;
			pTSprite->cstat &= ~(CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP);
			if (((IsItemSprite(pTSprite) || IsAmmoSprite(pTSprite)) && gGameOptions.nItemSettings == 2)
				|| (IsWeaponSprite(pTSprite) && gGameOptions.nWeaponSettings == 3))
			{
				pTSprite->xrepeat = pTSprite->yrepeat = 48;
			}
			else
			{
				pTSprite->xrepeat = pTSprite->yrepeat = 0;
			}
		}
		if (owneractor->hasX() && owneractor->xspr.burnTime > 0)
		{
			pTSprite->shade = ClipRange(pTSprite->shade - 16 - QRandom(8), -128, 127);
		}
		if (pTSprite->flags & 256)
		{
			viewAddEffect(tsprites, nTSprite, kViewEffectSmokeHigh);
		}
		if (pTSprite->flags & 1024)
		{
			pTSprite->cstat |= CSTAT_SPRITE_XFLIP;
		}
		if (pTSprite->flags & 2048)
		{
			pTSprite->cstat |= CSTAT_SPRITE_YFLIP;
		}
		switch (pTSprite->statnum) {
		case kStatDecoration: {
			switch (pTSprite->type) {
			case kDecorationCandle:
				if (!owneractor->hasX() || owneractor->xspr.state == 1) {
					pTSprite->shade = -128;
					viewAddEffect(tsprites, nTSprite, kViewEffectPhase);
				}
				else {
					pTSprite->shade = -8;
				}
				break;
			case kDecorationTorch:
				if (!owneractor->hasX() || owneractor->xspr.state == 1) {
					pTSprite->picnum++;
					viewAddEffect(tsprites, nTSprite, kViewEffectTorchHigh);
				}
				else {
					viewAddEffect(tsprites, nTSprite, kViewEffectSmokeHigh);
				}
				break;
			default:
				viewApplyDefaultPal(pTSprite, pSector);
				break;
			}
		}
							break;
		case kStatItem: {
			switch (pTSprite->type) {
			case kItemFlagABase:
				if (owneractor->hasX() && owneractor->xspr.state > 0 && gGameOptions.nGameType == 3) {
					auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectBigFlag);
					if (pNTSprite) pNTSprite->pal = 10;
				}
				break;
			case kItemFlagBBase:
				if (owneractor->hasX() && owneractor->xspr.state > 0 && gGameOptions.nGameType == 3) {
					auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectBigFlag);
					if (pNTSprite) pNTSprite->pal = 7;
				}
				break;
			case kItemFlagA:
				pTSprite->pal = 10;
				pTSprite->cstat |= CSTAT_SPRITE_BLOOD_BIT2;
				break;
			case kItemFlagB:
				pTSprite->pal = 7;
				pTSprite->cstat |= CSTAT_SPRITE_BLOOD_BIT2;
				break;
			default:
				if (pTSprite->type >= kItemKeySkull && pTSprite->type < kItemKeyMax)
					pTSprite->shade = -128;

				viewApplyDefaultPal(pTSprite, pSector);
				break;
			}
		}
					  break;
		case kStatProjectile: {
			switch (pTSprite->type) {
			case kMissileTeslaAlt:
				pTSprite->yrepeat = 128;
				pTSprite->cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
				break;
			case kMissileTeslaRegular:
				viewAddEffect(tsprites, nTSprite, kViewEffectTesla);
				break;
			case kMissileButcherKnife:
				viewAddEffect(tsprites, nTSprite, kViewEffectTrail);
				break;
			case kMissileFlareRegular:
			case kMissileFlareAlt:
				if (pTSprite->statnum == kStatFlare) {
					if (owneractor->GetTarget() == gView->actor)
					{
						pTSprite->xrepeat = 0;
						break;
					}
				}

				viewAddEffect(tsprites, nTSprite, kViewEffectFlareHalo);
				if (pTSprite->type != kMissileFlareRegular) break;
				sectortype* pSector1 = pTSprite->sectp;

				double zDiff = pTSprite->pos.Z - pSector1->ceilingz;
				if ((pSector1->ceilingstat & CSTAT_SECTOR_SKY) == 0 && zDiff < 64) 
				{
					viewAddEffect(tsprites, nTSprite, kViewEffectCeilGlow);
				}

				zDiff = (pSector1->floorz - pTSprite->pos.Z);
				if ((pSector1->floorstat & CSTAT_SECTOR_SKY) == 0 && zDiff < 64) 
				{
					viewAddEffect(tsprites, nTSprite, kViewEffectFloorGlow);
				}
				break;
			}
			break;
		}
		case kStatDude:
		{
			if (pTSprite->type == kDudeHand && owneractor->hasX() && owneractor->xspr.aiState == &hand13A3B4)
			{
				auto target = owneractor->GetTarget();
				if (target && target->IsPlayerActor())
				{
					pTSprite->xrepeat = 0;
					break;
				}
			}

			if (pXSector && pXSector->color) copyfloorpal(pTSprite, pSector);
			if (powerupCheck(gView, kPwUpBeastVision) > 0) pTSprite->shade = -128;

			if (IsPlayerSprite(pTSprite)) {
				PLAYER* pPlayer = &gPlayer[pTSprite->type - kDudePlayer1];
				if (powerupCheck(pPlayer, kPwUpShadowCloak) && !powerupCheck(gView, kPwUpBeastVision)) {
					pTSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
					pTSprite->pal = 5;
				}
				else if (powerupCheck(pPlayer, kPwUpDeathMask)) {
					pTSprite->shade = -128;
					pTSprite->pal = 5;
				}
				else if (powerupCheck(pPlayer, kPwUpDoppleganger)) {
					pTSprite->pal = 11 + (gView->teamId & 3);
				}

				if (powerupCheck(pPlayer, kPwUpReflectShots)) {
					viewAddEffect(tsprites, nTSprite, kViewEffectReflectiveBall);
				}

				if (cl_showweapon && gGameOptions.nGameType > 0 && gView) {
					viewAddEffect(tsprites, nTSprite, kViewEffectShowWeapon);
				}

				if (pPlayer->flashEffect && (gView != pPlayer || gViewPos != VIEWPOS_0)) {
					auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectShoot);
					if (pNTSprite) {
						POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
						pNTSprite->add_int_x(MulScale(pPosture->zOffset, Cos(pTSprite->int_ang()), 28));
						pNTSprite->add_int_y(MulScale(pPosture->zOffset, Sin(pTSprite->int_ang()), 28));
						pNTSprite->set_int_z(pPlayer->actor->int_pos().Z - pPosture->xOffset);
					}
				}

				if (pPlayer->hasFlag > 0 && gGameOptions.nGameType == 3) {
					if (pPlayer->hasFlag & 1) {
						auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectFlag);
						if (pNTSprite)
						{
							pNTSprite->pal = 10;
							pNTSprite->cstat |= CSTAT_SPRITE_XFLIP;
						}
					}
					if (pPlayer->hasFlag & 2) {
						auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectFlag);
						if (pNTSprite)
						{
							pNTSprite->pal = 7;
							pNTSprite->cstat |= CSTAT_SPRITE_XFLIP;
						}
					}
				}
			}

			if (pTSprite->ownerActor != gView->actor || gViewPos != VIEWPOS_0) {
				if (getflorzofslopeptr(pTSprite->sectp, pTSprite->int_pos().X, pTSprite->int_pos().Y) >= cZ)
				{
					viewAddEffect(tsprites, nTSprite, kViewEffectShadow);
				}
			}

			if (gModernMap && owneractor->hasX()) { // add target spot indicator for patrol dudes
				if (owneractor->xspr.dudeFlag4 && aiInPatrolState(owneractor->xspr.aiState) && owneractor->xspr.data3 > 0 && owneractor->xspr.data3 <= kMaxPatrolSpotValue)
					viewAddEffect(tsprites, nTSprite, kViewEffectSpotProgress);
			}
			break;
		}
		case kStatTraps: {
			if (pTSprite->type == kTrapSawCircular) {
				if (owneractor->xspr.state) {
					if (owneractor->xspr.data1) {
						pTSprite->picnum = 772;
						if (owneractor->xspr.data2)
							viewAddEffect(tsprites, nTSprite, kViewEffectSpear);
					}
				}
				else if (owneractor->xspr.data1) pTSprite->picnum = 773;
				else pTSprite->picnum = 656;

			}
			break;
		}
		case kStatThing: {
			viewApplyDefaultPal(pTSprite, pSector);

			if (pTSprite->type < kThingBase || pTSprite->type >= kThingMax || owneractor->hit.florhit.type == kHitNone)
			{
				if ((pTSprite->flags & kPhysMove) && getflorzofslopeptr(pTSprite->sectp, pTSprite->int_pos().X, pTSprite->int_pos().Y) >= cZ)
					viewAddEffect(tsprites, nTSprite, kViewEffectShadow);
			}
		}
					   break;
		}
	}

	for (int nTSprite = int(tsprites.Size() - 1); nTSprite >= nViewSprites; nTSprite--)
	{
		tspritetype* pTSprite = tsprites.get(nTSprite);
		int nAnim = 0;
		switch (picanm[pTSprite->picnum].extra & 7)
		{
		case 1:
		{
			int dX = cX - pTSprite->int_pos().X;
			int dY = cY - pTSprite->int_pos().Y;
			RotateVector(&dX, &dY, 128 - pTSprite->int_ang());
			nAnim = GetOctant(dX, dY);
			if (nAnim <= 4)
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
			}
			else
			{
				nAnim = 8 - nAnim;
				pTSprite->cstat |= CSTAT_SPRITE_XFLIP;
			}
			break;
		}
		case 2:
		{
			int dX = cX - pTSprite->int_pos().X;
			int dY = cY - pTSprite->int_pos().Y;
			RotateVector(&dX, &dY, 128 - pTSprite->int_ang());
			nAnim = GetOctant(dX, dY);
			break;
		}
		}
		while (nAnim > 0)
		{
			pTSprite->picnum += picanm[pTSprite->picnum].num + 1;
			nAnim--;
		}
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::processSprites(tspriteArray& tsprites, int viewx, int viewy, int viewz, DAngle viewang, double smoothRatio)
{
	viewProcessSprites(tsprites, viewx, viewy, viewz, viewang, int(smoothRatio));
}

int display_mirror;

void GameInterface::EnterPortal(DCoreActor* viewer, int type)
{
	if (type == PORTAL_WALL_MIRROR)
	{
		display_mirror++;
		if (viewer) viewer->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
	}
}

void GameInterface::LeavePortal(DCoreActor* viewer, int type)
{
	if (type == PORTAL_WALL_MIRROR)
	{
		display_mirror--;
		if (viewer && display_mirror == 0 && !(viewer->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)) viewer->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	}
}

END_BLD_NS
