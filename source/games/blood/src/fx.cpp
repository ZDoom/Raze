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

BEGIN_BLD_NS

CFX gFX;

struct FXDATA {
    CALLBACK_ID funcID; // callback
    uint8_t detail; // detail
    short seq; // seq
    short flags; // flags
    int gravity; // gravity
    int drag; // air drag
    int ate;
    short picnum; // picnum
    uint8_t xrepeat; // xrepeat
    uint8_t yrepeat; // yrepeat
    short cstat; // cstat
    int8_t shade; // shade
    uint8_t pal; // pal
};

FXDATA gFXData[] = {
    { kCallbackNone, 0, 49, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 50, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 51, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 52, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 44, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 45, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 46, 1, -128, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 42, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 43, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 48, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 60, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBloodBits, 2, 0, 1, 46603, 2048, 480, 2154, 40, 40, 0, -12, 0 },
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 2269, 24, 24, 0, -128, 0 },
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 1720, 24, 24, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 2280, 48, 48, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 3135, 48, 48, 0, -128, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 3261, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3265, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3269, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3273, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3277, 32, 32, 0, 0, 0 },
    { kCallbackNone, 2, 0, 1, -27962, 8192, 600, 1128, 16, 16, 514, -16, 0 }, // bubble 1
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1128, 12, 12, 514, -16, 0 }, // bubble 2
    { kCallbackNone, 2, 0, 1, -9320, 8192, 600, 1128, 8, 8, 514, -16, 0 }, // bubble 3
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1131, 32, 32, 514, -16, 0 },
    { kCallbackFXBloodBits, 2, 0, 3, 27962, 4096, 480, 733, 32, 32, 0, -16, 0 },
    { kCallbackNone, 1, 0, 3, 18641, 4096, 120, 2261, 12, 12, 0, -128, 0 },
    { kCallbackNone, 0, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 3328, 480, 2185, 48, 48, 0, 0, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 2620, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 55, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 56, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 57, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 58, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, 610, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 62, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 63, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 64, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 65, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 66, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 67, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, 80, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 2078, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 1106, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 3328, 480, 2406, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 46603, 4096, 480, 3511, 64, 64, 0, -128, 0 },
    { kCallbackNone, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, 0, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 30, 3, 0, 0, 0, 0, 40, 40, 80, -8, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4023, 32, 32, 0, -16, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4028, 32, 32, 0, -16, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 480, 926, 32, 32, 610, -12, 0 },
    { kCallbackNone, 1, 70, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 }
};

void CFX::destroy(int nSprite)
{
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    evKill(nSprite, 3);
    if (sprite[nSprite].extra > 0)
        seqKill(3, sprite[nSprite].extra);
    DeleteSprite(nSprite);
}

void CFX::remove(int nSprite)
{
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    if (pSprite->statnum != kStatFree)
        actPostSprite(nSprite, kStatFree);
}

spritetype * CFX::fxSpawn(FX_ID nFx, int nSector, int x, int y, int z, unsigned int a6)
{
    if (nSector < 0 || nSector >= numsectors)
        return NULL;
    int nSector2 = nSector;
    if (!FindSector(x, y, z, &nSector2))
        return NULL;
    if (adult_lockout && gGameOptions.nGameType <= 0)
    {
        switch (nFx)
        {
        case FX_0:
        case FX_1:
        case FX_2:
        case FX_3:
        case FX_13:
        case FX_34:
        case FX_35:
        case FX_36:
            return NULL;
        default:
            break;
        }
    }
    if (nFx < 0 || nFx >= kFXMax)
        return NULL;
    FXDATA *pFX = &gFXData[nFx];
    if (gStatCount[1] == 512)
    {
        StatIterator it(kStatFX);
        int nSprite = it.NextIndex();
        while (nSprite != -1 && (sprite[nSprite].flags & 32))
            nSprite = it.NextIndex();
        if (nSprite == -1)
            return NULL;
        destroy(nSprite);
    }
    spritetype *pSprite = actSpawnSprite_(nSector, x, y, z, 1, 0);
    pSprite->type = nFx;
    pSprite->picnum = pFX->picnum;
    pSprite->cstat |= pFX->cstat;
    pSprite->shade = pFX->shade;
    pSprite->pal = pFX->pal;
    sprite[pSprite->index].detail = pFX->detail;
    if (pFX->xrepeat > 0)
        pSprite->xrepeat = pFX->xrepeat;
    if (pFX->yrepeat > 0)
        pSprite->yrepeat = pFX->yrepeat;
    if ((pFX->flags & 1) && Chance(0x8000))
        pSprite->cstat |= 4;
    if ((pFX->flags & 2) && Chance(0x8000))
        pSprite->cstat |= 8;
    if (pFX->seq)
    {
        int nXSprite = dbInsertXSprite(pSprite->index);
        seqSpawn(pFX->seq, 3, nXSprite, -1);
    }
    if (a6 == 0)
        a6 = pFX->ate;
    if (a6)
        evPost((int)pSprite->index, 3, a6+Random2(a6>>1), kCallbackRemove);
    return pSprite;
}

void CFX::fxProcess(void)
{
    int nSprite;
    StatIterator it(kStatFX);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        spritetype *pSprite = &sprite[nSprite];
        viewBackupSpriteLoc(nSprite, pSprite);
        short nSector = pSprite->sectnum;
        assert(nSector >= 0 && nSector < kMaxSectors);
        assert(pSprite->type < kFXMax);
        FXDATA *pFXData = &gFXData[pSprite->type];
        actAirDrag(pSprite, pFXData->drag);
        if (xvel[nSprite])
            pSprite->x += xvel[nSprite]>>12;
        if (yvel[nSprite])
            pSprite->y += yvel[nSprite]>>12;
        if (zvel[nSprite])
            pSprite->z += zvel[nSprite]>>8;
        // Weird...
        if (xvel[nSprite] || (yvel[nSprite] && pSprite->z >= sector[pSprite->sectnum].floorz))
        {
            updatesector(pSprite->x, pSprite->y, &nSector);
            if (nSector == -1)
            {
                remove(nSprite);
                continue;
            }
            if (getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) <= pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    remove(nSprite);
                    continue;
                }
                assert(gCallback[pFXData->funcID] != NULL);
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
            if (nSector != pSprite->sectnum)
            {
                assert(nSector >= 0 && nSector < kMaxSectors);
                ChangeSpriteSect(nSprite, nSector);
            }
        }
        if (xvel[nSprite] || yvel[nSprite] || zvel[nSprite])
        {
            int32_t floorZ, ceilZ;
            getzsofslope(nSector, pSprite->x, pSprite->y, &ceilZ, &floorZ);
            if (ceilZ > pSprite->z && !(sector[nSector].ceilingstat&1))
            {
                remove(nSprite);
                continue;
            }
            if (floorZ < pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    remove(nSprite);
                    continue;
                }
                assert(gCallback[pFXData->funcID] != NULL);
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
        }
        zvel[nSprite] += pFXData->gravity;
    }
}

void fxSpawnBlood(spritetype *pSprite, int )
{
    if (pSprite->sectnum < 0 || pSprite->sectnum >= numsectors)
        return;
    int nSector = pSprite->sectnum;
    if (!FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector))
        return;
    if (adult_lockout && gGameOptions.nGameType <= 0)
        return;
    spritetype *pBlood = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pBlood)
    {
        pBlood->ang = 1024;
        xvel[pBlood->index] = Random2(0x6aaaa);
        yvel[pBlood->index] = Random2(0x6aaaa);
        zvel[pBlood->index] = -(int)Random(0x10aaaa)-100;
        evPost(pBlood->index, 3, 8, kCallbackFXBloodSpurt);
    }
}

void sub_746D4(spritetype *pSprite, int )
{
    if (pSprite->sectnum < 0 || pSprite->sectnum >= numsectors)
        return;
    int nSector = pSprite->sectnum;
    if (!FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector))
        return;
    if (adult_lockout && gGameOptions.nGameType <= 0)
        return;
    spritetype *pSpawn;
    if (pSprite->type == kDudePodGreen)
        pSpawn = gFX.fxSpawn(FX_53, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    else
        pSpawn = gFX.fxSpawn(FX_54, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 0);
    if (pSpawn)
    {
        pSpawn->ang = 1024;
        xvel[pSpawn->index] = Random2(0x6aaaa);
        yvel[pSpawn->index] = Random2(0x6aaaa);
        zvel[pSpawn->index] = -(int)Random(0x10aaaa)-100;
        evPost(pSpawn->index, 3, 8, kCallbackFXPodBloodSpray);
    }
}

void fxSpawnEjectingBrass(spritetype *pSprite, int z, int a3, int a4)
{
    int x = pSprite->x+MulScale(pSprite->clipdist-4, Cos(pSprite->ang), 28);
    int y = pSprite->y+MulScale(pSprite->clipdist-4, Sin(pSprite->ang), 28);
    x += MulScale(a3, Cos(pSprite->ang+512), 30);
    y += MulScale(a3, Sin(pSprite->ang+512), 30);
    spritetype *pBrass = gFX.fxSpawn((FX_ID)(FX_37+Random(3)), pSprite->sectnum, x, y, z, 0);
    if (pBrass)
    {
        if (!VanillaMode())
            pBrass->ang = Random(2047);
        int nDist = (a4<<18)/120+Random2(((a4/4)<<18)/120);
        int nAngle = pSprite->ang+Random2(56)+512;
        xvel[pBrass->index] = MulScale(nDist, Cos(nAngle), 30);
        yvel[pBrass->index] = MulScale(nDist, Sin(nAngle), 30);
        zvel[pBrass->index] = zvel[pSprite->index]-(0x20000+(Random2(40)<<18)/120);
    }
}

void fxSpawnEjectingShell(spritetype *pSprite, int z, int a3, int a4)
{
    int x = pSprite->x+MulScale(pSprite->clipdist-4, Cos(pSprite->ang), 28);
    int y = pSprite->y+MulScale(pSprite->clipdist-4, Sin(pSprite->ang), 28);
    x += MulScale(a3, Cos(pSprite->ang+512), 30);
    y += MulScale(a3, Sin(pSprite->ang+512), 30);
    spritetype *pShell = gFX.fxSpawn((FX_ID)(FX_40+Random(3)), pSprite->sectnum, x, y, z, 0);
    if (pShell)
    {
        if (!VanillaMode())
            pShell->ang = Random(2047);
        int nDist = (a4<<18)/120+Random2(((a4/4)<<18)/120);
        int nAngle = pSprite->ang+Random2(56)+512;
        xvel[pShell->index] = MulScale(nDist, Cos(nAngle), 30);
        yvel[pShell->index] = MulScale(nDist, Sin(nAngle), 30);
        zvel[pShell->index] = zvel[pSprite->index]-(0x20000+(Random2(20)<<18)/120);
    }
}

void fxPrecache()
{
    for (int i = 0; i < kFXMax; i++)
    {
        tilePrecacheTile(gFXData[i].picnum, 0, 0);
        if (gFXData[i].seq)
            seqPrecacheId(gFXData[i].seq, 0);
    }
}

END_BLD_NS
