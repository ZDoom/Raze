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
#include "choke.h"
#include "zstring.h"
#include "razemenu.h"
#include "gstrings.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "v_font.h"

BEGIN_BLD_NS

void fakePlayerProcess(PLAYER* pPlayer, InputPacket* pInput);
void fakeActProcessSprites(void);

bool gPrediction = true;
VIEW predict, predictOld;
static VIEW predictFifo[256];

void viewInitializePrediction(void)
{
#if 0
	predict.angle = gMe->angle.ang;
	predict.horiz = gMe->horizon.horiz;
	predict.horizoff = gMe->horizon.horizoff;
	predict.slope = gMe->slope;
	predict.cantJump = gMe->cantJump;
	predict.isRunning = gMe->isRunning;
	predict.isUnderwater = gMe->isUnderwater;
	predict.jump = !!(gMe->input.actions & SB_JUMP);
	predict.x = gMe->actor->spr.x;
	predict.y = gMe->actor->spr.y;
	predict.z = gMe->actor->spr.z;
	predict.sector = gMe->actor->spr.sector;
	predict.sprflags = gMe->actor->spr.flags;
	predict.xvel = gMe->actor->xvel;
	predict.yvel = gMe->actor->yvel;
	predict.zvel = gMe->actor->zvel;
	predict.floordist = gMe->actor->xspr.height;
	predict.posture = gMe->posture;
	predict.spin = gMe->angle.spin;
	predict.lookCenter = !!(gMe->input.actions & SB_CENTERVIEW);
	predict.hit = gMe->actor->hit;
	predict.bobPhase = gMe->bobPhase;
	predict.Kills = gMe->bobAmp;
	predict.bobHeight = gMe->bobHeight;
	predict.bobWidth = gMe->bobWidth;
	predict.swayPhase = gMe->swayPhase;
	predict.swayAmp = gMe->swayAmp;
	predict.shakeBobY = gMe->swayHeight;
	predict.shakeBobX = gMe->swayWidth;
	predict.weaponZ = gMe->zWeapon - gMe->zView - (12 << 8);
	predict.viewz = gMe->zView;
	predict.zViewVel = gMe->zViewVel;
	predict.zWeapon = gMe->zWeapon;
	predict.zWeaponVel = gMe->zWeaponVel;
	predictOld = predict;
#endif
}

void viewUpdatePrediction(InputPacket* pInput)
{
	predictOld = predict;
	auto bakCstat = gMe->actor->spr.cstat;
	gMe->actor->spr.cstat = 0;
	fakePlayerProcess(gMe, pInput);
	fakeActProcessSprites();
	gMe->actor->spr.cstat = bakCstat;
	//predictFifo[gPredictTail&255] = predict;
	//gPredictTail++;
}

static void fakeProcessInput(PLAYER* pPlayer, InputPacket* pInput)
{
#if 0
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][predict.posture];

	predict.isRunning = !!(gMe->input.actions & SB_RUN);
	predict.jump = !!(gMe->input.actions & SB_JUMP);
	if (predict.posture == 1)
	{
		int x = Cos(predict.angle.asbuild());
		int y = Sin(predict.angle.asbuild());
		if (pInput->fvel)
		{
			int forward = pInput->fvel;
			if (forward > 0)
				forward = MulScale(pPosture->frontAccel, forward, 8);
			else
				forward = MulScale(pPosture->backAccel, forward, 8);
			predict.xvel += MulScale(forward, x, 30);
			predict.yvel += MulScale(forward, y, 30);
		}
		if (pInput->svel)
		{
			int strafe = pInput->svel;
			strafe = MulScale(pPosture->sideAccel, strafe, 8);
			predict.xvel += MulScale(strafe, y, 30);
			predict.yvel -= MulScale(strafe, x, 30);
		}
	}
	else if (predict.floordist < 0x100)
	{
		int speed = 0x10000;
		if (predict.floordist > 0)
			speed -= DivScale(predict.floordist, 0x100, 16);
		int x = Cos(predict.angle.asbuild());
		int y = Sin(predict.angle.asbuild());
		if (pInput->fvel)
		{
			int forward = pInput->fvel;
			if (forward > 0)
				forward = MulScale(pPosture->frontAccel, forward, 8);
			else
				forward = MulScale(pPosture->backAccel, forward, 8);
			if (predict.floordist)
				forward = MulScale(forward, speed, 16);
			predict.xvel += MulScale(forward, x, 30);
			predict.yvel += MulScale(forward, y, 30);
		}
		if (pInput->svel)
		{
			int strafe = pInput->svel;
			strafe = MulScale(pPosture->sideAccel, strafe, 8);
			if (predict.floordist)
				strafe = MulScale(strafe, speed, 16);
			predict.xvel += MulScale(strafe, y, 30);
			predict.yvel -= MulScale(strafe, x, 30);
		}
	}
	if (pInput->avel)
		predict.angle = degang(pInput->avel);
	if (pInput->actions & SB_TURNAROUND)
		if (!predict.spin)
			predict.spin = -1024;
	if (predict.spin < 0)
	{
		int speed;
		if (predict.posture == 1)
			speed = 64;
		else
			speed = 128;

		predict.spin = min(int(predict.spin) + speed, 0);
		predict.angle += DAngle::fromBuild(speed);
	}

	if (!predict.jump)
		predict.cantJump = 0;

	switch (predict.posture)
	{
	case 1:
		if (predict.jump)
			predict.zvel -= pPosture->normalJumpZ;//0x5b05;
		if (pInput->actions & SB_CROUCH)
			predict.zvel += pPosture->normalJumpZ;//0x5b05;
		break;
	case 2:
		if (!(pInput->actions & SB_CROUCH))
			predict.posture = 0;
		break;
	default:
		if (!predict.cantJump && predict.jump && predict.floordist == 0) {
			if (packItemActive(pPlayer, 4)) predict.zvel = pPosture->pwupJumpZ;//-0x175555;
			else predict.zvel = pPosture->normalJumpZ;//-0xbaaaa;
			predict.cantJump = 1;
		}
		if (pInput->actions & SB_CROUCH)
			predict.posture = 2;
		break;
	}

#if 0
	if (predict.lookCenter && !(pInput->actions & (SB_LOOK_UP | SB_LOOK_DOWN)))
	{
		if (predict.at20 < 0)
			predict.at20 = min(predict.at20 + IntToFixed(4), 0);
		if (predict.at20 > 0)
			predict.at20 = max(predict.at20 - IntToFixed(4), 0);
		if (predict.at20 == 0)
			predict.lookCenter = 0;
	}
	else
	{
		if (pInput->actions & SB_LOOK_UP)
			predict.at20 = min(predict.at20 + IntToFixed(4), IntToFixed(60));
		if (pInput->actions & SB_LOOK_DOWN)
			predict.at20 = max(predict.at20 - IntToFixed(4), IntToFixed(-60));
	}
	predict.at20 = clamp(predict.at20 + pInput->horz, IntToFixed(-60), IntToFixed(60));

	if (predict.at20 > 0)
		predict.at24 = FloatToFixed(MulScaleF(120., bsinf(FixedToFloat(predict.at20) * 8., 16)), 30);
	else if (predict.at20 < 0)
		predict.at24 = FloatToFixed(MulScaleF(180., bsinf(FixedToFloat(predict.at20) * 8., 16)), 30);
	else
		predict.at24 = 0;
#endif

	int nSector = predict.sector;
	int florhit = predict.hit.florhit.type;
	bool va = (predict.floordist < 16 && (florhit == kHitSector || florhit == 0));

	if (va && (sector[nSector].floorstat & 2) != 0)
	{
		int z1 = getflorzofslope(nSector, predict.x, predict.y);
		int x2 = predict.x + MulScale(64, Cos(predict.angle.asbuild()), 30);
		int y2 = predict.y + MulScale(64, Sin(predict.angle.asbuild()), 30);
		int nSector2 = nSector;
		updatesector(x2, y2, &nSector2);
		if (nSector2 == nSector)
		{
			int z2 = getflorzofslope(nSector2, x2, y2);
			predict.horizoff = interpolatedvalue(predict.horizoff, q16horiz((z1 - z2) << 13), 0x4000);
		}
	}
	else
	{
		predict.horizoff = interpolatedvalue(predict.horizoff, q16horiz(0), 0x4000);
		if (abs(predict.horizoff.asq16()) < 4)
			predict.horizoff = q16horiz(0);
	}
	predict.slope = -predict.horiz.asq16() >> 9;
#endif
}

void fakePlayerProcess(PLAYER* pPlayer, InputPacket* pInput)
{
#if 0
	auto pSprite = pPlayer->actor;
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][predict.posture];

	int top, bottom;
	GetSpriteExtents(pSprite, &top, &bottom);

	top += predict.z - pSprite->z;
	bottom += predict.z - pSprite->z;

	int dzb = (bottom - predict.z) / 4;
	int dzt = (predict.z - top) / 4;

	int dw = pSprite->clipdist << 2;
	int nSector = predict.sector;
	if (!gNoClip)
	{
		pushmove(&predict.pos, &predict.sector, dw, dzt, dzb, CLIPMASK0);
		if (predict.sector == -1)
			predict.sector = nSector;
	}
	fakeProcessInput(pPlayer, pInput);

	int nSpeed = approxDist(predict.xvel, predict.yvel);

	predict.zViewVel = interpolatedvalue(predict.zViewVel, predict.zvel, 0x7000 * (1. / MaxSmoothRatio));
	int dz = predict.z - pPosture->eyeAboveZ - predict.viewz;
	if (dz > 0)
		predict.zViewVel += MulScale(dz << 8, 0xa000, 16);
	else
		predict.zViewVel += MulScale(dz << 8, 0x1800, 16);
	predict.viewz += predict.zViewVel >> 8;

	predict.zWeaponVel = interpolatedvalue(predict.zWeaponVel, predict.zvel, 0x5000 * (1. / MaxSmoothRatio));
	dz = predict.z - pPosture->weaponAboveZ - predict.zWeapon;
	if (dz > 0)
		predict.azWeaponVelt44 += MulScale(dz << 8, 0x8000, 16);
	else
		predict.zWeaponVel += MulScale(dz << 8, 0xc00, 16);
	predict.zWeapon += predict.zWeaponVel >> 8;

	predict.weaponZ = predict.zWeapon - predict.viewz - (12 << 8);

	predict.bobPhase = ClipLow(predict.bobPhase - 4, 0);

	nSpeed >>= FRACBITS;
	if (predict.posture == 1)
	{
		predict.Kills = (predict.Kills + 17) & 2047;
		predict.at14 = (predict.at14 + 17) & 2047;
		predict.bobHeight = MulScale(10 * pPosture->bobV, Sin(predict.Kills * 2), 30);
		predict.bobWidth = MulScale(predict.bobPhase * pPosture->bobH, Sin(predict.Kills - 256), 30);
		predict.shakeBobY = MulScale(predict.bobPhase * pPosture->swayV, Sin(predict.at14 * 2), 30);
		predict.shakeBobX = MulScale(predict.bobPhase * pPosture->swayH, Sin(predict.at14 - 0x155), 30);
	}
	else
	{
		if (pXSprite->height < 256)
		{
			predict.Kills = (predict.Kills + (pPosture->pace[predict.isRunning] * 4)) & 2047;
			predict.at14 = (predict.at14 + (pPosture->pace[predict.isRunning] * 4) / 2) & 2047;
			if (predict.isRunning)
			{
				if (predict.bobPhase < 60)
					predict.bobPhase = ClipHigh(predict.bobPhase + nSpeed, 60);
			}
			else
			{
				if (predict.bobPhase < 30)
					predict.bobPhase = ClipHigh(predict.bobPhase + nSpeed, 30);
			}
		}
		predict.bobHeight = MulScale(predict.bobPhase * pPosture->bobV, Sin(predict.Kills * 2), 30);
		predict.bobWidth = MulScale(predict.bobPhase * pPosture->bobH, Sin(predict.Kills - 256), 30);
		predict.shakeBobY = MulScale(predict.bobPhase * pPosture->swayV, Sin(predict.at14 * 2), 30);
		predict.shakeBobX = MulScale(predict.bobPhase * pPosture->swayH, Sin(predict.at14 - 0x155), 30);
	}
	if (!pXSprite->health)
		return;
	predict.isUnderwater = 0;
	if (predict.posture == 1)
	{
		predict.isUnderwater = 1;
		int nSector = predict.sector;
		auto nLink = getLowerLink(nSector);
		if (nLink && (nLink->spr.type == kMarkerLowGoo || nLink->spr.type == kMarkerLowWater))
		{
			if (getceilzofslope(nSector, predict.x, predict.y) > predict.viewz)
				predict.isUnderwater = 0;
		}
	}
#endif
}

static void fakeMoveDude(DBloodActor* actor)
{
#if 0 // not needed for single player, temporarily disabled due to icompatibilities with the refactored API.
	PLAYER* pPlayer = NULL;
	int bottom, top;
	if (IsPlayerSprite(pSprite))
		pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	GetSpriteExtents(pSprite, &top, &bottom);
	top += predict.z - pSprite->z;
	bottom += predict.z - pSprite->z;
	int bz = (bottom - predict.z) / 4;
	int tz = (predict.z - top) / 4;
	int wd = pSprite->clipdist * 4;
	int nSector = predict.sector;
	assert(validSectorIndex(nSector));
	if (predict.xvel || predict.yvel)
	{
		if (pPlayer && gNoClip)
		{
			predict.x += predict.xvel >> 12;
			predict.y += predict.yvel >> 12;
			if (!FindSector(predict.x, predict.y, &nSector))
				nSector = predict.sector;
		}
		else
		{
			auto bakCstat = pSprite->cstat;
			pSprite->cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			ClipMove(&predict.pos, &nSector, predict.xvel >> 12, predict.yvel >> 12, wd, tz, bz, CLIPMASK0, predict.hit.hit);
			if (nSector == -1)
				nSector = predict.sector;

			if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate)
			{
				int nSector2 = nSector;
				pushmove(&predict.pos, &nSector2, wd, tz, bz, CLIPMASK0);
				if (nSector2 != -1)
					nSector = nSector2;
			}

			assert(nSector >= 0);

			pSprite->cstat = bakCstat;
		}
		switch (predict.hit.hit.type)
		{
		case kHitSprite:
		{
			int nHitWall = predict.hit.hit.index;
			walltype* pHitWall = &wall[nHitWall];
			if (pHitWall->twoSided())
			{
				sectortype* pHitSector = &pHitWall->nextSector();
				if (top < pHitSector->ceiling z || bottom > pHitSector->floor z)
				{
					// ???
				}
			}
			actWallBounceVector(&predict.xvel, &predict.yvel, pHitWall, 0);
			break;
		}
		}
	}
	if (predict.sector != nSector)
	{
		assert(validSectorIndex(nSector));
		predict.sector = nSector;
	}
	bool bUnderwater = 0;
	bool bDepth = 0;
	if (sector[sector].hasX())
	{
		XSECTOR* pXSector = &sector[sector].xs();
		if (pXSector->Underwater)
			bUnderwater = 1;
		if (pXSector->Depth)
			bDepth = 1;
	}
	auto nUpperLink = getUpperLink(nSector);
	auto nLowerLink = getLowerLink(nSector);
	if (nUpperLink >= 0 && (nUpperLink->spr.type == kMarkerUpWater || nUpperLink->spr.type == kMarkerUpGoo))
		bDepth = 1;
	if (nLowerLink >= 0 && (nLowerLink->spr.type == kMarkerLowWater || nLowerLink->spr.type == kMarkerLowGoo))
		bDepth = 1;
	if (pPlayer)
		wd += 16;

	if (predict.zvel)
		predict.z += predict.zvel >> 8;

	spritetype pSpriteBak; memcpy(&pSpriteBak, pSprite, sizeof(pSpriteBak));
	auto pTempSprite = pSprite;
	pTempSprite->x = predict.x;
	pTempSprite->y = predict.y;
	pTempSprite->z = predict.z;
	pTempSprite->sector = predict.sector;
	int ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(pTempSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, wd, CLIPMASK0);
	GetSpriteExtents(pTempSprite, &top, &bottom);
	if (predict.sprflags & 2)
	{
		int vc = 58254;
		if (bDepth)
		{
			if (bUnderwater)
			{
				int cz = getceilzofslope(nSector, predict.x, predict.y);
				if (cz > top)
					vc += ((bottom - cz) * -80099) / (bottom - top);
				else
					vc = 0;
			}
			else
			{
				int fz = getflorzofslope(nSector, predict.x, predict.y);
				if (fz < bottom)
					vc += ((bottom - fz) * -80099) / (bottom - top);
			}
		}
		else
		{
			if (bUnderwater)
				vc = 0;
			else if (bottom >= floorZ)
				vc = 0;
		}
		if (vc)
		{
			predict.z += ((vc * 4) / 2) >> 8;
			predict.zvel += vc;
		}
	}
	GetSpriteExtents(pTempSprite, &top, &bottom);
	if (bottom >= floorZ)
	{
		int floorZ2 = floorZ;
		auto floorHit2 = floorColl;
		GetZRange(pTempSprite, &ceilZ, &ceilColl, &floorZ, &floorColl, pSprite->clipdist << 2, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (bottom <= floorZ && predict.z - floorZ2 < bz)
		{
			floorZ = floorZ2;
			floorColl = floorHit2;
		}
	}
	if (floorZ <= bottom)
	{
		predict.hit.florhit = floorColl;
		predict.z += floorZ - bottom;
		int var44 = predict.zvel - predict.sector()->velFloor;
		if (var44 > 0)
		{
			actFloorBounceVector(&predict.xvel, &predict.yvel, &var44, predict.sector, 0);
			predict.zvel = var44;
			if (abs(predict.zvel) < 0x10000)
			{
				predict.zvel = predict.sector()->velFloor;
				predict.sprflags &= ~4;
			}
			else
				predict.sprflags |= 4;
		}
		else if (predict.zvel == 0)
			predict.sprflags &= ~4;
	}
	else
	{
		predict.hit.florhit.setNone();
		if (predict.sprflags & 2)
			predict.sprflags |= 4;
	}
	if (top <= ceilZ)
	{
		predict.hit.ceilhit = ceilColl;
		predict.z += ClipLow(ceilZ - top, 0);
		if (predict.zvel <= 0 && (predict.sprflags & 4))
			predict.zvel = MulScale(-predict.zvel, 0x2000, 16);
	}
	else
		predict.hit.ceilhit = 0;

	GetSpriteExtents(pTempSprite, &top, &bottom);
	memcpy(pSprite, &pSpriteBak, sizeof(pSpriteBak));
	predict.floordist = ClipLow(floorZ - bottom, 0) >> 8;
	if (predict.xvel || predict.yvel)
	{
		if (floorColl.type == kHitSprite)
		{
			auto hitactor = floorColl.actor;
			if ((hitactor->spr.cstat & 0x30) == 0)
			{
				predict.xvel += MulScale(4, predict.x - hitactor->spr.x, 2);
				predict.yvel += MulScale(4, predict.y - hitactor->spr.y, 2);
				return;
			}
		}
		if (pSprite->sector()->hasX() && pSprite->sector()->xs().Underwater)
			return;
		if (predict.floordist >= 0x100)
			return;
		int nDrag = gDudeDrag;
		if (predict.floordist > 0)
			nDrag -= scale(gDudeDrag, predict.floordist, 0x100);
		predict.xvel -= mulscale16r(predict.xvel, nDrag);
		predict.yvel -= mulscale16r(predict.yvel, nDrag);
		if (approxDist(predict.xvel, predict.yvel) < 0x1000)
			predict.xvel = predict.yvel = 0;
	}
#endif
}

static void fakeActAirDrag(DBloodActor*, int num)
{
#if 0
	int xvec = 0;
	int yvec = 0;
	assert(validSectorIndex(predict.sector));
	sectortype* pSector = &sector[predict.sector];
	if (pSector->hasX())
	{
		XSECTOR* pXSector = &pSector->xs();
		if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
		{
			int vel = pXSector->windVel << 12;
			if (!pXSector->windAlways && pXSector->busy)
				vel = MulScale(vel, pXSector->busy, 16);
			xvec = MulScale(vel, Cos(pXSector->windAng), 30);
			yvec = MulScale(vel, Sin(pXSector->windAng), 30);
		}
	}
	predict.xvel += MulScale(xvec - predict.xvel, num, 16);
	predict.yvel += MulScale(yvec - predict.yvel, num, 16);
	predict.zvel -= MulScale(predict.zvel, num, 16);
#endif
}

void fakeActProcessSprites(void)
{
#if 0
	auto pSprite = gMe->actor;
	if (pSprite->statnum == kStatDude)
	{
		int nSector = predict.sector;
		auto pSector = &sector[predict.sector];
		auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;
		if (pXSector)
		{
			int top, bottom;
			GetSpriteExtents(pSprite, &top, &bottom);
			top += predict.z - pSprite->z;
			bottom += predict.z - pSprite->z;
			if (getflorzofslope(nSector, predict.x, predict.y) < bottom)
			{
				int angle = pXSector->panAngle;
				int speed = 0;
				if (pXSector->panAlways || pXSector->state || pXSector->busy)
				{
					speed = pXSector->panVel << 9;
					if (!pXSector->panAlways && pXSector->busy)
						speed = MulScale(speed, pXSector->busy, 16);
				}
				if (pSector->floorstat & 64)
					angle = (GetWallAngle(pSector->firstWall()) + 512) & 2047;
				predict.xvel += MulScale(speed, Cos(angle), 30);
				predict.yvel += MulScale(speed, Sin(angle), 30);
			}
		}
		if (pXSector && pXSector->Underwater)
			fakeActAirDrag(pSprite, 5376);
		else
			fakeActAirDrag(pSprite, 128);

		if ((predict.sprflags & 4) != 0 || predict.xvel != 0 || predict.yvel != 0 || predict.zvel != 0 || predict.sector()->velFloor != 0 || predict.sector()->velCeil != 0)
		{
			fakeMoveDude(pSprite);
		}
	}
#endif
}

void viewCorrectPrediction(void)
{
#if 0
	auto pSprite = gMe->actor;
	VIEW* pView = &predictFifo[(gNetFifoTail - 1) & 255];
	if (gMe->angle.ang != pView->at30 || pView->at24 != gMe->horizon.horiz || pView->at50 != pSprite->x || pView->at54 != pSprite->y || pView->at58 != pSprite->z)
	{
		viewInitializePrediction();
		predictOld = gPrevView[myconnectindex];
		gPredictTail = gNetFifoTail;
		while (gPredictTail < gNetFifoHead[myconnectindex])
		{
			viewUpdatePrediction(&gFifoInput[gPredictTail & 255][myconnectindex]);
		}
	}
#endif
}

END_BLD_NS
