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

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "common_game.h"

#include "actor.h"
#include "ai.h"
#include "aistate.h"
#include "blood.h"
#include "db.h"
#include "dude.h"
#include "eventq.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sound.h"

BEGIN_BLD_NS

static void TommySeqCallback(int, int);
static void TeslaSeqCallback(int, int);
static void ShotSeqCallback(int, int);
static void ThrowSeqCallback(int, int);
static void sub_68170(int, int);
static void sub_68230(int, int);
static void thinkSearch(spritetype *, XSPRITE *);
static void thinkGoto(spritetype *, XSPRITE *);
static void thinkChase(spritetype *, XSPRITE *);

static int nTommyClient = seqRegisterClient(TommySeqCallback);
static int nTeslaClient = seqRegisterClient(TeslaSeqCallback);
static int nShotClient = seqRegisterClient(ShotSeqCallback);
static int nThrowClient = seqRegisterClient(ThrowSeqCallback);
static int n68170Client = seqRegisterClient(sub_68170);
static int n68230Client = seqRegisterClient(sub_68230);

AISTATE cultistIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistProneIdle = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE fanaticProneIdle = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistProneIdle3 = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistChase = { kAiStateChase, 9, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE fanaticChase = { kAiStateChase, 0, -1, 0, NULL, aiMoveTurn, thinkChase, NULL };
AISTATE cultistDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &cultistChase };
AISTATE cultistGoto = { kAiStateMove, 9, -1, 600, NULL, aiMoveForward, thinkGoto, &cultistIdle };
AISTATE cultistProneChase = { kAiStateChase, 14, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE cultistProneDodge = { kAiStateMove, 14, -1, 90, NULL, aiMoveDodge, NULL, &cultistProneChase };
AISTATE cultistTThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistTFire };
AISTATE cultistSThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistSFire };
AISTATE cultistTsThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistTsFire };
AISTATE cultistDThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistChase };
AISTATE cultist139A78 = { kAiStateChase, 7, n68170Client, 120, NULL, NULL, NULL, &cultistChase };
AISTATE cultist139A94 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, NULL, &cultistIdle };
AISTATE cultist139AB0 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, thinkSearch, &cultist139A94 };
AISTATE cultist139ACC = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, thinkSearch, &cultist139AB0 };
AISTATE cultist139AE8 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, thinkSearch, &cultist139AE8 };
AISTATE cultistSearch = { kAiStateSearch, 9, -1, 1800, NULL, aiMoveForward, thinkSearch, &cultistIdle };
AISTATE cultistSFire = { kAiStateChase, 6, nShotClient, 60, NULL, NULL, NULL, &cultistChase };
AISTATE cultistTFire = { kAiStateChase, 6, nTommyClient, 0, NULL, aiMoveTurn, thinkChase, &cultistTFire };
AISTATE cultistTsFire = { kAiStateChase, 6, nTeslaClient, 0, NULL, aiMoveTurn, thinkChase, &cultistChase };
AISTATE cultistSProneFire = { kAiStateChase, 8, nShotClient, 60, NULL, NULL, NULL, &cultistProneChase };
AISTATE cultistTProneFire = { kAiStateChase, 8, nTommyClient, 0, NULL, aiMoveTurn, thinkChase, &cultistTProneFire };
AISTATE cultistTsProneFire = { kAiStateChase, 8, nTeslaClient, 0, NULL, aiMoveTurn, NULL, &cultistTsProneFire };
AISTATE cultistRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistDodge };
AISTATE cultistProneRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistProneDodge };
AISTATE cultistTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &cultistDodge };
AISTATE cultistSwimIdle = { kAiStateIdle, 13, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistSwimChase = { kAiStateChase, 13, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE cultistSwimDodge = { kAiStateMove, 13, -1, 90, NULL, aiMoveDodge, NULL, &cultistSwimChase };
AISTATE cultistSwimGoto = { kAiStateMove, 13, -1, 600, NULL, aiMoveForward, thinkGoto, &cultistSwimIdle };
AISTATE cultistSwimSearch = { kAiStateSearch, 13, -1, 1800, NULL, aiMoveForward, thinkSearch, &cultistSwimIdle };
AISTATE cultistSSwimFire = { kAiStateChase, 8, nShotClient, 60, NULL, NULL, NULL, &cultistSwimChase };
AISTATE cultistTSwimFire = { kAiStateChase, 8, nTommyClient, 0, NULL, aiMoveTurn, thinkChase, &cultistTSwimFire };
AISTATE cultistTsSwimFire = { kAiStateChase, 8, nTeslaClient, 0, NULL, aiMoveTurn, thinkChase, &cultistTsSwimFire };
AISTATE cultistSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistSwimDodge };

static void TommySeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int dz = gDudeSlope[nXSprite];
    dx += Random3((5-gGameOptions.nDifficulty)*1000);
    dy += Random3((5-gGameOptions.nDifficulty)*1000);
    dz += Random3((5-gGameOptions.nDifficulty)*500);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_2);
    sfxPlay3DSound(pSprite, 4001, -1, 0);
}

static void TeslaSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    if (Chance(dword_138BB0[gGameOptions.nDifficulty]))
    {
        int dx = CosScale16(pSprite->ang);
        int dy = SinScale16(pSprite->ang);
        int dz = gDudeSlope[nXSprite];
        dx += Random3((5-gGameOptions.nDifficulty)*1000);
        dy += Random3((5-gGameOptions.nDifficulty)*1000);
        dz += Random3((5-gGameOptions.nDifficulty)*500);
        actFireMissile(pSprite, 0, 0, dx, dy, dz, kMissileTeslaRegular);
        sfxPlay3DSound(pSprite, 470, -1, 0);
    }
}

static void ShotSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int dz = gDudeSlope[nXSprite];
    dx += Random2((5-gGameOptions.nDifficulty)*1000-500);
    dy += Random2((5-gGameOptions.nDifficulty)*1000-500);
    dz += Random2((5-gGameOptions.nDifficulty)*500);
    for (int i = 0; i < 8; i++)
    {
        int r1 = Random3(500);
        int r2 = Random3(1000);
        int r3 = Random3(1000);
        actFireVector(pSprite, 0, 0, dx+r3, dy+r2, dz+r1, VECTOR_TYPE_1);
    }
    if (Chance(0x8000))
        sfxPlay3DSound(pSprite, 1001, -1, 0);
    else
        sfxPlay3DSound(pSprite, 1002, -1, 0);
}

static void ThrowSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    char v4 = Chance(0x6000);
    sfxPlay3DSound(pSprite, 455, -1, 0);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int dx = pTarget->x - pSprite->x;
    int dy = pTarget->y - pSprite->y;
    int dz = pTarget->z - pSprite->z;
    int nDist = approxDist(dx, dy);
    int nDist2 = nDist / 540;
    if (nDist > 0x1e00)
        v4 = 0;
    spritetype *pMissile = actFireThing(pSprite, 0, 0, dz/128-14500, nMissile, (nDist2<<23)/120);
    if (v4)
        xsprite[pMissile->extra].Impact = 1;
    else
        evPost(pMissile->index, 3, 120*(1+Random(2)), kCmdOn);
}

static void sub_68170(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    sfxPlay3DSound(pSprite, 455, -1, 0);
    spritetype *pMissile = actFireThing(pSprite, 0, 0, gDudeSlope[nXSprite]-9460, nMissile, 0x133333);
    evPost(pMissile->index, 3, 120*(2+Random(2)), kCmdOn);
}

static void sub_68230(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    sfxPlay3DSound(pSprite, 455, -1, 0);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int dx = pTarget->x - pSprite->x;
    int dy = pTarget->y - pSprite->y;
    int dz = pTarget->z - pSprite->z;
    int nDist = approxDist(dx, dy);
    int nDist2 = nDist / 540;
    spritetype *pMissile = actFireThing(pSprite, 0, 0, dz/128-14500, nMissile, (nDist2<<17)/120);
    xsprite[pMissile->extra].Impact = 1;
}

static char TargetNearExplosion(spritetype *pSprite)
{
    for (short nSprite = headspritesect[pSprite->sectnum]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].type == kThingArmedTNTStick || sprite[nSprite].statnum == kStatExplosion)
            return 1;
    }
    return 0;
}

static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    sub_5F15C(pSprite, pXSprite);
}

static void thinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 5120 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(pSprite, pXSprite, &cultistSearch);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(pSprite, pXSprite, &cultistSwimSearch);
            break;
        }
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(pSprite, pXSprite, &cultistGoto);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(pSprite, pXSprite, &cultistSwimGoto);
            break;
        }
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(pSprite, pXSprite, &cultistSearch);
            if (pSprite->type == kDudeCultistTommy)
                aiPlay3DSound(pSprite, 4021+Random(4), AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1021+Random(4), AI_SFX_PRIORITY_1, -1);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(pSprite, pXSprite, &cultistSwimSearch);
            break;
        }
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(pSprite, pXSprite, &cultistSearch);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(pSprite, pXSprite, &cultistSwimSearch);
            break;
        }
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int nXSprite = sprite[pXSprite->reference].extra;
                gDudeSlope[nXSprite] = divscale(pTarget->z-pSprite->z, nDist, 10);
                switch (pSprite->type) {
                case kDudeCultistTommy:
                    if (nDist < 0x1e00 && nDist > 0xe00 && klabs(nDeltaAngle) < 85 && !TargetNearExplosion(pTarget)
                        && (pTarget->flags&2) && gGameOptions.nDifficulty > 2 && IsPlayerSprite(pTarget) && gPlayer[pTarget->type-kDudePlayer1].isRunning
                        && Chance(0x8000))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTThrow);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultistTThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x4600 && klabs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTProneFire);
                            else if (sub_5BDA8(pSprite, 13) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
                                aiNewState(pSprite, pXSprite, &cultistTSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun)
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistTFire);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistTProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistTSwimFire);
                            }
                            else
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistDodge);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTSwimFire);
                            break;
                        }
                    }
                    break;
                case kDudeCultistShotgun:
                    if (nDist < 0x2c00 && nDist > 0x1400 && !TargetNearExplosion(pTarget)
                        && (pTarget->flags&2) && gGameOptions.nDifficulty >= 2 && IsPlayerSprite(pTarget) && !gPlayer[pTarget->type-kDudePlayer1].isRunning
                        && Chance(0x8000))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && klabs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistSFire);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistSProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            }
                            else
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistDodge);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            break;
                        }
                    }
                    break;
                case kDudeCultistTesla:
                    if (nDist < 0x1e00 && nDist > 0xe00 && !TargetNearExplosion(pTarget)
                        && (pTarget->flags&2) && gGameOptions.nDifficulty > 2 && IsPlayerSprite(pTarget) && gPlayer[pTarget->type-kDudePlayer1].isRunning
                        && Chance(0x8000))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTsThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTsThrow);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultistTsThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && klabs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTsFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTsProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTsSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistTsFire);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistTsProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistTsSwimFire);
                            }
                            else
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistDodge);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTsFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistTsProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistTsSwimFire);
                            break;
                        }
                    }
                    break;
                case kDudeCultistTNT:
                    if (nDist < 0x2c00 && nDist > 0x1400 && klabs(nDeltaAngle) < 85
                        && (pTarget->flags&2) && IsPlayerSprite(pTarget))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistDThrow);
                            break;
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistDThrow);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultistDThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x1400 && klabs(nDeltaAngle) < 85
                        && (pTarget->flags&2) && IsPlayerSprite(pTarget))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != 1 && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultist139A78);
                            break;
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultist139A78);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultist139A78);
                            break;
                        }
                    }
                    break;
                case kDudeCultistBeast:
                    if (nDist < 0x1e00 && nDist > 0xe00 && !TargetNearExplosion(pTarget)
                        && (pTarget->flags&2) && gGameOptions.nDifficulty > 2 && IsPlayerSprite(pTarget) && gPlayer[pTarget->type-kDudePlayer1].isRunning
                        && Chance(0x8000))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &cultistSThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && klabs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistSFire);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistSProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            }
                            else
                            {
                                if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistDodge);
                                else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSFire);
                            else if (sub_5BDA8(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(pSprite, pXSprite, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(pSprite, pXSprite, &cultistSSwimFire);
                            break;
                        }
                    }
                    break;
                }
                return;
            }
        }
    }
    switch (pXSprite->medium)
    {
    case kMediumNormal:
        aiNewState(pSprite, pXSprite, &cultistGoto);
        break;
    case kMediumWater:
    case kMediumGoo:
        aiNewState(pSprite, pXSprite, &cultistSwimGoto);
        break;
    }
    pXSprite->target = -1;
}

END_BLD_NS
