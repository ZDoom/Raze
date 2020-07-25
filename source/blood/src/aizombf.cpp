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
#include "eventq.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "trig.h"

BEGIN_BLD_NS

static void HackSeqCallback(int, int);
static void PukeSeqCallback(int, int);
static void ThrowSeqCallback(int, int);
static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite);
static void thinkGoto(spritetype *pSprite, XSPRITE *pXSprite);
static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite);

static int nHackClient = seqRegisterClient(HackSeqCallback);
static int nPukeClient = seqRegisterClient(PukeSeqCallback);
static int nThrowClient = seqRegisterClient(ThrowSeqCallback);

AISTATE zombieFIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombieFChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE zombieFGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, thinkGoto, &zombieFIdle };
AISTATE zombieFDodge = { kAiStateMove, 8, -1, 0, NULL, aiMoveDodge, thinkChase, &zombieFChase };
AISTATE zombieFHack = { kAiStateChase, 6, nHackClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFPuke = { kAiStateChase, 9, nPukeClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFThrow = { kAiStateChase, 6, nThrowClient, 120, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFSearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, thinkSearch, &zombieFIdle };
AISTATE zombieFRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &zombieFChase };
AISTATE zombieFTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &zombieFChase };

static void HackSeqCallback(int, int nXSprite)
{
    if (nXSprite <= 0 || nXSprite >= kMaxXSprites)
        return;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    if (nXSprite < 0 || nXSprite >= kMaxSprites)
        return;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->type != kDudeZombieButcher)
        return;
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int height = (pDudeInfo->eyeHeight*pSprite->yrepeat);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height2 = (pDudeInfoT->eyeHeight*pTarget->yrepeat);
    actFireVector(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, height-height2, VECTOR_TYPE_11);
}

static void PukeSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pDudeInfo->eyeHeight*pSprite->yrepeat);
    int height2 = (pDudeInfoT->eyeHeight*pTarget->yrepeat);
    int tx = pXSprite->targetX-pSprite->x;
    int ty = pXSprite->targetY-pSprite->y;
    int UNUSED(nDist) = approxDist(tx, ty);
    int nAngle = getangle(tx, ty);
    int dx = Cos(nAngle)>>16;
    int dy = Sin(nAngle)>>16;
    sfxPlay3DSound(pSprite, 1203, 1, 0);
    actFireMissile(pSprite, 0, -(height-height2), dx, dy, 0, kMissilePukeGreen);
}

static void ThrowSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    actFireMissile(pSprite, 0, -getDudeInfo(pSprite->type)->eyeHeight, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, kMissileButcherKnife);
}

static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(pSprite, pXSprite);
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
        aiNewState(pSprite, pXSprite, &zombieFSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &zombieFGoto);
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
        aiNewState(pSprite, pXSprite, &zombieFSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && (powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpDeathMaskUseless) > 0))
    {
        aiNewState(pSprite, pXSprite, &zombieFSearch);
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
                        aiNewState(pSprite, pXSprite, &zombieFThrow);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(pSprite, pXSprite, &zombieFThrow);
                        else
                            aiNewState(pSprite, pXSprite, &zombieFDodge);
                        break;
                    default:
                        aiNewState(pSprite, pXSprite, &zombieFThrow);
                        break;
                    }
                }
                else if (nDist < 0x1400 && nDist > 0x600 && klabs(nDeltaAngle) < 85)
                {
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        aiNewState(pSprite, pXSprite, &zombieFPuke);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(pSprite, pXSprite, &zombieFPuke);
                        else
                            aiNewState(pSprite, pXSprite, &zombieFDodge);
                        break;
                    default:
                        aiNewState(pSprite, pXSprite, &zombieFPuke);
                        break;
                    }
                }
                else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                {
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        aiNewState(pSprite, pXSprite, &zombieFHack);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            aiNewState(pSprite, pXSprite, &zombieFHack);
                        else
                            aiNewState(pSprite, pXSprite, &zombieFDodge);
                        break;
                    default:
                        aiNewState(pSprite, pXSprite, &zombieFHack);
                        break;
                    }
                }
                return;
            }
        }
    }

    aiNewState(pSprite, pXSprite, &zombieFSearch);
    pXSprite->target = -1;
}

END_BLD_NS
