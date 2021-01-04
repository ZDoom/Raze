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
#include "pragmas.h"
#include "mmulti.h"

#include "blood.h"

BEGIN_BLD_NS

static void eelThinkTarget(DBloodActor *);
static void eelThinkSearch(DBloodActor *);
static void eelThinkGoto(DBloodActor *);
static void eelThinkPonder(DBloodActor *);
static void eelMoveDodgeUp(DBloodActor *);
static void eelMoveDodgeDown(DBloodActor *);
static void eelThinkChase(DBloodActor *);
static void eelMoveForward(DBloodActor *);
static void eelMoveSwoop(DBloodActor *);
static void eelMoveAscend(DBloodActor *actor);
static void eelMoveToCeil(DBloodActor *);


AISTATE eelIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelFlyIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelChase = { kAiStateChase, 0, -1, 0, NULL, eelMoveForward, eelThinkChase, &eelIdle };
AISTATE eelPonder = { kAiStateOther, 0, -1, 0, NULL, NULL, eelThinkPonder, NULL };
AISTATE eelGoto = { kAiStateMove, 0, -1, 600, NULL, NULL, eelThinkGoto, &eelIdle };
AISTATE eelBite = { kAiStateChase, 7, nEelBiteClient, 60, NULL, NULL, NULL, &eelChase };
AISTATE eelRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &eelChase };
AISTATE eelSearch = { kAiStateSearch, 0, -1, 120, NULL, eelMoveForward, eelThinkSearch, &eelIdle };
AISTATE eelSwoop = { kAiStateOther, 0, -1, 60, NULL, eelMoveSwoop, eelThinkChase, &eelChase };
AISTATE eelFly = { kAiStateMove, 0, -1, 0, NULL, eelMoveAscend, eelThinkChase, &eelChase };
AISTATE eelTurn = { kAiStateMove, 0, -1, 60, NULL, aiMoveTurn, NULL, &eelChase };
AISTATE eelHide = { kAiStateOther, 0, -1, 0, NULL, eelMoveToCeil, eelMoveForward, NULL };
AISTATE eelDodgeUp = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeDown = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };

void eelBiteSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype *pSprite = &actor->s();
    spritetype *pTarget = &sprite[pXSprite->target];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pSprite->yrepeat*pDudeInfo->eyeHeight)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->eyeHeight)<<2;
    /*
     * workaround for 
     * pXSprite->target >= 0 && pXSprite->target < kMaxSprites in file NBlood/source/blood/src/aiboneel.cpp at line 86
     * The value of pXSprite->target is -1. 
     * copied from lines 177:181
     * resolves this case, but may cause other issues? 
     */
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &eelSearch);
        return;
    }
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    actFireVector(pSprite, 0, 0, dx, dy, height2-height, VECTOR_TYPE_7);
}

static void eelThinkTarget(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
    if (pDudeExtraE->xval3 && pDudeExtraE->xval2 < 10)
        pDudeExtraE->xval2++;
    else if (pDudeExtraE->xval2 >= 10 && pDudeExtraE->xval3)
    {
        pDudeExtraE->xval2 = 0;
        pXSprite->goalAng += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(actor, &eelTurn);
        return;
    }
    if (Chance(pDudeInfo->alertChance))
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            PLAYER *pPlayer = &gPlayer[p];
            if (pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
                continue;
            int x = pPlayer->pSprite->x;
            int y = pPlayer->pSprite->y;
            int z = pPlayer->pSprite->z;
            int nSector = pPlayer->pSprite->sectnum;
            int dx = x-pSprite->x;
            int dy = y-pSprite->y;
            int nDist = approxDist(dx, dy);
            if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
                continue;
            if (!cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z-((pDudeInfo->eyeHeight*pSprite->yrepeat)<<2), pSprite->sectnum))
                continue;
            int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                pDudeExtraE->xval2 = 0;
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(&bloodActors[pXSprite->reference]);
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                pDudeExtraE->xval2 = 0;
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(&bloodActors[pXSprite->reference]);
            }
            else
                continue;
            break;
        }
    }
}

static void eelThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    eelThinkTarget(actor);
}

static void eelThinkGoto(DBloodActor* actor)
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
        aiNewState(actor, &eelSearch);
    eelThinkTarget(actor);
}

static void eelThinkPonder(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &eelSearch);
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
        aiNewState(actor, &eelSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        int height2 = (getDudeInfo(pTarget->type)->eyeHeight*pTarget->yrepeat)<<2;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            aiSetTarget(pXSprite, pXSprite->target);
            if (height2-height < -0x2000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
                aiNewState(actor, &eelDodgeUp);
            else if (height2-height > 0xccc && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
                aiNewState(actor, &eelDodgeDown);
            else if (height2-height < 0xccc && nDist < 0x399 && abs(nDeltaAngle) < 85)
                aiNewState(actor, &eelDodgeUp);
            else if (height2-height > 0xccc && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
                aiNewState(actor, &eelDodgeDown);
            else if (height2-height < -0x2000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
                aiNewState(actor, &eelDodgeUp);
            else if (height2-height < -0x2000 && abs(nDeltaAngle) < 85 && nDist > 0x1400)
                aiNewState(actor, &eelDodgeUp);
            else if (height2-height > 0xccc)
                aiNewState(actor, &eelDodgeDown);
            else
                aiNewState(actor, &eelDodgeUp);
            return;
        }
    }
    aiNewState(actor, &eelGoto);
    pXSprite->target = -1;
}

static void eelMoveDodgeUp(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = actor->xvel();
    int dy = actor->yvel();
    int t1 = DMulScale(dx, nCos, dy, nSin, 30);
    int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel() = -0x8000;
}

static void eelMoveDodgeDown(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->dodgeDir == 0)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = actor->xvel();
    int dy = actor->yvel();
    int t1 = DMulScale(dx, nCos, dy, nSin, 30);
    int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel() = 0x44444;
}

static void eelThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &eelGoto);
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
        aiNewState(actor, &eelSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(actor, &eelSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int top2, bottom2;
        GetSpriteExtents(pTarget, &top2, &bottom2);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                if (nDist < 0x399 && top2 > top && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &eelSwoop);
                else if (nDist <= 0x399 && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &eelBite);
                else if (bottom2 > top && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &eelSwoop);
                else if (top2 < top && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &eelFly);
            }
        }
        return;
    }

    pXSprite->target = -1;
    aiNewState(actor, &eelSearch);
}

static void eelMoveForward(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<26)/120)/120)<<2;
    if (abs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (nDist <= 0x399)
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
        t1 += nAccel>>1;
    actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void eelMoveSwoop(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<26)/120)/120)<<2;
    if (abs(nAng) > 341)
        return;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (Chance(0x8000) && nDist <= 0x399)
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
    actor->zvel() = 0x22222;
}

static void eelMoveAscend(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = (pDudeInfo->frontSpeed-(((4-gGameOptions.nDifficulty)<<26)/120)/120)<<2;
    if (abs(nAng) > 341)
        return;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x399)
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
    actor->zvel() = -0x8000;
}

void eelMoveToCeil(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector = pSprite->sectnum;
    if (z - pXSprite->targetZ < 0x1000)
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 0;
        pSprite->flags = 0;
        aiNewState(actor, &eelIdle);
    }
    else
        aiSetTarget(pXSprite, x, y, sector[nSector].ceilingz);
}

END_BLD_NS
