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
#include "build.h"
#include "common_game.h"
#include "dude.h"
#include "seq.h"
#include "view.h"
#include "fx.h"
#include "gib.h"
#include "g_input.h"

BEGIN_BLD_NS

#define gotpic blafasl

int nPrecacheCount;

void fxPrecache(HitList &hits);
void gibPrecache(HitList &hits);


void tilePreloadTile(int nTile)
{
    if (!r_precache) return;
    int n = 1;
    switch (picanm[nTile].extra & 7)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    case 6:
    case 7:
        if (voxelIndex[nTile] < 0 || voxelIndex[nTile] >= kMaxVoxels)
        {
            voxelIndex[nTile] = -1;
            picanm[nTile].extra &= ~7;
        }
        break;
    }

    while (n--)
    {
        if (picanm[nTile].sf & PICANM_ANIMTYPE_MASK)
        {
            for (int frame = picanm[nTile].num; frame >= 0; frame--)
            {
                if ((picanm[nTile].sf & PICANM_ANIMTYPE_MASK) == PICANM_ANIMTYPE_BACK)
                    PrecacheHardwareTextures(nTile - frame);
                else
                    PrecacheHardwareTextures(nTile + frame);
            }
        }
        else
            PrecacheHardwareTextures(nTile);
        nTile += 1 + picanm[nTile].num;
    }
}

void tilePrecacheTile(int nTile, int nType, HitList &hits)
{
    int n = 1;
    switch (picanm[nTile].extra & 7)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    }
    while (n--)
    {
        if (picanm[nTile].sf & PICANM_ANIMTYPE_MASK)
        {
            for (int frame = picanm[nTile].num; frame >= 0; frame--)
            {
                int tile;
                if ((picanm[nTile].sf & PICANM_ANIMTYPE_MASK) == PICANM_ANIMTYPE_BACK)
                    tile = nTile - frame;
                else
                    tile = nTile + frame;
                if (!hits[tile])
                {
                    nPrecacheCount++;
                    hits.Set(tile);
                }
            }
        }
        else
        {
            if (!hits[nTile])
            {
                nPrecacheCount++;
                hits.Set(nTile);
            }
        }
        nTile += 1 + picanm[nTile].num;
    }
}


// To do: This needs to handle the sprite palettes as well to properly precache the needed content.

void viewPrecacheTiles(HitList &hits)
{
    tilePrecacheTile(2173, 0, hits);
    tilePrecacheTile(2200, 0, hits);
    tilePrecacheTile(2201, 0, hits);
    tilePrecacheTile(2202, 0, hits);
    tilePrecacheTile(2207, 0, hits);
    tilePrecacheTile(2208, 0, hits);
    tilePrecacheTile(2209, 0, hits);
    tilePrecacheTile(2229, 0, hits);
    tilePrecacheTile(2260, 0, hits);
    tilePrecacheTile(2559, 0, hits);
    tilePrecacheTile(2169, 0, hits);
    tilePrecacheTile(2578, 0, hits);
    tilePrecacheTile(2586, 0, hits);
    tilePrecacheTile(2602, 0, hits);
    for (int i = 0; i < 10; i++)
    {
        tilePrecacheTile(2190 + i, 0, hits);
        tilePrecacheTile(2230 + i, 0, hits);
        tilePrecacheTile(2240 + i, 0, hits);
        tilePrecacheTile(2250 + i, 0, hits);
        tilePrecacheTile(kSBarNumberHealth + i, 0, hits);
        tilePrecacheTile(kSBarNumberAmmo + i, 0, hits);
        tilePrecacheTile(kSBarNumberInv + i, 0, hits);
        tilePrecacheTile(kSBarNumberArmor1 + i, 0, hits);
        tilePrecacheTile(kSBarNumberArmor2 + i, 0, hits);
        tilePrecacheTile(kSBarNumberArmor3 + i, 0, hits);
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
        tilePrecacheTile(2220 + i, 0, hits);
        tilePrecacheTile(2552 + i, 0, hits);
    }
}



void PrecacheDude(spritetype *pSprite, HitList &hits)
{
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    seqPrecacheId(pDudeInfo->seqStartID  , hits);
    seqPrecacheId(pDudeInfo->seqStartID+5, hits);
    seqPrecacheId(pDudeInfo->seqStartID+1, hits);
    seqPrecacheId(pDudeInfo->seqStartID+2, hits);
    switch (pSprite->type)
    {
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
        seqPrecacheId(pDudeInfo->seqStartID+6 , hits);
        seqPrecacheId(pDudeInfo->seqStartID+7 , hits);
        seqPrecacheId(pDudeInfo->seqStartID+8 , hits);
        seqPrecacheId(pDudeInfo->seqStartID+9 , hits);
        seqPrecacheId(pDudeInfo->seqStartID+13, hits);
        seqPrecacheId(pDudeInfo->seqStartID+14, hits);
        seqPrecacheId(pDudeInfo->seqStartID+15, hits);
        break;
    case kDudeZombieButcher:
    case kDudeGillBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        seqPrecacheId(pDudeInfo->seqStartID+8, hits);
        seqPrecacheId(pDudeInfo->seqStartID+9, hits);
        seqPrecacheId(pDudeInfo->seqStartID+10, hits);
        seqPrecacheId(pDudeInfo->seqStartID+11, hits);
        break;
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+6, hits); //???
        fallthrough__;
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        seqPrecacheId(pDudeInfo->seqStartID+8, hits);
        seqPrecacheId(pDudeInfo->seqStartID+9, hits);
        break;
    case kDudePhantasm:
    case kDudeHellHound:
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
    case kDudeSpiderMother:
    case kDudeTchernobog:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        seqPrecacheId(pDudeInfo->seqStartID+8, hits);
        break;
    case kDudeCerberusTwoHead:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        fallthrough__;
    case kDudeHand:
    case kDudeBoneEel:
    case kDudeBat:
    case kDudeRat:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        break;
    case kDudeCultistBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        break;
    case kDudeZombieAxeBuried:
        seqPrecacheId(pDudeInfo->seqStartID+12, hits);
        seqPrecacheId(pDudeInfo->seqStartID+9, hits);
        fallthrough__;
    case kDudeZombieAxeLaying:
        seqPrecacheId(pDudeInfo->seqStartID+10, hits);
        fallthrough__;
    case kDudeZombieAxeNormal:
        seqPrecacheId(pDudeInfo->seqStartID+6, hits);
        seqPrecacheId(pDudeInfo->seqStartID+7, hits);
        seqPrecacheId(pDudeInfo->seqStartID+8, hits);
        seqPrecacheId(pDudeInfo->seqStartID+11, hits);
        seqPrecacheId(pDudeInfo->seqStartID+13, hits);
        seqPrecacheId(pDudeInfo->seqStartID+14, hits);
        break;
    }
}

void PrecacheThing(spritetype *pSprite, HitList &hits) {
    switch (pSprite->type) {
        case kThingGlassWindow: // worthless...
        case kThingFluorescent:
            seqPrecacheId(12, hits);
            break;
        case kThingSpiderWeb:
            seqPrecacheId(15, hits);
            break;
        case kThingMetalGrate:
            seqPrecacheId(21, hits);
            break;
        case kThingFlammableTree:
            seqPrecacheId(25, hits);
            seqPrecacheId(26, hits);
            break;
        case kTrapMachinegun:
            seqPrecacheId(38, hits);
            seqPrecacheId(40, hits);
            seqPrecacheId(28, hits);
            break;
        case kThingObjectGib:
        //case kThingObjectExplode: weird that only gib object is precached and this one is not
            break;
    }
    tilePrecacheTile(pSprite->picnum, -1, hits);
}

void PreloadTiles(HitList & hits)
{
    nPrecacheCount = 0;
    int skyTile = -1;
    hits.Zero();
    // Fonts
    for (int i = 0; i < numsectors; i++)
    {
        tilePrecacheTile(sector[i].floorpicnum, 0, hits);
        tilePrecacheTile(sector[i].ceilingpicnum, 0, hits);
        if ((sector[i].ceilingstat&1) != 0 && skyTile == -1)
            skyTile = sector[i].ceilingpicnum;
    }
    for (int i = 0; i < numwalls; i++)
    {
        tilePrecacheTile(wall[i].picnum, 0, hits);
        if (wall[i].overpicnum >= 0)
            tilePrecacheTile(wall[i].overpicnum, 0, hits);
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            spritetype *pSprite = &sprite[i];
            switch (pSprite->statnum)
            {
            case kStatDude:
                PrecacheDude(pSprite, hits);
                break;
            case kStatThing:
                PrecacheThing(pSprite, hits);
                break;
            default:
                tilePrecacheTile(pSprite->picnum, -1, hits);
                break;
            }
        }
    }

    // Precache common SEQs
    for (int i = 0; i < 100; i++)
    {
        seqPrecacheId(i, hits);
    }

    tilePrecacheTile(1147, -1, hits); // water drip
    tilePrecacheTile(1160, -1, hits); // blood drip

    // Player SEQs
    seqPrecacheId(dudeInfo[31].seqStartID+6, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+7, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+8, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+9, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+10, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+14, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+15, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+12, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+16, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+17, hits);
    seqPrecacheId(dudeInfo[31].seqStartID+18, hits);

    if (skyTile > -1 && skyTile < kMaxTiles)
    {
        for (int i = 1; i < gSkyCount; i++)
            tilePrecacheTile(skyTile+i, 0, hits);
    }

    WeaponPrecache(hits);
    viewPrecacheTiles(hits);
    fxPrecache(hits);
    gibPrecache(hits);

    I_GetEvent();
}

void PreloadCache()
{
	if (!r_precache) return;
    HitList hits;
    PreloadTiles(hits);
    int cnt = 0;
    int percentDisplayed = -1;

    for (int i = 0; i < kMaxTiles; i++)
    {
        if (hits[i])
        {
            PrecacheHardwareTextures(i);

            if ((++cnt & 7) == 0)
                I_GetEvent();
        }
    }
}

END_BLD_NS

