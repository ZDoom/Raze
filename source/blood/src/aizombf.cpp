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

static void zombfThinkSearch(DBloodActor *actor);
static void zombfThinkGoto(DBloodActor *actor);
static void zombfThinkChase(DBloodActor *actor);


AISTATE zombieFIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombieFChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, zombfThinkChase, NULL };
AISTATE zombieFGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, zombfThinkGoto, &zombieFIdle };
AISTATE zombieFDodge = { kAiStateMove, 8, -1, 0, NULL, aiMoveDodge, zombfThinkChase, &zombieFChase };
AISTATE zombieFHack = { kAiStateChase, 6, nZombfHackClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFPuke = { kAiStateChase, 9, nZombfPukeClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFThrow = { kAiStateChase, 6, nZombfThrowClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFSearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, zombfThinkSearch, &zombieFIdle };
AISTATE zombieFRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &zombieFChase };

void zombfHackSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    if (pSprite->type != kDudeZombieButcher)
        return;
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int height = (pDudeInfo->eyeHeight*pSprite->yrepeat);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height2 = (pDudeInfoT->eyeHeight*pTarget->yrepeat);
    actFireVector(pSprite, 0, 0, CosScale16(pSprite->ang), SinScale16(pSprite->ang), height-height2, VECTOR_TYPE_11);
}

void PukeSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pDudeInfo->eyeHeight*pSprite->yrepeat);
    int height2 = (pDudeInfoT->eyeHeight*pTarget->yrepeat);
    int tx = pXSprite->targetX-pSprite->x;
    int ty = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(tx, ty);
    int dx = CosScale16(nAngle);
    int dy = SinScale16(nAngle);
    sfxPlay3DSound(pSprite, 1203, 1, 0);
    actFireMissile(pSprite, 0, -(height-height2), dx, dy, 0, kMissilePukeGreen);
}

void ThrowSeqCallback(int, DBloodActor* actor)
{
    spritetype* pSprite = &actor->s();
    actFireMissile(pSprite, 0, -getDudeInfo(pSprite->type)->eyeHeight, CosScale16(pSprite->ang), SinScale16(pSprite->ang), 0, kMissileButcherKnife);
}

static void zombfThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void zombfThinkGoto(DBloodActor* actor)
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
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(actor, &zombieFSearch);
    aiThinkTarget(actor);
}

static void zombfThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &zombieFGoto);
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
        aiNewState(actor, &zombieFSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && (powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpDeathMaskUseless) > 0))
    {
        aiNewState(actor, &zombieFSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                if (nDist < 0x1400 && nDist > 0xe00 && klabs(nDeltaAngle) < 85)
                {
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        aiNewState(actor, &zombieFThrow);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(actor, &zombieFThrow);
                        else
                            aiNewState(actor, &zombieFDodge);
                        break;
                    default:
                        aiNewState(actor, &zombieFThrow);
                        break;
                    }
                }
                else if (nDist < 0x1400 && nDist > 0x600 && klabs(nDeltaAngle) < 85)
                {
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        aiNewState(actor, &zombieFPuke);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(actor, &zombieFPuke);
                        else
                            aiNewState(actor, &zombieFDodge);
                        break;
                    default:
                        aiNewState(actor, &zombieFPuke);
                        break;
                    }
                }
                else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                {
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        aiNewState(actor, &zombieFHack);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(actor, &zombieFHack);
                        else
                            aiNewState(actor, &zombieFDodge);
                        break;
                    default:
                        aiNewState(actor, &zombieFHack);
                        break;
                    }
                }
                return;
            }
        }
    }

    aiNewState(actor, &zombieFSearch);
    pXSprite->target = -1;
}

END_BLD_NS
