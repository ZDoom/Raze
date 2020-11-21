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

static void zombaThinkSearch(spritetype *, XSPRITE *);
static void zombaThinkGoto(spritetype *, XSPRITE *);
static void zombaThinkChase(spritetype *, XSPRITE *);
static void zombaThinkPonder(spritetype *, XSPRITE *);
static void myThinkTarget(spritetype *, XSPRITE *);
static void myThinkSearch(spritetype *, XSPRITE *);
static void entryEZombie(spritetype *, XSPRITE *);
static void entryAIdle(spritetype *, XSPRITE *);
static void entryEStand(spritetype *, XSPRITE *);


AISTATE zombieAIdle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, aiThinkTarget, NULL };
AISTATE zombieAChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, zombaThinkChase, NULL };
AISTATE zombieAPonder = { kAiStateOther, 0, -1, 0, NULL, aiMoveTurn, zombaThinkPonder, NULL };
AISTATE zombieAGoto = { kAiStateMove, 8, -1, 1800, NULL, aiMoveForward, zombaThinkGoto, &zombieAIdle };
AISTATE zombieAHack = { kAiStateChase, 6, nHackClient, 80, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieASearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, zombaThinkSearch, &zombieAIdle };
AISTATE zombieARecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieATeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieARecoil2 = { kAiStateRecoil, 1, -1, 360, NULL, NULL, NULL, &zombieAStand };
AISTATE zombieAStand = { kAiStateMove, 11, nStandClient, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieEIdle = { kAiStateIdle, 12, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombieEUp2 = { kAiStateMove, 0, -1, 1, entryEZombie, NULL, NULL, &zombieASearch };
AISTATE zombieEUp = { kAiStateMove, 9, -1, 180, entryEStand, NULL, NULL, &zombieEUp2 };
AISTATE zombie2Idle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, myThinkTarget, NULL };
AISTATE zombie2Search = { kAiStateSearch, 8, -1, 1800, NULL, NULL, myThinkSearch, &zombie2Idle };
AISTATE zombieSIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombie13AC2C = { kAiStateOther, 11, nStandClient, 0, entryEZombie, NULL, NULL, &zombieAPonder };

void HackSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int tx = pXSprite->targetX-pSprite->x;
    int ty = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(tx, ty);
    int height = (pSprite->yrepeat*pDudeInfo->eyeHeight)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->eyeHeight)<<2;
    int dz = height-height2;
    int dx = CosScale16(nAngle);
    int dy = SinScale16(nAngle);
    sfxPlay3DSound(pSprite, 1101, 1, 0);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_10);
}

void StandSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    sfxPlay3DSound(&sprite[nSprite], 1102, -1, 0);
}

static void zombaThinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    sub_5F15C(pSprite, pXSprite);
}

static void zombaThinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 921 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(pSprite, pXSprite, &zombieASearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void zombaThinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &zombieASearch);
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
        aiNewState(pSprite, pXSprite, &zombieASearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && (powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpDeathMaskUseless) > 0))
    {
        aiNewState(pSprite, pXSprite, &zombieAGoto);
        return;
    }
    // If the zombie gets whacked while rising from the grave it never executes this change and if it isn't done here at the very latest, will just aimlessly run around.
    if (!VanillaMode() && pSprite->type == kDudeZombieAxeBuried) 
        pSprite->type = kDudeZombieAxeNormal;

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
                if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &zombieAHack);
                return;
            }
        }
    }

    aiNewState(pSprite, pXSprite, &zombieAGoto);
    pXSprite->target = -1;
}

static void zombaThinkPonder(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &zombieASearch);
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
        aiNewState(pSprite, pXSprite, &zombieASearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && (powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpDeathMaskUseless) > 0))
    {
        aiNewState(pSprite, pXSprite, &zombieAGoto);
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
                if (nDist < 0x400)
                {
                    if (klabs(nDeltaAngle) < 85)
                    {
                        sfxPlay3DSound(pSprite, 1101, 1, 0);
                        aiNewState(pSprite, pXSprite, &zombieAHack);
                    }
                    return;
                }
            }
        }
    }

    aiNewState(pSprite, pXSprite, &zombieAChase);
}

static void myThinkTarget(spritetype *pSprite, XSPRITE *pXSprite)
{
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        PLAYER *pPlayer = &gPlayer[p];
        int nOwner = (pSprite->owner & 0x1000) ? (pSprite->owner&0xfff) : -1;
        if (nOwner == pPlayer->nSprite || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
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

static void myThinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    myThinkTarget(pSprite, pXSprite);
}

static void entryEZombie(spritetype *pSprite, XSPRITE *pXSprite)
{
    UNREFERENCED_PARAMETER(pXSprite);
    pSprite->type = kDudeZombieAxeNormal;
    pSprite->flags |= 1;
}

static void entryAIdle(spritetype *pSprite, XSPRITE *pXSprite)
{
    UNREFERENCED_PARAMETER(pSprite);
    pXSprite->target = -1;
}

static void entryEStand(spritetype *pSprite, XSPRITE *pXSprite)
{
    sfxPlay3DSound(pSprite, 1100, -1, 0);
    pSprite->ang = getangle(pXSprite->targetX-pSprite->x, pXSprite->targetY-pSprite->y);
}

END_BLD_NS
