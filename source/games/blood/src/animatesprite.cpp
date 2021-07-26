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
#include "v_font.h"
#include "hw_voxels.h"
#include "gamefuncs.h"
#include "glbackend/glbackend.h"

BEGIN_BLD_NS

static fixed_t gCameraAng;
int dword_172CE0[16][3];

static void RotateYZ(int *, int *pY, int *pZ, int ang)
{
	int oY, oZ, angSin, angCos;
	oY = *pY;
	oZ = *pZ;
	angSin = Sin(ang);
	angCos = Cos(ang);
	*pY = dmulscale30r(oY,angCos,oZ,-angSin);
	*pZ = dmulscale30r(oY,angSin,oZ,angCos);
}

static void RotateXZ(int *pX, int *, int *pZ, int ang)
{
	int oX, oZ, angSin, angCos;
	oX = *pX;
	oZ = *pZ;
    angSin = Sin(ang);
    angCos = Cos(ang);
	*pX = dmulscale30r(oX,angCos,oZ,-angSin);
	*pZ = dmulscale30r(oX,angSin,oZ,angCos);
}

template<typename T> tspritetype* viewInsertTSprite(spritetype* tsprite, int& spritesortcnt, int nSector, int nStatnum, T const * const pSprite)
{
    if (spritesortcnt >= MAXSPRITESONSCREEN)
        return nullptr;

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
    pTSprite->x += Cos(gCameraAng)>>25;
    pTSprite->y += Sin(gCameraAng)>>25;
    return pTSprite;
}

static const int effectDetail[kViewEffectMax] = {
    4, 4, 4, 4, 0, 0, 0, 0, 0, 1, 4, 4, 0, 0, 0, 1, 0, 0, 0
};


struct WEAPONICON {
    short nTile;
    char zOffset;
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


static tspritetype *viewAddEffect(spritetype* tsprite, int& spritesortcnt, int nTSprite, VIEW_EFFECT nViewEffect)
{
    assert(nViewEffect >= 0 && nViewEffect < kViewEffectMax);
    auto pTSprite = &tsprite[nTSprite];
    if (gDetail < effectDetail[nViewEffect] || nTSprite >= MAXSPRITESONSCREEN) return NULL;
    switch (nViewEffect)
    {
    case kViewEffectAtom:
        for (int i = 0; i < 16; i++)
        {
            auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
            if (!pNSprite)
                break;

            int ang = (PlayClock*2048)/120;
            int nRand1 = dword_172CE0[i][0];
            int nRand2 = dword_172CE0[i][1];
            int nRand3 = dword_172CE0[i][2];
            ang += nRand3;
            int x = MulScale(512, Cos(ang), 30);
            int y = MulScale(512, Sin(ang), 30);
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
    case kViewEffectFlag:
    case kViewEffectBigFlag:
    {
        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->shade = -128;
        pNSprite->pal = 0;
        pNSprite->z = top;
        if (nViewEffect == kViewEffectFlag)
            pNSprite->xrepeat = pNSprite->yrepeat = 24;
        else
            pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 3558;
        return pNSprite;
    }
    case kViewEffectTesla:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->z = pTSprite->z;
        pNSprite->cstat |= 2;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        pNSprite->picnum = 2135;
        break;
    }
    case kViewEffectShoot:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
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
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->shade = 26;
        pNSprite->pal = 0;
        pNSprite->cstat |= 2;
        pNSprite->xrepeat = pNSprite->yrepeat = 64;
        pNSprite->picnum = 2089;
        break;
    }
    case kViewEffectPhase:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

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
    case kViewEffectTrail:
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
        for (int i = 0; i < 5 && spritesortcnt < MAXSPRITESONSCREEN; i++)
        {
            int nSector = pTSprite->sectnum;
            auto pNSprite = viewInsertTSprite<tspritetype>(tsprite, spritesortcnt, nSector, 32767, NULL);
            if (!pNSprite)
                break;

            int nLen = 128+(i<<7);
            int x = MulScale(nLen, Cos(nAng), 30);
            pNSprite->x = pTSprite->x + x;
            int y = MulScale(nLen, Sin(nAng), 30);
            pNSprite->y = pTSprite->y + y;
            pNSprite->z = pTSprite->z;
            assert(nSector >= 0 && nSector < kMaxSectors);
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
    case kViewEffectFlame:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->shade = -128;
        pNSprite->z = pTSprite->z;
        pNSprite->picnum = 908;
        pNSprite->statnum = kStatDecoration;
        pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum)*pTSprite->xrepeat)/64;
        break;
    }
    case kViewEffectSmokeHigh:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

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
    case kViewEffectSmokeLow:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

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
    case kViewEffectTorchHigh:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        pNSprite->z = top;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum)*pTSprite->xrepeat)/32;
        break;
    }
    case kViewEffectTorchLow:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        int top, bottom;
        GetSpriteExtents(pTSprite, &top, &bottom);
        pNSprite->z = bottom;
        pNSprite->picnum = 2101;
        pNSprite->shade = -128;
        pNSprite->xrepeat = pNSprite->yrepeat = (tileWidth(pTSprite->picnum)*pTSprite->xrepeat)/32;
        break;
    }
    case kViewEffectShadow:
    {
        if (r_shadows)
        {
            auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
            if (!pNSprite)
                break;

            pNSprite->z = getflorzofslope(pTSprite->sectnum, pNSprite->x, pNSprite->y);
            pNSprite->shade = 127;
            pNSprite->cstat |= 2;
            pNSprite->xrepeat = pTSprite->xrepeat;
            pNSprite->yrepeat = pTSprite->yrepeat >> 2;
            pNSprite->picnum = pTSprite->picnum;
            pNSprite->pal = 5;
            int height = tileHeight(pNSprite->picnum);
            int center = height / 2 + tileTopOffset(pNSprite->picnum);
            pNSprite->z -= (pNSprite->yrepeat << 2) * (height - center);
        }
        break;
    }
    case kViewEffectFlareHalo:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->shade = -128;
        pNSprite->pal = 2;
        pNSprite->cstat |= 2;
        pNSprite->z = pTSprite->z;
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = pTSprite->yrepeat;
        pNSprite->picnum = 2427;
        break;
    }
    case kViewEffectCeilGlow:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

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
    case kViewEffectFloorGlow:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

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
    case kViewEffectSpear:
    {
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->z = pTSprite->z;
        if (gDetail > 1)
            pNSprite->cstat |= 514;
        pNSprite->shade = ClipLow(pTSprite->shade-32, -128);
        pNSprite->xrepeat = pTSprite->xrepeat;
        pNSprite->yrepeat = 64;
        pNSprite->picnum = 775;
        break;
    }
    case kViewEffectShowWeapon:
    {
        assert(pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8);
        PLAYER *pPlayer = &gPlayer[pTSprite->type-kDudePlayer1];
        WEAPONICON weaponIcon = gWeaponIcon[pPlayer->curWeapon];
        const int nTile = weaponIcon.nTile;
        if (nTile < 0) break;
        auto pNSprite = viewInsertTSprite(tsprite, spritesortcnt, pTSprite->sectnum, 32767, pTSprite);
        if (!pNSprite)
            break;

        pNSprite->x = pTSprite->x;
        pNSprite->y = pTSprite->y;
        pNSprite->z = pTSprite->z-(32<<8);
        pNSprite->picnum = nTile;
        pNSprite->shade = pTSprite->shade;
        pNSprite->xrepeat = 32;
        pNSprite->yrepeat = 32;
        const int nVoxel = voxelIndex[nTile];
        if (cl_showweapon == 2 && r_voxels && nVoxel != -1)
        {
            pNSprite->cstat |= 48;
            pNSprite->cstat &= ~8;
            pNSprite->picnum = nVoxel;
            pNSprite->z -= weaponIcon.zOffset<<8;
            const int lifeLeech = 9;
            if (pPlayer->curWeapon == lifeLeech)
            {
                pNSprite->x -=  MulScale(128, Cos(pNSprite->ang), 30);
                pNSprite->y -= MulScale(128, Sin(pNSprite->ang), 30);
            }
        }
        break;
    }
    }
    return NULL;
}

static void viewApplyDefaultPal(tspritetype *pTSprite, sectortype const *pSector)
{
    int const nXSector = pSector->extra;
    XSECTOR const *pXSector = nXSector >= 0 ? &xsector[nXSector] : NULL;
    if (pXSector && pXSector->color && (VanillaMode() || pSector->floorpal != 0))
    {
        copyfloorpal(pTSprite, pSector);
    }
}

void viewProcessSprites(spritetype* tsprite, int& spritesortcnt, int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smoothratio)
{
	// shift before interpolating to increase precision.
	int myclock = (PlayClock<<3) + MulScale(4<<3, smoothratio, 16);
    assert(spritesortcnt <= MAXSPRITESONSCREEN);
    gCameraAng = cA;
    int nViewSprites = spritesortcnt;
    for (int nTSprite = spritesortcnt-1; nTSprite >= 0; nTSprite--)
    {
        tspritetype *pTSprite = &tsprite[nTSprite];
        //int nXSprite = pTSprite->extra;
        int nXSprite = sprite[pTSprite->owner].extra;
        XSPRITE *pTXSprite = NULL;
        if (sprite[pTSprite->owner].detail > gDetail)
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
        if (cl_interpolate && gInterpolateSprite[nSprite] && !(pTSprite->flags&512))
        {
            pTSprite->pos = pTSprite->interpolatedvec3(gInterpolate);
            pTSprite->ang = pTSprite->interpolatedang(gInterpolate);
        }
        int nAnim = 0;
        switch (picanm[nTile].extra & 7) {
            case 0:
                //assert(nXSprite > 0 && nXSprite < kMaxXSprites);
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
                if (hw_models && md_tilehasmodel(pTSprite->picnum, pTSprite->pal) >= 0 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                    break;

                // Can be overridden by def script
                if (r_voxels && tiletovox[pTSprite->picnum] == -1 && voxelIndex[pTSprite->picnum] != -1 && !(spriteext[nSprite].flags&SPREXT_NOTMD))
                {
                    if ((pTSprite->flags&kHitagRespawn) == 0)
                    {
                        pTSprite->cstat |= 48;
                        pTSprite->cstat &= ~(4|8);
                        pTSprite->yoffset += tileTopOffset(pTSprite->picnum);
                        pTSprite->picnum = voxelIndex[pTSprite->picnum];
                        if ((picanm[nTile].extra&7) == 7)
                        {
                            pTSprite->ang = myclock & 2047;
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

        if ((pTSprite->cstat&48) != 48 && r_voxels && !(spriteext[nSprite].flags&SPREXT_NOTMD))
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

            if (nVoxel != -1 && (picanm[nRootTile].extra & 7) == 7)
                pTSprite->cstat |= CSTAT_SPRITE_MDLROTATE; // per-sprite rotation setting.
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
                    pTSprite->cstat |= CSTAT_SPRITE_MDLROTATE; // per-sprite rotation setting.
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
            assert(pTXSprite != NULL);
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
        if (spritesortcnt >= MAXSPRITESONSCREEN) continue;
        if (pTXSprite && pTXSprite->burnTime > 0)
        {
            pTSprite->shade = ClipRange(pTSprite->shade-16-QRandom(8), -128, 127);
        }
        if (pTSprite->flags&256)
        {
            viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectSmokeHigh);
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
                        viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectPhase);
                    } else {
                        pTSprite->shade = -8;
                    }
                    break;
                case kDecorationTorch:
                    if (!pTXSprite || pTXSprite->state == 1) {
                        pTSprite->picnum++;
                        viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectTorchHigh);
                    } else {
                        viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectSmokeHigh);
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
                        auto pNTSprite = viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectBigFlag);
                        if (pNTSprite) pNTSprite->pal = 10;
                    }
                    break;
                case kItemFlagBBase:
                    if (pTXSprite && pTXSprite->state > 0 && gGameOptions.nGameType == 3) {
                        auto pNTSprite = viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectBigFlag);
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
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectTesla);
                    break;
                case kMissileButcherKnife:
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectTrail);
                    break;
                case kMissileFlareRegular:
                case kMissileFlareAlt:
                    if (pTSprite->statnum == kStatFlare) {
                        assert(pTXSprite != NULL);
                        if (pTXSprite->target == gView->nSprite) {
                            pTSprite->xrepeat = 0;
                            break;
                        }
                    }
                    
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectFlareHalo);
                    if (pTSprite->type != kMissileFlareRegular) break;
                    sectortype *pSector = &sector[pTSprite->sectnum];
                    
                    int zDiff = (pTSprite->z - pSector->ceilingz) >> 8;
                    if ((pSector->ceilingstat&1) == 0 && zDiff < 64) {
                        viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectCeilGlow);
                    }
                    
                    zDiff = (pSector->floorz - pTSprite->z) >> 8;
                    if ((pSector->floorstat&1) == 0 && zDiff < 64) {
                        viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectFloorGlow);
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
                assert(pTXSprite != NULL && pTTarget != NULL);
                if (IsPlayerSprite(pTTarget))
                {
                    pTSprite->xrepeat = 0;
                    break;
                }
            }
            
            if (pXSector && pXSector->color) copyfloorpal(pTSprite, pSector);
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
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectReflectiveBall);
                }
                
                if (cl_showweapon && gGameOptions.nGameType > 0 && gView) {
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectShowWeapon);
                }
                
                if (pPlayer->flashEffect && (gView != pPlayer || gViewPos != VIEWPOS_0)) {
                    auto pNTSprite = viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectShoot);
                    if (pNTSprite) {
                        POSTURE *pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
                        pNTSprite->x += MulScale(pPosture->zOffset, Cos(pTSprite->ang), 28);
                        pNTSprite->y += MulScale(pPosture->zOffset, Sin(pTSprite->ang), 28);
                        pNTSprite->z = pPlayer->pSprite->z-pPosture->xOffset;
                    }
                }
                
                if (pPlayer->hasFlag > 0 && gGameOptions.nGameType == 3) {
                    if (pPlayer->hasFlag&1)  {
                        auto pNTSprite = viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectFlag);
                        if (pNTSprite)
                        {
                            pNTSprite->pal = 10;
                            pNTSprite->cstat |= 4;
                        }
                    }
                    if (pPlayer->hasFlag&2) {
                        auto pNTSprite = viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectFlag);
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
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectShadow);
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
                            viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectSpear);
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
                    viewAddEffect(tsprite, spritesortcnt, nTSprite, kViewEffectShadow);
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


void GameInterface::processSprites(spritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio)
{
    viewProcessSprites(tsprite, spritesortcnt, viewx, viewy, viewz, viewang.asbuild(), int(smoothRatio));
}

int display_mirror;

void GameInterface::EnterPortal(spritetype* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR)
    {
        display_mirror++;
        if (viewer) viewer->cstat &= ~CSTAT_SPRITE_INVISIBLE;
    }
}

void GameInterface::LeavePortal(spritetype* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR)
    {
        display_mirror--;
        if (viewer && display_mirror == 0 && !(viewer->cstat & CSTAT_SPRITE_TRANSLUCENT)) viewer->cstat |= CSTAT_SPRITE_INVISIBLE;
    }
}

END_BLD_NS
