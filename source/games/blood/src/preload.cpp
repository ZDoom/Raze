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
#include "blood.h"
#include "view.h"
#include "g_input.h"
#include "precache.h"

BEGIN_BLD_NS

int nPrecacheCount;

void fxPrecache();
void gibPrecache();


void tilePrecacheTile(int nTile, int nType, int palette)
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

                markTileForPrecache(tile, palette);
            }
        }
        else
        {
            markTileForPrecache(nTile, palette);
        }
        nTile += 1 + picanm[nTile].num;
    }
}


// To do: This needs to handle the sprite palettes as well to properly precache the needed content.

void viewPrecacheTiles()
{
    tilePrecacheTile(2173, 0, 0);
    tilePrecacheTile(2200, 0, 0);
    tilePrecacheTile(2201, 0, 0);
    tilePrecacheTile(2202, 0, 0);
    tilePrecacheTile(2207, 0, 0);
    tilePrecacheTile(2208, 0, 0);
    tilePrecacheTile(2209, 0, 0);
    tilePrecacheTile(2229, 0, 0);
    tilePrecacheTile(2260, 0, 0);
    tilePrecacheTile(2559, 0, 0);
    tilePrecacheTile(2169, 0, 0);
    tilePrecacheTile(2578, 0, 0);
    tilePrecacheTile(2586, 0, 0);
    tilePrecacheTile(2602, 0, 0);
    for (int i = 0; i < 10; i++)
    {
        tilePrecacheTile(2190 + i, 0, 0);
        tilePrecacheTile(2230 + i, 0, 0);
        tilePrecacheTile(2240 + i, 0, 0);
        tilePrecacheTile(2250 + i, 0, 0);
        tilePrecacheTile(kSBarNumberHealth + i, 0, 0);
        tilePrecacheTile(kSBarNumberAmmo + i, 0, 0);
        tilePrecacheTile(kSBarNumberInv + i, 0, 0);
        tilePrecacheTile(kSBarNumberArmor1 + i, 0, 0);
        tilePrecacheTile(kSBarNumberArmor2 + i, 0, 0);
        tilePrecacheTile(kSBarNumberArmor3 + i, 0, 0);
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
        tilePrecacheTile(2220 + i, 0, 0);
        tilePrecacheTile(2552 + i, 0, 0);
    }
}



void PrecacheDude(spritetype *pSprite)
{
    int palette = pSprite->pal;
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    seqPrecacheId(pDudeInfo->seqStartID  , palette);
    seqPrecacheId(pDudeInfo->seqStartID+5, palette);
    seqPrecacheId(pDudeInfo->seqStartID+1, palette);
    seqPrecacheId(pDudeInfo->seqStartID+2, palette);
    switch (pSprite->type)
    {
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
        seqPrecacheId(pDudeInfo->seqStartID+6 , palette);
        seqPrecacheId(pDudeInfo->seqStartID+7 , palette);
        seqPrecacheId(pDudeInfo->seqStartID+8 , palette);
        seqPrecacheId(pDudeInfo->seqStartID+9 , palette);
        seqPrecacheId(pDudeInfo->seqStartID+13, palette);
        seqPrecacheId(pDudeInfo->seqStartID+14, palette);
        seqPrecacheId(pDudeInfo->seqStartID+15, palette);
        break;
    case kDudeZombieButcher:
    case kDudeGillBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        seqPrecacheId(pDudeInfo->seqStartID+8, palette);
        seqPrecacheId(pDudeInfo->seqStartID+9, palette);
        seqPrecacheId(pDudeInfo->seqStartID+10, palette);
        seqPrecacheId(pDudeInfo->seqStartID+11, palette);
        break;
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+6, palette); //???
        fallthrough__;
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        seqPrecacheId(pDudeInfo->seqStartID+8, palette);
        seqPrecacheId(pDudeInfo->seqStartID+9, palette);
        break;
    case kDudePhantasm:
    case kDudeHellHound:
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
    case kDudeSpiderMother:
    case kDudeTchernobog:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        seqPrecacheId(pDudeInfo->seqStartID+8, palette);
        break;
    case kDudeCerberusTwoHead:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        fallthrough__;
    case kDudeHand:
    case kDudeBoneEel:
    case kDudeBat:
    case kDudeRat:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        break;
    case kDudeCultistBeast:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        break;
    case kDudeZombieAxeBuried:
        seqPrecacheId(pDudeInfo->seqStartID+12, palette);
        seqPrecacheId(pDudeInfo->seqStartID+9, palette);
        fallthrough__;
    case kDudeZombieAxeLaying:
        seqPrecacheId(pDudeInfo->seqStartID+10, palette);
        fallthrough__;
    case kDudeZombieAxeNormal:
        seqPrecacheId(pDudeInfo->seqStartID+6, palette);
        seqPrecacheId(pDudeInfo->seqStartID+7, palette);
        seqPrecacheId(pDudeInfo->seqStartID+8, palette);
        seqPrecacheId(pDudeInfo->seqStartID+11, palette);
        seqPrecacheId(pDudeInfo->seqStartID+13, palette);
        seqPrecacheId(pDudeInfo->seqStartID+14, palette);
        break;
    }
}

void PrecacheThing(spritetype *pSprite) 
{
    int palette = pSprite->pal;
    switch (pSprite->type) {
        case kThingGlassWindow: // worthless...
        case kThingFluorescent:
            seqPrecacheId(12, palette);
            break;
        case kThingSpiderWeb:
            seqPrecacheId(15, palette);
            break;
        case kThingMetalGrate:
            seqPrecacheId(21, palette);
            break;
        case kThingFlammableTree:
            seqPrecacheId(25, palette);
            seqPrecacheId(26, palette);
            break;
        case kTrapMachinegun:
            seqPrecacheId(38, palette);
            seqPrecacheId(40, palette);
            seqPrecacheId(28, palette);
            break;
        case kThingObjectGib:
        //case kThingObjectExplode: weird that only gib object is precached and this one is not
            break;
    }
    tilePrecacheTile(pSprite->picnum, -1, palette);
}

void PreloadCache()
{
    if (!r_precache) return;
    int skyTile = -1;
    // Fonts
    for (int i = 0; i < numsectors; i++)
    {
        tilePrecacheTile(sector[i].floorpicnum, 0, sector[i].floorpal);
        tilePrecacheTile(sector[i].ceilingpicnum, 0, sector[i].ceilingpal);
        if ((sector[i].ceilingstat&1) != 0 && skyTile == -1)
            skyTile = sector[i].ceilingpicnum;
    }
    for (int i = 0; i < numwalls; i++)
    {
        tilePrecacheTile(wall[i].picnum, 0, wall[i].pal);
        if (wall[i].overpicnum >= 0)
            tilePrecacheTile(wall[i].overpicnum, 0, wall[i].pal);
    }
    BloodSpriteIterator it;
    while (auto actor = it.Next())
    {
        spritetype *pSprite = &actor->s();
        switch (pSprite->statnum)
        {
        case kStatDude:
            PrecacheDude(pSprite);
            break;
        case kStatThing:
            PrecacheThing(pSprite);
            break;
        default:
            tilePrecacheTile(pSprite->picnum, -1, pSprite->pal);
            break;
        }
    }

    // Precache common SEQs
    for (int i = 0; i < 100; i++)
    {
        seqPrecacheId(i, 0);
    }

    tilePrecacheTile(1147, -1, 0); // water drip
    tilePrecacheTile(1160, -1, 0); // blood drip

    // Player SEQs
    seqPrecacheId(dudeInfo[31].seqStartID+6, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+7, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+8, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+9, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+10, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+14, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+15, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+12, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+16, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+17, 0);
    seqPrecacheId(dudeInfo[31].seqStartID+18, 0);

    if (skyTile > -1 && skyTile < kMaxTiles)
    {
        for (int i = 1; i < gSkyCount; i++)
            tilePrecacheTile(skyTile+i, 0, 0);
    }

    WeaponPrecache();
    viewPrecacheTiles();
    fxPrecache();
    gibPrecache();

    I_GetEvent();
    precacheMarkedTiles();
}

END_BLD_NS

