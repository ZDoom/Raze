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

#include "blood.h"

BEGIN_BLD_NS

static void cultThinkSearch(DBloodActor *);
static void cultThinkGoto(DBloodActor *);
static void cultThinkChase(DBloodActor *);

AISTATE cultistIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistProneIdle = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE fanaticProneIdle = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistProneIdle3 = { kAiStateIdle, 17, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistChase = { kAiStateChase, 9, -1, 0, NULL, aiMoveForward, cultThinkChase, NULL };
AISTATE fanaticChase = { kAiStateChase, 0, -1, 0, NULL, aiMoveTurn, cultThinkChase, NULL };
AISTATE cultistDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &cultistChase };
AISTATE cultistGoto = { kAiStateMove, 9, -1, 600, NULL, aiMoveForward, cultThinkGoto, &cultistIdle };
AISTATE cultistProneChase = { kAiStateChase, 14, -1, 0, NULL, aiMoveForward, cultThinkChase, NULL };
AISTATE cultistProneDodge = { kAiStateMove, 14, -1, 90, NULL, aiMoveDodge, NULL, &cultistProneChase };
AISTATE cultistTThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistTFire };
AISTATE cultistSThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistSFire };
AISTATE cultistTsThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistTsFire };
AISTATE cultistDThrow = { kAiStateChase, 7, nThrowClient, 120, NULL, NULL, NULL, &cultistChase };
AISTATE cultist139A78 = { kAiStateChase, 7, n68170Client, 120, NULL, NULL, NULL, &cultistChase };
AISTATE cultist139A94 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, NULL, &cultistIdle };
AISTATE cultist139AB0 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, cultThinkSearch, &cultist139A94 };
AISTATE cultist139ACC = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, cultThinkSearch, &cultist139AB0 };
AISTATE cultist139AE8 = { kAiStateChase, 7, n68230Client, 120, NULL, NULL, cultThinkSearch, &cultist139AE8 };
AISTATE cultistSearch = { kAiStateSearch, 9, -1, 1800, NULL, aiMoveForward, cultThinkSearch, &cultistIdle };
AISTATE cultistSFire = { kAiStateChase, 6, nShotClient, 60, NULL, NULL, NULL, &cultistChase };
AISTATE cultistTFire = { kAiStateChase, 6, nTommyClient, 0, NULL, aiMoveTurn, cultThinkChase, &cultistTFire };
AISTATE cultistTsFire = { kAiStateChase, 6, nTeslaClient, 0, NULL, aiMoveTurn, cultThinkChase, &cultistChase };
AISTATE cultistSProneFire = { kAiStateChase, 8, nShotClient, 60, NULL, NULL, NULL, &cultistProneChase };
AISTATE cultistTProneFire = { kAiStateChase, 8, nTommyClient, 0, NULL, aiMoveTurn, cultThinkChase, &cultistTProneFire };
AISTATE cultistTsProneFire = { kAiStateChase, 8, nTeslaClient, 0, NULL, aiMoveTurn, NULL, &cultistTsProneFire };
AISTATE cultistRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistDodge };
AISTATE cultistProneRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistProneDodge };
AISTATE cultistTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &cultistDodge };
AISTATE cultistSwimIdle = { kAiStateIdle, 13, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistSwimChase = { kAiStateChase, 13, -1, 0, NULL, aiMoveForward, cultThinkChase, NULL };
AISTATE cultistSwimDodge = { kAiStateMove, 13, -1, 90, NULL, aiMoveDodge, NULL, &cultistSwimChase };
AISTATE cultistSwimGoto = { kAiStateMove, 13, -1, 600, NULL, aiMoveForward, cultThinkGoto, &cultistSwimIdle };
AISTATE cultistSwimSearch = { kAiStateSearch, 13, -1, 1800, NULL, aiMoveForward, cultThinkSearch, &cultistSwimIdle };
AISTATE cultistSSwimFire = { kAiStateChase, 8, nShotClient, 60, NULL, NULL, NULL, &cultistSwimChase };
AISTATE cultistTSwimFire = { kAiStateChase, 8, nTommyClient, 0, NULL, aiMoveTurn, cultThinkChase, &cultistTSwimFire };
AISTATE cultistTsSwimFire = { kAiStateChase, 8, nTeslaClient, 0, NULL, aiMoveTurn, cultThinkChase, &cultistTsSwimFire };
AISTATE cultistSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cultistSwimDodge };

void TommySeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int dz = actor->dudeSlope;
    dx += Random3((5-gGameOptions.nDifficulty)*1000);
    dy += Random3((5-gGameOptions.nDifficulty)*1000);
    dz += Random3((5-gGameOptions.nDifficulty)*500);
    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorBullet);
    sfxPlay3DSound(pSprite, 4001, -1, 0);
}

void TeslaSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    if (Chance(dword_138BB0[gGameOptions.nDifficulty]))
    {
        int dx = CosScale16(pSprite->ang);
        int dy = SinScale16(pSprite->ang);
        int dz = actor->dudeSlope;
        dx += Random3((5-gGameOptions.nDifficulty)*1000);
        dy += Random3((5-gGameOptions.nDifficulty)*1000);
        dz += Random3((5-gGameOptions.nDifficulty)*500);
        actFireMissile(pSprite, 0, 0, dx, dy, dz, kMissileTeslaRegular);
        sfxPlay3DSound(pSprite, 470, -1, 0);
    }
}

void ShotSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int dz = actor->dudeSlope;
    dx += Random2((5-gGameOptions.nDifficulty)*1000-500);
    dy += Random2((5-gGameOptions.nDifficulty)*1000-500);
    dz += Random2((5-gGameOptions.nDifficulty)*500);
    for (int i = 0; i < 8; i++)
    {
        int r1 = Random3(500);
        int r2 = Random3(1000);
        int r3 = Random3(1000);
        actFireVector(pSprite, 0, 0, dx+r3, dy+r2, dz+r1, kVectorShell);
    }
    if (Chance(0x8000))
        sfxPlay3DSound(pSprite, 1001, -1, 0);
    else
        sfxPlay3DSound(pSprite, 1002, -1, 0);
}

void cultThrowSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    char v4 = Chance(0x6000);
    sfxPlay3DSound(pSprite, 455, -1, 0);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int dx = pTarget->x - pSprite->x;
    int dy = pTarget->y - pSprite->y;
    int dz = pTarget->z - pSprite->z;
    int nDist = approxDist(dx, dy);
    int nDist2 = nDist / 540;
    if (nDist > 0x1e00)
        v4 = 0;
    spritetype *pMissile = actFireThing_(pSprite, 0, 0, dz/128-14500, nMissile, (nDist2<<23)/120);
    if (v4)
        xsprite[pMissile->extra].Impact = 1;
    else
        evPost(pMissile->index, 3, 120*(1+Random(2)), kCmdOn);
}

void sub_68170(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    sfxPlay3DSound(pSprite, 455, -1, 0);
    spritetype* pMissile = actFireThing_(pSprite, 0, 0, actor->dudeSlope - 9460, nMissile, 0x133333);
    evPost(pMissile->index, 3, 120*(2+Random(2)), kCmdOn);
}

void sub_68230(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int nMissile = kThingArmedTNTStick;
    if (gGameOptions.nDifficulty > 2)
        nMissile = kThingArmedTNTBundle;
    sfxPlay3DSound(pSprite, 455, -1, 0);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int dx = pTarget->x - pSprite->x;
    int dy = pTarget->y - pSprite->y;
    int dz = pTarget->z - pSprite->z;
    int nDist = approxDist(dx, dy);
    int nDist2 = nDist / 540;
    spritetype *pMissile = actFireThing_(pSprite, 0, 0, dz/128-14500, nMissile, (nDist2<<17)/120);
    xsprite[pMissile->extra].Impact = 1;
}

static char TargetNearExplosion(spritetype *pSprite)
{
    int nSprite;
    SectIterator it(pSprite->sectnum);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        if (sprite[nSprite].type == kThingArmedTNTStick || sprite[nSprite].statnum == kStatExplosion)
            return 1;
    }
    return 0;
}

static void cultThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    sub_5F15C(pSprite, pXSprite);
}

static void cultThinkGoto(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 5120 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(actor, &cultistSearch);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(actor, &cultistSwimSearch);
            break;
        }
    }
    aiThinkTarget(actor);
}

static void cultThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(actor, &cultistGoto);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(actor, &cultistSwimGoto);
            break;
        }
        return;
    }
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
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
            aiNewState(actor, &cultistSearch);
            if (pSprite->type == kDudeCultistTommy)
                aiPlay3DSound(pSprite, 4021+Random(4), AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1021+Random(4), AI_SFX_PRIORITY_1, -1);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(actor, &cultistSwimSearch);
            break;
        }
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        switch (pXSprite->medium)
        {
        case kMediumNormal:
            aiNewState(actor, &cultistSearch);
            break;
        case kMediumWater:
        case kMediumGoo:
            aiNewState(actor, &cultistSwimSearch);
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
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                actor->dudeSlope = DivScale(pTarget->z-pSprite->z, nDist, 10);
                switch (pSprite->type) {
                case kDudeCultistTommy:
                    if (nDist < 0x1e00 && nDist > 0xe00 && abs(nDeltaAngle) < 85 && !TargetNearExplosion(pTarget)
                        && (pTarget->flags&2) && gGameOptions.nDifficulty > 2 && IsPlayerSprite(pTarget) && gPlayer[pTarget->type-kDudePlayer1].isRunning
                        && Chance(0x8000))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistTThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistTThrow);
                            break;
                        default:
                            aiNewState(actor, &cultistTThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x4600 && abs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTProneFire);
                            else if (dudeIsPlayingSeq(pSprite, 13) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
                                aiNewState(actor, &cultistTSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun)
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistTFire);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistTProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistTSwimFire);
                            }
                            else
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistDodge);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistTSwimFire);
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
                                aiNewState(actor, &cultistSThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistSThrow);
                            break;
                        default:
                            aiNewState(actor, &cultistSThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && abs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistSSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistSFire);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistSProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSSwimFire);
                            }
                            else
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistDodge);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistSSwimFire);
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
                                aiNewState(actor, &cultistTsThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistTsThrow);
                            break;
                        default:
                            aiNewState(actor, &cultistTsThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && abs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTsFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTsProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistTsSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistTsFire);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistTsProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistTsSwimFire);
                            }
                            else
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistDodge);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTsFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistTsProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistTsSwimFire);
                            break;
                        }
                    }
                    break;
                case kDudeCultistTNT:
                    if (nDist < 0x2c00 && nDist > 0x1400 && abs(nDeltaAngle) < 85
                        && (pTarget->flags&2) && IsPlayerSprite(pTarget))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistDThrow);
                            break;
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistDThrow);
                            break;
                        default:
                            aiNewState(actor, &cultistDThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x1400 && abs(nDeltaAngle) < 85
                        && (pTarget->flags&2) && IsPlayerSprite(pTarget))
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (pXSprite->medium != 1 && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultist139A78);
                            break;
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultist139A78);
                            break;
                        default:
                            aiNewState(actor, &cultist139A78);
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
                                aiNewState(actor, &cultistSThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistShotgun && pXSprite->medium != kMediumWater && pXSprite->medium != kMediumGoo)
                                aiNewState(actor, &cultistSThrow);
                            break;
                        default:
                            aiNewState(actor, &cultistSThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x3200 && abs(nDeltaAngle) < 28)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistSSwimFire);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeCultistTommy)
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistSFire);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistSProneFire);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSSwimFire);
                            }
                            else
                            {
                                if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistDodge);
                                else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                    aiNewState(actor, &cultistProneDodge);
                                else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                    aiNewState(actor, &cultistSwimDodge);
                            }
                            break;
                        default:
                            if (!dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSFire);
                            else if (dudeIsPlayingSeq(pSprite, 14) && pXSprite->medium == kMediumNormal)
                                aiNewState(actor, &cultistSProneFire);
                            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                                aiNewState(actor, &cultistSSwimFire);
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
        aiNewState(actor, &cultistGoto);
        break;
    case kMediumWater:
    case kMediumGoo:
        aiNewState(actor, &cultistSwimGoto);
        break;
    }
    pXSprite->target = -1;
}

END_BLD_NS
