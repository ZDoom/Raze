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
#include "blood.h"
#include "db.h"
#include "dude.h"
#include "player.h"
#include "seq.h"

BEGIN_BLD_NS

static void BiteSeqCallback(int, int);
static void thinkTarget(spritetype *, XSPRITE *);
static void thinkSearch(spritetype *, XSPRITE *);
static void thinkGoto(spritetype *, XSPRITE *);
static void thinkPonder(spritetype *, XSPRITE *);
static void MoveDodgeUp(spritetype *, XSPRITE *);
static void MoveDodgeDown(spritetype *, XSPRITE *);
static void thinkChase(spritetype *, XSPRITE *);
static void MoveForward(spritetype *, XSPRITE *);
static void MoveSwoop(spritetype *, XSPRITE *);
static void MoveFly(spritetype *, XSPRITE *);
static void MoveToCeil(spritetype *, XSPRITE *);

static int nBiteClient = seqRegisterClient(BiteSeqCallback);

AISTATE batIdle = {kAiStateIdle, 0, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE batFlyIdle = {kAiStateIdle, 6, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE batChase = {kAiStateChase, 6, -1, 0, NULL, MoveForward, thinkChase, &batFlyIdle };
AISTATE batPonder = {kAiStateOther, 6, -1, 0, NULL, NULL, thinkPonder, NULL };
AISTATE batGoto = {kAiStateMove, 6, -1, 600, NULL, MoveForward, thinkGoto, &batFlyIdle };
AISTATE batBite = {kAiStateChase, 7, nBiteClient, 60, NULL, NULL, NULL, &batPonder };
AISTATE batRecoil = {kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &batChase };
AISTATE batSearch = {kAiStateSearch, 6, -1, 120, NULL, MoveForward, thinkSearch, &batFlyIdle };
AISTATE batSwoop = {kAiStateOther, 6, -1, 60, NULL, MoveSwoop, thinkChase, &batChase };
AISTATE batFly = { kAiStateMove, 6, -1, 0, NULL, MoveFly, thinkChase, &batChase };
AISTATE batTurn = {kAiStateMove, 6, -1, 60, NULL, aiMoveTurn, NULL, &batChase };
AISTATE batHide = {kAiStateOther, 6, -1, 0, NULL, MoveToCeil, MoveForward, NULL };
AISTATE batDodgeUp = {kAiStateMove, 6, -1, 120, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpRight = {kAiStateMove, 6, -1, 90, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpLeft = {kAiStateMove, 6, -1, 90, NULL, MoveDodgeUp, 0, &batChase };
AISTATE batDodgeDown = {kAiStateMove, 6, -1, 120, NULL, MoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownRight = {kAiStateMove, 6, -1, 90, NULL, MoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownLeft = {kAiStateMove, 6, -1, 90, NULL, MoveDodgeDown, 0, &batChase };

static void BiteSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    spritetype *pSprite = &sprite[pXSprite->reference];
    spritetype *pTarget = &sprite[pXSprite->target];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pSprite->yrepeat*pDudeInfo->eyeHeight)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->eyeHeight)<<2;
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    actFireVector(pSprite, 0, 0, dx, dy, height2-height, VECTOR_TYPE_6);
}

static void thinkTarget(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
    if (pDudeExtraE->at8 && pDudeExtraE->at4 < 10)
        pDudeExtraE->at4++;
    else if (pDudeExtraE->at4 >= 10 && pDudeExtraE->at8)
    {
        pDudeExtraE->at4 = 0;
        pXSprite->goalAng += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(pSprite, pXSprite, &batTurn);
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
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(pSprite, pXSprite);
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(pSprite, pXSprite);
            }
            else
                continue;
            break;
        }
    }
}

static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    thinkTarget(pSprite, pXSprite);
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
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(pSprite, pXSprite, &batSearch);
    thinkTarget(pSprite, pXSprite);
}

static void thinkPonder(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &batSearch);
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
        aiNewState(pSprite, pXSprite, &batSearch);
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
            if (height2-height < 0x3000 && nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x5000 && nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else if (height2-height < 0x2000 && nDist < 0x200 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x6000 && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else if (height2-height < 0x2000 && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height < 0x2000 && klabs(nDeltaAngle) < 85 && nDist > 0x1400)
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            else if (height2-height > 0x4000)
                aiNewState(pSprite, pXSprite, &batDodgeDown);
            else
                aiNewState(pSprite, pXSprite, &batDodgeUp);
            return;
        }
    }
    aiNewState(pSprite, pXSprite, &batGoto);
    pXSprite->target = -1;
}

static void MoveDodgeUp(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -0x52aaa;
}

static void MoveDodgeDown(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->dodgeDir == 0)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = 0x44444;
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &batGoto);
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
        aiNewState(pSprite, pXSprite, &batSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(pSprite, pXSprite, &batSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        // Should be dudeInfo[pTarget->type-kDudeBase]
        int height2 = (pDudeInfo->eyeHeight*pTarget->yrepeat)<<2;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                int floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                if (height2-height < 0x2000 && nDist < 0x200 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batBite);
                else if ((height2-height > 0x5000 || floorZ-bottom > 0x5000) && nDist < 0x1400 && nDist > 0x800 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batSwoop);
                else if ((height2-height < 0x3000 || floorZ-bottom < 0x3000) && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &batFly);
                return;
            }
        }
        else
        {
            aiNewState(pSprite, pXSprite, &batFly);
            return;
        }
    }

    pXSprite->target = -1;
    aiNewState(pSprite, pXSprite, &batHide);
}

static void MoveForward(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->frontSpeed<<2;
    if (klabs(nAng) > 341)
        return;
    if (pXSprite->target == -1)
        pSprite->ang = (pSprite->ang+256)&2047;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if ((unsigned int)Random(64) < 32 && nDist <= 0x200)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void MoveSwoop(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->frontSpeed<<2;
    if (klabs(nAng) > 341)
    {
        pXSprite->goalAng = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x200)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = 0x44444;
}

static void MoveFly(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nAccel = pDudeInfo->frontSpeed<<2;
    if (klabs(nAng) > 341)
    {
        pSprite->ang = (pSprite->ang+512)&2047;
        return;
    }
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x200)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel>>1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -0x2d555;
}

void MoveToCeil(spritetype *pSprite, XSPRITE *pXSprite)
{
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int nSector = pSprite->sectnum;
    if (z - pXSprite->targetZ < 0x1000)
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 0;
        pSprite->flags = 0;
        aiNewState(pSprite, pXSprite, &batIdle);
    }
    else
        aiSetTarget(pXSprite, x, y, sector[nSector].ceilingz);
}

END_BLD_NS
