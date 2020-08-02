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


VIEW gPrevView[kMaxPlayers];
VIEWPOS gViewPos;
int gViewIndex;

struct INTERPOLATE {
    void *pointer;
    int value;
    int value2;
    INTERPOLATE_TYPE type;
};

int gViewMode = 3;
int gViewSize = 2;

int gInterpolate;
int nInterpolations;
char gInterpolateSprite[(kMaxSprites+7)>>3];
char gInterpolateWall[(kMaxWalls+7)>>3];
char gInterpolateSector[(kMaxSectors+7)>>3];

#define kMaxInterpolations 16384

INTERPOLATE gInterpolation[kMaxInterpolations];

int gViewXCenter, gViewYCenter;
int gViewX0, gViewY0, gViewX1, gViewY1;
int gViewX0S, gViewY0S, gViewX1S, gViewY1S;
int xscale, yscale, xstep, ystep;

int gScreenTilt;



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

FFont *gFont[kFontNum];

void FontSet(int id, int tile, int space)
{
	if (id < 0 || id >= kFontNum || tile < 0 || tile >= kMaxTiles)
		return;

	GlyphSet glyphs;
	for (int i = 1; i < 96; i++)
	{
		auto tex = tileGetTexture(tile + i);
		if (tex && tex->isValid() && tex->GetTexelWidth() > 0 && tex->GetTexelHeight() > 0)
		{
			glyphs.Insert(i + 32, tex);
			tex->SetOffsetsNotForFont();
		}

	}
	const char *names[] = { "smallfont", "bigfont", "gothfont", "smallfont2", "digifont"};
	const char *defs[] = { "defsmallfont", "defbigfont", nullptr, "defsmallfont2", nullptr};
	FFont ** ptrs[] = { &SmallFont, &BigFont, nullptr, &SmallFont2, nullptr};
	gFont[id] =	new ::FFont(names[id], nullptr, defs[id], 0, 0, 0, 0, tileWidth(tile), false, false, false, &glyphs);
	gFont[id]->SetKerning(space);
	if (ptrs[id]) *ptrs[id] = gFont[id];
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

void viewBackupView(int nPlayer)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at30 = pPlayer->q16ang;
    pView->at50 = pPlayer->pSprite->x;
    pView->at54 = pPlayer->pSprite->y;
    pView->at38 = pPlayer->zView;
    pView->at34 = pPlayer->zWeapon-pPlayer->zView-0xc00;
    pView->at24 = pPlayer->q16horiz;
    pView->at28 = pPlayer->q16slopehoriz;
    pView->at2c = pPlayer->slope;
    pView->at8 = pPlayer->bobHeight;
    pView->atc = pPlayer->bobWidth;
    pView->at18 = pPlayer->swayHeight;
    pView->at1c = pPlayer->swayWidth;
}

void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos)
{
    PLAYER *pPlayer = &gPlayer[nPlayer];
    VIEW *pView = &gPrevView[nPlayer];
    pView->at50 += pPlayer->pSprite->x-oldpos->x;
    pView->at54 += pPlayer->pSprite->y-oldpos->y;
    pView->at38 += pPlayer->pSprite->z-oldpos->z;
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
    if (nInterpolations == kMaxInterpolations)
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
    if (nFont < 0 || nFont >= kFontNum || !pString) return;
    FFont *pFont = gFont[nFont];

    //y += pFont->yoff;

	if (position) x -= pFont->StringWidth(pString) / 2;

	if (shadow)
	{
		DrawText(twod, pFont, CR_UNDEFINED, x+1, y+1, pString, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_Color, 0xff000000, DTA_Alpha, 0.5, TAG_DONE);
	}
	DrawText(twod, pFont, CR_UNDEFINED, x, y, pString, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, nPalette),
			 DTA_Color, shadeToLight(nShade), DTA_Alpha, alpha / 255., TAG_DONE);

}

void InitStatusBar(void)
{
    tileLoadTile(2200);
}
GameStats GameInterface::getStats()
{
	return { gKillMgr.at4, gKillMgr.at0, gSecretMgr.at4, gSecretMgr.at0, gLevelTime / kTicsPerSec, gPlayer[myconnectindex].fragCount };
}

void viewDrawMapTitle(void)
{
    if (!hud_showmapname || M_Active())
        return;

    int const fadeStartTic = kTicsPerSec;
    int const fadeEndTic = int(1.5f*kTicsPerSec);
    if (gLevelTime > fadeEndTic)
        return;
    int const alpha = 255 - clamp((gLevelTime-fadeStartTic)*255/(fadeEndTic-fadeStartTic), 0, 255);

    if (alpha != 0)
    {
        viewDrawText(1, levelGetTitle(), 160, 50, -128, 0, 1, 1, 0, alpha);
    }
}

void viewDrawAimedPlayerName(void)
{
    if (!cl_idplayers || (gView->aim.dx == 0 && gView->aim.dy == 0))
        return;

    int hit = HitScan(gView->pSprite, gView->pSprite->z, gView->aim.dx, gView->aim.dy, gView->aim.dz, CLIPMASK0, 512);
    if (hit == 3)
    {
        spritetype* pSprite = &sprite[gHitInfo.hitsprite];
        if (IsPlayerSprite(pSprite))
        {
            char nPlayer = pSprite->type-kDudePlayer1;
            char* szName = gProfile[nPlayer].name;
            int nPalette = (gPlayer[nPlayer].teamId&3)+11;
            viewDrawText(4, szName, 160, 125, -128, nPalette, 1, 1);
        }
    }
}

void viewPrecacheTiles(void)
{
    tilePrecacheTile(2173, 0);
    tilePrecacheTile(2200, 0);
    tilePrecacheTile(2201, 0);
    tilePrecacheTile(2202, 0);
    tilePrecacheTile(2207, 0);
    tilePrecacheTile(2208, 0);
    tilePrecacheTile(2209, 0);
    tilePrecacheTile(2229, 0);
    tilePrecacheTile(2260, 0);
    tilePrecacheTile(2559, 0);
    tilePrecacheTile(2169, 0);
    tilePrecacheTile(2578, 0);
    tilePrecacheTile(2586, 0);
    tilePrecacheTile(2602, 0);
    for (int i = 0; i < 10; i++)
    {
        tilePrecacheTile(2190 + i, 0);
        tilePrecacheTile(2230 + i, 0);
        tilePrecacheTile(2240 + i, 0);
        tilePrecacheTile(2250 + i, 0);
        tilePrecacheTile(kSBarNumberHealth + i, 0);
        tilePrecacheTile(kSBarNumberAmmo + i, 0);
        tilePrecacheTile(kSBarNumberInv + i, 0);
        tilePrecacheTile(kSBarNumberArmor1 + i, 0);
        tilePrecacheTile(kSBarNumberArmor2 + i, 0);
        tilePrecacheTile(kSBarNumberArmor3 + i, 0);
    }
    /*
    for (int i = 0; i < 5; i++)
    {
        tilePrecacheTile(gPackIcons[i], 0);
        tilePrecacheTile(gPackIcons2[i].nTile, 0);
    }
    */
    for (int i = 0; i < 6; i++)
    {
        tilePrecacheTile(2220 + i, 0);
        tilePrecacheTile(2552 + i, 0);
    }
}

static TArray<uint8_t> lensdata;
int *lensTable;

int gZoom = 1024;

int dword_172CE0[16][3];

void viewInit(void)
{
    Printf("Initializing status bar\n");
    InitStatusBar();
    FontSet(0, 4096, 0);
    FontSet(1, 4192, 1);
    FontSet(2, 4288, 1);
    FontSet(3, 4384, 1);
    FontSet(4, 4480, 0);
    enginePostInit(); // This must not be done earlier!

    lensdata = fileSystem.LoadFile("lens.dat");
    dassert(lensdata.Size() == kLensSize * kLensSize * sizeof(int));

    lensTable = (int*)lensdata.Data();
#if B_BIG_ENDIAN == 1
    for (int i = 0; i < kLensSize*kLensSize; i++)
    {
        lensTable[i] = B_LITTLE32(lensTable[i]);
    }
#endif
    uint8_t *data = tileAllocTile(4077, kLensSize, kLensSize);
    memset(data, TRANSPARENT_INDEX, kLensSize*kLensSize);

    for (int i = 0; i < 16; i++)
    {
        dword_172CE0[i][0] = mulscale16(wrand(), 2048);
        dword_172CE0[i][1] = mulscale16(wrand(), 2048);
        dword_172CE0[i][2] = mulscale16(wrand(), 2048);
    }
    gViewMap.sub_25C38(0, 0, gZoom, 0, gFollowMap);
}

void viewResizeView(int size)
{
    int xdimcorrect = ClipHigh(scale(ydim, 4, 3), xdim);
    gViewXCenter = xdim-xdim/2;
    gViewYCenter = ydim-ydim/2;
    xscale = divscale16(xdim, 320);
    int xscalecorrect = divscale16(xdimcorrect, 320);
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
        if (gGameOptions.nGameType > 0 && gGameOptions.nGameType <= 3)
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
        int gy1 = (25 * ydim) / 200;
        if (gViewSize == 3) // full status bar must scale the bottom to the actual display height.
            gy1 = Scale(gy1, hud_scale, 100);
        gViewY1 = ydim-1- gy1;
        if (gGameOptions.nGameType > 0 && gGameOptions.nGameType <= 3)
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
}


void viewDrawInterface(ClockTicks arg)
{
    UpdateStatusBar(arg);
}

static fix16_t gCameraAng;

template<typename T> tspritetype* viewInsertTSprite(int nSector, int nStatnum, T const * const pSprite)
{
    int nTSprite = spritesortcnt;
    tspritetype *pTSprite = &tsprite[nTSprite];
    memset(pTSprite, 0, sizeof(tspritetype));
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
        pTSprite->x += Cos(gCameraAng)>>25;
        pTSprite->y += Sin(gCameraAng)>>25;
    }
    return pTSprite;
}

int effectDetail[] = {
    4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};


struct WEAPONICON {
    short nTile;
    char zOffset;
};

WEAPONICON gWeaponIcon[] = {
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


tspritetype *viewAddEffect(int nTSprite, VIEW_EFFECT nViewEffect)
{
    dassert(nViewEffect >= 0 && nViewEffect < kViewEffectMax);
    auto pTSprite = &tsprite[nTSprite];
    if (gDetail < effectDetail[nViewEffect] || nTSprite >= kMaxViewSprites) return NULL;
    switch (nViewEffect)
    {
    case VIEW_EFFECT_18:
        for (int i = 0; i < 16; i++)
        {
            auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
            int ang = ((int)gFrameClock*2048)/120;
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
        GetSpriteExtents(pTSprite, &top, &bottom);
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->pal = 0;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 2605;
        return pNSprite;
    }
    case VIEW_EFFECT_13:
    {
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = 26;
        pNSprite->pal = 0;
        pNSprite->cstat |= 2;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 2089;
        break;
    }
    case VIEW_EFFECT_11:
    {
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
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
            auto pNSprite = viewInsertTSprite<tspritetype>(nSector, 32767, NULL);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->shade = -128;
        pNSprite->z = pTSprite->z;
        pNSprite->picnum = 908;
        pNSprite->statnum = kStatDecoration;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/64;
        break;
    }
    case VIEW_EFFECT_6:
    {
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        pNSprite->z = top;
        if (IsDudeSprite(pTSprite))
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        pNSprite->z = top;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/32;
        break;
    }
    case VIEW_EFFECT_5:
    {
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        pNSprite->z = bottom;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tilesiz[pTSprite->picnum].x*pTSprite->xrepeat)/32;
        break;
    }
    case VIEW_EFFECT_0:
    {
        if (r_shadows)
        {
            auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
            pNSprite->z = getflorzofslope(pTSprite->sectnum, pNSprite->x, pNSprite->y);
            pNSprite->shade = 127;
            pNSprite->cstat |= 2;
            pNSprite->xrepeat = pTSprite->xrepeat;
            pNSprite->yrepeat = pTSprite->yrepeat >> 2;
            pNSprite->picnum = pTSprite->picnum;
            pNSprite->pal = 5;
            int height = tilesiz[pNSprite->picnum].y;
            int center = height / 2 + tileTopOffset(pNSprite->picnum);
            pNSprite->z -= (pNSprite->yrepeat << 2) * (height - center);
        }
        break;
    }
    case VIEW_EFFECT_1:
    {
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
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
        WEAPONICON weaponIcon = gWeaponIcon[pPlayer->curWeapon];
        const int nTile = weaponIcon.nTile;
        if (nTile < 0) break;
        auto pNSprite = viewInsertTSprite(pTSprite->sectnum, 32767, pTSprite);
        pNSprite->x = pTSprite->x;
        pNSprite->y = pTSprite->y;
        pNSprite->z = pTSprite->z-(32<<8);
        pNSprite->picnum = nTile;
        pNSprite->shade = pTSprite->shade;
        pNSprite->xrepeat = 32;
        pNSprite->yrepeat = 32;
        const int nVoxel = voxelIndex[nTile];
        if (cl_showweapon == 2 && r_voxels && gDetail >= 4 && videoGetRenderMode() != REND_POLYMER && nVoxel != -1)
        {
            pNSprite->cstat |= 48;
            pNSprite->cstat &= ~8;
            pNSprite->picnum = nVoxel;
            pNSprite->z -= weaponIcon.zOffset<<8;
            const int lifeLeech = 9;
            if (pPlayer->curWeapon == lifeLeech)
            {
                pNSprite->x -=  mulscale30(128, Cos(pNSprite->ang));
                pNSprite->y -= mulscale30(128, Sin(pNSprite->ang));
            }
        }
        break;
    }
    }
    return NULL;
}

LOCATION gPrevSpriteLoc[kMaxSprites];

static void viewApplyDefaultPal(tspritetype *pTSprite, sectortype const *pSector)
{
    int const nXSector = pSector->extra;
    XSECTOR const *pXSector = nXSector >= 0 ? &xsector[nXSector] : NULL;
    if (pXSector && pXSector->color && (VanillaMode() || pSector->floorpal != 0))
    {
        pTSprite->pal = pSector->floorpal;
    }
}

void viewProcessSprites(int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smooth)
{
    UNREFERENCED_PARAMETER(smooth);
    dassert(spritesortcnt <= kMaxViewSprites);
    gCameraAng = cA;
    int nViewSprites = spritesortcnt;
    for (int nTSprite = spritesortcnt-1; nTSprite >= 0; nTSprite--)
    {
        tspritetype *pTSprite = &tsprite[nTSprite];
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
        if (cl_interpolate && TestBitString(gInterpolateSprite, nSprite) && !(pTSprite->flags&512))
        {
            LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
            pTSprite->x = interpolate(pPrevLoc->x, pTSprite->x, gInterpolate);
            pTSprite->y = interpolate(pPrevLoc->y, pTSprite->y, gInterpolate);
            pTSprite->z = interpolate(pPrevLoc->z, pTSprite->z, gInterpolate);
            pTSprite->ang = pPrevLoc->ang+mulscale16(((pTSprite->ang-pPrevLoc->ang+1024)&2047)-1024, gInterpolate);
        }
        int nAnim = 0;
        switch (picanm[nTile].extra & 7) {
            case 0:
                //dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
                if (nXSprite <= 0 || nXSprite >= kMaxXSprites) break;
                switch (pTSprite->type) {
                    case kSwitchToggle:
                    case kSwitchOneWay:
                        if (xsprite[nXSprite].state) nAnim = 1;
                        break;
                    case kSwitchCombo:
                        nAnim = xsprite[nXSprite].data1;
                        break;
                }
                break;
            case 1:
            {
                if (tilehasmodelorvoxel(pTSprite->picnum, pTSprite->pal) && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    pTSprite->cstat &= ~4;
                    break;
                }
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
                if (tilehasmodelorvoxel(pTSprite->picnum, pTSprite->pal) && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    pTSprite->cstat &= ~4;
                    break;
                }
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
                    GetSpriteExtents(pTSprite, &top, &bottom);
                    if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) > bottom)
                        nAnim = 1;
                }
                break;
            }
            case 6:
            case 7:
            {
#ifdef USE_OPENGL
                if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                    break;
#endif
                // Can be overridden by def script
                if (r_voxels && gDetail >= 4 && videoGetRenderMode() != REND_POLYMER && tiletovox[pTSprite->picnum] == -1 && voxelIndex[pTSprite->picnum] != -1 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    if ((pTSprite->flags&kHitagRespawn) == 0)
                    {
                        pTSprite->cstat |= 48;
                        pTSprite->cstat &= ~(4|8);
                        pTSprite->yoffset += tileTopOffset(pTSprite->picnum);
                        pTSprite->picnum = voxelIndex[pTSprite->picnum];
                        if ((picanm[nTile].extra&7) == 7)
                        {
                            pTSprite->ang = ((int)totalclock<<3)&2047;
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

        if ((pTSprite->cstat&48) != 48 && r_voxels && videoGetRenderMode() != REND_POLYMER && !(spriteext[nSprite].flags&SPREXT_NOTMD))
        {
            int const nRootTile = pTSprite->picnum;
            int nAnimTile = pTSprite->picnum + animateoffs_replace(pTSprite->picnum, 32768+pTSprite->owner);

#if 0
            if (tiletovox[nAnimTile] != -1)
            {
                pTSprite->yoffset += tileTopOffset(nAnimTile);
                pTSprite->xoffset += tileLeftOffset(nAnimTile);
            }
#endif

            int const nVoxel = tiletovox[pTSprite->picnum];

            if (nVoxel != -1 && ((voxrotate[nVoxel>>3]&pow2char[nVoxel&7]) != 0 || (picanm[nRootTile].extra&7) == 7))
                pTSprite->ang = (pTSprite->ang+((int)totalclock<<3))&2047;
        }

        if ((pTSprite->cstat&48) != 48 && hw_models && !(spriteext[nSprite].flags&SPREXT_NOTMD))
        {
            int const nRootTile = pTSprite->picnum;
            int nAnimTile = pTSprite->picnum + animateoffs_replace(pTSprite->picnum, 32768+pTSprite->owner);

            if (tile2model[Ptile2tile(nAnimTile, pTSprite->pal)].modelid >= 0 &&
                tile2model[Ptile2tile(nAnimTile, pTSprite->pal)].framenum >= 0)
            {
                pTSprite->yoffset += tileTopOffset(nAnimTile);
                pTSprite->xoffset += tileLeftOffset(nAnimTile);

                if ((picanm[nRootTile].extra&7) == 7)
                    pTSprite->ang = (pTSprite->ang+((int)totalclock<<3))&2047;
            }
        }

        sectortype *pSector = &sector[pTSprite->sectnum];
        XSECTOR *pXSector;
        int nShade = pTSprite->shade;
        if (pSector->extra > 0)
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
        if ((pTSprite->flags&kHitagRespawn) && sprite[pTSprite->owner].owner == 3)
        {
            dassert(pTXSprite != NULL);
            pTSprite->xrepeat = 48;
            pTSprite->yrepeat = 48;
            pTSprite->shade = -128;
            pTSprite->picnum = 2272 + 2*pTXSprite->respawnPending;
            pTSprite->cstat &= ~514;
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
        if (spritesortcnt >= kMaxViewSprites) continue;
        if (pTXSprite && pTXSprite->burnTime > 0)
        {
            pTSprite->shade = ClipRange(pTSprite->shade-16-QRandom(8), -128, 127);
        }
        if (pTSprite->flags&256)
        {
            viewAddEffect(nTSprite, VIEW_EFFECT_6);
        }
        if (pTSprite->flags&1024)
        {
            pTSprite->cstat |= 4;
        }
        if (pTSprite->flags&2048)
        {
            pTSprite->cstat |= 8;
        }
        switch (pTSprite->statnum) {
        case kStatDecoration: {
            switch (pTSprite->type) {
                case kDecorationCandle:
                    if (!pTXSprite || pTXSprite->state == 1) {
                        pTSprite->shade = -128;
                        viewAddEffect(nTSprite, VIEW_EFFECT_11);
                    } else {
                        pTSprite->shade = -8;
                    }
                    break;
                case kDecorationTorch:
                    if (!pTXSprite || pTXSprite->state == 1) {
                        pTSprite->picnum++;
                        viewAddEffect(nTSprite, VIEW_EFFECT_4);
                    } else {
                        viewAddEffect(nTSprite, VIEW_EFFECT_6);
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
                    if (pTXSprite && pTXSprite->state > 0 && gGameOptions.nGameType == 3) {
                        auto pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                        if (pNTSprite) pNTSprite->pal = 10;
                    }
                    break;
                case kItemFlagBBase:
                    if (pTXSprite && pTXSprite->state > 0 && gGameOptions.nGameType == 3) {
                        auto pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_17);
                        if (pNTSprite) pNTSprite->pal = 7;
                    }
                    break;
                case kItemFlagA:
                    pTSprite->pal = 10;
                    pTSprite->cstat |= 1024;
                    break;
                case kItemFlagB:
                    pTSprite->pal = 7;
                    pTSprite->cstat |= 1024;
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
                    pTSprite->cstat |= 32;
                    break;
                case kMissileTeslaRegular:
                    viewAddEffect(nTSprite, VIEW_EFFECT_15);
                    break;
                case kMissileButcherKnife:
                    viewAddEffect(nTSprite, VIEW_EFFECT_10);
                    break;
                case kMissileFlareRegular:
                case kMissileFlareAlt:
                    if (pTSprite->statnum == kStatFlare) {
                        dassert(pTXSprite != NULL);
                        if (pTXSprite->target == gView->nSprite) {
                            pTSprite->xrepeat = 0;
                            break;
                        }
                    }
                    
                    viewAddEffect(nTSprite, VIEW_EFFECT_1);
                    if (pTSprite->type != kMissileFlareRegular) break;
                    sectortype *pSector = &sector[pTSprite->sectnum];
                    
                    int zDiff = (pTSprite->z - pSector->ceilingz) >> 8;
                    if ((pSector->ceilingstat&1) == 0 && zDiff < 64) {
                        viewAddEffect(nTSprite, VIEW_EFFECT_2);
                    }
                    
                    zDiff = (pSector->floorz - pTSprite->z) >> 8;
                    if ((pSector->floorstat&1) == 0 && zDiff < 64) {
                        viewAddEffect(nTSprite, VIEW_EFFECT_3);
                    }
                    break;
                }
            break;
        }
        case kStatDude:
        {
            if (pTSprite->type == kDudeHand && pTXSprite->aiState == &hand13A3B4)
            {
                spritetype *pTTarget = &sprite[pTXSprite->target];
                dassert(pTXSprite != NULL && pTTarget != NULL);
                if (IsPlayerSprite(pTTarget))
                {
                    pTSprite->xrepeat = 0;
                    break;
                }
            }
            
            if (pXSector && pXSector->color) pTSprite->pal = pSector->floorpal;
            if (powerupCheck(gView, kPwUpBeastVision) > 0) pTSprite->shade = -128;

            if (IsPlayerSprite(pTSprite)) {
                PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
                if (powerupCheck(pPlayer, kPwUpShadowCloak) && !powerupCheck(gView, kPwUpBeastVision)) {
                    pTSprite->cstat |= 2;
                    pTSprite->pal = 5;
                }  else if (powerupCheck(pPlayer, kPwUpDeathMask)) {
                    pTSprite->shade = -128;
                    pTSprite->pal = 5;
                } else if (powerupCheck(pPlayer, kPwUpDoppleganger)) {
                    pTSprite->pal = 11+(gView->teamId&3);
                }
                
                if (powerupCheck(pPlayer, kPwUpReflectShots)) {
                    viewAddEffect(nTSprite, VIEW_EFFECT_13);
                }
                
                if (cl_showweapon && gGameOptions.nGameType > 0 && gView) {
                    viewAddEffect(nTSprite, VIEW_EFFECT_12);
                }
                
                if (pPlayer->flashEffect && (gView != pPlayer || gViewPos != VIEWPOS_0)) {
                    auto pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_14);
                    if (pNTSprite) {
                        POSTURE *pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
                        pNTSprite->x += mulscale28(pPosture->zOffset, Cos(pTSprite->ang));
                        pNTSprite->y += mulscale28(pPosture->zOffset, Sin(pTSprite->ang));
                        pNTSprite->z = pPlayer->pSprite->z-pPosture->xOffset;
                    }
                }
                
                if (pPlayer->hasFlag > 0 && gGameOptions.nGameType == 3) {
                    if (pPlayer->hasFlag&1)  {
                        auto pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                        if (pNTSprite)
                        {
                            pNTSprite->pal = 10;
                            pNTSprite->cstat |= 4;
                        }
                    }
                    if (pPlayer->hasFlag&2) {
                        auto pNTSprite = viewAddEffect(nTSprite, VIEW_EFFECT_16);
                        if (pNTSprite)
                        {
                            pNTSprite->pal = 7;
                            pNTSprite->cstat |= 4;
                        }
                    }
                }
            }
            
            if (pTSprite->owner != gView->pSprite->index || gViewPos != VIEWPOS_0) {
                if (getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                {
                    viewAddEffect(nTSprite, VIEW_EFFECT_0);
                }
            }
            break;
        }
        case kStatTraps: {
            if (pTSprite->type == kTrapSawCircular) {
                if (pTXSprite->state) {
                    if (pTXSprite->data1) {
                        pTSprite->picnum = 772;
                        if (pTXSprite->data2)
                            viewAddEffect(nTSprite, VIEW_EFFECT_9);
                    }
                } 
                else if (pTXSprite->data1) pTSprite->picnum = 773;
                else pTSprite->picnum = 656;
                
            }
            break;
        }
        case kStatThing: {
            viewApplyDefaultPal(pTSprite, pSector);

            if (pTSprite->type < kThingBase || pTSprite->type >= kThingMax || !gSpriteHit[nXSprite].florhit) {
                if ((pTSprite->flags & kPhysMove) && getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= cZ)
                    viewAddEffect(nTSprite, VIEW_EFFECT_0);
            }
        }
        break;
        }
    }

    for (int nTSprite = spritesortcnt-1; nTSprite >= nViewSprites; nTSprite--)
    {
        tspritetype *pTSprite = &tsprite[nTSprite];
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
int othercameraclock, cameraclock;

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
    othercameradist = ClipHigh(othercameradist+(((int)(totalclock-othercameraclock))<<10), 65536);
    othercameraclock = (int)totalclock;
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
    cameradist = ClipHigh(cameradist+(((int)(totalclock-cameraclock))<<10), 65536);
    cameraclock = (int)totalclock;
    dassert(*vsectnum >= 0 && *vsectnum < kMaxSectors);
    FindSector(*pX, *pY, *pZ, vsectnum);
    pSprite->cstat = bakCstat;
}

// by NoOne: show warning msgs in game instead of throwing errors (in some cases)
void viewSetSystemMessage(const char* pMessage, ...) {
    char buffer[1024]; va_list args; va_start(args, pMessage);
    vsprintf(buffer, pMessage, args);
    
    Printf(PRINT_HIGH | PRINT_NOTIFY, "%s\n", buffer); // print it also in console
}

void viewSetMessage(const char *pMessage, const int pal, const MESSAGE_PRIORITY priority)
{
	int printlevel = priority < 0 ? PRINT_LOW : priority < MESSAGE_PRIORITY_SYSTEM ? PRINT_MEDIUM : PRINT_HIGH;
    Printf(printlevel|PRINT_NOTIFY, "%s\n", pMessage);
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
	// To investigate whether this can be implemented as a shader effect.
    auto d = tileData(4077);
    dassert(d != NULL);
	auto s = tilePtr(4079);
    dassert(s != NULL);
    for (int i = 0; i < kLensSize*kLensSize; i++, d++)
        if (lensTable[i] >= 0)
            *d = s[lensTable[i]];
    tileInvalidate(4077, -1, -1);
}

void UpdateDacs(int nPalette, bool bNoTint)
{
    gLastPal = 0;
    auto& tint = lookups.tables[MAXPALOOKUPS - 1];
    tint.tintFlags = 0;
    switch (nPalette)
    {
    case 0:
    default:
        tint.tintColor.r = 255;
        tint.tintColor.g = 255;
        tint.tintColor.b = 255;
        break;
    case 1:
        tint.tintColor.r = 132;
        tint.tintColor.g = 164;
        tint.tintColor.b = 255;
        break;
    case 2:
        tint.tintColor.r = 255;
        tint.tintColor.g = 126;
        tint.tintColor.b = 105;
        break;
    case 3:
        tint.tintColor.r = 162;
        tint.tintColor.g = 186;
        tint.tintColor.b = 15;
        break;
    case 4:
        tint.tintColor.r = 255;
        tint.tintColor.g = 255;
        tint.tintColor.b = 255;
        break;
    }
    videoSetPalette(nPalette);
}

void UpdateBlend()
{
    int nRed = 0;
    int nGreen = 0;
    int nBlue = 0;

    nRed += gView->pickupEffect;
    nGreen += gView->pickupEffect;
    nBlue -= gView->pickupEffect;

    nRed += ClipHigh(gView->painEffect, 85) * 2;
    nGreen -= ClipHigh(gView->painEffect, 85) * 3;
    nBlue -= ClipHigh(gView->painEffect, 85) * 3;

    nRed -= gView->blindEffect;
    nGreen -= gView->blindEffect;
    nBlue -= gView->blindEffect;

    nRed -= gView->chokeEffect >> 6;
    nGreen -= gView->chokeEffect >> 5;
    nBlue -= gView->chokeEffect >> 6;

    nRed = ClipRange(nRed, -255, 255);
    nGreen = ClipRange(nGreen, -255, 255);
    nBlue = ClipRange(nBlue, -255, 255);

    videoTintBlood(nRed, nGreen, nBlue);
}

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
	if ((powerCount = powerupCheck(gView, kPwUpDeliriumShroom)) != 0)
	{
		int tilt1 = 170, tilt2 = 170, pitch = 20;
        int timer = (int)gFrameClock*4;
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
    if (gView->flickerEffect)
    {
        int nValue = ClipHigh(gView->flickerEffect * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
    if (gView->quakeEffect)
    {
        int nValue = ClipHigh(gView->quakeEffect * 8, 2000);
        shakeHoriz += QRandom2(nValue >> 8);
        shakeAngle += QRandom2(nValue >> 8);
        shakeX += QRandom2(nValue >> 4);
        shakeY += QRandom2(nValue >> 4);
        shakeZ += QRandom2(nValue);
        shakeBobX += QRandom2(nValue);
        shakeBobY += QRandom2(nValue);
    }
}


int gLastPal = 0;

int32_t g_frameRate;

void viewDrawScreen(bool sceneonly)
{
    int nPalette = 0;
    static ClockTicks lastUpdate;
    int defaultHoriz = r_horizcenter ? 100 : 90;

#ifdef USE_OPENGL
    polymostcenterhoriz = defaultHoriz;
#endif
    ClockTicks delta = totalclock - lastUpdate;
    if (delta < 0)
        delta = 0;
    lastUpdate = totalclock;
    if (!paused && (!M_Active() || gGameOptions.nGameType != 0))
    {
        gInterpolate = CalcSmoothRatio(totalclock, gNetFifoClock - 4, 30);
    }
    else gInterpolate = 65536;

    if (cl_interpolate)
    {
        CalcInterpolations();
    }

    if (gViewMode == 3 || gViewMode == 4 || gOverlayMap)
    {
        DoSectorLighting();
    }
    if (gViewMode == 3 || gOverlayMap)
    {
        int basepal = 0;
        if (powerupCheck(gView, kPwUpDeathMask) > 0) basepal = 4;
        else if (powerupCheck(gView, kPwUpReflectShots) > 0) basepal = 1;
        else if (gView->isUnderwater) {
            if (gView->nWaterPal) basepal = gView->nWaterPal;
            else {
                if (gView->pXSprite->medium == kMediumWater) basepal = 1;
                else if (gView->pXSprite->medium == kMediumGoo) basepal = 3;
                else basepal = 2;
            }
        }
        UpdateDacs(basepal);
        UpdateBlend();

        int yxAspect = yxaspect;
        int viewingRange = viewingrange;
        if (r_usenewaspect)
        {
            newaspect_enable = 1;
            videoSetCorrectedAspect();
        }

        int v1 = Blrintf(double(viewingrange) * tan(r_fov * (PI / 360.)));

        renderSetAspect(v1, yxaspect);
        int cX = gView->pSprite->x;
        int cY = gView->pSprite->y;
        int cZ = gView->zView;
        int zDelta = gView->zWeapon - gView->zView - (12 << 8);
        fix16_t cA = gView->q16ang;
        fix16_t q16horiz = gView->q16horiz;
        fix16_t q16slopehoriz = gView->q16slopehoriz;
        int v74 = gView->bobWidth;
        int v8c = gView->bobHeight;
        int v4c = gView->swayWidth;
        int v48 = gView->swayHeight;
        int nSectnum = gView->pSprite->sectnum;
        if (cl_interpolate)
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
                VIEW* pView = &gPrevView[gViewIndex];
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
        if (gView == gMe && (numplayers <= 1 || gPrediction) && gView->pXSprite->health != 0 && !VanillaMode())
        {
            int upAngle = 289;
            int downAngle = -347;
            fix16_t q16look;
            cA = gViewAngle;
            q16look = gViewLook;
            q16horiz = fix16_from_float(100.f * tanf(fix16_to_float(q16look) * fPI / 1024.f));
        }
        viewUpdateShake();
        q16horiz += fix16_from_int(shakeHoriz);
        cA += fix16_from_int(shakeAngle);
        cX += shakeX;
        cY += shakeY;
        cZ += shakeZ;
        v4c += shakeBobX;
        v48 += shakeBobY;
        q16horiz += fix16_from_int(mulscale30(0x40000000 - Cos(gView->tiltEffect << 2), 30));
        if (gViewPos == 0)
        {
            if (cl_viewhbob)
            {
                cX -= mulscale30(v74, Sin(fix16_to_int(cA))) >> 4;
                cY += mulscale30(v74, Cos(fix16_to_int(cA))) >> 4;
            }
            if (cl_viewvbob)
            {
                cZ += v8c;
            }
            if (cl_slopetilting)
            {
                q16horiz += q16slopehoriz;
            }
            cZ += fix16_to_int(q16horiz * 10);
            cameradist = -1;
            cameraclock = (int)totalclock;
        }
        else
        {
            CalcPosition(gView->pSprite, (int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum, fix16_to_int(cA), q16horiz);
        }
        CheckLink((int*)&cX, (int*)&cY, (int*)&cZ, &nSectnum);
        int v78 = interpolateang(gScreenTiltO, gScreenTilt, gInterpolate);
        uint8_t v14 = 0;
        uint8_t v10 = 0;
        bool bDelirium = powerupCheck(gView, kPwUpDeliriumShroom) > 0;
        static bool bDeliriumOld = false;
        //int tiltcs, tiltdim;
        uint8_t v4 = powerupCheck(gView, kPwUpCrystalBall) > 0;
#ifdef USE_OPENGL
        renderSetRollAngle(0);
#endif
        if (v78 || bDelirium)
        {
            renderSetRollAngle(v78);
        }
        else if (v4 && gNetPlayers > 1)
        {
#if 0       // needs to be redone for pure hardware rendering.
            int tmp = ((int)totalclock / 240) % (gNetPlayers - 1);
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
            PLAYER* pOther = &gPlayer[i];
            //othercameraclock = gGameClock;
            if (!tileData(4079))
            {
                tileAllocTile(4079, 128, 128);
            }
            r enderSetTarget(4079, 128, 128);
            renderSetAspect(65536, 78643);
            int vd8 = pOther->pSprite->x;
            int vd4 = pOther->pSprite->y;
            int vd0 = pOther->zView;
            int vcc = pOther->pSprite->sectnum;
            int v50 = pOther->pSprite->ang;
            int v54 = 0;
            if (pOther->flickerEffect)
            {
                int nValue = ClipHigh(pOther->flickerEffect * 8, 2000);
                v54 += QRandom2(nValue >> 8);
                v50 += QRandom2(nValue >> 8);
                vd8 += QRandom2(nValue >> 4);
                vd4 += QRandom2(nValue >> 4);
                vd0 += QRandom2(nValue);
            }
            if (pOther->quakeEffect)
            {
                int nValue = ClipHigh(pOther->quakeEffect * 8, 2000);
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
            memcpy(bakMirrorGotpic, gotpic + 510, 2);
            memcpy(gotpic + 510, otherMirrorGotpic, 2);
            g_visibility = (int32_t)(ClipLow(gVisibility - 32 * pOther->visibility, 0));
            int vc4, vc8;
            getzsofslope(vcc, vd8, vd4, &vc8, &vc4);
            if (vd0 >= vc4)
            {
                vd0 = vc4 - (gUpperLink[vcc] >= 0 ? 0 : (8 << 8));
            }
            if (vd0 <= vc8)
            {
                vd0 = vc8 + (gLowerLink[vcc] >= 0 ? 0 : (8 << 8));
            }
            v54 = ClipRange(v54, -200, 200);
        RORHACKOTHER:
            int ror_status[16];
            for (int i = 0; i < 16; i++)
                ror_status[i] = TestBitString(gotpic, 4080 + i);
            yax_preparedrawrooms();
            DrawMirrors(vd8, vd4, vd0, fix16_from_int(v50), fix16_from_int(v54 + defaultHoriz), gInterpolate, -1);
            drawrooms(vd8, vd4, vd0, v50, v54 + defaultHoriz, vcc);
            yax_drawrooms(viewProcessSprites, vcc, 0, gInterpolate);
            bool do_ror_hack = false;
            for (int i = 0; i < 16; i++)
                if (ror_status[i] != TestBitString(gotpic, 4080 + i))
                    do_ror_hack = true;
            if (do_ror_hack)
            {
                spritesortcnt = 0;
                goto RORHACKOTHER;
            }
            memcpy(otherMirrorGotpic, gotpic+510, 2);
            memcpy(gotpic+510, bakMirrorGotpic, 2);
            viewProcessSprites(vd8, vd4, vd0, v50, gInterpolate);
            renderDrawMasks();
            renderRestoreTarget();
#endif
        }
        else
        {
            othercameraclock = (int)totalclock;
        }

        if (!bDelirium)
        {
            deliriumTilt = 0;
            deliriumTurn = 0;
            deliriumPitch = 0;
        }
        int nSprite = headspritestat[kStatExplosion];
        int unk = 0;
        while (nSprite >= 0)
        {
            spritetype* pSprite = &sprite[nSprite];
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE* pXSprite = &xsprite[nXSprite];
            if (TestBitString(gotsector, pSprite->sectnum))
            {
                unk += pXSprite->data3 * 32;
            }
            nSprite = nextspritestat[nSprite];
        }
        nSprite = headspritestat[kStatProjectile];
        while (nSprite >= 0) {
            spritetype* pSprite = &sprite[nSprite];
            switch (pSprite->type) {
            case kMissileFlareRegular:
            case kMissileTeslaAlt:
            case kMissileFlareAlt:
            case kMissileTeslaRegular:
                if (TestBitString(gotsector, pSprite->sectnum)) unk += 256;
                break;
            }
            nSprite = nextspritestat[nSprite];
        }
        g_visibility = (int32_t)(ClipLow(gVisibility - 32 * gView->visibility - unk, 0));
        cA = (cA + interpolateangfix16(fix16_from_int(deliriumTurnO), fix16_from_int(deliriumTurn), gInterpolate)) & 0x7ffffff;
        int vfc, vf8;
        getzsofslope(nSectnum, cX, cY, &vfc, &vf8);
        if (cZ >= vf8)
        {
            cZ = vf8 - (gUpperLink[nSectnum] >= 0 ? 0 : (8 << 8));
        }
        if (cZ <= vfc)
        {
            cZ = vfc + (gLowerLink[nSectnum] >= 0 ? 0 : (8 << 8));
        }
        q16horiz = ClipRange(q16horiz, fix16_from_int(-200), fix16_from_int(200));
    RORHACK:
        int ror_status[16];
        for (int i = 0; i < 16; i++)
            ror_status[i] = TestBitString(gotpic, 4080 + i);
        fix16_t deliriumPitchI = interpolate(fix16_from_int(deliriumPitchO), fix16_from_int(deliriumPitch), gInterpolate);
        DrawMirrors(cX, cY, cZ, cA, q16horiz + fix16_from_int(defaultHoriz) + deliriumPitchI, gInterpolate, gViewIndex);
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
        viewProcessSprites(cX, cY, cZ, fix16_to_int(cA), gInterpolate);
        bool do_ror_hack = false;
        for (int i = 0; i < 16; i++)
            if (ror_status[i] != TestBitString(gotpic, 4080 + i))
                do_ror_hack = true;
        if (do_ror_hack)
        {
            gView->pSprite->cstat = bakCstat;
            spritesortcnt = 0;
            goto RORHACK;
        }
        sub_5571C(1);
        int nSpriteSortCnt = spritesortcnt;
        renderDrawMasks();
        spritesortcnt = nSpriteSortCnt;
        sub_5571C(0);
        sub_557C4(cX, cY, gInterpolate);
        renderDrawMasks();
        gView->pSprite->cstat = bakCstat;

        if ((v78 || bDelirium) && !sceneonly)
        {
            if (videoGetRenderMode() == REND_POLYMOST && gDeliriumBlur)
            {
                // todo: Implement using modern techniques instead of relying on deprecated old stuff that isn't well supported anymore.
                /* names broken up so that searching for GL keywords won't find them anymore
                if (!bDeliriumOld)
                {
                    g lAccum(GL_LOAD, 1.f);
                }
                else
                {
                    const float fBlur = pow(1.f/3.f, 30.f/g_frameRate);
                    g lAccum(GL _MULT, fBlur);
                    g lAccum(GL _ACCUM, 1.f-fBlur);
                    g lAccum(GL _RETURN, 1.f);
                }
                */
            }
        }

        bDeliriumOld = bDelirium && gDeliriumBlur;

        if (r_usenewaspect)
            newaspect_enable = 0;
        renderSetAspect(viewingRange, yxAspect);
        int nClipDist = gView->pSprite->clipdist << 2;
        int ve8, vec, vf0, vf4;
        GetZRange(gView->pSprite, &vf4, &vf0, &vec, &ve8, nClipDist, 0);
        if (sceneonly) return;
#if 0
        int tmpSect = nSectnum;
        if ((vf0 & 0xc000) == 0x4000)
        {
            tmpSect = vf0 & (kMaxWalls - 1);
        }
        int v8 = byte_1CE5C2 > 0 && (sector[tmpSect].ceilingstat & 1);
        if (gWeather.at12d8 > 0 || v8)
        {
            gWeather.Draw(cX, cY, cZ, cA, q16horiz + defaultHoriz + deliriumPitch, gWeather.at12d8);
            if (v8)
            {
                gWeather.at12d8 = ClipRange(delta * 8 + gWeather.at12d8, 0, 4095);
            }
            else
            {
                gWeather.at12d8 = ClipRange(gWeather.at12d8 - delta * 64, 0, 4095);
            }
        }
#endif
        hudDraw(gView, nSectnum, defaultHoriz, v4c, v48, zDelta, basepal);
    }
    UpdateDacs(0, true);    // keep the view palette active only for the actual 3D view and its overlays.
    if (gViewMode == 4)
    {
        gViewMap.sub_25DB0(gView->pSprite);
    }
    viewDrawInterface(delta);
    int zn = ((gView->zWeapon-gView->zView-(12<<8))>>7)+220;
    PLAYER *pPSprite = &gPlayer[gMe->pSprite->type-kDudePlayer1];
    if (IsPlayerSprite(gMe->pSprite) && pPSprite->hand == 1)
    {
        //static int lastClock;
        gChoke.sub_84110(160, zn, 0);
        //if ((gGameClock % 5) == 0 && gGameClock != lastClock)
        //{
        //    gChoke.swayV(pPSprite);
        //}
        //lastClock = gGameClock;
    }
#if 0
    if (byte_1A76C6)
    {
        DrawStatSprite(2048, xdim-15, 20);
    }
#endif
    CalcFrameRate();

    viewDrawMapTitle();
    viewDrawAimedPlayerName();
    if (paused)
    {
        viewDrawText(1, GStrings("TXTB_PAUSED"), 160, 10, 0, 0, 1, 0);
    }
    else if (gView != gMe)
    {
        FStringf gTempStr("] %s [", gProfile[gView->nPlayer].name);
        viewDrawText(0, gTempStr, 160, 10, 0, 0, 1, 0);
    }
    if (errMsg[0])
    {
        viewDrawText(0, errMsg, 160, 20, 0, 0, 1, 0);
    }
    if (cl_interpolate)
    {
        RestoreInterpolations();
    }
}

bool GameInterface::GenerateSavePic()
{
    viewDrawScreen(true);
    return true;
}


#define LOW_FPS 60
#define SLOW_FRAME_TIME 20

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

FString GameInterface::statFPS(void)
{
	FString output;
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

        //if (r_showfps)
        {
			output.AppendFormat("%.1f ms, %5.1f fps\n", frameDelay, lastFPS);

            if (r_showfps > 1)
            {
				output.AppendFormat("max: %5.1f fps\n", maxFPS);
				output.AppendFormat("min: %5.1f fps\n", minFPS);
            }
            if (r_showfps > 2)
            {
                if (g_gameUpdateTime > maxGameUpdate) maxGameUpdate = g_gameUpdateTime;
                if (g_gameUpdateTime < minGameUpdate) minGameUpdate = g_gameUpdateTime;

				output.AppendFormat("Game Update: %2.2f ms + draw: %2.2f ms\n", g_gameUpdateTime, g_gameUpdateAndDrawTime - g_gameUpdateTime);
				output.AppendFormat("GU min/max/avg: %5.2f/%5.2f/%5.2f ms\n", minGameUpdate, maxGameUpdate, g_gameUpdateAvgTime);

				output.AppendFormat("bufferjitter: %i\n", gBufferJitter);
#if 0
				output.AppendFormat("G_MoveActors(): %.3f ms\n", g_moveActorsTime);
				output.AppendFormat("G_MoveWorld(): %.3f ms\n", g_moveWorldTime);
#endif
            }
#if 0
            // lag meter
            if (g_netClientPeer)
            {
				output.AppendFormat("%d +- %d ms\n", (g_netClientPeer->lastRoundTripTime + g_netClientPeer->roundTripTime)/2,
                    (g_netClientPeer->lastRoundTripTimeVariance + g_netClientPeer->roundTripTimeVariance)/2);
            }
#endif
        }

        if (cumulativeFrameDelay >= 1000.0)
        {
            lastFPS = 1000.f * frameCount / cumulativeFrameDelay;
            g_frameRate = Blrintf(lastFPS);
            frameCount = 0;
            cumulativeFrameDelay = 0.0;

            if (r_showfps > 1)
            {
                if (lastFPS > maxFPS) maxFPS = lastFPS;
                if (lastFPS < minFPS) minFPS = lastFPS;

                static int secondCounter;

                if (++secondCounter >= r_showfpsperiod)
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
	return output;
}

FString GameInterface::GetCoordString()
{
    return "Player pos is unknown"; // todo: output at least something useful.
}


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

END_BLD_NS
