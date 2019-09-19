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
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "a.h"
#include "build.h"
#include "colmatch.h"
#include "pragmas.h"
#include "mmulti.h"
#include "osd.h"
#include "common_game.h"

#include "aihand.h"
#include "blood.h"
#include "choke.h"
#include "config.h"
#include "db.h"
#include "endgame.h"
#include "gamemenu.h"
#include "gameutil.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "map2d.h"
#include "messages.h"
#include "menu.h"
#include "mirrors.h"
#include "network.h"
#include "player.h"
#include "replace.h"
#include "screen.h"
#include "sectorfx.h"
#include "tile.h"
#include "trig.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"

struct VIEW {
	int at0;
	int at4;
	int at8; // bob height
	int atc; // bob width
	int at10;
	int at14;
	int at18; // bob sway y
	int at1c; // bob sway x
	fix16_t at20;
	fix16_t at24; // horiz
	int at28; // horizoff
	int at2c;
	fix16_t at30; // angle
	int at34; // weapon z
	int at38; // view z
	int at3c;
	int at40;
	int at44;
	int at48; // posture
	int at4c; // spin
	int at50; // x
	int at54; // y
	int at58; // z
	int at5c; //xvel
	int at60; //yvel
	int at64; //zvel
	short at68; // sectnum
	unsigned int at6a; // floordist
	char at6e; // look center
	char at6f;
	char at70; // run
	char at71; // jump
	char at72; // underwater
	short at73; // sprite flags
	SPRITEHIT at75;
};

VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

struct INTERPOLATE {
    void *pointer;
    int value;
    int value2;
    INTERPOLATE_TYPE type;
};

int pcBackground;
int gViewMode = 3;
int gViewSize = 2;

VIEW predict, predictOld;

VIEW predictFifo[256];

int gInterpolate;
int nInterpolations;
char gInterpolateSprite[512];
char gInterpolateWall[1024];
char gInterpolateSector[128];

INTERPOLATE gInterpolation[4096];

int gViewXCenter, gViewYCenter;
int gViewX0, gViewY0, gViewX1, gViewY1;
int gViewX0S, gViewY0S, gViewX1S, gViewY1S;
int xscale, xscalecorrect, yscale, xstep, ystep;

int gScreenTilt;

CGameMessageMgr gGameMessageMgr;

bool bLoadScreenCrcMatch = false;

void RotateYZ(int *pX, int *pY, int *pZ, int ang)
{
    UNREFERENCED_PARAMETER(pX);
	int oY, oZ, angSin, angCos;
	oY = *pY;
	oZ = *pZ;
	angSin = Sin(ang);
	angCos = Cos(ang);
	*pY = dmulscale30r(oY,angCos,oZ,-angSin);
	*pZ = dmulscale30r(oY,angSin,oZ,angCos);
}

void RotateXZ(int *pX, int *pY, int *pZ, int ang)
{
    UNREFERENCED_PARAMETER(pY);
	int oX, oZ, angSin, angCos;
	oX = *pX;
	oZ = *pZ;
    angSin = Sin(ang);
    angCos = Cos(ang);
	*pX = dmulscale30r(oX,angCos,oZ,-angSin);
	*pZ = dmulscale30r(oX,angSin,oZ,angCos);
}

void RotateXY(int *pX, int *pY, int *pZ, int ang)
{
    UNREFERENCED_PARAMETER(pZ);
	int oX, oY, angSin, angCos;
	oX = *pX;
	oY = *pY;
    angSin = Sin(ang);
    angCos = Cos(ang);
	*pX = dmulscale30r(oX,angCos,oY,-angSin);
	*pY = dmulscale30r(oX,angSin,oY,angCos);
}

FONT gFont[5];

void FontSet(int id, int tile, int space)
{
	if (id < 0 || id >= 5 || tile < 0 || tile >= kMaxTiles)
		return;

	FONT *pFont = &gFont[id];
	int xSize = 0;
	int ySize = 0;
	pFont->tile = tile;
	for (int i = 0; i < 96; i++)
	{
		if (tilesiz[tile+i].x > xSize)
			xSize = tilesiz[tile+i].x;
		if (tilesiz[tile+i].y > ySize)
			ySize = tilesiz[tile+i].y;
	}
	pFont->xSize = xSize;
	pFont->ySize = ySize;
	pFont->space = space;
}

void viewGetFontInfo(int id, const char *unk1, int *pXSize, int *pYSize)
{
	if (id < 0 || id >= 5)
		return;
	FONT *pFont = &gFont[id];
	if (!unk1)
	{
		if (pXSize)
			*pXSize = pFont->xSize;
		if (pYSize)
			*pYSize = pFont->ySize;
	}
	else
	{
		int width = -pFont->space;
		for (const char *pBuf = unk1; *pBuf != 0; pBuf++)
		{
			int tile = ((*pBuf-32)&127)+pFont->tile;
			if (tilesiz[tile].x != 0 && tilesiz[tile].y != 0)
				width += tilesiz[tile].x+pFont->space;
		}
		if (pXSize)
			*pXSize = width;
		if (pYSize)
			*pYSize = pFont->ySize;
	}
}

void viewUpdatePages(void)
{
	pcBackground = numpages;
}

void viewToggle(int viewMode)
{
	if (viewMode == 3)
		gViewMode = 4;
	else
	{
		gViewMode = 3;
		viewResizeView(gViewSize);
	}
}

void viewInitializePrediction(void)
{
	predict.at30 = gMe->q16ang;
	predict.at20 = gMe->q16look;
	predict.at24 = gMe->q16horiz;
	predict.at28 = gMe->q16slopehoriz;
	predict.at2c = gMe->at83;
	predict.at6f = gMe->at31c;
	predict.at70 = gMe->at2e;
	predict.at72 = gMe->at87;
	predict.at71 = gMe->atc.buttonFlags.jump;
	predict.at50 = gMe->pSprite->x;
	predict.at54 = gMe->pSprite->y;
	predict.at58 = gMe->pSprite->z;
	predict.at68 = gMe->pSprite->sectnum;
	predict.at73 = gMe->pSprite->hitag;
	predict.at5c = xvel[gMe->pSprite->index];
	predict.at60 = yvel[gMe->pSprite->index];
	predict.at64 = zvel[gMe->pSprite->index];
	predict.at6a = gMe->pXSprite->height;
	predict.at48 = gMe->at2f;
	predict.at4c = gMe->at316;
	predict.at6e = gMe->atc.keyFlags.lookCenter;
	memcpy(&predict.at75,&gSpriteHit[gMe->pSprite->extra],sizeof(SPRITEHIT));
	predict.at0 = gMe->at37;
	predict.at4 = gMe->at3b;
	predict.at8 = gMe->at3f;
	predict.atc = gMe->at43;
	predict.at10 = gMe->at47;
	predict.at14 = gMe->at4b;
	predict.at18 = gMe->at4f;
	predict.at1c = gMe->at53;
	predict.at34 = gMe->at6f-gMe->at67-(12<<8);
	predict.at38 = gMe->at67;
	predict.at3c = gMe->at6b;
	predict.at40 = gMe->at6f;
	predict.at44 = gMe->at73;
    predictOld = predict;
}

void viewUpdatePrediction(GINPUT *pInput)
{
    predictOld = predict;
	short bakCstat = gMe->pSprite->cstat;
    gMe->pSprite->cstat = 0;
    fakePlayerProcess(gMe, pInput);
    fakeActProcessSprites();
    gMe->pSprite->cstat = bakCstat;
    predictFifo[gPredictTail&255] = predict;
    gPredictTail++;
}

void sub_158B4(PLAYER *pPlayer)
{
    predict.at38 = predict.at58 - gPosture[pPlayer->at5f][predict.at48].at24;
    predict.at40 = predict.at58 - gPosture[pPlayer->at5f][predict.at48].at28;
}

void fakeProcessInput(PLAYER *pPlayer, GINPUT *pInput)
{
    POSTURE *pPosture = &gPosture[pPlayer->at5f][predict.at48];
    predict.at70 = pInput->syncFlags.run;
    predict.at70 = 0;
    predict.at71 = pInput->buttonFlags.jump;
    if (predict.at48 == 1)
    {
        int x = Cos(fix16_to_int(predict.at30));
        int y = Sin(fix16_to_int(predict.at30));
        if (pInput->forward)
        {
            int forward = pInput->forward;
            if (forward > 0)
                forward = mulscale8(pPosture->at0, forward);
            else
                forward = mulscale8(pPosture->at8, forward);
            predict.at5c += mulscale30(forward, x);
            predict.at60 += mulscale30(forward, y);
        }
        if (pInput->strafe)
        {
            int strafe = pInput->strafe;
            strafe = mulscale8(pPosture->at4, strafe);
            predict.at5c += mulscale30(strafe, y);
            predict.at60 -= mulscale30(strafe, x);
        }
    }
    else if (predict.at6a < 0x100)
    {
        int speed = 0x10000;
        if (predict.at6a > 0)
            speed -= divscale16(predict.at6a, 0x100);
        int x = Cos(fix16_to_int(predict.at30));
        int y = Sin(fix16_to_int(predict.at30));
        if (pInput->forward)
        {
            int forward = pInput->forward;
            if (forward > 0)
                forward = mulscale8(pPosture->at0, forward);
            else
                forward = mulscale8(pPosture->at8, forward);
            if (predict.at6a)
                forward = mulscale16(forward, speed);
            predict.at5c += mulscale30(forward, x);
            predict.at60 += mulscale30(forward, y);
        }
        if (pInput->strafe)
        {
            int strafe = pInput->strafe;
            strafe = mulscale8(pPosture->at4, strafe);
            if (predict.at6a)
                strafe = mulscale16(strafe, speed);
            predict.at5c += mulscale30(strafe, y);
            predict.at60 -= mulscale30(strafe, x);
        }
    }
    if (pInput->q16turn)
        predict.at30 = (predict.at30+pInput->q16turn)&0x7ffffff;
    if (pInput->keyFlags.spin180)
        if (!predict.at4c)
            predict.at4c = -1024;
    if (predict.at4c < 0)
    {
        int speed;
        if (predict.at48 == 1)
            speed = 64;
        else
            speed = 128;

        predict.at4c = ClipLow(predict.at4c+speed, 0);
        predict.at30 += fix16_from_int(speed);
    }

    if (!predict.at71)
        predict.at6f = 0;

    switch (predict.at48)
    {
    case 1:
        if (predict.at71)
            predict.at64 -= 0x5b05;
        if (pInput->buttonFlags.crouch)
            predict.at64 += 0x5b05;
        break;
    case 2:
        if (!pInput->buttonFlags.crouch)
            predict.at48 = 0;
        break;
    default:
        if (!predict.at6f && predict.at71 && predict.at6a == 0)
        {
            if (packItemActive(pPlayer, 4))
                predict.at64 = -0x175555;
            else
                predict.at64 = -0xbaaaa;
            predict.at6f = 1;
        }
        if (pInput->buttonFlags.crouch)
            predict.at48 = 2;
        break;
    }
#if 0
    if (predict.at6e && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
    {
        if (predict.at20 < 0)
            predict.at20 = fix16_min(predict.at20+F16(4), F16(0));
        if (predict.at20 > 0)
            predict.at20 = fix16_max(predict.at20-F16(4), F16(0));
        if (predict.at20 == 0)
            predict.at6e = 0;
    }
    else
    {
        if (pInput->buttonFlags.lookUp)
            predict.at20 = fix16_min(predict.at20+F16(4), F16(60));
        if (pInput->buttonFlags.lookDown)
            predict.at20 = fix16_max(predict.at20-F16(4), F16(-60));
    }
    predict.at20 = fix16_clamp(predict.at20+pInput->q16mlook, F16(-60), F16(60));

    if (predict.at20 > 0)
        predict.at24 = mulscale30(F16(120), Sin(fix16_to_int(predict.at20<<3)));
    else if (predict.at20 < 0)
        predict.at24 = mulscale30(F16(180), Sin(fix16_to_int(predict.at20<<3)));
    else
        predict.at24 = 0;
#endif
    CONSTEXPR int upAngle = 289;
    CONSTEXPR int downAngle = -347;
    CONSTEXPR double lookStepUp = 4.0*upAngle/60.0;
    CONSTEXPR double lookStepDown = -4.0*downAngle/60.0;
    if (predict.at6e && !pInput->buttonFlags.lookUp && !pInput->buttonFlags.lookDown)
    {
        if (predict.at20 < 0)
            predict.at20 = fix16_min(predict.at20+F16(lookStepDown), F16(0));
        if (predict.at20 > 0)
            predict.at20 = fix16_max(predict.at20-F16(lookStepUp), F16(0));
        if (predict.at20 == 0)
            predict.at6e = 0;
    }
    else
    {
        if (pInput->buttonFlags.lookUp)
            predict.at20 = fix16_min(predict.at20+F16(lookStepUp), F16(upAngle));
        if (pInput->buttonFlags.lookDown)
            predict.at20 = fix16_max(predict.at20-F16(lookStepDown), F16(downAngle));
    }
    predict.at20 = fix16_clamp(predict.at20+(pInput->q16mlook<<3), F16(downAngle), F16(upAngle));
    predict.at24 = fix16_from_float(100.f*tanf(fix16_to_float(predict.at20)*fPI/1024.f));

    int nSector = predict.at68;
    int florhit = predict.at75.florhit & 0xe000;
    char va;
    if (predict.at6a < 16 && (florhit == 0x4000 || florhit == 0))
        va = 1;
    else
        va = 0;
    if (va && (sector[nSector].floorstat&2) != 0)
    {
        int z1 = getflorzofslope(nSector, predict.at50, predict.at54);
        int x2 = predict.at50+mulscale30(64, Cos(fix16_to_int(predict.at30)));
        int y2 = predict.at54+mulscale30(64, Sin(fix16_to_int(predict.at30)));
        short nSector2 = nSector;
        updatesector(x2, y2, &nSector2);
        if (nSector2 == nSector)
        {
            int z2 = getflorzofslope(nSector2, x2, y2);
            predict.at28 = interpolate(predict.at28, fix16_from_int(z1-z2)>>3, 0x4000);
        }
    }
    else
    {
        predict.at28 = interpolate(predict.at28, 0, 0x4000);
        if (klabs(predict.at28) < 4)
            predict.at28 = 0;
    }
    predict.at2c = (-fix16_to_int(predict.at24))<<7;
}

void fakePlayerProcess(PLAYER *pPlayer, GINPUT *pInput)
{
    spritetype *pSprite = pPlayer->pSprite;
    XSPRITE *pXSprite = pPlayer->pXSprite;
    POSTURE *pPosture = &gPosture[pPlayer->at5f][predict.at48];

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
    int dz = predict.at58-pPosture->at24-predict.at38;
    if (dz > 0)
        predict.at3c += mulscale16(dz<<8, 0xa000);
    else
        predict.at3c += mulscale16(dz<<8, 0x1800);
    predict.at38 += predict.at3c>>8;

    predict.at44 = interpolate(predict.at44, predict.at64, 0x5000);
    dz = predict.at58-pPosture->at28-predict.at40;
    if (dz > 0)
        predict.at44 += mulscale16(dz<<8, 0x8000);
    else
        predict.at44 += mulscale16(dz<<8, 0xc00);
    predict.at40 += predict.at44>>8;

    predict.at34 = predict.at40 - predict.at38 - (12<<8);

    predict.at0 = ClipLow(predict.at0-4, 0);

    nSpeed >>= 16;
	if (predict.at48 == 1)
	{
		predict.at4 = (predict.at4+17)&2047;
		predict.at14 = (predict.at14+17)&2047;
		predict.at8 = mulscale30(10*pPosture->at14,Sin(predict.at4*2));
		predict.atc = mulscale30(predict.at0*pPosture->at18,Sin(predict.at4-256));
		predict.at18 = mulscale30(predict.at0*pPosture->at1c,Sin(predict.at14*2));
		predict.at1c = mulscale30(predict.at0*pPosture->at20,Sin(predict.at14-0x155));
	}
	else
	{
		if (pXSprite->height < 256)
		{
			predict.at4 = (predict.at4+(pPosture->atc[predict.at70]*4))&2047;
			predict.at14 = (predict.at14+(pPosture->atc[predict.at70]*4)/2)&2047;
			if (predict.at70)
			{
				if (predict.at0 < 60)
                    predict.at0 = ClipHigh(predict.at0 + nSpeed, 60);
			}
			else
			{
				if (predict.at0 < 30)
                    predict.at0 = ClipHigh(predict.at0 + nSpeed, 30);
			}
		}
		predict.at8 = mulscale30(predict.at0*pPosture->at14,Sin(predict.at4*2));
		predict.atc = mulscale30(predict.at0*pPosture->at18,Sin(predict.at4-256));
		predict.at18 = mulscale30(predict.at0*pPosture->at1c,Sin(predict.at14*2));
		predict.at1c = mulscale30(predict.at0*pPosture->at20,Sin(predict.at14-0x155));
	}
	if (!pXSprite->health)
        return;
	predict.at72 = 0;
	if (predict.at48 == 1)
	{
		predict.at72 = 1;
        int nSector = predict.at68;
        int nLink = gLowerLink[nSector];
		if (nLink > 0 && (sprite[nLink].type == 14 || sprite[nLink].type == 10))
		{
			if (getceilzofslope(nSector, predict.at50, predict.at54) > predict.at38)
				predict.at72 = 0;
		}
	}
}

void fakeMoveDude(spritetype *pSprite)
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
            predict.at75.hit = ClipMove(&predict.at50, &predict.at54, &predict.at58, &nSector, predict.at5c >> 12, predict.at60 >> 12, wd, tz, bz, 0x13001);
            if (nSector == -1)
                nSector = predict.at68;
                    
            if (sector[nSector].lotag >= 612 && sector[nSector].lotag <= 617)
            {
                short nSector2 = nSector;
                pushmove_old((int32_t*)&predict.at50, (int32_t*)&predict.at54, (int32_t*)&predict.at58, &nSector2, wd, tz, bz, 0x10001);
                if (nSector2 != -1)
                    nSector = nSector2;
            }

            dassert(nSector >= 0);

            pSprite->cstat = bakCstat;
        }
        switch (predict.at75.hit&0xe000)
        {
        case 0x8000:
        {
            int nHitWall = predict.at75.hit&0x1fff;
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
    if (nUpperLink >= 0 && (sprite[nUpperLink].type == 9 || sprite[nUpperLink].type == 13))
        bDepth = 1;
    if (nLowerLink >= 0 && (sprite[nLowerLink].type == 10 || sprite[nLowerLink].type == 14))
        bDepth = 1;
    if (pPlayer)
        wd += 16;

    if (predict.at64)
        predict.at58 += predict.at64 >> 8;

    spritetype pSpriteBak = *pSprite;
    spritetype *pTempSprite = pSprite;
    pTempSprite->x = predict.at50;
    pTempSprite->y = predict.at54;
    pTempSprite->z = predict.at58;
    pTempSprite->sectnum = predict.at68;
    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pTempSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, wd, 0x10001);
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
        GetZRange(pTempSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist<<2, 0x13001);
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
    *pSprite = pSpriteBak;
    predict.at6a = ClipLow(floorZ-bottom, 0)>>8;
    if (predict.at5c || predict.at60)
    {
        if ((floorHit & 0xe000) == 0xc000)
        {
            int nHitSprite = floorHit & 0x1fff;
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

void fakeActAirDrag(spritetype *pSprite, int num)
{
    UNREFERENCED_PARAMETER(pSprite);
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
	if (pSprite->statnum == 6)
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
    if (gGameOptions.nGameType == 0) return;
    spritetype *pSprite = gMe->pSprite;
    VIEW *pView = &predictFifo[(gNetFifoTail-1)&255];
    if (gMe->q16ang != pView->at30 || pView->at24 != gMe->q16horiz || pView->at50 != pSprite->x || pView->at54 != pSprite->y || pView->at58 != pSprite->z)
    {
        viewInitializePrediction();
        predictOld = gPrevView[myconnectindex];
        gPredictTail = gNetFifoTail;
        while (gPredictTail < gNetFifoHead[myconnectindex])
        {
            viewUpdatePrediction(&gFifoInput[gPredictTail&255][myconnectindex]);
        }
    }
}

void viewBackupView(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at30 = pPlayer->q16ang;
    pView->at50 = pPlayer->pSprite->x;
    pView->at54 = pPlayer->pSprite->y;
    pView->at38 = pPlayer->at67;
    pView->at34 = pPlayer->at6f-pPlayer->at67-0xc00;
    pView->at24 = pPlayer->q16horiz;
    pView->at28 = pPlayer->q16slopehoriz;
    pView->at2c = pPlayer->at83;
    pView->at8 = pPlayer->at3f;
    pView->atc = pPlayer->at43;
    pView->at18 = pPlayer->at4f;
    pView->at1c = pPlayer->at53;
}

void viewClearInterpolations(void)
{
    nInterpolations = 0;
    memset(gInterpolateSprite, 0, sizeof(gInterpolateSprite));
    memset(gInterpolateWall, 0, sizeof(gInterpolateWall));
    memset(gInterpolateSector, 0, sizeof(gInterpolateSector));
}

void viewAddInterpolation(void *data, INTERPOLATE_TYPE type)
{
    if (nInterpolations == 4096)
        ThrowError("Too many interpolations");
    INTERPOLATE *pInterpolate = &gInterpolation[nInterpolations++];
    pInterpolate->pointer = data;
    pInterpolate->type = type;
    switch (type)
    {
    case INTERPOLATE_TYPE_INT:
        pInterpolate->value = *((int*)data);
        break;
    case INTERPOLATE_TYPE_SHORT:
        pInterpolate->value = *((short*)data);
        break;
    }
}

void CalcInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
        {
            pInterpolate->value2 = *((int*)pInterpolate->pointer);
            int newValue = interpolate(pInterpolate->value, *((int*)pInterpolate->pointer), gInterpolate);
            *((int*)pInterpolate->pointer) = newValue;
            break;
        }
        case INTERPOLATE_TYPE_SHORT:
        {
            pInterpolate->value2 = *((short*)pInterpolate->pointer);
            int newValue = interpolate(pInterpolate->value, *((short*)pInterpolate->pointer), gInterpolate);
            *((short*)pInterpolate->pointer) = newValue;
            break;
        }
        }
    }
}

void RestoreInterpolations(void)
{
    int i;
    INTERPOLATE *pInterpolate = gInterpolation;
    for (i = 0; i < nInterpolations; i++, pInterpolate++)
    {
        switch (pInterpolate->type)
        {
        case INTERPOLATE_TYPE_INT:
            *((int*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        case INTERPOLATE_TYPE_SHORT:
            *((short*)pInterpolate->pointer) = pInterpolate->value2;
            break;
        }
    }
}

void viewDrawText(int nFont, const char *pString, int x, int y, int nShade, int nPalette, int position, char shadow, unsigned int nStat, uint8_t alpha)
{
    if (nFont < 0 || nFont >= 5 || !pString) return;
    FONT *pFont = &gFont[nFont];

    if (position)
    {
        const char *s = pString;
        int width = -pFont->space;
        while (*s)
        {
            int nTile = ((*s-' ')&127)+pFont->tile;
            if (tilesiz[nTile].x && tilesiz[nTile].y)
                width += tilesiz[nTile].x+pFont->space;
            s++;
        }
        if (position == 1)
            width >>= 1;
        x -= width;
    }
    const char *s = pString;
    while (*s)
    {
        int nTile = ((*s-' ')&127) + pFont->tile;
        if (tilesiz[nTile].x && tilesiz[nTile].y)
        {
            if (shadow)
            {
                rotatesprite_fs_alpha((x+1)<<16, (y+1)<<16, 65536, 0, nTile, 127, nPalette, 26|nStat, alpha);
            }
            rotatesprite_fs_alpha(x<<16, y<<16, 65536, 0, nTile, nShade, nPalette, 26|nStat, alpha);
            x += tilesiz[nTile].x+pFont->space;
        }
        s++;
    }
}

void viewTileSprite(int nTile, int nShade, int nPalette, int x1, int y1, int x2, int y2)
{
    Rect rect1 = Rect(x1, y1, x2, y2);
    Rect rect2 = Rect(0, 0, xdim, ydim);
    rect1 &= rect2;

    if (!rect1)
        return;

    dassert(nTile >= 0 && nTile < kMaxTiles);
    int width = tilesiz[nTile].x;
    int height = tilesiz[nTile].y;
    int bx1 = DecBy(rect1.x1+1, width);
    int by1 = DecBy(rect1.y1+1, height);
    int bx2 = IncBy(rect1.x2-1, width);
    int by2 = IncBy(rect1.y2-1, height);
    for (int x = bx1; x < bx2; x += width)
        for (int y = by1; y < by2; y += height)
            rotatesprite(x<<16, y<<16, 65536, 0, nTile, nShade, nPalette, 64+16+8, x1, y1, x2-1, y2-1);
}

void InitStatusBar(void)
{
    tileLoadTile(2200);
}
void DrawStatSprite(int nTile, int x, int y, int nShade, int nPalette, unsigned int nStat, int nScale)
{
    rotatesprite(x<<16, y<<16, nScale, 0, nTile, nShade, nPalette, nStat | 74, 0, 0, xdim-1, ydim-1);
}
void DrawStatMaskedSprite(int nTile, int x, int y, int nShade, int nPalette, unsigned int nStat, int nScale)
{
    rotatesprite(x<<16, y<<16, nScale, 0, nTile, nShade, nPalette, nStat | 10, 0, 0, xdim-1, ydim-1);
}

void DrawStatNumber(const char *pFormat, int nNumber, int nTile, int x, int y, int nShade, int nPalette, unsigned int nStat, int nScale)
{
    char tempbuf[80];
    int width = tilesiz[nTile].x+1;
    x <<= 16;
    sprintf(tempbuf, pFormat, nNumber);
    for (unsigned int i = 0; i < strlen(tempbuf); i++, x += width*nScale)
    {
        if (tempbuf[i] == ' ') continue;
        rotatesprite(x, y<<16, nScale, 0, nTile+tempbuf[i]-'0', nShade, nPalette, nStat | 10, 0, 0, xdim-1, ydim-1);
    }
}

void TileHGauge(int nTile, int x, int y, int nMult, int nDiv, int nStat, int nScale)
{
    int bx = scale(mulscale16(tilesiz[nTile].x,nScale),nMult,nDiv)+x;
    int sbx;
    switch (nStat&(512+256))
    {
    case 256:
        sbx = mulscale16(bx, xscalecorrect)-1;
        break;
    case 512:
        bx -= 320;
        sbx = xdim+mulscale16(bx, xscalecorrect)-1;
        break;
    default:
        bx -= 160;
        sbx = (xdim>>1)+mulscale16(bx, xscalecorrect)-1;
        break;
    }
    rotatesprite(x<<16, y<<16, nScale, 0, nTile, 0, 0, nStat|90, 0, 0, sbx, ydim-1);
}

int gPackIcons[5] = {
    2569, 2564, 2566, 2568, 2560
};

struct PACKICON2 {
    short nTile;
    int nScale;
    int nYOffs;
};

PACKICON2 gPackIcons2[] = {
    { 519, (int)(65536*0.5), 0 },
    { 830, (int)(65536*0.3), 0 },
    { 760, (int)(65536*0.6), 0 },
    { 839, (int)(65536*0.5), -4 },
    { 827, (int)(65536*0.4), 0 },
};

struct AMMOICON {
    short nTile;
    int nScale;
    int nYOffs;
};

AMMOICON gAmmoIcons[] = {
    { -1, 0, 0 },
    { 816, (int)(65536 * 0.5), 0 },
    { 619, (int)(65536 * 0.8), 0 },
    { 817, (int)(65536 * 0.7), 3 },
    { 801, (int)(65536 * 0.5), -6 },
    { 589, (int)(65536 * 0.7), 2 },
    { 618, (int)(65536 * 0.5), 4 },
    { 548, (int)(65536 * 0.3), -6 },
    { 820, (int)(65536 * 0.3), -6 },
    { 525, (int)(65536 * 0.6), -6 },
    { 811, (int)(65536 * 0.5), 2 },
    { 810, (int)(65536 * 0.45), 2 },
};

struct WEAPONICON {
    short nTile;
    char xRepeat;
    char yRepeat;
};

WEAPONICON gWeaponIcon[] = {
    { -1, 0, 0 },
    { -1, 0, 0 },
    { 524, 32, 32 },
    { 559, 32, 32 },
    { 558, 32, 32 },
    { 526, 32, 32 },
    { 589, 32, 32 },
    { 618, 32, 32 },
    { 539, 32, 32 },
    { 800, 32, 32 },
    { 525, 32, 32 },
    { 811, 32, 32 },
    { 810, 32, 32 },
    { -1, 0, 0 },
};

int dword_14C508;

void viewDrawStats(PLAYER *pPlayer, int x, int y)
{
    const int nFont = 3;
    char buffer[128];
    if (!gLevelStats)
        return;

    int nHeight;
    viewGetFontInfo(nFont, NULL, NULL, &nHeight);
    sprintf(buffer, "T:%d:%02d.%02d",
        (gLevelTime/(kTicsPerSec*60)),
        (gLevelTime/kTicsPerSec)%60,
        ((gLevelTime%kTicsPerSec)*33)/10
        );
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
    y += nHeight+1;
    if (gGameOptions.nGameType == 0 || gGameOptions.nGameType == 2)
        sprintf(buffer, "K:%d/%d", gKillMgr.at4, gKillMgr.at0);
    else
        sprintf(buffer, "K:%d", pPlayer->at2c6);
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
    y += nHeight+1;
    sprintf(buffer, "S:%d/%d", gSecretMgr.at4+gSecretMgr.at8, gSecretMgr.at0);
    viewDrawText(3, buffer, x, y, 20, 0, 0, true, 256);
}

#define kSBarNumberHealth 9220
#define kSBarNumberAmmo 9230
#define kSBarNumberInv 9240
#define kSBarNumberArmor1 9250
#define kSBarNumberArmor2 9260
#define kSBarNumberArmor3 9270

struct POWERUPDISPLAY
{
    int nTile;
    float nScaleRatio;
    int yOffset;
    int remainingDuration;
};

void sortPowerUps(POWERUPDISPLAY* powerups) {
    for (int i = 1; i < 5; i++)
    {
        for (int j = 0; j < 5-i; j++)
        {
            if (powerups[j].remainingDuration > powerups[j+1].remainingDuration)
            {
                POWERUPDISPLAY temp = powerups[j];
                powerups[j] = powerups[j+1];
                powerups[j+1] = temp;
            }
        }
    }
}

void viewDrawPowerUps(PLAYER* pPlayer)
{
    if (!gPowerupDuration)
        return;

    const int nCloakOfInvisibility = 13;
    const int nReflectiveShots = 24;
    const int nDeathMask = 14; // invulnerability
    const int nGunsAkimbo = 17;
    const int nCloakOfShadow = 26; // does nothing, only appears at near the end of Cryptic Passage's Lost Monastery (CP04)

    POWERUPDISPLAY powerups[5];
    powerups[0] = { 896, 0.4f, 0, pPlayer->at202[nCloakOfInvisibility] };
    powerups[1] = { 2428, 0.4f, 5, pPlayer->at202[nReflectiveShots] };
    powerups[2] = { 825, 0.3f, 9, pPlayer->at202[nDeathMask] };
    powerups[3] = { 829, 0.3f, 5, pPlayer->at202[nGunsAkimbo] };
    powerups[4] = { 768, 0.4f, 9, pPlayer->at202[nCloakOfShadow] };

    sortPowerUps(powerups);

    const int x = 15;
    int y = 50;
    for (int i = 0; i < 5; i++)
    {
        if (powerups[i].remainingDuration)
        {
            int remainingSeconds = powerups[i].remainingDuration / 100;
            if (remainingSeconds > 5 || (gGameClock & 32))
            {
                DrawStatMaskedSprite(powerups[i].nTile, x, y + powerups[i].yOffset, 0, 0, 256, (int)(65536 * powerups[i].nScaleRatio));
            }

            DrawStatNumber("%d", remainingSeconds, kSBarNumberInv, x + 15, y, 0, 0, 256, 65536 * 0.5);
            y += 20;
        }
    }
}

void viewDrawMapTitle(void)
{
    if (!gShowMapTitle || gGameMenuMgr.m_bActive)
        return;

    int seconds = (gLevelTime / kTicsPerSec);
    int millisecs = (gLevelTime % kTicsPerSec) * 33;
    if (seconds > 3)
        return;

    const int noAlphaForSecs = 1;
    uint8_t alpha = videoGetRenderMode() != REND_CLASSIC || numalphatabs >= 15 ?
        seconds < noAlphaForSecs ? 0 : clamp(((seconds-noAlphaForSecs)*1000+millisecs)/4, 0, 255)
        : 0;

    if (alpha != 255)
    {
        viewDrawText(1, levelGetTitle(), 160, 50, -128, 0, 1, 1, 0, alpha);
    }
}

void viewDrawPack(PLAYER *pPlayer, int x, int y)
{
    int packs[5];
    if (pPlayer->at31d)
    {
        int nPacks = 0;
        int width = 0;
        for (int i = 0; i < 5; i++)
        {
            if (pPlayer->packInfo[i].at1)
            {
                packs[nPacks++] = i;
                width += tilesiz[gPackIcons[i]].x + 1;
            }
        }
        width /= 2;
        x -= width;
        for (int i = 0; i < nPacks; i++)
        {
            int nPack = packs[i];
            DrawStatSprite(2568, x+1, y-8);
            DrawStatSprite(2568, x+1, y-6);
            DrawStatSprite(gPackIcons[nPack], x+1, y+1);
            if (nPack == pPlayer->at321)
                DrawStatMaskedSprite(2559, x+1, y+1);
            int nShade;
            if (pPlayer->packInfo[nPack].at0)
                nShade = 4;
            else
                nShade = 24;
            DrawStatNumber("%3d", pPlayer->packInfo[nPack].at1, 2250, x-4, y-13, nShade, 0);
            x += tilesiz[gPackIcons[nPack]].x + 1;
        }
    }
    if (pPlayer->at31d != dword_14C508)
    {
        viewUpdatePages();
    }
    dword_14C508 = pPlayer->at31d;
}

void DrawPackItemInStatusBar(PLAYER *pPlayer, int x, int y, int x2, int y2, int nStat)
{
    if (pPlayer->at321 < 0) return;

    DrawStatSprite(gPackIcons[pPlayer->at321], x, y, 0, 0, nStat);
    DrawStatNumber("%3d", pPlayer->packInfo[pPlayer->at321].at1, 2250, x2, y2, 0, 0, nStat);
}

void DrawPackItemInStatusBar2(PLAYER *pPlayer, int x, int y, int x2, int y2, int nStat, int nScale)
{
    if (pPlayer->at321 < 0) return;

    DrawStatMaskedSprite(gPackIcons2[pPlayer->at321].nTile, x, y+gPackIcons2[pPlayer->at321].nYOffs, 0, 0, nStat, gPackIcons2[pPlayer->at321].nScale);
    DrawStatNumber("%3d", pPlayer->packInfo[pPlayer->at321].at1, kSBarNumberInv, x2, y2, 0, 0, nStat, nScale);
}

char gTempStr[128];

void UpdateStatusBar(int arg)
{
    PLAYER *pPlayer = gView;
    XSPRITE *pXSprite = pPlayer->pXSprite;

    int nPalette = 0;

    if (gGameOptions.nGameType == 3)
    {
        if (pPlayer->at2ea & 1)
            nPalette = 7;
        else
            nPalette = 10;
    }

    if (gViewSize < 0) return;

    if (gViewSize == 1)
    {
        DrawStatMaskedSprite(2169, 12, 195, 0, 0, 256, (int)(65536*0.56));
        DrawStatNumber("%d", pXSprite->health>>4, kSBarNumberHealth, 28, 187, 0, 0, 256);
        if (pPlayer->at33e[1])
        {
            DrawStatMaskedSprite(2578, 70, 186, 0, 0, 256, (int)(65536*0.5));
            DrawStatNumber("%3d", pPlayer->at33e[1]>>4, kSBarNumberArmor2, 83, 187, 0, 0, 256, (int)(65536*0.65));
        }
        if (pPlayer->at33e[0])
        {
            DrawStatMaskedSprite(2586, 112, 195, 0, 0, 256, (int)(65536*0.5));
            DrawStatNumber("%3d", pPlayer->at33e[0]>>4, kSBarNumberArmor1, 125, 187, 0, 0, 256, (int)(65536*0.65));
        }
        if (pPlayer->at33e[2])
        {
            DrawStatMaskedSprite(2602, 155, 196, 0, 0, 256, (int)(65536*0.5));
            DrawStatNumber("%3d", pPlayer->at33e[2]>>4, kSBarNumberArmor3, 170, 187, 0, 0, 256, (int)(65536*0.65));
        }

        DrawPackItemInStatusBar2(pPlayer, 225, 194, 240, 187, 512, (int)(65536*0.7));

        if (pPlayer->atbd && pPlayer->atc7 != -1)
        {
            int num = pPlayer->at181[pPlayer->atc7];
            if (pPlayer->atc7 == 6)
                num /= 10;
            if ((unsigned int)gAmmoIcons[pPlayer->atc7].nTile < kMaxTiles)
                DrawStatMaskedSprite(gAmmoIcons[pPlayer->atc7].nTile, 304, 192+gAmmoIcons[pPlayer->atc7].nYOffs,
                    0, 0, 512, gAmmoIcons[pPlayer->atc7].nScale);
            DrawStatNumber("%3d", num, kSBarNumberAmmo, 267, 187, 0, 0, 512);
        }

        for (int i = 0; i < 6; i++)
        {
            if (pPlayer->at88[i+1])
                DrawStatMaskedSprite(2552+i, 260+10*i, 170, 0, 0, 512, (int)(65536*0.25));
        }

        if (pPlayer->at1ba)
            TileHGauge(2260, 124, 175-10, pPlayer->at1ba, 65536);
        else
            viewDrawPack(pPlayer, 166, 200-tilesiz[2201].y/2-30);
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }
    else if (gViewSize <= 2)
    {
        if (pPlayer->at1ba)
            TileHGauge(2260, 124, 175, pPlayer->at1ba, 65536);
        else
            viewDrawPack(pPlayer, 166, 200-tilesiz[2201].y/2);
    }
    if (gViewSize == 2)
    {
        DrawStatSprite(2201, 34, 187, 16, nPalette, 256);
        if (pXSprite->health >= 16 || (gGameClock&16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health>>4, 2190, 8, 183, 0, 0, 256);
        }
        if (pPlayer->atbd && pPlayer->atc7 != -1)
        {
            int num = pPlayer->at181[pPlayer->atc7];
            if (pPlayer->atc7 == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 42, 183, 0, 0, 256);
        }
        DrawStatSprite(2173, 284, 187, 16, nPalette, 512);
        if (pPlayer->at33e[1])
        {
            TileHGauge(2207, 250, 175, pPlayer->at33e[1], 3200, 512);
            DrawStatNumber("%3d", pPlayer->at33e[1]>>4, 2230, 255, 178, 0, 0, 512);
        }
        if (pPlayer->at33e[0])
        {
            TileHGauge(2209, 250, 183, pPlayer->at33e[0], 3200, 512);
            DrawStatNumber("%3d", pPlayer->at33e[0]>>4, 2230, 255, 186, 0, 0, 512);
        }
        if (pPlayer->at33e[2])
        {
            TileHGauge(2208, 250, 191, pPlayer->at33e[2], 3200, 512);
            DrawStatNumber("%3d", pPlayer->at33e[2]>>4, 2230, 255, 194, 0, 0, 512);
        }
        DrawPackItemInStatusBar(pPlayer, 286, 186, 302, 183, 512);

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220+i;
            int x, nStat = 0;
            int y = 200-6;
            if (i&1)
            {
                x = 320-(78+(i>>1)*10);
                nStat |= 512;
            }
            else
            {
                x = 73+(i>>1)*10;
                nStat |= 256;
            }
            if (pPlayer->at88[i+1])
                DrawStatSprite(nTile, x, y, 0, 0, nStat);
#if 0
            else
                DrawStatSprite(nTile, x, y, 40, 5, nStat);
#endif
        }
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }
    else if (gViewSize > 2)
    {
        viewDrawPack(pPlayer, 160, 200-tilesiz[2200].y);
        DrawStatMaskedSprite(2200, 160, 172, 16, nPalette);
        DrawPackItemInStatusBar(pPlayer, 265, 186, 260, 172);
        if (pXSprite->health >= 16 || (gGameClock&16) || pXSprite->health == 0)
        {
            DrawStatNumber("%3d", pXSprite->health>>4, 2190, 86, 183, 0, 0);
        }
        if (pPlayer->atbd && pPlayer->atc7 != -1)
        {
            int num = pPlayer->at181[pPlayer->atc7];
            if (pPlayer->atc7 == 6)
                num /= 10;
            DrawStatNumber("%3d", num, 2240, 216, 183, 0, 0);
        }
        for (int i = 9; i >= 1; i--)
        {
            int x = 135+((i-1)/3)*23;
            int y = 182+((i-1)%3)*6;
            int num = pPlayer->at181[i];
            if (i == 6)
                num /= 10;
            if (i == pPlayer->atc7)
            {
                DrawStatNumber("%3d", num, 2230, x, y, -128, 10);
            }
            else
            {
                DrawStatNumber("%3d", num, 2230, x, y, 32, 10);
            }
        }

        if (pPlayer->atc7 == 10)
        {
            DrawStatNumber("%2d", pPlayer->at181[10], 2230, 291, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->at181[10], 2230, 291, 194, 32, 10);
        }

        if (pPlayer->atc7 == 11)
        {
            DrawStatNumber("%2d", pPlayer->at181[11], 2230, 309, 194, -128, 10);
        }
        else
        {
            DrawStatNumber("%2d", pPlayer->at181[11], 2230, 309, 194, 32, 10);
        }

        if (pPlayer->at33e[1])
        {
            TileHGauge(2207, 44, 174, pPlayer->at33e[1], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[1]>>4, 2230, 50, 177, 0, 0);
        }
        if (pPlayer->at33e[0])
        {
            TileHGauge(2209, 44, 182, pPlayer->at33e[0], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[0]>>4, 2230, 50, 185, 0, 0);
        }
        if (pPlayer->at33e[2])
        {
            TileHGauge(2208, 44, 190, pPlayer->at33e[2], 3200);
            DrawStatNumber("%3d", pPlayer->at33e[2]>>4, 2230, 50, 193, 0, 0);
        }
        sprintf(gTempStr, "v%s", GetVersionString());
        viewDrawText(3, gTempStr, 20, 191, 32, 0, 1, 0);

        for (int i = 0; i < 6; i++)
        {
            int nTile = 2220+i;
            int x = 73+(i&1)*173;
            int y = 171+(i>>1)*11;
            if (pPlayer->at88[i+1])
                DrawStatSprite(nTile, x, y);
            else
                DrawStatSprite(nTile, x, y, 40, 5);
        }
        DrawStatMaskedSprite(2202, 118, 185, pPlayer->at2e ? 16 : 40);
        DrawStatMaskedSprite(2202, 201, 185, pPlayer->at2e ? 16 : 40);
        if (pPlayer->at1ba)
        {
            TileHGauge(2260, 124, 175, pPlayer->at1ba, 65536);
        }
        viewDrawStats(pPlayer, 2, 140);
        viewDrawPowerUps(pPlayer);
    }

    viewDrawMapTitle();

    if (gGameOptions.nGameType < 1) return;

    if (gGameOptions.nGameType == 3)
    {
        int x = 1, y = 1;
        if (dword_21EFD0[0] == 0 || (gGameClock & 8))
        {
            viewDrawText(0, "BLUE", x, y, -128, 10, 0, 0, 256);
            dword_21EFD0[0] = ClipLow(dword_21EFD0[0]-arg, 0);
            sprintf(gTempStr, "%-3d", dword_21EFB0[0]);
            viewDrawText(0, gTempStr, x, y+10, -128, 10, 0, 0, 256);
        }
        x = 319;
        if (dword_21EFD0[1] == 0 || (gGameClock & 8))
        {
            viewDrawText(0, "RED", x, y, -128, 7, 2, 0, 512);
            dword_21EFD0[1] = ClipLow(dword_21EFD0[1]-arg, 0);
            sprintf(gTempStr, "%3d", dword_21EFB0[1]);
            viewDrawText(0, gTempStr, x, y+10, -128, 7, 2, 0, 512);
        }
        return;
    }
    for (int nRows = (gNetPlayers-1) / 4; nRows >= 0; nRows--)
    {
        for (int nCol = 0; nCol < 4; nCol++)
        {
            DrawStatSprite(2229, 40+nCol*80, 4+nRows*9, 16);
        }
    }
    for (int i = 0, p = connecthead; p >= 0; i++, p = connectpoint2[p])
    {
        int x = 80*(i&3);
        int y = 9*(i/4);
        int col = gPlayer[p].at2ea&3;
        char *name = gProfile[p].name;
        if (gProfile[p].skill == 2)
            sprintf(gTempStr, "%s", name);
        else
            sprintf(gTempStr, "%s [%d]", name, gProfile[p].skill);
        Bstrupr(gTempStr);
        viewDrawText(4, gTempStr, x+4, y+1, -128, 11+col, 0, 0);
        sprintf(gTempStr, "%2d", gPlayer[p].at2c6);
        viewDrawText(4, gTempStr, x+76, y+1, -128, 11+col, 2, 0);
    }
}

int *lensTable;

int gZoom = 1024;

int dword_172CE0[16][3];

void viewInit(void)
{
    initprintf("Initializing status bar\n");
    InitStatusBar();
    FontSet(0, 4096, 0);
    FontSet(1, 4192, 1);
    FontSet(2, 4288, 1);
    FontSet(3, 4384, 1);
    FontSet(4, 4480, 0);

    DICTNODE *hLens = gSysRes.Lookup("LENS", "DAT");
    dassert(hLens != NULL);
    dassert(gSysRes.Size(hLens) == kLensSize * kLensSize * sizeof(int));

    lensTable = (int*)gSysRes.Lock(hLens);
#if B_BIG_ENDIAN == 1
    for (int i = 0; i < kLensSize*kLensSize; i++)
    {
        lensTable[i] = B_LITTLE32(lensTable[i]);
    }
#endif
    char *data = tileAllocTile(4077, kLensSize, kLensSize, 0, 0);
    memset(data, 255, kLensSize*kLensSize);
    gGameMessageMgr.SetState(gMessageState);
    gGameMessageMgr.SetCoordinates(1, 1);
    char nFont;
    if (gMessageFont == 0)
        nFont = 3;
    else
        nFont = 0;

    gGameMessageMgr.SetFont(nFont);
    gGameMessageMgr.SetMaxMessages(gMessageCount);
    gGameMessageMgr.SetMessageTime(gMessageTime);

    for (int i = 0; i < 16; i++)
    {
        dword_172CE0[i][0] = mulscale16(wrand(), 2048);
        dword_172CE0[i][1] = mulscale16(wrand(), 2048);
        dword_172CE0[i][2] = mulscale16(wrand(), 2048);
    }
    gViewMap.sub_25C38(0, 0, gZoom, 0, gFollowMap);

    g_frameDelay = calcFrameDelay(r_maxfps + r_maxfpsoffset);

    bLoadScreenCrcMatch = tileCRC(kLoadScreen) == kLoadScreenCRC;
}

void viewResizeView(int size)
{
    int xdimcorrect = ClipHigh(scale(ydim, 4, 3), xdim);
    gViewXCenter = xdim-xdim/2;
    gViewYCenter = ydim-ydim/2;
    xscale = divscale16(xdim, 320);
    xscalecorrect = divscale16(xdimcorrect, 320);
    yscale = divscale16(ydim, 200);
    xstep = divscale16(320, xdim);
    ystep = divscale16(200, ydim);
    gViewSize = ClipRange(size, 0, 7);
    if (gViewSize <= 2)
    {
        gViewX0 = 0;
        gViewX1 = xdim-1;
        gViewY0 = 0;
        gViewY1 = ydim-1;
        if (gGameOptions.nGameType > 0 && gGameOptions.nGameType < 3)
        {
            gViewY0 = (tilesiz[2229].y*ydim*((gNetPlayers+3)/4))/200;
        }
        gViewX0S = divscale16(gViewX0, xscalecorrect);
        gViewY0S = divscale16(gViewY0, yscale);
        gViewX1S = divscale16(gViewX1, xscalecorrect);
        gViewY1S = divscale16(gViewY1, yscale);
    }
    else
    {
        gViewX0 = 0;
        gViewY0 = 0;
        gViewX1 = xdim-1;
        gViewY1 = ydim-1-(25*ydim)/200;
        if (gGameOptions.nGameType > 0 && gGameOptions.nGameType < 3)
        {
            gViewY0 = (tilesiz[2229].y*ydim*((gNetPlayers+3)/4))/200;
        }

        int height = gViewY1-gViewY0;
        gViewX0 += mulscale16(xdim*(gViewSize-3),4096);
        gViewX1 -= mulscale16(xdim*(gViewSize-3),4096);
        gViewY0 += mulscale16(height*(gViewSize-3),4096);
        gViewY1 -= mulscale16(height*(gViewSize-3),4096);
        gViewX0S = divscale16(gViewX0, xscalecorrect);
        gViewY0S = divscale16(gViewY0, yscale);
        gViewX1S = divscale16(gViewX1, xscalecorrect);
        gViewY1S = divscale16(gViewY1, yscale);
    }
    videoSetViewableArea(gViewX0, gViewY0, gViewX1, gViewY1);
    gGameMessageMgr.SetCoordinates(gViewX0S + 1, gViewY0S + 1);
    viewSetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);
    viewUpdatePages();
}

#define kBackTile 253

void UpdateFrame(void)
{
    viewTileSprite(kBackTile, 0, 0, 0, 0, xdim, gViewY0-3);
    viewTileSprite(kBackTile, 0, 0, 0, gViewY1+4, xdim, ydim);
    viewTileSprite(kBackTile, 0, 0, 0, gViewY0-3, gViewX0-3, gViewY1+4);
    viewTileSprite(kBackTile, 0, 0, gViewX1+4, gViewY0-3, xdim, gViewY1+4);

    viewTileSprite(kBackTile, 20, 0, gViewX0-3, gViewY0-3, gViewX0, gViewY1+1);
    viewTileSprite(kBackTile, 20, 0, gViewX0, gViewY0-3, gViewX1+4, gViewY0);
    viewTileSprite(kBackTile, 10, 1, gViewX1+1, gViewY0, gViewX1+4, gViewY1+4);
    viewTileSprite(kBackTile, 10, 1, gViewX0-3, gViewY1+1, gViewX1+1, gViewY1+4);
}

void viewDrawInterface(int arg)
{
    if (gViewMode == 3/* && gViewSize >= 3*/ && (pcBackground != 0 || videoGetRenderMode() >= REND_POLYMOST))
    {
        UpdateFrame();
        pcBackground--;
    }
    UpdateStatusBar(arg);
}

uspritetype *viewInsertTSprite(int nSector, int nStatnum, uspritetype *pSprite)
{
    int nTSprite = spritesortcnt;
    uspritetype *pTSprite = &tsprite[nTSprite];
    memset(pTSprite, 0, sizeof(uspritetype));
    pTSprite->cstat = 128;
    pTSprite->xrepeat = 64;
    pTSprite->yrepeat = 64;
    pTSprite->owner = -1;
    pTSprite->extra = -1;
    pTSprite->type = -spritesortcnt;
    pTSprite->statnum = nStatnum;
    pTSprite->sectnum = nSector;
    spritesortcnt++;
    if (pSprite)
    {
        pTSprite->x = pSprite->x;
        pTSprite->y = pSprite->y;
        pTSprite->z = pSprite->z;
        pTSprite->owner = pSprite->owner;
        pTSprite->ang = pSprite->ang;
    }
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        int nAngle = getangle(pTSprite->x-gView->pSprite->x, pTSprite->y-gView->pSprite->y);
        pTSprite->x += Cos(nAngle)>>25;
        pTSprite->y += Sin(nAngle)>>25;
    }
    return &tsprite[nTSprite];
}

int effectDetail[] = {
    4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};

uspritetype *viewAddEffect(int nTSprite, VIEW_EFFECT nViewEffect)
{
    dassert(nViewEffect >= 0 && nViewEffect < kViewEffectMax);
    uspritetype *pTSprite = &tsprite[nTSprite];
    if (gDetail < effectDetail[nViewEffect] || nTSprite >= kMaxViewSprites) return NULL;
    switch (nViewEffect)
    {
    case VIEW_EFFECT_18:
        for (int i = 0; i < 16; i++)
        {
            uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
            int ang = (gFrameClock*2048)/120;
            int nRand1 = dword_172CE0[i][0];
            int nRand2 = dword_172CE0[i][1];
            int nRand3 = dword_172CE0[i][2];
            ang += nRand3;
            int x = mulscale30(512, Cos(ang));
            int y = mulscale30(512, Sin(ang));
            int z = 0;
            RotateYZ(&x, &y, &z, nRand1);
            RotateXZ(&x, &y, &z, nRand2);
            pNSprite->x = pTSprite->x + x;
            pNSprite->y = pTSprite->y + y;
            pNSprite->z = pTSprite->z + (z<<4);
            pNSprite->picnum = 1720;
            pNSprite->shade = -128;
        }
        break;
    case VIEW_EFFECT_16:
    case VIEW_EFFECT_17:
    {
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->pal = 0;
        pNSprite->z = top;
        if (nViewEffect == VIEW_EFFECT_16)
            pNSprite->xrepeat = pNSprite->yrepeat = 24;
        else
            pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 3558;
        return pNSprite;
    }
    case VIEW_EFFECT_15:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->z = pTSprite->z;
        pNSprite->cstat |= 2;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        pNSprite->picnum = 2135;
        break;
    }
    case VIEW_EFFECT_14:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->pal = 0;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 2605;
        return pNSprite;
    }
    case VIEW_EFFECT_13:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = 26;
        pNSprite->pal = 0;
        pNSprite->cstat |= 2;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 2089;
        break;
    }
    case VIEW_EFFECT_11:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->shade = 26;
        pNSprite->pal = 0;
        pNSprite->cstat |= 2;
        pNSprite->xrepeat = pNSprite->yrepeat = 24;
        pNSprite->picnum = 626;
        pNSprite->z = top;
        break;
    }
    case VIEW_EFFECT_10:
    {
        int nAng = pTSprite->ang;
        if (pTSprite->cstat & 16)
        {
            nAng = (nAng+512)&2047;
        }
        else
        {
            nAng = (nAng+1024)&2047;
        }
        for (int i = 0; i < 5 && spritesortcnt < kMaxViewSprites; i++)
        {
            int nSector = pTSprite->sectnum;
            uspritetype *pNSprite = viewInsertTSprite(nSector, 32767, NULL);
            int nLen = 128+(i<<7);
            int x = mulscale30(nLen, Cos(nAng));
            pNSprite->x = pTSprite->x + x;
            int y = mulscale30(nLen, Sin(nAng));
            pNSprite->y = pTSprite->y + y;
            pNSprite->z = pTSprite->z;
            dassert(nSector >= 0 && nSector < kMaxSectors);
            FindSector(pNSprite->x, pNSprite->y, pNSprite->z, &nSector);
            pNSprite->sectnum = nSector;
            pNSprite->owner = pTSprite->owner;
            pNSprite->picnum = pTSprite->picnum;
            pNSprite->cstat |= 2;
            if (i < 2)
                pNSprite->cstat |= 514;
            pNSprite->shade = ClipLow(pTSprite->shade-16, -128);
            pNSprite->xrepeat = pTSprite->xrepeat;
            pNSprite->yrepeat = pTSprite->yrepeat;
            pNSprite->picnum = pTSprite->picnum;
        }
        break;
    }
    case VIEW_EFFECT_8:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->z = pTSprite->z;
        pNSprite->picnum = 908;
        pNSprite->statnum = 0;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/64;
        break;
    }
    case VIEW_EFFECT_6:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->z = top;
        if (IsDudeSprite((spritetype *)pTSprite))
            pNSprite->picnum = 672;
        else
            pNSprite->picnum = 754;
        pNSprite->cstat |= 2;
        pNSprite->shade = 8;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        break;
    }
    case VIEW_EFFECT_7:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->z = bottom;
        if (pTSprite->type >= kDudeBase && pTSprite->type < kDudeMax)
            pNSprite->picnum = 672;
        else
            pNSprite->picnum = 754;
        pNSprite->cstat |= 2;
        pNSprite->shade = 8;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        break;
    }
    case VIEW_EFFECT_4:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->z = top;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/32;
        break;
    }
    case VIEW_EFFECT_5:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->z = bottom;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/32;
        break;
    }
    case VIEW_EFFECT_0:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->z = getflorzofslope(pTSprite->sectnum, pNSprite->x, pNSprite->y);
        pNSprite->shade = 127;
        pNSprite->cstat |= 2;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat>>2;
        pNSprite->picnum = pTSprite->picnum;
        pNSprite->pal = 5;
        int height = tilesiz[pNSprite->picnum].y;
        int center = height/2+picanm[pNSprite->picnum].yofs;
        pNSprite->z -= (pNSprite->yrepeat<<2)*(height-center);
        break;
    }
    case VIEW_EFFECT_1:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->pal = 2;
        pNSprite->cstat |= 2;
        pNSprite->z = pTSprite->z;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        pNSprite->picnum = 2427;
        break;
    }
    case VIEW_EFFECT_2:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        sectortype *pSector = &sector[pTSprite->sectnum];
        pNSprite->x = pTSprite->x;
        pNSprite->y = pTSprite->y;
        pNSprite->z = pSector->ceilingz;
        pNSprite->picnum = 624;
        pNSprite->shade = ((pTSprite->z-pSector->ceilingz)>>8)-64;
        pNSprite->pal = 2;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->cstat |= 106;
        pNSprite->ang = pTSprite->ang;
        pNSprite->owner = pTSprite->owner;
        break;
    }
    case VIEW_EFFECT_3:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        sectortype *pSector = &sector[pTSprite->sectnum];
        pNSprite->x = pTSprite->x;
        pNSprite->y = pTSprite->y;
        pNSprite->z = pSector->floorz;
        pNSprite->picnum = 624;
        char nShade = (pSector->floorz-pTSprite->z)>>8; 
        pNSprite->shade = nShade-32;
        pNSprite->pal = 2;
        pNSprite->xrepeat = pNSprite->yrepeat = nShade;
        pNSprite->cstat |= 98;
        pNSprite->ang = pTSprite->ang;
        pNSprite->owner = pTSprite->owner;
        break;
    }
    case VIEW_EFFECT_9:
    {
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->z = pTSprite->z;
        if (gDetail > 1)
            pNSprite->cstat |= 514;
        pNSprite->shade = ClipLow(pTSprite->shade-32, -128);
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = 64;
        pNSprite->picnum = 775;
        break;
    }
    case VIEW_EFFECT_12:
    {
        dassert(pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8);
        PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
        if (gWeaponIcon[pPlayer->atbd].nTile < 0) break;
        uspritetype *pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
        pNSprite->x = pTSprite->x;
        pNSprite->y = pTSprite->y;
        pNSprite->z = pTSprite->z-(32<<8);
        pNSprite->picnum = gWeaponIcon[pPlayer->atbd].nTile;
        pNSprite->shade = pTSprite->shade;
        pNSprite->xrepeat = gWeaponIcon[pPlayer->atbd].xRepeat;
        pNSprite->yrepeat = gWeaponIcon[pPlayer->atbd].yRepeat;
        break;
    }
    }
    return NULL;
}

LOCATION gPrevSpriteLoc[kMaxSprites];

void viewProcessSprites(int cX, int cY, int cZ)
{
    dassert(spritesortcnt <= kMaxViewSprites);
    int nViewSprites = spritesortcnt;
    for (int nTSprite = nViewSprites-1; nTSprite >= 0; nTSprite--)
    {
        uspritetype *pTSprite = &tsprite[nTSprite];
        //int nXSprite = pTSprite->extra;
        int nXSprite = sprite[pTSprite->owner].extra;
        XSPRITE *pTXSprite = NULL;
        if (qsprite_filler[pTSprite->owner] > gDetail)
        {
            pTSprite->xrepeat = 0;
            continue;
        }
        if (nXSprite > 0)
        {
            pTXSprite = &xsprite[nXSprite];
        }
        int nTile = pTSprite->picnum;
        if (nTile < 0 || nTile >= kMaxTiles)
        {
            continue;
        }

        int nSprite = pTSprite->owner;
        if (gViewInterpolate && TestBitString(gInterpolateSprite, nSprite) && !(pTSprite->hitag&512))
        {
            LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
            pTSprite->x = interpolate(pPrevLoc->x, pTSprite->x, gInterpolate);
            pTSprite->y = interpolate(pPrevLoc->y, pTSprite->y, gInterpolate);
            pTSprite->z = interpolate(pPrevLoc->z, pTSprite->z, gInterpolate);
            pTSprite->ang = pPrevLoc->ang+mulscale16(((pTSprite->ang-pPrevLoc->ang+1024)&2047)-1024, gInterpolate);
        }
        int nAnim = 0;
        switch (picanm[nTile].extra&7)
        {
            case 0:
                if (nXSprite > 0)
                {
                    dassert(nXSprite < kMaxXSprites);
                    switch (pTSprite->type)
                    {
                    case 20:
                    case 21:
                        if (xsprite[nXSprite].state)
                        {
                            nAnim = 1;
                        }
                        break;
                    case 22:
                        nAnim = xsprite[nXSprite].data1;
                        break;
                    }
                }
                break;
            case 1:
            {
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    pTSprite->cstat &= ~4;
                    break;
                }
#endif
                int dX = cX - pTSprite->x;
                int dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, 128-pTSprite->ang);
                nAnim = GetOctant(dX, dY);
                if (nAnim <= 4)
                {
                    pTSprite->cstat &= ~4;
                }
                else
                {
                    nAnim = 8 - nAnim;
                    pTSprite->cstat |= 4;
                }
                break;
            }
            case 2:
            {
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    pTSprite->cstat &= ~4;
                    break;
                }
#endif
                int dX = cX - pTSprite->x;
                int dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, 128-pTSprite->ang);
                nAnim = GetOctant(dX, dY);
                break;
            }
            case 3:
            {
                if (nXSprite > 0)
                {
                    if (gSpriteHit[nXSprite].florhit == 0)
                        nAnim = 1;
                }
                else
                {
                    int top, bottom;
                    GetSpriteExtents((spritetype *)pTSprite, &top, &bottom);
                    if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) > bottom)
                        nAnim = 1;
                }
                break;
            }
            case 6:
            case 7:
            {
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && usemodels && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                    break;
#endif
                // Can be overridden by def script
                if (usevoxels && gDetail >= 4 && videoGetRenderMode() != REND_POLYMER && tiletovox[pTSprite->picnum] == -1 && voxelIndex[pTSprite->picnum] != -1)
                {
                    if ((pTSprite->hitag&16) == 0)
                    {
                        pTSprite->cstat |= 48;
                        pTSprite->yoffset += picanm[pTSprite->picnum].yofs;
                        pTSprite->picnum = voxelIndex[pTSprite->picnum];
                        if (!voxoff[pTSprite->picnum])
                            qloadvoxel(pTSprite->picnum);
                        if ((picanm[nTile].extra&7) == 7)
                        {
                            pTSprite->ang = (gGameClock<<3)&2047;
                        }
                    }
                }
                break;
            }
        }
        while (nAnim > 0)
        {
            pTSprite->picnum += picanm[pTSprite->picnum].num+1;
            nAnim--;
        }

        if (usevoxels && videoGetRenderMode() != REND_POLYMER && tiletovox[pTSprite->picnum] != -1)
        {
            pTSprite->yoffset += picanm[pTSprite->picnum].yofs;
        }

        sectortype *pSector = &sector[pTSprite->sectnum];
        XSECTOR *pXSector;
        int nShade = pTSprite->shade;
        if (pSector->extra)
        {
            pXSector = &xsector[pSector->extra];
        }
        else
        {
            pXSector = NULL;
        }
        if ((pSector->ceilingstat&1) && (pSector->floorstat&32768) == 0)
        {
            nShade += tileShade[pSector->ceilingpicnum]+pSector->ceilingshade;
        }
        else
        {
            nShade += tileShade[pSector->floorpicnum]+pSector->floorshade;
        }
        nShade += tileShade[pTSprite->picnum];
        pTSprite->shade = ClipRange(nShade, -128, 127);
        if ((pTSprite->hitag&16) && sprite[pTSprite->owner].owner == 3)
        {
            dassert(pTXSprite != NULL);
            pTSprite->xrepeat = 48;
            pTSprite->yrepeat = 48;
            pTSprite->shade = -128;
            pTSprite->picnum = 2272 + 2*pTXSprite->respawnPending;
            pTSprite->cstat &= ~514;
            if (((IsItemSprite((spritetype *)pTSprite) || IsAmmoSprite((spritetype *)pTSprite)) && gGameOptions.nItemSettings == 2)
                || (IsWeaponSprite((spritetype *)pTSprite) && gGameOptions.nWeaponSettings == 3))
            {
                pTSprite->xrepeat = pTSprite->yrepeat = 48;
            }
            else
            {
                pTSprite->xrepeat = pTSprite->yrepeat = 0;
            }
        }
        if (spritesortcnt >= kMaxViewSprites) continue;
        if (pTXSprite && pTXSprite->burnTime > 0)
        {
            pTSprite->shade = ClipRange(pTSprite->shade-16-QRandom(8), -128, 127);
        }
        if (pTSprite->hitag&256)
        {
            viewAddEffect(nTSprite, VIEW_EFFECT_6);
        }
        if (pTSprite->hitag&1024)
        {
            pTSprite->cstat |= 4;
        }
        if (pTSprite->hitag&2048)
        {
            pTSprite->cstat |= 8;
        }
        switch (pTSprite->statnum)
        {
        case 0:
        {
            switch (pTSprite->type)
            {
            case 32:
                if (pTXSprite)
                {
                    if (pTXSprite->state > 0)
                    {
                        pTSprite->shade = -128;
                        viewAddEffect(nTSprite, VIEW_EFFECT_11);
                    }
                    else
                    {
                        pTSprite->shade = -8;
                    }
                }
                else
                {
                    pTSprite->shade = -128;
                    viewAddEffect(nTSprite, VIEW_EFFECT_11);
                }
                break;
            case 30:
                if (pTXSprite)
                {
                    if (pTXSprite->state > 0)
                    {
                        pTSprite->picnum++;
                        viewAddEffect(nTSprite, VIEW_EFFECT_4);
                    }
                    else
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_6);
                    }
                }
                else
                {
                    pTSprite->picnum++;
                    viewAddEffect(nTSprite, VIEW_EFFECT_4);
                }
                break;
            default:
                if (pXSector && pXSector->color)
                {
                    pTSprite->pal = pSector->floorpal;
                }
                break;
            }
            break;
        }
        case 3:
        {
            switch (pTSprite->type)
            {
            case 145:
                if (pTXSprite && pTXSprite->state > 0 && gGameOptions.nGameType == 3)
                {
                    uspritetype *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                    if (pNTSprite)
                        pNTSprite->pal = 10;
                }
                break;
            case 146:
                if (pTXSprite && pTXSprite->state > 0 && gGameOptions.nGameType == 3)
                {
                    uspritetype *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                    if (pNTSprite)
                        pNTSprite->pal = 7;
                }
                break;
            case 147:
                pTSprite->pal = 10;
                pTSprite->cstat |= 1024;
                break;
            case 148:
                pTSprite->pal = 7;
                pTSprite->cstat |= 1024;
                break;
            default:
                if (pTSprite->type >= 100 && pTSprite->type <= 106)
                    pTSprite->shade = -128;
                if (pXSector && pXSector->color)
                {
                    pTSprite->pal = pSector->floorpal;
                }
                break;
            }
            break;
        }
        case 5:
        {
            switch (pTSprite->type)
            {
            case 302:
                pTSprite->yrepeat = 128;
                pTSprite->cstat |= 32;
                break;
            case 306:
                viewAddEffect(nTSprite, VIEW_EFFECT_15);
                break;
            case 300:
                viewAddEffect(nTSprite, VIEW_EFFECT_10);
                break;
            case 301:
            case 303:
                if (pTSprite->statnum == 14)
                {
                    dassert(pTXSprite != NULL);
                    if (pTXSprite->target == gView->at5b)
                    {
                        pTSprite->xrepeat = 0;
                        break;
                    }
                }
                viewAddEffect(nTSprite, VIEW_EFFECT_1);
                if (pTSprite->type == 301)
                {
                    sectortype *pSector = &sector[pTSprite->sectnum];
                    int zDiff = (pTSprite->z-pSector->ceilingz)>>8;
                    if ((pSector->ceilingstat&1) == 0 && zDiff < 64)
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_2);
                    }
                    zDiff = (pSector->floorz-pTSprite->z)>>8;
                    if ((pSector->floorstat&1) == 0 && zDiff < 64)
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_3);
                    }
                }
                break;
            }
            break;
        }
        case 6:
        {
            if (pTSprite->type == 212 && pTXSprite->aiState == &hand13A3B4)
            {
                spritetype *pTTarget = &sprite[pTXSprite->target];
                dassert(pTXSprite != NULL && pTTarget != NULL);
                if (IsPlayerSprite(pTTarget))
                {
                    pTSprite->xrepeat = 0;
                    break;
                }
            }
            if (pXSector && pXSector->color)
            {
                pTSprite->pal = pSector->floorpal;
            }
            if (powerupCheck(gView, 25) > 0)
            {
                pTSprite->shade = -128;
            }
            if (IsPlayerSprite((spritetype *)pTSprite))
            {
                PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
                if (powerupCheck(pPlayer, 13) && !powerupCheck(gView, 25))
                {
                    pTSprite->cstat |= 2;
                    pTSprite->pal = 5;
                }
                else if (powerupCheck(pPlayer, 14))
                {
                    pTSprite->shade = -128;
                    pTSprite->pal = 5;
                }
                else if (powerupCheck(pPlayer, 23))
                {
                    pTSprite->pal = 11+(gView->at2ea&3);
                }
                if (powerupCheck(pPlayer, 24))
                {
                    viewAddEffect(nTSprite, VIEW_EFFECT_13);
                }
                if (gShowWeapon && gGameOptions.nGameType > 0 && gView)
                {
                    viewAddEffect(nTSprite, VIEW_EFFECT_12);
                }
                if (pPlayer->at37b && (gView != pPlayer || gViewPos != VIEWPOS_0))
                {
                    uspritetype *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_14);
                    if (pNTSprite)
                    {
                        POSTURE *pPosture = &gPosture[pPlayer->at5f][pPlayer->at2f];
                        pNTSprite->x += mulscale28(pPosture->at30, Cos(pTSprite->ang));
                        pNTSprite->y += mulscale28(pPosture->at30, Sin(pTSprite->ang));
                        pNTSprite->z = pPlayer->pSprite->z-pPosture->at2c;
                    }
                }
                if (pPlayer->at90 > 0 && gGameOptions.nGameType == 3)
                {
                    if (pPlayer->at90&1)
                    {
                        uspritetype *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                        if (pNTSprite)
                        {
                            pNTSprite->pal = 10;
                            pNTSprite->cstat |= 4;
                        }
                    }
                    if (pPlayer->at90&2)
                    {
                        uspritetype *pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                        if (pNTSprite)
                        {
                            pNTSprite->pal = 7;
                            pNTSprite->cstat |= 4;
                        }
                    }
                }
            }
            if (pTSprite->owner != gView->pSprite->index || gViewPos != VIEWPOS_0)
            {
                if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                {
                    viewAddEffect(nTSprite, VIEW_EFFECT_0);
                }
            }
            break;
        }
        case 11:
        {
            if (pTSprite->type == 454)
            {
                if (pTXSprite->state)
                {
                    if (pTXSprite->data1)
                    {
                        pTSprite->picnum = 772;
                        if (pTXSprite->data2)
                        {
                            viewAddEffect(nTSprite, VIEW_EFFECT_9);
                        }
                    }
                }
                else
                {
                    if (pTXSprite->data1)
                    {
                        pTSprite->picnum = 773;
                    }
                    else
                    {
                        pTSprite->picnum = 656;
                    }
                }
            }
            break;
        }
        case 4:
        {
            if (pXSector && pXSector->color)
            {
                pTSprite->pal = pSector->floorpal;
            }
            if (pTSprite->hitag&1)
            {
                if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                {
                    if (pTSprite->type < 400 || pTSprite->type >= 433 || !gSpriteHit[nXSprite].florhit)
                    {
                        viewAddEffect(nTSprite, VIEW_EFFECT_0);
                    }
                }
            }
            break;
        }
        }
    }

    for (int nTSprite = spritesortcnt-1; nTSprite >= nViewSprites; nTSprite--)
    {
        uspritetype *pTSprite = &tsprite[nTSprite];
        int nAnim = 0;
        switch (picanm[pTSprite->picnum].extra&7)
        {
            case 1:
            {
                int dX = cX - pTSprite->x;
                int dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, 128-pTSprite->ang);
                nAnim = GetOctant(dX, dY);
                if (nAnim <= 4)
                {
                    pTSprite->cstat &= ~4;
                }
                else
                {
                    nAnim = 8 - nAnim;
                    pTSprite->cstat |= 4;
                }
                break;
            }
            case 2:
            {
                int dX = cX - pTSprite->x;
                int dY = cY - pTSprite->y;
                RotateVector(&dX, &dY, 128-pTSprite->ang);
                nAnim = GetOctant(dX, dY);
                break;
            }
        }
        while (nAnim > 0)
        {
            pTSprite->picnum += picanm[pTSprite->picnum].num+1;
            nAnim--;
        }
    }
}

int othercameradist = 1280;
int cameradist = -1;
int othercameraclock;
int cameraclock;

void CalcOtherPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fix16_t zm)
{
    int vX = mulscale30(-Cos(nAng), 1280);
    int vY = mulscale30(-Sin(nAng), 1280);
    int vZ = fix16_to_int(mulscale(zm, 1280, 3))-(16<<8);
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    short nHSector;
    int hX, hY;
    vec3_t pos = {*pX, *pY, *pZ};
    hitdata_t hitdata;
    hitscan(&pos, *vsectnum, vX, vY, vZ, &hitdata, CLIPMASK1);
    nHSector = hitdata.sect;
    hX = hitdata.pos.x;
    hY = hitdata.pos.y;
    int dX = hX-*pX;
    int dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        int nDist;
        if (klabs(vX) > klabs(vY))
        {
            nDist = ClipHigh(divscale16(dX,vX), othercameradist);
        }
        else
        {
            nDist = ClipHigh(divscale16(dY,vY), othercameradist);
        }
        othercameradist = nDist;
    }
    *pX += mulscale16(vX, othercameradist);
    *pY += mulscale16(vY, othercameradist);
    *pZ += mulscale16(vZ, othercameradist);
    othercameradist = ClipHigh(othercameradist+((gGameClock-othercameraclock)<<10), 65536);
    othercameraclock = gGameClock;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

void CalcPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fix16_t zm)
{
    int vX = mulscale30(-Cos(nAng), 1280);
    int vY = mulscale30(-Sin(nAng), 1280);
    int vZ = fix16_to_int(mulscale(zm, 1280, 3))-(16<<8);
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    short nHSector;
    int hX, hY;
    hitscangoal.x = hitscangoal.y = 0x1fffffff;
    vec3_t pos = { *pX, *pY, *pZ };
    hitdata_t hitdata;
    hitscan(&pos, *vsectnum, vX, vY, vZ, &hitdata, CLIPMASK1);
    nHSector = hitdata.sect;
    hX = hitdata.pos.x;
    hY = hitdata.pos.y;
    int dX = hX-*pX;
    int dY = hY-*pY;
    if (klabs(vX)+klabs(vY) > klabs(dX)+klabs(dY))
    {
        *vsectnum = nHSector;
        dX -= ksgn(vX)<<6;
        dY -= ksgn(vY)<<6;
        int nDist;
        if (klabs(vX) > klabs(vY))
        {
            nDist = ClipHigh(divscale16(dX,vX), cameradist);
        }
        else
        {
            nDist = ClipHigh(divscale16(dY,vY), cameradist);
        }
        cameradist = nDist;
    }
    *pX += mulscale16(vX, cameradist);
    *pY += mulscale16(vY, cameradist);
    *pZ += mulscale16(vZ, cameradist);
    cameradist = ClipHigh(cameradist+((gGameClock-cameraclock)<<10), 65536);
    cameraclock = gGameClock;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

struct {
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

void viewBurnTime(int gScale)
{
    if (!gScale) return;

    for (int i = 0; i < 9; i++)
    {
        int nTile = burnTable[i].nTile+qanimateoffs(burnTable[i].nTile,32768+i);
        int nScale = burnTable[i].nScale;
        if (gScale < 600)
        {
            nScale = scale(nScale, gScale, 600);
        }
        rotatesprite(burnTable[i].nX<<16, burnTable[i].nY<<16, nScale, 0, nTile,
            0, burnTable[i].nPal, burnTable[i].nStat, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
    }
}

void viewSetMessage(const char *pMessage)
{
    OSD_Printf("%s\n", pMessage);
    gGameMessageMgr.Add(pMessage, 15);
}

void viewDisplayMessage(void)
{
    gGameMessageMgr.Display();
}

char errMsg[256];

void viewSetErrorMessage(const char *pMessage)
{
    if (!pMessage)
    {
        strcpy(errMsg, "");
    }
    else
    {
        strcpy(errMsg, pMessage);
    }
}

void DoLensEffect(void)
{
    char *d = (char*)waloff[4077];
    dassert(d != NULL);
    char *s = (char*)waloff[4079];
    dassert(s != NULL);
    for (int i = 0; i < kLensSize*kLensSize; i++, d++)
        if (lensTable[i] >= 0)
            *d = s[lensTable[i]];
    tileInvalidate(4077, -1, -1);
}

void UpdateDacs(int nPalette, bool bNoTint)
{
    static RGB newDAC[256];
    static int oldPalette;
    if (oldPalette != nPalette)
    {
        scrSetPalette(nPalette);
        oldPalette = nPalette;
    }

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        gLastPal = 0;
        polytint_t *tint = &hictinting[MAXPALOOKUPS-1];
        int nRed = 0;
        int nGreen = 0;
        int nBlue = 0;
        tint->f = 0;
        switch (nPalette)
        {
        case 0:
        default:
            tint->r = 255;
            tint->g = 255;
            tint->b = 255;
            break;
        case 1:
            tint->r = 132;
            tint->g = 164;
            tint->b = 255;
            break;
        case 2:
            tint->r = 255;
            tint->g = 126;
            tint->b = 105;
            break;
        case 3:
            tint->r = 162;
            tint->g = 186;
            tint->b = 15;
            break;
        case 4:
            tint->r = 255;
            tint->g = 255;
            tint->b = 255;
            break;
        }
        if (!bNoTint)
        {
            nRed += gView->at377;
            nGreen += gView->at377;
            nBlue -= gView->at377;

            nRed += ClipHigh(gView->at366, 85)*2;
            nGreen -= ClipHigh(gView->at366, 85)*3;
            nBlue -= ClipHigh(gView->at366, 85)*3;

            nRed -= gView->at36a;
            nGreen -= gView->at36a;
            nBlue -= gView->at36a;

            nRed -= gView->at36e>>6;
            nGreen -= gView->at36e>>5;
            nBlue -= gView->at36e>>6;
        }
        nRed = ClipRange(nRed, -255, 255);
        nGreen = ClipRange(nGreen, -255, 255);
        nBlue = ClipRange(nBlue, -255, 255);

        videoSetPalette(0, nPalette, 2);
        videoTintBlood(nRed, nGreen, nBlue);
    }
    else
#endif
    {
        gLastPal = nPalette;
        if (bNoTint)
        {
            memcpy(newDAC, baseDAC, sizeof(newDAC));
        }
        else
        {
            for (int i = 0; i < 256; i++)
            {
                int nRed = baseDAC[i].red;
                int nGreen = baseDAC[i].green;
                int nBlue = baseDAC[i].blue;
                nRed += gView->at377;
                nGreen += gView->at377;
                nBlue -= gView->at377;

                nRed += ClipHigh(gView->at366, 85)*2;
                nGreen -= ClipHigh(gView->at366, 85)*3;
                nBlue -= ClipHigh(gView->at366, 85)*3;

                nRed -= gView->at36a;
                nGreen -= gView->at36a;
                nBlue -= gView->at36a;

                nRed -= gView->at36e>>6;
                nGreen -= gView->at36e>>5;
                nBlue -= gView->at36e>>6;

                newDAC[i].red = ClipRange(nRed, 0, 255);
                newDAC[i].green = ClipRange(nGreen, 0, 255);
                newDAC[i].blue = ClipRange(nBlue, 0, 255);
            }
        }
        if (memcmp(newDAC, curDAC, 768) != 0)
        {
            memcpy(curDAC, newDAC, 768);
            gSetDacRange(0, 256, curDAC);
        }
    }
}

bool gPrediction = true;

char otherMirrorGotpic[2];
char bakMirrorGotpic[2];
// int gVisibility;

int deliriumTilt, deliriumTurn, deliriumPitch;
int gScreenTiltO, deliriumTurnO, deliriumPitchO;

int gShowFrameRate = 1;

void viewUpdateDelirium(void)
{
    gScreenTiltO = gScreenTilt;
    deliriumTurnO = deliriumTurn;
    deliriumPitchO = deliriumPitch;
	int powerCount;
	if ((powerCount = powerupCheck(gView,28)) != 0)
	{
		int tilt1 = 170, tilt2 = 170, pitch = 20;
        int timer = gFrameClock*4;
		if (powerCount < 512)
		{
			int powerScale = (powerCount<<16) / 512;
			tilt1 = mulscale16(tilt1, powerScale);
			tilt2 = mulscale16(tilt2, powerScale);
			pitch = mulscale16(pitch, powerScale);
		}
		int sin2 = costable[(2*timer-512)&2047] / 2;
		int sin3 = costable[(3*timer-512)&2047] / 2;
		gScreenTilt = mulscale30(sin2+sin3,tilt1);
		int sin4 = costable[(4*timer-512)&2047] / 2;
		deliriumTurn = mulscale30(sin3+sin4,tilt2);
		int sin5 = costable[(5*timer-512)&2047] / 2;
		deliriumPitch = mulscale30(sin4+sin5,pitch);
		return;
	}
	gScreenTilt = ((gScreenTilt+1024)&2047)-1024;
	if (gScreenTilt > 0)
	{
		gScreenTilt -= 8;
		if (gScreenTilt < 0)
			gScreenTilt = 0;
	}
	else if (gScreenTilt < 0)
	{
		gScreenTilt += 8;
		if (gScreenTilt >= 0)
			gScreenTilt = 0;
	}
}

int shakeHoriz, shakeAngle, shakeX, shakeY, shakeZ, shakeBobX, shakeBobY;

void viewUpdateShake(void)
{
    shakeHoriz = 0;
    shakeAngle = 0;
    shakeX = 0;
    shakeY = 0;
    shakeZ = 0;
    shakeBobX = 0;
    shakeBobY = 0;
    if (gView->at35a)
    {
        int nValue = ClipHigh(gView->at35a * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
    if (gView->at37f)
    {
        int nValue = ClipHigh(gView->at37f * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
}

int32_t r_maxfps = 60;
int32_t r_maxfpsoffset = 0;
double g_frameDelay = 0.0;

int viewFPSLimit(void)
{
    static auto nextPageTicks = (double)timerGetTicksU64();
    static unsigned frameWaiting  = 0;

    if (frameWaiting)
    {
        frameWaiting--;
        videoNextPage();
    }

    auto const frameTicks = (double)timerGetTicksU64();

    if (!r_maxfps || frameTicks >= nextPageTicks)
    {
        if (frameTicks >= nextPageTicks + g_frameDelay)
            nextPageTicks = frameTicks;

        nextPageTicks += g_frameDelay;
        frameWaiting++;
    }

    return frameWaiting;
}

float r_ambientlight = 1.0, r_ambientlightrecip = 1.0;

int gLastPal = 0;

int32_t g_frameRate;

void viewDrawScreen(void)
{
    int nPalette = 0;
    static int lastUpdate;
    int defaultHoriz = gCenterHoriz ? 100 : 90;

#ifdef USE_OPENGL
    polymostcenterhoriz = defaultHoriz;
#endif

    timerUpdate();
    int delta = ClipLow(gGameClock - lastUpdate, 0);
    lastUpdate = gGameClock;
    if (!gPaused && (!CGameMenuMgr::m_bActive || gGameOptions.nGameType != 0))
    {
        gInterpolate = divscale16(gGameClock-gNetFifoClock+4, 4);
    }
    if (gInterpolate < 0 || gInterpolate > 65536)
    {
        gInterpolate = ClipRange(gInterpolate, 0, 65536);
    }
    if (gViewInterpolate)
    {
        CalcInterpolations();
    }

    if (gViewMode == 3 || gViewMode == 4 || gOverlayMap)
    {
        DoSectorLighting();
    }
    if (gViewMode == 3 || gOverlayMap)
    {
        int yxAspect = yxaspect;
        int viewingRange = viewingrange;
        if (r_usenewaspect)
        {
            newaspect_enable = 1;
            videoSetCorrectedAspect();
        }
        renderSetAspect(Blrintf(float(viewingrange) * tanf(gFov * (PI/360.f))), yxaspect);
        int cX = gView->pSprite->x;
        int cY = gView->pSprite->y;
        int cZ = gView->at67;
        int zDelta = gView->at6f-gView->at67-(12<<8);
        fix16_t cA = gView->q16ang;
        fix16_t q16horiz = gView->q16horiz;
        fix16_t q16slopehoriz = gView->q16slopehoriz;
        int v74 = gView->at43;
        int v8c = gView->at3f;
        int v4c = gView->at53;
        int v48 = gView->at4f;
        int nSectnum = gView->pSprite->sectnum;
        if (gViewInterpolate)
        {
            if (numplayers > 1 && gView == gMe && gPrediction && gMe->pXSprite->health > 0)
            {
                nSectnum = predict.at68;
                cX = interpolate(predictOld.at50, predict.at50, gInterpolate);
                cY = interpolate(predictOld.at54, predict.at54, gInterpolate);
                cZ = interpolate(predictOld.at38, predict.at38, gInterpolate);
                zDelta = interpolate(predictOld.at34, predict.at34, gInterpolate);
                cA = interpolateangfix16(predictOld.at30, predict.at30, gInterpolate);
                q16horiz = interpolate(predictOld.at24, predict.at24, gInterpolate);
                q16slopehoriz = interpolate(predictOld.at28, predict.at28, gInterpolate);
                v74 = interpolate(predictOld.atc, predict.atc, gInterpolate);
                v8c = interpolate(predictOld.at8, predict.at8, gInterpolate);
                v4c = interpolate(predictOld.at1c, predict.at1c, gInterpolate);
                v48 = interpolate(predictOld.at18, predict.at18, gInterpolate);
            }
            else
            {
                VIEW *pView = &gPrevView[gViewIndex];
                cX = interpolate(pView->at50, cX, gInterpolate);
                cY = interpolate(pView->at54, cY, gInterpolate);
                cZ = interpolate(pView->at38, cZ, gInterpolate);
                zDelta = interpolate(pView->at34, zDelta, gInterpolate);
                cA = interpolateangfix16(pView->at30, cA, gInterpolate);
                q16horiz = interpolate(pView->at24, q16horiz, gInterpolate);
                q16slopehoriz = interpolate(pView->at28, q16slopehoriz, gInterpolate);
                v74 = interpolate(pView->atc, v74, gInterpolate);
                v8c = interpolate(pView->at8, v8c, gInterpolate);
                v4c = interpolate(pView->at1c, v4c, gInterpolate);
                v48 = interpolate(pView->at18, v48, gInterpolate);
            }
        }
        viewUpdateShake();
        q16horiz += fix16_from_int(shakeHoriz);
        cA += fix16_from_int(shakeAngle);
        cX += shakeX;
        cY += shakeY;
        cZ += shakeZ;
        v4c += shakeBobX;
        v48 += shakeBobY;
        q16horiz += fix16_from_int(mulscale30(0x40000000-Cos(gView->at35e<<2), 30));
        if (gViewPos == 0)
        {
            if (gViewHBobbing)
            {
                cX -= mulscale30(v74, Sin(fix16_to_int(cA)))>>4;
                cY += mulscale30(v74, Cos(fix16_to_int(cA)))>>4;
            }
            if (gViewVBobbing)
            {
                cZ += v8c;
            }
            if (gSlopeTilting)
            {
                q16horiz += q16slopehoriz;
            }
            cZ += fix16_to_int(q16horiz*10);
            cameradist = -1;
            cameraclock = gGameClock;
        }
        else
        {
            CalcPosition(gView->pSprite, (int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum, fix16_to_int(cA), q16horiz);
        }
        CheckLink((int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum);
        int v78 = interpolateang(gScreenTiltO, gScreenTilt, gInterpolate);
        char v14 = 0;
        char v10 = 0;
        bool bDelirium = powerupCheck(gView, 28) > 0;
        static bool bDeliriumOld = false;
        int tiltcs, tiltdim;
        char v4 = powerupCheck(gView, 21) > 0;
#ifdef USE_OPENGL
        renderSetRollAngle(0);
#endif
        if (v78 || bDelirium)
        {
            if (videoGetRenderMode() == REND_CLASSIC)
            {
                int vr = viewingrange;
                walock[TILTBUFFER] = 255;
                if (!waloff[TILTBUFFER])
                {
                    tileAllocTile(TILTBUFFER, 640, 640, 0, 0);
                }
                if (xdim >= 640 && ydim >= 640)
                {
                    tiltcs = 1;
                    tiltdim = 640;
                }
                else
                {
                    tiltcs = 0;
                    tiltdim = 320;
                }
                renderSetTarget(TILTBUFFER, tiltdim, tiltdim);
                int nAng = v78&511;
                if (nAng > 256)
                {
                    nAng = 512-nAng;
                }
                renderSetAspect(mulscale16(vr, dmulscale32(Cos(nAng), 262144, Sin(nAng), 163840)), yxaspect);
            }
#ifdef USE_OPENGL
            else
                renderSetRollAngle(v78);
#endif
        }
        else if (v4 && gNetPlayers > 1)
        {
            int tmp = (gGameClock/240)%(gNetPlayers-1);
            int i = connecthead;
            while (1)
            {
                if (i == gViewIndex)
                    i = connectpoint2[i];
                if (tmp == 0)
                    break;
                i = connectpoint2[i];
                tmp--;
            }
            PLAYER *pOther = &gPlayer[i];
            //othercameraclock = gGameClock;
            if (!waloff[4079])
            {
                tileAllocTile(4079, 128, 128, 0, 0);
            }
            renderSetTarget(4079, 128, 128);
            renderSetAspect(65536, 78643);
            int vd8 = pOther->pSprite->x;
            int vd4 = pOther->pSprite->y;
            int vd0 = pOther->at67;
            int vcc = pOther->pSprite->sectnum;
            int v50 = pOther->pSprite->ang;
            int v54 = 0;
            if (pOther->at35a)
            {
                int nValue = ClipHigh(pOther->at35a*8, 2000);
                v54 += QRandom2(nValue>>8);
                v50 += QRandom2(nValue>>8);
                vd8 += QRandom2(nValue>>4);
                vd4 += QRandom2(nValue>>4);
                vd0 += QRandom2(nValue);
            }
            if (pOther->at37f)
            {
                int nValue = ClipHigh(pOther->at37f*8, 2000);
                v54 += QRandom2(nValue >> 8);
                v50 += QRandom2(nValue >> 8);
                vd8 += QRandom2(nValue >> 4);
                vd4 += QRandom2(nValue >> 4);
                vd0 += QRandom2(nValue);
            }
            CalcOtherPosition(pOther->pSprite, &vd8, &vd4, &vd0, &vcc, v50, 0);
            CheckLink(&vd8, &vd4, &vd0, &vcc);
            if (IsUnderwaterSector(vcc))
            {
                v14 = 10;
            }
            memcpy(bakMirrorGotpic, gotpic+510, 2);
            memcpy(gotpic+510, otherMirrorGotpic, 2);
            g_visibility = (int32_t)(ClipLow(gVisibility-32*pOther->at362, 0) * (numplayers > 1 ? 1.f : r_ambientlightrecip));
            int vc4, vc8;
            getzsofslope(vcc, vd8, vd4, &vc8, &vc4);
            if (vd0 >= vc4)
            {
                vd0 = vc4-(8<<4);
            }
            if (vd0 <= vc8)
            {
                vd0 = vc8+(8<<4);
            }
            v54 = ClipRange(v54, -200, 200);
RORHACKOTHER:
            int ror_status[16];
            for (int i = 0; i < 16; i++)
                ror_status[i] = TestBitString(gotpic, 4080 + i);
            DrawMirrors(vd8, vd4, vd0, fix16_from_int(v50), fix16_from_int(v54 + defaultHoriz));
            drawrooms(vd8, vd4, vd0, v50, v54 + defaultHoriz, vcc);
            bool do_ror_hack = false;
            for (int i = 0; i < 16; i++)
                if (!ror_status[i] && TestBitString(gotpic, 4080 + i))
                    do_ror_hack = true;
            if (do_ror_hack)
                goto RORHACKOTHER;
            memcpy(otherMirrorGotpic, gotpic+510, 2);
            memcpy(gotpic+510, bakMirrorGotpic, 2);
            viewProcessSprites(vd8, vd4, vd0);
            renderDrawMasks();
            renderRestoreTarget();
        }
        else
        {
            othercameraclock = gGameClock;
        }

        if (!bDelirium)
        {
            deliriumTilt = 0;
            deliriumTurn = 0;
            deliriumPitch = 0;
        }
        int nSprite = headspritestat[2];
        int unk = 0;
        while (nSprite >= 0)
        {
            spritetype *pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (TestBitString(gotsector, pSprite->sectnum))
            {
                unk += pXSprite->data3*32;
            }
            nSprite = nextspritestat[nSprite];
        }
        nSprite = headspritestat[5];
        while (nSprite >= 0)
        {
            spritetype *pSprite = &sprite[nSprite];
            switch (pSprite->type)
            {
            case 301:
            case 302:
            case 303:
            case 306:
                if (TestBitString(gotsector, pSprite->sectnum))
                {
                    unk += 256;
                }
                break;
            }
            nSprite = nextspritestat[nSprite];
        }
        g_visibility = (int32_t)(ClipLow(gVisibility - 32 * gView->at362 - unk, 0) * (numplayers > 1 ? 1.f : r_ambientlightrecip));
        cA = (cA + interpolateangfix16(fix16_from_int(deliriumTurnO), fix16_from_int(deliriumTurn), gInterpolate)) & 0x7ffffff;
        int vfc, vf8;
        getzsofslope(nSectnum, cX, cY, &vfc, &vf8);
        if (cZ >= vf8)
        {
            cZ = vf8-(8<<8);
        }
        if (cZ <= vfc)
        {
            cZ = vfc+(8<<8);
        }
        q16horiz = ClipRange(q16horiz, F16(-200), F16(200));
RORHACK:
        int ror_status[16];
        for (int i = 0; i < 16; i++)
            ror_status[i] = TestBitString(gotpic, 4080+i);
        fix16_t deliriumPitchI = interpolate(fix16_from_int(deliriumPitchO), fix16_from_int(deliriumPitch), gInterpolate);
        DrawMirrors(cX, cY, cZ, cA, q16horiz + fix16_from_int(defaultHoriz) + deliriumPitchI);
        int bakCstat = gView->pSprite->cstat;
        if (gViewPos == 0)
        {
            gView->pSprite->cstat |= 32768;
        }
        else
        {
            gView->pSprite->cstat |= 514;
        }
        renderDrawRoomsQ16(cX, cY, cZ, cA, q16horiz + fix16_from_int(defaultHoriz) + deliriumPitchI, nSectnum);
        bool do_ror_hack = false;
        for (int i = 0; i < 16; i++)
            if (!ror_status[i] && TestBitString(gotpic, 4080+i))
                do_ror_hack = true;
        if (do_ror_hack)
        {
            gView->pSprite->cstat = bakCstat;
            goto RORHACK;
        }

        viewProcessSprites(cX, cY, cZ);
        sub_5571C(1);
        renderDrawMasks();
        sub_5571C(0);
        sub_557C4(cX, cY, gInterpolate);
        renderDrawMasks();
        gView->pSprite->cstat = bakCstat;

        if (v78 || bDelirium)
        {
            if (videoGetRenderMode() == REND_CLASSIC)
            {
                dassert(waloff[ TILTBUFFER ] != 0);
                renderRestoreTarget();
                int vrc = 64+4+2+1024;
                if (bDelirium)
                {
                    vrc = 64+32+4+2+1+1024;
                }
                int nAng = v78 & 511;
                if (nAng > 256)
                {
                    nAng = 512 - nAng;
                }
                int nScale = dmulscale32(Cos(nAng), 262144, Sin(nAng), 163840)>>tiltcs;
                rotatesprite(160<<16, 100<<16, nScale, v78+512, TILTBUFFER, 0, 0, vrc, gViewX0, gViewY0, gViewX1, gViewY1);
            }
#ifdef USE_OPENGL
            else
            {
                if (videoGetRenderMode() == REND_POLYMOST && gDeliriumBlur)
                {
                    if (!bDeliriumOld)
                    {
                        glAccum(GL_LOAD, 1.f);
                    }
                    else
                    {
                        const float fBlur = pow(1.f/3.f, 30.f/g_frameRate);
                        glAccum(GL_MULT, fBlur);
                        glAccum(GL_ACCUM, 1.f-fBlur);
                        glAccum(GL_RETURN, 1.f);
                    }
                }
            }
#endif
        }

        bDeliriumOld = bDelirium && gDeliriumBlur;

        if (r_usenewaspect)
            newaspect_enable = 0;
        renderSetAspect(viewingRange, yxAspect);
        int nClipDist = gView->pSprite->clipdist<<2;
        int ve8, vec, vf0, vf4;
        GetZRange(gView->pSprite, &vf4, &vf0, &vec, &ve8, nClipDist, 0);
#if 0
        int tmpSect = nSectnum;
        if ((vf0 & 0xe000) == 0x4000)
        {
            tmpSect = vf0 & (kMaxWalls-1);
        }
        int v8 = byte_1CE5C2 > 0 && (sector[tmpSect].ceilingstat&1);
        if (gWeather.at12d8 > 0 || v8)
        {
            gWeather.Draw(cX, cY, cZ, cA, q16horiz + defaultHoriz + deliriumPitch, gWeather.at12d8);
            if (v8)
            {
                gWeather.at12d8 = ClipRange(delta*8+gWeather.at12d8, 0, 4095);
            }
            else
            {
                gWeather.at12d8 = ClipRange(gWeather.at12d8-delta*64, 0, 4095);
            }
        }
#endif
        if (gViewPos == 0)
        {
            if (gAimReticle)
            {
                rotatesprite(160<<16, defaultHoriz<<16, 65536, 0, kCrosshairTile, 0, CROSSHAIR_PAL, 2, gViewX0, gViewY0, gViewX1, gViewY1);
            }
            cX = (v4c>>8)+160;
            cY = (v48>>8)+220+(zDelta>>7);
            int nShade = sector[nSectnum].floorshade;
            int nPalette = 0;
            if (sector[gView->pSprite->sectnum].extra > 0)
            {
                sectortype *pSector = &sector[gView->pSprite->sectnum];
                XSECTOR *pXSector = &xsector[pSector->extra];
                if (pXSector->color)
                {
                    nPalette = pSector->floorpal;
                }
            }
            WeaponDraw(gView, nShade, cX, cY, nPalette);
        }
        if (gViewPos == 0 && gView->pXSprite->burnTime > 60)
        {
            viewBurnTime(gView->pXSprite->burnTime);
        }
        if (packItemActive(gView, 1))
        {
            rotatesprite(0, 0, 65536, 0, 2344, 0, 0, 256+18, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 0, 65536, 1024, 2344, 0, 0, 512+22, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(0, 200<<16, 65536, 0, 2344, 0, 0, 256+22, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 200<<16, 65536, 1024, 2344, 0, 0, 512+18, gViewX0, gViewY0, gViewX1, gViewY1);
            if (gDetail >= 4)
            {
                rotatesprite(15<<16, 3<<16, 65536, 0, 2346, 32, 0, 256+19, gViewX0, gViewY0, gViewX1, gViewY1);
                rotatesprite(212<<16, 77<<16, 65536, 0, 2347, 32, 0, 512+19, gViewX0, gViewY0, gViewX1, gViewY1);
            }
        }
        if (powerupCheck(gView, 39) > 0)
        {
            rotatesprite(0, 200<<16, 65536, 0, 2358, 0, 0, 256+22, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(320<<16, 200<<16, 65536, 1024, 2358, 0, 0, 512+18, gViewX0, gViewY0, gViewX1, gViewY1);
        }
        if (v4 && gNetPlayers > 1)
        {
            DoLensEffect();
            viewingRange = viewingrange;
            yxAspect = yxaspect;
            renderSetAspect(65536, 54613);
            rotatesprite(280<<16, 35<<16, 53248, 512, 4077, v10, v14, 512+6, gViewX0, gViewY0, gViewX1, gViewY1);
            rotatesprite(280<<16, 35<<16, 53248, 0, 1683, v10, 0, 512+35, gViewX0, gViewY0, gViewX1, gViewY1);
            renderSetAspect(viewingRange, yxAspect);
        }
        if (powerupCheck(gView, 14) > 0)
        {
            nPalette = 4;
        }
        else if(powerupCheck(gView, 24) > 0)
        {
            nPalette = 1;
        }
        else if (gView->at87)
        {
            if (gView->nWaterPal)
                nPalette = gView->nWaterPal;
            else
            {
                if (gView->pXSprite->medium == 1)
                {
                    nPalette = 1;
                }
                else if (gView->pXSprite->medium == 2)
                {
                    nPalette = 3;
                }
                else
                {
                    nPalette = 2;
                }
            }
        }
    }
    if (gViewMode == 4)
    {
        gViewMap.sub_25DB0(gView->pSprite);
    }
    viewDrawInterface(delta);
    int zn = ((gView->at6f-gView->at67-(12<<8))>>7)+220;
    PLAYER *pPSprite = &gPlayer[gMe->pSprite->type-kDudePlayer1];
    if (pPSprite->at376 == 1)
    {
        //static int lastClock;
        gChoke.sub_84110(160, zn);
        //if ((gGameClock % 5) == 0 && gGameClock != lastClock)
        //{
        //    gChoke.at1c(pPSprite);
        //}
        //lastClock = gGameClock;
    }
    if (byte_1A76C6)
    {
        DrawStatSprite(2048, xdim-15, 20);
    }
    viewDisplayMessage();
    CalcFrameRate();
#if 0
    if (gShowFrameRate)
    {
        int fX, fY;
        if (gViewMode == 3)
        {
            fX = gViewX1;
        }
        else
        {
            fX = xdim;
        }
        if (gViewMode == 3)
        {
            fY = gViewY0;
        }
        else
        {
            fY = 0;
        }
        if (gViewMode == 4)
        {
            fY += mulscale16(20, yscale);
        }
        sprintf(gTempStr, "%3i", gFrameRate);
        printext256(fX-12, fY, 31, -1, gTempStr, 1);
        fY += 8;
        sprintf(gTempStr, "pos=%d,%d,%d", gView->pSprite->x, gView->pSprite->y, gView->pSprite->z);
        printext256(fX-strlen(gTempStr)*4, fY, 31, -1, gTempStr, 1);
    }
#endif
    viewPrintFPS();
    if (gPaused)
    {
        viewDrawText(1, "PAUSED", 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        sprintf(gTempStr, "] %s [", gProfile[gView->at57].name);
        viewDrawText(0, gTempStr, 160, 10, 0, 0, 1, 0);
    }
    if (errMsg[0])
    {
        viewDrawText(0, errMsg, 160, 20, 0, 0, 1, 0);
    }
    if (gViewInterpolate)
    {
        RestoreInterpolations();
    }
    UpdateDacs(nPalette);
}

int nLoadingScreenTile;
char pzLoadingScreenText1[256], pzLoadingScreenText2[256], pzLoadingScreenText3[256];

void viewLoadingScreenWide(void)
{
    videoClearScreen(0);
#ifdef USE_OPENGL
    if ((blood_globalflags&BLOOD_FORCE_WIDELOADSCREEN) || (bLoadScreenCrcMatch && !(usehightile && h_xsize[kLoadScreen])))
#else
    if ((blood_globalflags&BLOOD_FORCE_WIDELOADSCREEN) || bLoadScreenCrcMatch)
#endif
    {
        if (yxaspect >= 65536)
        {
            rotatesprite(160<<16, 100<<16, 65536, 0, kLoadScreen, 0, 0, 1024+64+8+2, 0, 0, xdim-1, ydim-1);
        }
        else
        {
            int width = roundscale(xdim, 240, ydim);
            int nCount = (width+kLoadScreenWideBackWidth-1)/kLoadScreenWideBackWidth;
            for (int i = 0; i < nCount; i++)
            {
                rotatesprite_fs((i*kLoadScreenWideBackWidth)<<16, 0, 65536, 0, kLoadScreenWideBack, 0, 0, 256+64+16+8+2);
            }
            rotatesprite_fs((kLoadScreenWideSideWidth>>1)<<16, 200<<15, 65536, 0, kLoadScreenWideLeft, 0, 0, 256+8+2);
            rotatesprite_fs((320-(kLoadScreenWideSideWidth>>1))<<16, 200<<15, 65536, 0, kLoadScreenWideRight, 0, 0, 512+8+2);
            rotatesprite_fs(320<<15, 200<<15, 65536, 0, kLoadScreenWideMiddle, 0, 0, 8+2);
        }
    }
    else
        rotatesprite(160<<16, 100<<16, 65536, 0, kLoadScreen, 0, 0, 64+8+2, 0, 0, xdim-1, ydim-1);
}

void viewLoadingScreenUpdate(const char *pzText4, int nPercent)
{
    int vc;
    gMenuTextMgr.GetFontInfo(1, NULL, NULL, &vc);
    if (nLoadingScreenTile == kLoadScreen)
        viewLoadingScreenWide();
    else if (nLoadingScreenTile)
    {
        videoClearScreen(0);
        rotatesprite(160<<16, 100<<16, 65536, 0, nLoadingScreenTile, 0, 0, 74, 0, 0, xdim-1, ydim-1);
    }
    if (pzLoadingScreenText1[0])
    {
        rotatesprite(160<<16, 20<<16, 65536, 0, 2038, -128, 0, 78, 0, 0, xdim-1, ydim-1);
        viewDrawText(1, pzLoadingScreenText1, 160, 20-vc/2, -128, 0, 1, 1);
    }
    if (pzLoadingScreenText2[0])
    {
        viewDrawText(1, pzLoadingScreenText2, 160, 50, -128, 0, 1, 1);
    }
    if (pzLoadingScreenText3[0])
    {
        viewDrawText(1, pzLoadingScreenText3, 160, 70, -128, 0, 1, 1);
    }
    if (pzText4)
    {
        viewDrawText(3, pzText4, 160, 124, -128, 0, 1, 1);
    }

    if (nPercent != -1)
        TileHGauge(2260, 86, 110, nPercent, 100, 0, 131072);

    viewDrawText(3, "Please Wait", 160, 134, -128, 0, 1, 1);
    scrNextPage();
}

void viewLoadingScreen(int nTile, const char *pText, const char *pText2, const char *pText3)
{
    UpdateDacs(0, true);
    nLoadingScreenTile = nTile;
    if (pText)
        strncpy(pzLoadingScreenText1, pText, 256);
    else
        pzLoadingScreenText1[0] = 0;
    if (pText2)
        strncpy(pzLoadingScreenText2, pText2, 256);
    else
        pzLoadingScreenText2[0] = 0;
    if (pText3)
        strncpy(pzLoadingScreenText3, pText3, 256);
    else
        pzLoadingScreenText3[0] = 0;
    viewLoadingScreenUpdate(NULL, -1);
}

palette_t CrosshairColors = { 255, 255, 255, 0 };
bool g_isAlterDefaultCrosshair = false;

void viewSetCrosshairColor(int32_t r, int32_t g, int32_t b)
{
    if (!g_isAlterDefaultCrosshair)
        return;

    CrosshairColors.r = r;
    CrosshairColors.g = g;
    CrosshairColors.b = b;

    tileLoad(kCrosshairTile);

    if (!waloff[kCrosshairTile])
        return;

    char *ptr = (char *)waloff[kCrosshairTile];

    int32_t ii = tilesiz[kCrosshairTile].x * tilesiz[kCrosshairTile].y;

    dassert(ii > 0);

    int32_t i = (videoGetRenderMode() == REND_CLASSIC)
        ? paletteGetClosestColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b)
        : paletteGetClosestColor(255, 255, 255);  // use white in GL so we can tint it to the right color

    do
    {
        if (*ptr != 255)
            *ptr = i;
        ptr++;
    } while (--ii);

    paletteMakeLookupTable(CROSSHAIR_PAL, NULL, CrosshairColors.r, CrosshairColors.g, CrosshairColors.b, 1);

#ifdef USE_OPENGL
    // XXX: this makes us also load all hightile textures tinted with the crosshair color!
    polytint_t & crosshairtint = hictinting[CROSSHAIR_PAL];
    crosshairtint.r = CrosshairColors.r;
    crosshairtint.g = CrosshairColors.g;
    crosshairtint.b = CrosshairColors.b;
    crosshairtint.f = HICTINT_USEONART | HICTINT_GRAYSCALE;
#endif
    tileInvalidate(kCrosshairTile, -1, -1);
}

void viewResetCrosshairToDefault(void)
{
    paletteFreeLookupTable(CROSSHAIR_PAL);
    tileLoad(kCrosshairTile);
}

#define COLOR_RED redcol
#define COLOR_WHITE whitecol

#define LOW_FPS 60
#define SLOW_FRAME_TIME 20

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

#define FPS_COLOR(x) ((x) ? COLOR_RED : COLOR_WHITE)

int32_t gShowFps, gFramePeriod;

void viewPrintFPS(void)
{
    char tempbuf[128];
    static int32_t frameCount;
    static double cumulativeFrameDelay;
    static double lastFrameTime;
    static float lastFPS, minFPS = FLT_MAX, maxFPS;
    static double minGameUpdate = DBL_MAX, maxGameUpdate;

    double frameTime = timerGetHiTicks();
    double frameDelay = frameTime - lastFrameTime;
    cumulativeFrameDelay += frameDelay;

    if (frameDelay >= 0)
    {
        int32_t x = (xdim <= 640);

        if (gShowFps)
        {
            int32_t chars = Bsprintf(tempbuf, "%.1f ms, %5.1f fps", frameDelay, lastFPS);

            printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+2+FPS_YOFFSET, 0, -1, tempbuf, x);
            printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+1+FPS_YOFFSET,
                FPS_COLOR(lastFPS < LOW_FPS), -1, tempbuf, x);

            if (gShowFps > 1)
            {
                chars = Bsprintf(tempbuf, "max: %5.1f fps", maxFPS);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+10+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+10+FPS_YOFFSET,
                    FPS_COLOR(maxFPS < LOW_FPS), -1, tempbuf, x);

                chars = Bsprintf(tempbuf, "min: %5.1f fps", minFPS);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+20+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+20+FPS_YOFFSET,
                    FPS_COLOR(minFPS < LOW_FPS), -1, tempbuf, x);
            }
            if (gShowFps > 2)
            {
                if (g_gameUpdateTime > maxGameUpdate) maxGameUpdate = g_gameUpdateTime;
                if (g_gameUpdateTime < minGameUpdate) minGameUpdate = g_gameUpdateTime;

                chars = Bsprintf(tempbuf, "Game Update: %2.2f ms + draw: %2.2f ms", g_gameUpdateTime, g_gameUpdateAndDrawTime);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+30+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+30+FPS_YOFFSET,
                    FPS_COLOR(g_gameUpdateAndDrawTime >= SLOW_FRAME_TIME), -1, tempbuf, x);

                chars = Bsprintf(tempbuf, "GU min/max/avg: %5.2f/%5.2f/%5.2f ms", minGameUpdate, maxGameUpdate, g_gameUpdateAvgTime);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+40+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+40+FPS_YOFFSET,
                    FPS_COLOR(maxGameUpdate >= SLOW_FRAME_TIME), -1, tempbuf, x);
                
                chars = Bsprintf(tempbuf, "bufferjitter: %i", gBufferJitter);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+50+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+50+FPS_YOFFSET,
                    COLOR_WHITE, -1, tempbuf, x);
#if 0
                chars = Bsprintf(tempbuf, "G_MoveActors(): %.3e ms", g_moveActorsTime);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+50+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+50+FPS_YOFFSET,
                    COLOR_WHITE, -1, tempbuf, x);

                chars = Bsprintf(tempbuf, "G_MoveWorld(): %.3e ms", g_moveWorldTime);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+60+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+60+FPS_YOFFSET,
                    COLOR_WHITE, -1, tempbuf, x);
#endif
            }
#if 0
            // lag meter
            if (g_netClientPeer)
            {
                chars = Bsprintf(tempbuf, "%d +- %d ms", (g_netClientPeer->lastRoundTripTime + g_netClientPeer->roundTripTime)/2,
                    (g_netClientPeer->lastRoundTripTimeVariance + g_netClientPeer->roundTripTimeVariance)/2);

                printext256(windowxy2.x-(chars<<(3-x))+1, windowxy1.y+30+2+FPS_YOFFSET, 0, -1, tempbuf, x);
                printext256(windowxy2.x-(chars<<(3-x)), windowxy1.y+30+1+FPS_YOFFSET, FPS_COLOR(g_netClientPeer->lastRoundTripTime > 200), -1, tempbuf, x);
            }
#endif
        }

        if (cumulativeFrameDelay >= 1000.0)
        {
            lastFPS = 1000.f * frameCount / cumulativeFrameDelay;
            g_frameRate = Blrintf(lastFPS);
            frameCount = 0;
            cumulativeFrameDelay = 0.0;

            if (gShowFps > 1)
            {
                if (lastFPS > maxFPS) maxFPS = lastFPS;
                if (lastFPS < minFPS) minFPS = lastFPS;

                static int secondCounter;

                if (++secondCounter >= gFramePeriod)
                {
                    maxFPS = (lastFPS + maxFPS) * .5f;
                    minFPS = (lastFPS + minFPS) * .5f;
                    maxGameUpdate = (g_gameUpdateTime + maxGameUpdate) * 0.5;
                    minGameUpdate = (g_gameUpdateTime + minGameUpdate) * 0.5;
                    secondCounter = 0;
                }
            }
        }
        frameCount++;
    }
    lastFrameTime = frameTime;
}

#undef FPS_COLOR

class ViewLoadSave : public LoadSave {
public:
    void Load(void);
    void Save(void);
};

static ViewLoadSave *myLoadSave;

static int messageTime;
static char message[256];

void ViewLoadSave::Load(void)
{
    Read(&messageTime, sizeof(messageTime));
    Read(message, sizeof(message));
    Read(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Read(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Read(&gScreenTilt, sizeof(gScreenTilt));
    Read(&deliriumTilt, sizeof(deliriumTilt));
    Read(&deliriumTurn, sizeof(deliriumTurn));
    Read(&deliriumPitch, sizeof(deliriumPitch));
}

void ViewLoadSave::Save(void)
{
    Write(&messageTime, sizeof(messageTime));
    Write(message, sizeof(message));
    Write(otherMirrorGotpic, sizeof(otherMirrorGotpic));
    Write(bakMirrorGotpic, sizeof(bakMirrorGotpic));
    Write(&gScreenTilt, sizeof(gScreenTilt));
    Write(&deliriumTilt, sizeof(deliriumTilt));
    Write(&deliriumTurn, sizeof(deliriumTurn));
    Write(&deliriumPitch, sizeof(deliriumPitch));
}

void ViewLoadSaveConstruct(void)
{
    myLoadSave = new ViewLoadSave();
}
