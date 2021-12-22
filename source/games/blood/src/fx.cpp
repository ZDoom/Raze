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
#include "bloodactor.h"

BEGIN_BLD_NS

CFX gFX;

struct FXDATA {
    CALLBACK_ID funcID; // callback
    uint8_t detail; // detail
    int16_t seq; // seq
    int16_t flags; // flags
    int32_t gravity; // gravity
    int32_t drag; // air drag
    int32_t ate;
    int16_t picnum; // picnum
    uint8_t xrepeat; // xrepeat
    uint8_t yrepeat; // yrepeat
    ESpriteFlags cstat; // cstat
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
    { kCallbackNone, 2, 0, 1, -27962, 8192, 600, 1128, 16, 16, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP, -16, 0 }, // bubble 1
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1128, 12, 12, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP, -16, 0 }, // bubble 2
    { kCallbackNone, 2, 0, 1, -9320, 8192, 600, 1128, 8, 8, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP, -16, 0 }, // bubble 3
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1131, 32, 32, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP, -16, 0 },
    { kCallbackFXBloodBits, 2, 0, 3, 27962, 4096, 480, 733, 32, 32, 0, -16, 0 },
    { kCallbackNone, 1, 0, 3, 18641, 4096, 120, 2261, 12, 12, 0, -128, 0 },
    { kCallbackNone, 0, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 3328, 480, 2185, 48, 48, 0, 0, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 2620, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 55, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 56, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 57, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 58, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 62, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 63, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 64, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 65, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 66, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 67, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_WALL, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 2078, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 1106, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 3328, 480, 2406, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 46603, 4096, 480, 3511, 64, 64, 0, -128, 0 },
    { kCallbackNone, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, 0, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 30, 3, 0, 0, 0, 0, 40, 40, CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_WALL, -8, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4023, 32, 32, 0, -16, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4028, 32, 32, 0, -16, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 480, 926, 32, 32, CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR, -12, 0 },
    { kCallbackNone, 1, 70, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 }
};

void CFX::destroy(DBloodActor* actor)
{
    if (!actor) return;
    evKillActor(actor);
    if (actor->hasX()) seqKill(actor);
    DeleteSprite(actor);
}

void CFX::remove(DBloodActor* actor)
{
    if (!actor) return;
    spritetype *pSprite = &actor->s();
    if (actor->hasX()) seqKill(actor);
    if (pSprite->statnum != kStatFree)
        actPostSprite(actor, kStatFree);
}

DBloodActor* CFX::fxSpawnActor(FX_ID nFx, sectortype* pSector, int x, int y, int z, unsigned int a6)
{
    if (pSector == nullptr)
        return nullptr;
    auto pSector2 = pSector;
    if (!FindSector(x, y, z, &pSector2))
        return nullptr;
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
            return nullptr;
        default:
            break;
        }
    }
    if (nFx < 0 || nFx >= kFXMax)
        return nullptr;
    FXDATA *pFX = &gFXData[nFx];
    if (gStatCount[1] == 512)
    {
        BloodStatIterator it(kStatFX);
        auto iactor = it.Next();
        while (iactor && (iactor->spr.flags & 32))
            iactor = it.Next();
        if (!iactor)
            return nullptr;
        destroy(iactor);
    }
    auto actor = actSpawnSprite(pSector, x, y, z, 1, 0);
    spritetype* pSprite = &actor->s();
    pSprite->type = nFx;
    pSprite->picnum = pFX->picnum;
    pSprite->cstat |= pFX->cstat;
    pSprite->shade = pFX->shade;
    pSprite->pal = pFX->pal;
    pSprite->detail = pFX->detail;
    if (pFX->xrepeat > 0)
        pSprite->xrepeat = pFX->xrepeat;
    if (pFX->yrepeat > 0)
        pSprite->yrepeat = pFX->yrepeat;
    if ((pFX->flags & 1) && Chance(0x8000))
        pSprite->cstat |= CSTAT_SPRITE_XFLIP;
    if ((pFX->flags & 2) && Chance(0x8000))
        pSprite->cstat |= CSTAT_SPRITE_YFLIP;
    if (pFX->seq)
    {
        actor->addX();
        seqSpawn(pFX->seq, actor, -1);
    }
    if (a6 == 0)
        a6 = pFX->ate;
    if (a6)
        evPostActor(actor, a6+Random2(a6>>1), kCallbackRemove);
    return actor;
}

void CFX::fxProcess(void)
{
    BloodStatIterator it(kStatFX);
    while (auto actor = it.Next())
    {
        spritetype *pSprite = &actor->s();
        viewBackupSpriteLoc(actor);
        auto pSector = pSprite->sector();
        assert(pSector);
        assert(pSprite->type < kFXMax);
        FXDATA *pFXData = &gFXData[pSprite->type];
        actAirDrag(actor, pFXData->drag);
        pSprite->pos.X += actor->xvel>>12;
        pSprite->pos.Y += actor->yvel>>12;
        pSprite->pos.Z += actor->zvel>>8;
        // Weird...
        if (actor->xvel || (actor->yvel && pSprite->pos.Z >= pSprite->sector()->floorz))
        {
            updatesector(pSprite->pos.X, pSprite->pos.Y, &pSector);
            if (pSector == nullptr)
            {
                remove(actor);
                continue;
            }
            if (getflorzofslopeptr(pSprite->sector(), pSprite->pos.X, pSprite->pos.Y) <= pSprite->pos.Z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    remove(actor);
                    continue;
                }
                gCallback[pFXData->funcID](actor, nullptr);
                continue;
            }
            if (pSector != pSprite->sector())
            {
                assert(pSector);
                ChangeActorSect(actor, pSector);
            }
        }
        if (actor->xvel || actor->yvel || actor->zvel)
        {
            int32_t floorZ, ceilZ;
            getzsofslopeptr(pSector, pSprite->pos.X, pSprite->pos.Y, &ceilZ, &floorZ);
            if (ceilZ > pSprite->pos.Z && !(pSector->ceilingstat & CSTAT_SECTOR_SKY))
            {
                remove(actor);
                continue;
            }
            if (floorZ < pSprite->pos.Z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    remove(actor);
                    continue;
                }
                gCallback[pFXData->funcID](actor, nullptr);
                continue;
            }
        }
        actor->zvel += pFXData->gravity;
    }
}

void fxSpawnBlood(DBloodActor *actor, int )
{
    spritetype* pSprite = &actor->s();
    if (!pSprite->insector())
        return;
    auto pSector = pSprite->sector();
    if (!FindSector(pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z, &pSector))
        return;
    if (adult_lockout && gGameOptions.nGameType <= 0)
        return;
    auto bloodactor = gFX.fxSpawnActor(FX_27, pSprite->sector(), pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z, 0);
    if (bloodactor)
    {
        bloodactor->spr.ang = 1024;
        bloodactor->xvel = Random2(0x6aaaa);
        bloodactor->yvel = Random2(0x6aaaa);
        bloodactor->zvel = -(int)Random(0x10aaaa)-100;
        evPostActor(bloodactor, 8, kCallbackFXBloodSpurt);
    }
}

void fxSpawnPodStuff(DBloodActor* actor, int )
{
    auto pSprite = &actor->s();
    if (!pSprite->insector())
        return;
    auto pSector = pSprite->sector();
    if (!FindSector(pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z, &pSector))
        return;
    if (adult_lockout && gGameOptions.nGameType <= 0)
        return;
    DBloodActor *spawnactor;
    if (pSprite->type == kDudePodGreen)
        spawnactor = gFX.fxSpawnActor(FX_53, pSprite->sector(), pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z, 0);
    else
        spawnactor = gFX.fxSpawnActor(FX_54, pSprite->sector(), pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z, 0);
    if (spawnactor)
    {
        spawnactor->spr.ang = 1024;
        spawnactor->xvel = Random2(0x6aaaa);
        spawnactor->yvel = Random2(0x6aaaa);
        spawnactor->zvel = -(int)Random(0x10aaaa)-100;
        evPostActor(spawnactor, 8, kCallbackFXPodBloodSpray);
    }
}

void fxSpawnEjectingBrass(DBloodActor* actor, int z, int a3, int a4)
{
    auto pSprite = &actor->s();
    int x = pSprite->pos.X + MulScale(pSprite->clipdist - 4, Cos(pSprite->ang), 28);
    int y = pSprite->pos.Y + MulScale(pSprite->clipdist - 4, Sin(pSprite->ang), 28);
    x += MulScale(a3, Cos(pSprite->ang + 512), 30);
    y += MulScale(a3, Sin(pSprite->ang + 512), 30);
    auto pBrass = gFX.fxSpawnActor((FX_ID)(FX_37 + Random(3)), pSprite->sector(), x, y, z, 0);
    if (pBrass)
    {
        if (!VanillaMode())
            pBrass->spr.ang = Random(2047);
        int nDist = (a4 << 18) / 120 + Random2(((a4 / 4) << 18) / 120);
        int nAngle = pSprite->ang + Random2(56) + 512;
        pBrass->xvel = MulScale(nDist, Cos(nAngle), 30);
        pBrass->yvel = MulScale(nDist, Sin(nAngle), 30);
        pBrass->zvel = actor->zvel - (0x20000 + (Random2(40) << 18) / 120);
    }
}

void fxSpawnEjectingShell(DBloodActor* actor, int z, int a3, int a4)
{
    auto pSprite = &actor->s();
    int x = pSprite->pos.X + MulScale(pSprite->clipdist - 4, Cos(pSprite->ang), 28);
    int y = pSprite->pos.Y + MulScale(pSprite->clipdist - 4, Sin(pSprite->ang), 28);
    x += MulScale(a3, Cos(pSprite->ang + 512), 30);
    y += MulScale(a3, Sin(pSprite->ang + 512), 30);
    auto pShell = gFX.fxSpawnActor((FX_ID)(FX_40 + Random(3)), pSprite->sector(), x, y, z, 0);
    if (pShell)
    {
        if (!VanillaMode())
            pShell->spr.ang = Random(2047);
        int nDist = (a4 << 18) / 120 + Random2(((a4 / 4) << 18) / 120);
        int nAngle = pSprite->ang + Random2(56) + 512;
        pShell->xvel = MulScale(nDist, Cos(nAngle), 30);
        pShell->yvel = MulScale(nDist, Sin(nAngle), 30);
        pShell->zvel = actor->zvel - (0x20000 + (Random2(20) << 18) / 120);
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
