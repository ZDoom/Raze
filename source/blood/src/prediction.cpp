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

void fakePlayerProcess(PLAYER* pPlayer, InputPacket* pInput);
void fakeActProcessSprites(void);

bool gPrediction = true;
VIEW predict, predictOld;
static VIEW predictFifo[256];

void viewInitializePrediction(void)
{
	predict.at30 = gMe->q16ang;
	predict.at24 = gMe->horizon.horiz;
	predict.at28 = gMe->horizon.horizoff;
	predict.at2c = gMe->slope;
	predict.at6f = gMe->cantJump;
	predict.at70 = gMe->isRunning;
	predict.at72 = gMe->isUnderwater;
    predict.at71 = !!(gMe->input.actions & SB_JUMP);
	predict.at50 = gMe->pSprite->x;
	predict.at54 = gMe->pSprite->y;
	predict.at58 = gMe->pSprite->z;
	predict.at68 = gMe->pSprite->sectnum;
	predict.at73 = gMe->pSprite->flags;
	predict.at5c = xvel[gMe->pSprite->index];
	predict.at60 = yvel[gMe->pSprite->index];
	predict.at64 = zvel[gMe->pSprite->index];
	predict.at6a = gMe->pXSprite->height;
	predict.at48 = gMe->posture;
	predict.at4c = gMe->spin;
	predict.at6e = !!(gMe->input.actions & SB_CENTERVIEW);
	memcpy(&predict.at75,&gSpriteHit[gMe->pSprite->extra],sizeof(SPRITEHIT));
	predict.TotalKills = gMe->bobPhase;
	predict.Kills = gMe->bobAmp;
	predict.at8 = gMe->bobHeight;
	predict.atc = gMe->bobWidth;
	predict.at10 = gMe->swayPhase;
	predict.at14 = gMe->swayAmp;
	predict.at18 = gMe->swayHeight;
	predict.at1c = gMe->swayWidth;
	predict.at34 = gMe->zWeapon-gMe->zView-(12<<8);
	predict.at38 = gMe->zView;
	predict.at3c = gMe->zViewVel;
	predict.at40 = gMe->zWeapon;
	predict.at44 = gMe->zWeaponVel;
    predictOld = predict;
}

void viewUpdatePrediction(InputPacket *pInput)
{
    predictOld = predict;
	short bakCstat = gMe->pSprite->cstat;
    gMe->pSprite->cstat = 0;
    fakePlayerProcess(gMe, pInput);
    fakeActProcessSprites();
    gMe->pSprite->cstat = bakCstat;
    //predictFifo[gPredictTail&255] = predict;
    //gPredictTail++;
}

static void sub_158B4(PLAYER *pPlayer)
{
    predict.at38 = predict.at58 - pPlayer->pPosture[pPlayer->lifeMode][predict.at48].eyeAboveZ;
    predict.at40 = predict.at58 - pPlayer->pPosture[pPlayer->lifeMode][predict.at48].weaponAboveZ;
}

static void fakeProcessInput(PLAYER *pPlayer, InputPacket *pInput)
{
    POSTURE *pPosture = &pPlayer->pPosture[pPlayer->lifeMode][predict.at48];

    predict.at70 = !!(gMe->input.actions & SB_RUN);
    predict.at71 = !!(gMe->input.actions & SB_JUMP);
    if (predict.at48 == 1)
    {
        int x = Cos(FixedToInt(predict.at30));
        int y = Sin(FixedToInt(predict.at30));
        if (pInput->fvel)
        {
            int forward = pInput->fvel;
            if (forward > 0)
                forward = mulscale8(pPosture->frontAccel, forward);
            else
                forward = mulscale8(pPosture->backAccel, forward);
            predict.at5c += mulscale30(forward, x);
            predict.at60 += mulscale30(forward, y);
        }
        if (pInput->svel)
        {
            int strafe = pInput->svel;
            strafe = mulscale8(pPosture->sideAccel, strafe);
            predict.at5c += mulscale30(strafe, y);
            predict.at60 -= mulscale30(strafe, x);
        }
    }
    else if (predict.at6a < 0x100)
    {
        int speed = 0x10000;
        if (predict.at6a > 0)
            speed -= divscale16(predict.at6a, 0x100);
        int x = Cos(FixedToInt(predict.at30));
        int y = Sin(FixedToInt(predict.at30));
        if (pInput->fvel)
        {
            int forward = pInput->fvel;
            if (forward > 0)
                forward = mulscale8(pPosture->frontAccel, forward);
            else
                forward = mulscale8(pPosture->backAccel, forward);
            if (predict.at6a)
                forward = mulscale16(forward, speed);
            predict.at5c += mulscale30(forward, x);
            predict.at60 += mulscale30(forward, y);
        }
        if (pInput->svel)
        {
            int strafe = pInput->svel;
            strafe = mulscale8(pPosture->sideAccel, strafe);
            if (predict.at6a)
                strafe = mulscale16(strafe, speed);
            predict.at5c += mulscale30(strafe, y);
            predict.at60 -= mulscale30(strafe, x);
        }
    }
    if (pInput->q16avel)
        predict.at30 = (predict.at30+pInput->q16avel)&0x7ffffff;
    if (pInput->actions & SB_TURNAROUND)
        if (!predict.at4c)
            predict.at4c = -1024;
    if (predict.at4c < 0)
    {
        int speed;
        if (predict.at48 == 1)
            speed = 64;
        else
            speed = 128;

        predict.at4c = min(predict.at4c+speed, 0);
        predict.at30 += IntToFixed(speed);
    }

    if (!predict.at71)
        predict.at6f = 0;

    switch (predict.at48)
    {
    case 1:
        if (predict.at71)
            predict.at64 -= pPosture->normalJumpZ;//0x5b05;
        if (pInput->actions & SB_CROUCH)
            predict.at64 += pPosture->normalJumpZ;//0x5b05;
        break;
    case 2:
        if (!(pInput->actions & SB_CROUCH))
            predict.at48 = 0;
        break;
    default:
        if (!predict.at6f && predict.at71 && predict.at6a == 0) {
            if (packItemActive(pPlayer, 4)) predict.at64 = pPosture->pwupJumpZ;//-0x175555;
            else predict.at64 = pPosture->normalJumpZ;//-0xbaaaa;
            predict.at6f = 1;
        }
        if (pInput->actions & SB_CROUCH)
            predict.at48 = 2;
        break;
    }

#if 0
    if (predict.at6e && !(pInput->actions & (SB_LOOK_UP | SB_LOOK_DOWN)))
    {
        if (predict.at20 < 0)
            predict.at20 = min(predict.at20+IntToFixed(4), 0);
        if (predict.at20 > 0)
            predict.at20 = max(predict.at20-IntToFixed(4), 0);
        if (predict.at20 == 0)
            predict.at6e = 0;
    }
    else
    {
        if (pInput->actions & SB_LOOK_UP)
            predict.at20 = min(predict.at20+IntToFixed(4), IntToFixed(60));
        if (pInput->actions & SB_LOOK_DOWN)
            predict.at20 = max(predict.at20-IntToFixed(4), IntToFixed(-60));
    }
    predict.at20 = clamp(predict.at20+pInput->horz, IntToFixed(-60), IntToFixed(60));

    if (predict.at20 > 0)
        predict.at24 = FloatToFixed(fmulscale30(120., Sinf(FixedToFloat(predict.at20) * 8.)));
    else if (predict.at20 < 0)
        predict.at24 = FloatToFixed(fmulscale30(180., Sinf(FixedToFloat(predict.at20) * 8.)));
    else
        predict.at24 = 0;
#endif

    int nSector = predict.at68;
    int florhit = predict.at75.florhit & 0xc000;
    char va;
    if (predict.at6a < 16 && (florhit == 0x4000 || florhit == 0))
        va = 1;
    else
        va = 0;
    if (va && (sector[nSector].floorstat&2) != 0)
    {
        int z1 = getflorzofslope(nSector, predict.at50, predict.at54);
        int x2 = predict.at50+mulscale30(64, Cos(FixedToInt(predict.at30)));
        int y2 = predict.at54+mulscale30(64, Sin(FixedToInt(predict.at30)));
        short nSector2 = nSector;
        updatesector(x2, y2, &nSector2);
        if (nSector2 == nSector)
        {
            int z2 = getflorzofslope(nSector2, x2, y2);
            predict.at28 = q16horiz(interpolate(predict.at28.asq16(), IntToFixed(z1 - z2) >> 3, 0x4000));
        }
    }
    else
    {
        predict.at28 = q16horiz(interpolate(predict.at28.asq16(), 0, 0x4000));
        if (klabs(predict.at28.asq16()) < 4)
            predict.at28 = q16horiz(0);
    }
    predict.at2c = -predict.at24.asq16() >> 9;
}

void fakePlayerProcess(PLAYER *pPlayer, InputPacket *pInput)
{
    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][predict.at48];

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);

    top += predict.at58-pSprite->z;
    bottom += predict.at58-pSprite->z;

    int dzb = (bottom-predict.at58)/4;
    int dzt = (predict.at58-top)/4;

    int dw = pSprite->clipdist<<2;
    short nSector = predict.at68;
    if (!gNoClip)
    {
        pushmove_old((int32_t*)&predict.at50, (int32_t*)&predict.at54, (int32_t*)&predict.at58, &predict.at68, dw, dzt, dzb, CLIPMASK0);
        if (predict.at68 == -1)
            predict.at68 = nSector;
    }
    fakeProcessInput(pPlayer, pInput);

    int nSpeed = approxDist(predict.at5c, predict.at60);

    predict.at3c = interpolate(predict.at3c, predict.at64, 0x7000);
    int dz = predict.at58-pPosture->eyeAboveZ-predict.at38;
    if (dz > 0)
        predict.at3c += mulscale16(dz<<8, 0xa000);
    else
        predict.at3c += mulscale16(dz<<8, 0x1800);
    predict.at38 += predict.at3c>>8;

    predict.at44 = interpolate(predict.at44, predict.at64, 0x5000);
    dz = predict.at58-pPosture->weaponAboveZ-predict.at40;
    if (dz > 0)
        predict.at44 += mulscale16(dz<<8, 0x8000);
    else
        predict.at44 += mulscale16(dz<<8, 0xc00);
    predict.at40 += predict.at44>>8;

    predict.at34 = predict.at40 - predict.at38 - (12<<8);

    predict.TotalKills = ClipLow(predict.TotalKills-4, 0);

    nSpeed >>= FRACBITS;
	if (predict.at48 == 1)
	{
		predict.Kills = (predict.Kills+17)&2047;
		predict.at14 = (predict.at14+17)&2047;
		predict.at8 = mulscale30(10*pPosture->bobV,Sin(predict.Kills*2));
		predict.atc = mulscale30(predict.TotalKills*pPosture->bobH,Sin(predict.Kills-256));
		predict.at18 = mulscale30(predict.TotalKills*pPosture->swayV,Sin(predict.at14*2));
		predict.at1c = mulscale30(predict.TotalKills*pPosture->swayH,Sin(predict.at14-0x155));
	}
	else
	{
		if (pXSprite->height < 256)
		{
			predict.Kills = (predict.Kills+(pPosture->pace[predict.at70]*4))&2047;
			predict.at14 = (predict.at14+(pPosture->pace[predict.at70]*4)/2)&2047;
			if (predict.at70)
			{
				if (predict.TotalKills < 60)
                    predict.TotalKills = ClipHigh(predict.TotalKills + nSpeed, 60);
			}
			else
			{
				if (predict.TotalKills < 30)
                    predict.TotalKills = ClipHigh(predict.TotalKills + nSpeed, 30);
			}
		}
		predict.at8 = mulscale30(predict.TotalKills*pPosture->bobV,Sin(predict.Kills*2));
		predict.atc = mulscale30(predict.TotalKills*pPosture->bobH,Sin(predict.Kills-256));
		predict.at18 = mulscale30(predict.TotalKills*pPosture->swayV,Sin(predict.at14*2));
		predict.at1c = mulscale30(predict.TotalKills*pPosture->swayH,Sin(predict.at14-0x155));
	}
	if (!pXSprite->health)
        return;
	predict.at72 = 0;
	if (predict.at48 == 1)
	{
		predict.at72 = 1;
        int nSector = predict.at68;
        int nLink = gLowerLink[nSector];
		if (nLink > 0 && (sprite[nLink].type == kMarkerLowGoo || sprite[nLink].type == kMarkerLowWater))
		{
			if (getceilzofslope(nSector, predict.at50, predict.at54) > predict.at38)
				predict.at72 = 0;
		}
	}
}

static void fakeMoveDude(spritetype *pSprite)
{
    PLAYER *pPlayer = NULL;
    int bottom, top;
    if (IsPlayerSprite(pSprite))
        pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    GetSpriteExtents(pSprite, &top, &bottom);
	top += predict.at58 - pSprite->z;
	bottom += predict.at58 - pSprite->z;
    int bz = (bottom-predict.at58)/4;
    int tz = (predict.at58-top)/4;
    int wd = pSprite->clipdist*4;
    int nSector = predict.at68;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    if (predict.at5c || predict.at60)
    {
        if (pPlayer && gNoClip)
        {
            predict.at50 += predict.at5c>>12;
            predict.at54 += predict.at60>>12;
            if (!FindSector(predict.at50, predict.at54, &nSector))
                nSector = predict.at68;
        }
        else
        {
            short bakCstat = pSprite->cstat;
            pSprite->cstat &= ~257;
            predict.at75.hit = ClipMove(&predict.at50, &predict.at54, &predict.at58, &nSector, predict.at5c >> 12, predict.at60 >> 12, wd, tz, bz, CLIPMASK0);
            if (nSector == -1)
                nSector = predict.at68;
                    
            if (sector[nSector].type >= kSectorPath && sector[nSector].type <= kSectorRotate)
            {
                short nSector2 = nSector;
                pushmove_old((int32_t*)&predict.at50, (int32_t*)&predict.at54, (int32_t*)&predict.at58, &nSector2, wd, tz, bz, CLIPMASK0);
                if (nSector2 != -1)
                    nSector = nSector2;
            }

            dassert(nSector >= 0);

            pSprite->cstat = bakCstat;
        }
        switch (predict.at75.hit&0xc000)
        {
        case 0x8000:
        {
            int nHitWall = predict.at75.hit&0x3fff;
            walltype *pHitWall = &wall[nHitWall];
            if (pHitWall->nextsector != -1)
            {
                sectortype *pHitSector = &sector[pHitWall->nextsector];
                if (top < pHitSector->ceilingz || bottom > pHitSector->floorz)
                {
                    // ???
                }
            }
            actWallBounceVector(&predict.at5c, &predict.at60, nHitWall, 0);
            break;
        }
        }
    }
    if (predict.at68 != nSector)
    {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        predict.at68 = nSector;
    }
    char bUnderwater = 0;
    char bDepth = 0;
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->Underwater)
            bUnderwater = 1;
        if (pXSector->Depth)
            bDepth = 1;
    }
    int nUpperLink = gUpperLink[nSector];
    int nLowerLink = gLowerLink[nSector];
    if (nUpperLink >= 0 && (sprite[nUpperLink].type == kMarkerUpWater || sprite[nUpperLink].type == kMarkerUpGoo))
        bDepth = 1;
    if (nLowerLink >= 0 && (sprite[nLowerLink].type == kMarkerLowWater || sprite[nLowerLink].type == kMarkerLowGoo))
        bDepth = 1;
    if (pPlayer)
        wd += 16;

    if (predict.at64)
        predict.at58 += predict.at64 >> 8;

    static_assert(sizeof(tspritetype) == sizeof(spritetype));
    tspritetype pSpriteBak; memcpy(&pSpriteBak, pSprite, sizeof(pSpriteBak)); // how dare you??? (Use a tspritetype here so that if the sprite storage gets refactored, this line gets flagged.)
    spritetype *pTempSprite = pSprite;
    pTempSprite->x = predict.at50;
    pTempSprite->y = predict.at54;
    pTempSprite->z = predict.at58;
    pTempSprite->sectnum = predict.at68;
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pTempSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, CLIPMASK0);
    GetSpriteExtents(pTempSprite, &top, &bottom);
    if (predict.at73 & 2)
    {
        int vc = 58254;
        if (bDepth)
        {
            if (bUnderwater)
            {
                int cz = getceilzofslope(nSector, predict.at50, predict.at54);
                if (cz > top)
                    vc += ((bottom-cz)*-80099) / (bottom-top);
                else
                    vc = 0;
            }
            else
            {
                int fz = getflorzofslope(nSector, predict.at50, predict.at54);
                if (fz < bottom)
                    vc += ((bottom-fz)*-80099) / (bottom-top);
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
            predict.at58 += ((vc*4)/2)>>8;
            predict.at64 += vc;
        }
    }
    GetSpriteExtents(pTempSprite, &top, &bottom);
    if (bottom >= floorZ)
    {
        int floorZ2 = floorZ;
        int floorHit2 = floorHit;
        GetZRange(pTempSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, CLIPMASK0, PARALLAXCLIP_CEILING|PARALLAXCLIP_FLOOR);
        if (bottom <= floorZ && predict.at58-floorZ2 < bz)
        {
            floorZ = floorZ2;
            floorHit = floorHit2;
        }
    }
    if (floorZ <= bottom)
    {
        predict.at75.florhit = floorHit;
        predict.at58 += floorZ-bottom;
        int var44 = predict.at64-velFloor[predict.at68];
        if (var44 > 0)
        {
            actFloorBounceVector(&predict.at5c, &predict.at60, &var44, predict.at68, 0);
            predict.at64 = var44;
            if (klabs(predict.at64) < 0x10000)
            {
                predict.at64 = velFloor[predict.at68];
                predict.at73 &= ~4;
            }
            else
                predict.at73 |= 4;
        }
        else if (predict.at64 == 0)
            predict.at73 &= ~4;
    }
    else
    {
        predict.at75.florhit = 0;
        if (predict.at73 & 2)
            predict.at73 |= 4;
    }
    if (top <= ceilZ)
    {
        predict.at75.ceilhit = ceilHit;
        predict.at58 += ClipLow(ceilZ-top, 0);
        if (predict.at64 <= 0 && (predict.at73&4))
            predict.at64 = mulscale16(-predict.at64, 0x2000);
    }
    else
        predict.at75.ceilhit = 0;

    GetSpriteExtents(pTempSprite, &top, &bottom);
    memcpy(pSprite, &pSpriteBak, sizeof(pSpriteBak));
    predict.at6a = ClipLow(floorZ-bottom, 0)>>8;
    if (predict.at5c || predict.at60)
    {
        if ((floorHit & 0xc000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                predict.at5c += mulscale(4, predict.at50 - sprite[nHitSprite].x, 2);
                predict.at60 += mulscale(4, predict.at54 - sprite[nHitSprite].y, 2);
                return;
            }
        }
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0 && xsector[nXSector].Underwater)
            return;
        if (predict.at6a >= 0x100)
            return;
        int nDrag = gDudeDrag;
        if (predict.at6a > 0)
            nDrag -= scale(gDudeDrag, predict.at6a, 0x100);
        predict.at5c -= mulscale16r(predict.at5c, nDrag);
        predict.at60 -= mulscale16r(predict.at60, nDrag);
        if (approxDist(predict.at5c, predict.at60) < 0x1000)
            predict.at5c = predict.at60 = 0;
    }
}

static void fakeActAirDrag(spritetype *, int num)
{
    int xvec = 0;
    int yvec = 0;
    int nSector = predict.at68;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    if (nXSector > 0)
    {
        dassert(nXSector < kMaxXSectors);
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->windVel && (pXSector->windAlways || pXSector->busy))
        {
            int vel = pXSector->windVel<<12;
            if (!pXSector->windAlways && pXSector->busy)
                vel = mulscale16(vel, pXSector->busy);
            xvec = mulscale30(vel, Cos(pXSector->windAng));
            yvec = mulscale30(vel, Sin(pXSector->windAng));
        }
    }
    predict.at5c += mulscale16(xvec-predict.at5c, num);
    predict.at60 += mulscale16(yvec-predict.at60, num);
    predict.at64 -= mulscale16(predict.at64, num);
}

void fakeActProcessSprites(void)
{
	spritetype *pSprite = gMe->pSprite;
	if (pSprite->statnum == kStatDude)
	{
		int nXSprite = pSprite->extra;
		dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
		int nSector = predict.at68;
		int nXSector = sector[nSector].extra;
        XSECTOR *pXSector = NULL;
        if (nXSector > 0)
        {
            dassert(nXSector > 0 && nXSector < kMaxXSectors);
            dassert(xsector[nXSector].reference == nSector);
            pXSector = &xsector[nXSector];
        }
		if (pXSector)
		{
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
			top += predict.at58 - pSprite->z;
			bottom += predict.at58 - pSprite->z;
			if (getflorzofslope(nSector, predict.at50, predict.at54) < bottom)
			{
				int angle = pXSector->panAngle;
                int speed = 0;
				if (pXSector->panAlways || pXSector->state || pXSector->busy)
				{
					speed = pXSector->panVel << 9;
					if (!pXSector->panAlways && pXSector->busy)
						speed = mulscale16(speed, pXSector->busy);
				}
				if (sector[nSector].floorstat&64)
					angle = (GetWallAngle(sector[nSector].wallptr)+512)&2047;
				predict.at5c += mulscale30(speed,Cos(angle));
				predict.at60 += mulscale30(speed,Sin(angle));
			}
		}
        if (pXSector && pXSector->Underwater)
            fakeActAirDrag(pSprite, 5376);
        else
            fakeActAirDrag(pSprite, 128);

        if ((predict.at73 & 4) != 0 || predict.at5c != 0 || predict.at60 != 0 || predict.at64 != 0 || velFloor[predict.at68] != 0 || velCeil[predict.at68] != 0)
        {
            fakeMoveDude(pSprite);
        }
	}
}

void viewCorrectPrediction(void)
{
#if 0
    spritetype *pSprite = gMe->pSprite;
    VIEW *pView = &predictFifo[(gNetFifoTail-1)&255];
    if (gMe->q16ang != pView->at30 || pView->at24 != gMe->horizon.horiz || pView->at50 != pSprite->x || pView->at54 != pSprite->y || pView->at58 != pSprite->z)
    {
        viewInitializePrediction();
        predictOld = gPrevView[myconnectindex];
        gPredictTail = gNetFifoTail;
        while (gPredictTail < gNetFifoHead[myconnectindex])
        {
            viewUpdatePrediction(&gFifoInput[gPredictTail&255][myconnectindex]);
        }
    }
#endif
}

END_BLD_NS
