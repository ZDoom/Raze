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
#include "texturemanager.h"
#include "texinfo.h"
#include "models/modeldata.h"

BEGIN_BLD_NS

static DAngle gCameraAng;
DAngle random_angles[16][3];

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
	pTSprite->scale = DVector2(1, 1);
	pTSprite->ownerActor = nullptr;
	pTSprite->statnum = nStatnum;
	pTSprite->sectp = pSector;

	DVector3 pos = { 0,0,0 };
	if (parentTSprite)
	{
		pos = parentTSprite->pos;
		pTSprite->ownerActor = parentTSprite->ownerActor;
		pTSprite->Angles.Yaw = parentTSprite->Angles.Yaw;
	}
	pos.XY() += gCameraAng.ToVector() * 2;
	pTSprite->pos = pos;
	return pTSprite;
}

static const int effectDetail[kViewEffectMax] = {
	4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};


struct WEAPONICON {
	int16_t nTile;
	uint8_t zOffset;
	FTextureID textureID() const { return aTexIds[nTile]; }
};

static const WEAPONICON gWeaponIcon[] = {
	{ -1, 0 },
	{ -1, 0 }, // 1: pitchfork
	{ kTexICONFLAREGUN, 6 }, // 2: flare gun
	{ kTexICONSHOTGUN, 6 }, // 3: shotgun
	{ kTexICONTOMMY, 8 }, // 4: tommy gun
	{ kTexICONNAPALM, 6 }, // 5: napalm launcher
	{ kTexAmmoIcon5, 11 }, // 6: dynamite
	{ kTexAmmoIcon6, 11 }, // 7: spray can
	{ kTexICONTESLA, 6 }, // 8: tesla gun
	{ kTexICONLEECH, 0 }, // 9: life leech
	{ kTexAmmoIcon9, 11 }, // 10: voodoo doll
	{ kTexAmmoIcon10, 11 }, // 11: proxy bomb
	{ kTexAmmoIcon11, 11 }, // 12: remote bomb
	{ -1, 0 },
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static tspritetype* viewAddEffect(tspriteArray& tsprites, int nTSprite, VIEW_EFFECT nViewEffect)
{
	double s;
	assert(nViewEffect >= 0 && nViewEffect < kViewEffectMax);
	auto pTSprite = tsprites.get(nTSprite);
	auto owneractor = static_cast<DBloodActor*>(pTSprite->ownerActor);
	if (gDetail < effectDetail[nViewEffect]) return NULL;
	auto pTTex = TexMan.GetGameTexture(pTSprite->spritetexture());
	switch (nViewEffect)
	{
#ifdef NOONE_EXTENSIONS
	case kViewEffectSpotProgress: {
		int perc = (100 * owneractor->xspr.data3) / kMaxPatrolSpotValue;
		int width = (94 * owneractor->xspr.data3) / kMaxPatrolSpotValue;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);

		auto pNSprite2 = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite2)
			break;

		pNSprite2->setspritetexture(aTexIds[kTexSPOTPROGRESS]);
		pNSprite2->scale = DVector2(width * REPEAT_SCALE, 0.3125);

		pNSprite2->pal = 10;
		if (perc >= 75) pNSprite2->pal = 0;
		else if (perc >= 50) pNSprite2->pal = 6;

		pNSprite2->pos.Z = (top - 8);
		pNSprite2->shade = -128;
		break;
	}
#endif
	case kViewEffectAtom:
		for (int i = 0; i < 16; i++)
		{
			auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
			if (!pNSprite)
				break;

			auto ang = mapangle((PlayClock * 2048) / 120).Normalized360();
			auto nRand1 = random_angles[i][0];
			auto nRand2 = random_angles[i][1];
			auto nRand3 = random_angles[i][2];
			ang += nRand3;
			auto vect = DVector3(32 * ang.ToVector(), 0);
			DVector2 pt(vect.Y, vect.Z);
			pt = pt.Rotated(nRand1);
			vect.Y = pt.X;
			pt.X = vect.X;
			pt = pt.Rotated(nRand2);
			vect.X = pt.X;
			vect.Z = pt.Y;

			pNSprite->pos = pTSprite->pos + vect;
			pNSprite->setspritetexture(aTexIds[kTexATOMEFFECT]);
			pNSprite->shade = -128;
		}
		break;
	case kViewEffectFlag:
	case kViewEffectBigFlag:
	{
		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pal = 0;
		pNSprite->pos.Z = top;
		if (nViewEffect == kViewEffectFlag)
			pNSprite->scale = DVector2(0.375, 0.375);
		else
			pNSprite->scale = DVector2(1, 1);
		pNSprite->setspritetexture(aTexIds[kTexFLAGHAVE]);
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
		pNSprite->scale = pTSprite->scale;
		pNSprite->setspritetexture(aTexIds[kTexTESLAEFFECT]);
		break;
	}
	case kViewEffectShoot:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->shade = -128;
		pNSprite->pal = 0;
		pNSprite->scale = DVector2(1, 1);
		pNSprite->setspritetexture(aTexIds[kTexSHOOTEFFECT]);
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
		pNSprite->scale = DVector2(1, 1);
		pNSprite->setspritetexture(aTexIds[kTexBALLEFFECT]);
		break;
	}
	case kViewEffectPhase:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->pos.Z = top;
		pNSprite->shade = 26;
		pNSprite->pal = 0;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->scale = DVector2(0.375, 0.375);
		pNSprite->setspritetexture(aTexIds[kTexPHASEEFFECT]);
		break;
	}
	case kViewEffectTrail:
	{
		auto nAng = pTSprite->Angles.Yaw;
		if (pTSprite->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)
		{
			nAng += DAngle90;
		}
		else
		{
			nAng += DAngle180;
		}
		for (int i = 0; i < 5; i++)
		{
			auto pSector = pTSprite->sectp;
			auto pNSprite = viewInsertTSprite(tsprites, pSector, 32767, NULL);
			if (!pNSprite)
				break;

			double nLen = 8.0 * (i + 1);
			auto vect = nAng.ToVector() * nLen;
			pNSprite->pos = pTSprite->pos + vect;
			assert(pSector);
			auto pSector2 = pSector;
			updatesectorz(pNSprite->pos, &pSector2);
			if (pSector2) pSector = pSector2;
			pNSprite->sectp = pSector;
			pNSprite->ownerActor = pTSprite->ownerActor;
			pNSprite->setspritetexture(pTSprite->spritetexture());
			pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
			if (i < 2)
				pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
			pNSprite->shade = ClipLow(pTSprite->shade - 16, -128);
			pNSprite->scale = pTSprite->scale;
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
		pNSprite->setspritetexture(aTexIds[kTexFLAMEEFFECT]);
		pNSprite->statnum = kStatDecoration;
		s = (pTTex->GetDisplayWidth() * pTSprite->scale.X) / 64.;
		pNSprite->scale = DVector2(s, s);
		break;
	}
	case kViewEffectSmokeHigh:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->pos.Z = top;
		if (owneractor->IsDudeActor())
			pNSprite->setspritetexture(aTexIds[kTexBIGSMOKEEFFECT]);
		else
			pNSprite->setspritetexture(aTexIds[kTexSMALLSMOKEEFFECT]);
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->shade = 8;
		pNSprite->scale = pTSprite->scale;
		break;
	}
	case kViewEffectSmokeLow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->pos.Z = bottom;
		if (owneractor->IsDudeActor())
			pNSprite->setspritetexture(aTexIds[kTexBIGSMOKEEFFECT]);
		else
			pNSprite->setspritetexture(aTexIds[kTexSMALLSMOKEEFFECT]);
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->shade = 8;
		pNSprite->scale = pTSprite->scale;
		break;
	}
	case kViewEffectTorchHigh:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->pos.Z = top;
		pNSprite->setspritetexture(aTexIds[kTexTORCHEFFECT]);
		pNSprite->shade = -128;
		s = (pTTex->GetDisplayWidth() * pTSprite->scale.X) / 32.;
		pNSprite->scale = DVector2(s, s);
		break;
	}
	case kViewEffectTorchLow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		double top, bottom;
		GetSpriteExtents(pTSprite, &top, &bottom);
		pNSprite->pos.Z = bottom;
		pNSprite->setspritetexture(aTexIds[kTexTORCHEFFECT]);
		pNSprite->shade = -128;
		s = (pTTex->GetDisplayWidth() * pTSprite->scale.X) / 32.;
		pNSprite->scale = DVector2(s, s);
		break;
	}
	case kViewEffectShadow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;
		pNSprite->pos.Z = getflorzofslopeptr(pTSprite->sectp, pNSprite->pos);
		if (pNSprite->sectp->portalflags == PORTAL_SECTOR_FLOOR && !VanillaMode()) // if floor has ror, find actual floor
		{
			DVector3 cPos = pNSprite->pos;
			double cZrel = cPos.Z;
			auto cSect = pNSprite->sectp;
			for (int i = 0; i < 16; i++) // scan through max stacked sectors
			{
				if (!CheckLink(cPos, &cSect)) // if no more floors underneath, abort
					break;
				const double newFloorZ = getflorzofslopeptr(cSect, cPos.X, cPos.Z);
				cZrel += newFloorZ - cPos.Z; // get height difference for next sector's ceiling/floor, and add to relative height for shadow
				if (cSect->portalflags != PORTAL_SECTOR_FLOOR) // if current sector is not open air, use as floor for shadow casting, otherwise continue to next sector
					break;
				cPos.Z = newFloorZ;
			}
			pNSprite->sectp = cSect;
			pNSprite->pos.Z = cZrel;
		}
		pNSprite->shade = 127;
		pNSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->scale.X = pTSprite->scale.X;
		pNSprite->scale.Y = pTSprite->scale.Y * 0.25;
		pNSprite->setspritetexture(pTSprite->spritetexture());
		if (!VanillaMode() && (owneractor->GetType() == kThingDroppedLifeLeech)) // fix shadow for thrown lifeleech
			pNSprite->setspritetexture(aTexIds[kTexICONLEECH]);
		pNSprite->pal = 5;
		auto tex = TexMan.GetGameTexture(pNSprite->spritetexture());
		double height = tex->GetDisplayHeight();
		double center = height / 2 + tex->GetDisplayTopOffset();
		pNSprite->pos.Z -= (pNSprite->scale.Y) * (height - center);
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
		pNSprite->scale = pTSprite->scale;
		pNSprite->setspritetexture(aTexIds[kTexHALOEFFECT]);
		break;
	}
	case kViewEffectCeilGlow:
	{
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		sectortype* pSector = pTSprite->sectp;
		pNSprite->pos = { pTSprite->pos.X, pTSprite->pos.Y, pSector->ceilingz };

		pNSprite->setspritetexture(aTexIds[kTexGLOWEFFECT]);
		pNSprite->shade = int(pTSprite->pos.Z - pSector->ceilingz) - 64;
		pNSprite->pal = 2;
		pNSprite->scale = DVector2(1, 1);
		pNSprite->cstat |= CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_YFLIP | CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->Angles.Yaw = pTSprite->Angles.Yaw;
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
		pNSprite->setspritetexture(aTexIds[kTexGLOWEFFECT]);
		uint8_t nShade = (uint8_t)clamp(pSector->floorz - pTSprite->pos.Z, 0., 255.);
		pNSprite->shade = nShade - 32;
		pNSprite->pal = 2;
		pNSprite->scale = DVector2(nShade * REPEAT_SCALE, nShade * REPEAT_SCALE);
		pNSprite->cstat |= CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_TRANSLUCENT;
		pNSprite->Angles.Yaw = pTSprite->Angles.Yaw;
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
		pNSprite->scale = DVector2(pTSprite->scale.X, 1);
		pNSprite->setspritetexture(aTexIds[kTexSAWBLOOD]);
		break;
	}
	case kViewEffectShowWeapon:
	{
		assert(owneractor->IsPlayerActor());
		auto pPlayer = getPlayer(owneractor);
		WEAPONICON weaponIcon = gWeaponIcon[pPlayer->curWeapon];
		auto nTex = weaponIcon.textureID();
		if (!nTex.isValid()) break;
		auto pNSprite = viewInsertTSprite(tsprites, pTSprite->sectp, 32767, pTSprite);
		if (!pNSprite)
			break;

		pNSprite->pos = pTSprite->pos.plusZ(-32 - weaponIcon.zOffset);
		pNSprite->setspritetexture(nTex);
		pNSprite->shade = pTSprite->shade;
		pNSprite->scale = DVector2(0.5, 0.5);
		int nVoxel = GetExtInfo(nTex).tiletovox;
		if (cl_showweapon == 2 && r_voxels && nVoxel != -1)
		{
			auto gView = getPlayer(gViewIndex);
			pNSprite->Angles.Yaw = gView->GetActor()->spr.Angles.Yaw + DAngle90; // always face viewer
			pNSprite->cstat &= ~CSTAT_SPRITE_YFLIP;
			if (pPlayer->curWeapon == kWeapLifeLeech) // position lifeleech behind player
			{
				pNSprite->pos.XY() += gView->GetActor()->spr.Angles.Yaw.ToVector() * 8;
			}
			if ((pPlayer->curWeapon == kWeapLifeLeech) || (pPlayer->curWeapon == kWeapVoodooDoll))  // make lifeleech/voodoo doll always face viewer like sprite
				pNSprite->Angles.Yaw += DAngle90;
		}
		else
		{
			pNSprite->cstat2 |= CSTAT2_SPRITE_NOMODEL;
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

static int GetOctant(const DVector2& dPos)
{
	static const uint8_t OctantTable[8] = { 5, 6, 2, 1, 4, 7, 3, 0 };
	double vc = fabs(dPos.X) - fabs(dPos.Y);
	return OctantTable[7 - (dPos.X < 0) - (dPos.Y < 0) * 2 - (vc < 0) * 4];
}

void viewProcessSprites(tspriteArray& tsprites, const DVector3& cPos, DAngle cA, double interpfrac)
{
	DBloodPlayer* pPlayer = getPlayer(gViewIndex);
	int nViewSprites = tsprites.Size();
	// shift before interpolating to increase precision.
	DAngle myclock = DAngle::fromBuild((PlayClock << 3) + (4 << 3) * interpfrac);
	gCameraAng = cA;
	// Because this goes backward it will only iterate over the originally added tsprites, not over the ones added here.
	for (int nTSprite = int(tsprites.Size()) - 1; nTSprite >= 0; nTSprite--)
	{
		tspritetype* pTSprite = tsprites.get(nTSprite);
		auto owneractor = static_cast<DBloodActor*>(pTSprite->ownerActor);
		if (owneractor->spr.detail > gDetail)
		{
			pTSprite->scale = DVector2(0, 0);
			continue;
		}
		auto nTex = pTSprite->spritetexture();
		if (!nTex.isValid())
		{
			pTSprite->scale = DVector2(0, 0);
			continue;
		}

		if (cl_interpolate && owneractor->interpolated && !(pTSprite->flags & 512))
		{
			pTSprite->pos = owneractor->interpolatedpos(interpfrac);
			pTSprite->Angles.Yaw = owneractor->interpolatedyaw(interpfrac);
		}
		int nAnim = 0;
		int nAnimType = GetExtInfo(nTex).picanm.extra & 7;
		switch (nAnimType)
		{
		case 0:
			if (!owneractor->hasX()) break;
			switch (owneractor->GetType())
			{
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
			if (tilehasmodelorvoxel(pTSprite->spritetexture(), pTSprite->pal) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}
			nAnim = GetOctant(DVector2(cPos.XY() - pTSprite->pos).Rotated(DAngle22_5 - pTSprite->Angles.Yaw));
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
			if (tilehasmodelorvoxel(pTSprite->spritetexture(), pTSprite->pal) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}
			nAnim = GetOctant(DVector2(cPos.XY() - pTSprite->pos).Rotated(DAngle22_5 - pTSprite->Angles.Yaw));
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
				double top, bottom;
				GetSpriteExtents(pTSprite, &top, &bottom);
				if (getflorzofslopeptr(pTSprite->sectp, pTSprite->pos) > bottom)
					nAnim = 1;
			}
			break;
		}
		case 6:
		case 7:
		{
			if (hw_models && modelManager.CheckModel(pTSprite->spritetexture(), pTSprite->pal) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
				break;

			// Can be overridden by def script
			if (tilehasvoxel(pTSprite->spritetexture()) && !(owneractor->sprext.renderflags & SPREXT_NOTMD))
			{
				if ((pTSprite->flags & kHitagRespawn) == 0)
				{
					pTSprite->cstat &= ~(CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
					auto tex = TexMan.GetGameTexture(pTSprite->spritetexture());
					auto ofs = GetExtInfo(nTex).voxoffs;
					pTSprite->yoffset += ofs;
					if (nAnimType == 7)
					{
						pTSprite->Angles.Yaw = myclock.Normalized360();
					}
				}
			}
			break;
		}
		}
		while (nAnim > 0)
		{
			pTSprite->setspritetexture(pTSprite->spritetexture() + GetExtInfo(pTSprite->spritetexture()).picanm.num + 1);
			nAnim--;
		}

		sectortype* pSector = pTSprite->sectp;
		XSECTOR const* pXSector = pSector->hasX() ? &pSector->xs() : nullptr;
		int nShade = pTSprite->shade;

		if ((pSector->ceilingstat & CSTAT_SECTOR_SKY) && (pSector->floorstat & CSTAT_SECTOR_NO_CEILINGSHADE) == 0)
		{
			nShade += GetExtInfo(pSector->ceilingtexture).tileshade + pSector->ceilingshade;
		}
		else
		{
			nShade += GetExtInfo(pSector->floortexture).tileshade + pSector->floorshade;
		}
		nShade += GetExtInfo(pTSprite->spritetexture()).tileshade;
		pTSprite->shade = ClipRange(nShade, -128, 127);
#if 0 // This was disabled because it seemingly cannot be activated (see comment below) and the sprites being used here are part of something else.
		if ((pTSprite->flags & kHitagRespawn) && pTSprite->ownerActor->spr.intowner == 3 && owneractor->hasX())    // Where does this 3 come from? Nothing sets it.
		{
			pTSprite->scale = DVector2(0.75, 0.75);
			pTSprite->shade = -128;
			pTSprite->p icnum = 2272 + 2 * owneractor->xspr.respawnPending;
			pTSprite->cstat &= ~(CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP);
			if (((IsItemSprite(pTSprite) || IsAmmoSprite(pTSprite)) && gGameOptions.nItemSettings == 2)
				|| (IsWeaponSprite(pTSprite) && gGameOptions.nWeaponSettings == 3))
			{
				pTSprite->scale = DVector2(0.75, 0.75);
			}
			else
			{
				pTSprite->scale = DVector2(0, 0);
			}
		}
#endif
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
			switch (owneractor->GetType())
			{
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
					pTSprite->setspritetexture(pTSprite->spritetexture() + 1);
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
			switch (owneractor->GetType())
			{
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
				if (owneractor->GetType() >= kItemKeySkull && owneractor->GetType() < kItemKeyMax)
					pTSprite->shade = -128;

				viewApplyDefaultPal(pTSprite, pSector);
				break;
			}
		}
					  break;
		case kStatProjectile: {
			switch (owneractor->GetType())
			{
			case kMissileTeslaAlt:
				pTSprite->scale.Y = (2);
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
					if (owneractor->GetTarget() == pPlayer->GetActor())
					{
						pTSprite->scale = DVector2(0, 0);
						break;
					}
				}

				viewAddEffect(tsprites, nTSprite, kViewEffectFlareHalo);
				if (owneractor->GetType() != kMissileFlareRegular) break;
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
			if (owneractor->GetType() == kDudeHand && owneractor->hasX() && owneractor->xspr.aiState == &hand13A3B4)
			{
				auto target = owneractor->GetTarget();
				if (target && target->IsPlayerActor())
				{
					pTSprite->scale = DVector2(0, 0);
					break;
				}
			}

			if (pXSector && pXSector->color) copyfloorpal(pTSprite, pSector);
			if (powerupCheck(pPlayer, kPwUpBeastVision) > 0) pTSprite->shade = -128;

			if (owneractor->IsPlayerActor()) {
				auto thisPlayer = getPlayer(owneractor);
				if (powerupCheck(thisPlayer, kPwUpShadowCloak) && !powerupCheck(pPlayer, kPwUpBeastVision)) {
					pTSprite->cstat |= CSTAT_SPRITE_TRANSLUCENT;
					pTSprite->pal = 5;
				}
				else if (powerupCheck(thisPlayer, kPwUpDeathMask)) {
					pTSprite->shade = -128;
					pTSprite->pal = 5;
				}
				else if (powerupCheck(thisPlayer, kPwUpDoppleganger)) {
					pTSprite->pal = 11 + (pPlayer->teamId & 3);
				}

				if (powerupCheck(thisPlayer, kPwUpReflectShots)) {
					viewAddEffect(tsprites, nTSprite, kViewEffectReflectiveBall);
				}

				if (cl_showweapon && gGameOptions.nGameType > 0 && pPlayer) {
					viewAddEffect(tsprites, nTSprite, kViewEffectShowWeapon);
				}

				if (thisPlayer->flashEffect && (pPlayer != thisPlayer || gViewPos != viewFirstPerson)) {
					auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectShoot);
					if (pNTSprite) {
						POSTURE* pPosture = &thisPlayer->pPosture[thisPlayer->lifeMode][thisPlayer->posture];
						pNTSprite->pos.XY() += pTSprite->Angles.Yaw.ToVector() * pPosture->xOffset;
						pNTSprite->pos.Z = thisPlayer->GetActor()->spr.pos.Z - pPosture->zOffset;
					}
				}

				if (thisPlayer->hasFlag > 0 && gGameOptions.nGameType == 3) {
					if (thisPlayer->hasFlag & 1) {
						auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectFlag);
						if (pNTSprite)
						{
							pNTSprite->pal = 10;
							pNTSprite->cstat |= CSTAT_SPRITE_XFLIP;
						}
					}
					if (thisPlayer->hasFlag & 2) {
						auto pNTSprite = viewAddEffect(tsprites, nTSprite, kViewEffectFlag);
						if (pNTSprite)
						{
							pNTSprite->pal = 7;
							pNTSprite->cstat |= CSTAT_SPRITE_XFLIP;
						}
					}
				}
			}

			if (pTSprite->ownerActor != pPlayer->GetActor() || gViewPos != viewFirstPerson) {
				if (getflorzofslopeptr(pTSprite->sectp, pTSprite->pos) >= cPos.Z)
				{
					viewAddEffect(tsprites, nTSprite, kViewEffectShadow);
				}
			}

#ifdef NOONE_EXTENSIONS
			if (gModernMap && owneractor->hasX()) { // add target spot indicator for patrol dudes
				if (owneractor->xspr.dudeFlag4 && aiInPatrolState(owneractor->xspr.aiState) && owneractor->xspr.data3 > 0 && owneractor->xspr.data3 <= kMaxPatrolSpotValue)
					viewAddEffect(tsprites, nTSprite, kViewEffectSpotProgress);
			}
#endif
			break;
		}
		case kStatTraps: {
			if (owneractor->GetType() == kTrapSawCircular) {
				if (owneractor->xspr.state) {
					if (owneractor->xspr.data1) {
						pTSprite->setspritetexture(aTexIds[kTexCIRCLESAW1]);
						if (owneractor->xspr.data2)
							viewAddEffect(tsprites, nTSprite, kViewEffectSpear);
					}
				}
				else if (owneractor->xspr.data1) pTSprite->setspritetexture(aTexIds[kTexCIRCLESAW2]);
				else pTSprite->setspritetexture(aTexIds[kTexCIRCLESAWOFF]);

			}
			break;
		}
		case kStatThing: {
			viewApplyDefaultPal(pTSprite, pSector);

			if (owneractor->GetType() < kThingBase || owneractor->GetType() >= kThingMax || owneractor->hit.florhit.type == kHitNone)
			{
				if ((pTSprite->flags & kPhysMove) && getflorzofslopeptr(pTSprite->sectp, pTSprite->pos) >= cPos.Z)
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
		switch (GetExtInfo(pTSprite->spritetexture()).picanm.extra & 7)
		{
		case 1:
		{
			nAnim = GetOctant(DVector2(cPos.XY() - pTSprite->pos).Rotated(DAngle22_5 - pTSprite->Angles.Yaw));
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
			nAnim = GetOctant(DVector2(cPos.XY() - pTSprite->pos).Rotated(DAngle22_5 - pTSprite->Angles.Yaw));
			break;
		}
		}
		while (nAnim > 0)
		{
			pTSprite->setspritetexture(pTSprite->spritetexture() + GetExtInfo(pTSprite->spritetexture()).picanm.num + 1);
			nAnim--;
		}
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac)
{
	viewProcessSprites(tsprites, view, viewang, interpfrac);
}

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
