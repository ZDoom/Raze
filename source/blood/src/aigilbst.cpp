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

#include "blood.h"

BEGIN_BLD_NS

static void gillThinkSearch(DBloodActor *);
static void gillThinkGoto(DBloodActor *);
static void gillThinkChase(DBloodActor *);
static void gillThinkSwimGoto(DBloodActor *);
static void gillThinkSwimChase(DBloodActor *);
static void sub_6CB00(DBloodActor *);
static void sub_6CD74(DBloodActor *);
static void sub_6D03C(DBloodActor *);


AISTATE gillBeastIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastChase = { kAiStateChase, 9, -1, 0, NULL, aiMoveForward, gillThinkChase, NULL };
AISTATE gillBeastDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastChase };
AISTATE gillBeastGoto = { kAiStateMove, 9, -1, 600, NULL, aiMoveForward, gillThinkGoto, &gillBeastIdle };
AISTATE gillBeastBite = { kAiStateChase, 6, nGillBiteClient, 120, NULL, NULL, NULL, &gillBeastChase };
AISTATE gillBeastSearch = { kAiStateMove, 9, -1, 120, NULL, aiMoveForward, gillThinkSearch, &gillBeastIdle };
AISTATE gillBeastRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gillBeastDodge };
AISTATE gillBeastSwimIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastSwimChase = { kAiStateChase, 10, -1, 0, NULL, sub_6CB00, gillThinkSwimChase, NULL };
AISTATE gillBeastSwimDodge = { kAiStateMove, 10, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastSwimChase };
AISTATE gillBeastSwimGoto = { kAiStateMove, 10, -1, 600, NULL, aiMoveForward, gillThinkSwimGoto, &gillBeastSwimIdle };
AISTATE gillBeastSwimSearch = { kAiStateSearch, 10, -1, 120, NULL, aiMoveForward, gillThinkSearch, &gillBeastSwimIdle };
AISTATE gillBeastSwimBite = { kAiStateChase, 7, nGillBiteClient, 0, NULL, NULL, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeastSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gillBeastSwimDodge };
AISTATE gillBeast13A138 = { kAiStateOther, 10, -1, 120, NULL, sub_6CD74, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A154 = { kAiStateOther, 10, -1, 0, NULL, sub_6D03C, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A170 = { kAiStateOther, 10, -1, 120, NULL, NULL, aiMoveTurn, &gillBeastSwimChase };

void GillBiteSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    spritetype *pTarget = &sprite[pXSprite->target];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int dz = pSprite->z-pTarget->z;
    dx += Random3(2000);
    dy += Random3(2000);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_8);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_8);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_8);
}

static void gillThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void gillThinkGoto(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
    {
        if (pXSector && pXSector->Underwater)
            aiNewState(actor, &gillBeastSwimSearch);
        else
            aiNewState(actor, &gillBeastSearch);
    }
    aiThinkTarget(actor);
}

static void gillThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(actor, &gillBeastSwimSearch);
        else
            aiNewState(actor, &gillBeastSearch);
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
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(actor, &gillBeastSwimSearch);
        else
            aiNewState(actor, &gillBeastSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(actor, &gillBeastSwimSearch);
        else
            aiNewState(actor, &gillBeastSearch);
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
                if (nDist < 921 && abs(nDeltaAngle) < 28)
                {
                    XSECTOR *pXSector;
                    int nXSector = sector[pSprite->sectnum].extra;
                    if (nXSector > 0)
                        pXSector = &xsector[nXSector];
                    else
                        pXSector = NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        if (pXSector && pXSector->Underwater)
                            aiNewState(actor, &gillBeastSwimBite);
                        else
                            aiNewState(actor, &gillBeastBite);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                        {
                            if (pXSector && pXSector->Underwater)
                                aiNewState(actor, &gillBeastSwimBite);
                            else
                                aiNewState(actor, &gillBeastBite);
                        }
                        else
                        {
                            if (pXSector && pXSector->Underwater)
                                aiNewState(actor, &gillBeastSwimDodge);
                            else
                                aiNewState(actor, &gillBeastDodge);
                        }
                        break;
                    default:
                        if (pXSector && pXSector->Underwater)
                            aiNewState(actor, &gillBeastSwimBite);
                        else
                            aiNewState(actor, &gillBeastBite);
                        break;
                    }
                }
            }
            return;
        }
    }

    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    if (pXSector && pXSector->Underwater)
        aiNewState(actor, &gillBeastSwimGoto);
    else
        aiNewState(actor, &gillBeastGoto);
    sfxPlay3DSound(pSprite, 1701, -1, 0);
    pXSprite->target = -1;
}

static void gillThinkSwimGoto(DBloodActor* actor)
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
    if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(actor, &gillBeastSwimSearch);
    aiThinkTarget(actor);
}

static void gillThinkSwimChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &gillBeastSwimSearch);
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
        aiNewState(actor, &gillBeastSwimSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(actor, &gillBeastSwimSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = pDudeInfo->eyeHeight+pSprite->z;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                if (nDist < 0x400 && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &gillBeastSwimBite);
                else
                {
                    aiPlay3DSound(pSprite, 1700, AI_SFX_PRIORITY_1, -1);
                    aiNewState(actor, &gillBeast13A154);
                }
            }
        }
        else
            aiNewState(actor, &gillBeast13A154);
        return;
    }
    aiNewState(actor, &gillBeastSwimGoto);
    pXSprite->target = -1;
}

static void sub_6CB00(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (abs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (Random(64) < 32 && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = DMulScale(vx, nCos, vy, nSin, 30);
    int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>2;
    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void sub_6CD74(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    spritetype *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
    int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (abs(nAng) > 341)
    {
        pXSprite->goalAng = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int dz = z2 - z;
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = DMulScale(vx, nCos, vy, nSin, 30);
    int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
    t1 += nAccel;
    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel() = -dz;
}

static void sub_6D03C(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    spritetype *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
    int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<27)/120)/120)<<2;
    if (abs(nAng) > 341)
    {
        pSprite->ang = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int dz = (z2 - z)<<3;
    int nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = DMulScale(vx, nCos, vy, nSin, 30);
    int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
    t1 += nAccel>>1;
    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel() = dz;
}

END_BLD_NS
