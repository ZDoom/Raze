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

static void spidThinkSearch(DBloodActor *);
static void spidThinkGoto(DBloodActor *);
static void spidThinkChase(DBloodActor *);


AISTATE spidIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE spidChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, spidThinkChase, NULL };
AISTATE spidDodge = { kAiStateMove, 7, -1, 90, NULL, aiMoveDodge, NULL, &spidChase };
AISTATE spidGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, spidThinkGoto, &spidIdle };
AISTATE spidSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, spidThinkSearch, &spidIdle };
AISTATE spidBite = { kAiStateChase, 6, nSpidBiteClient, 60, NULL, NULL, NULL, &spidChase };
AISTATE spidJump = { kAiStateChase, 8, nSpidJumpClient, 60, NULL, aiMoveForward, NULL, &spidChase };
AISTATE spid13A92C = { kAiStateOther, 0, dword_279B50, 60, NULL, NULL, NULL, &spidIdle };

static char sub_70D30(XSPRITE *pXDude, int a2, int a3)
{
    assert(pXDude != NULL);
    int nDude = pXDude->reference;
    spritetype *pDude = &sprite[nDude];
    if (IsPlayerSprite(pDude))
    {
        a2 <<= 4;
        a3 <<= 4;
        if (IsPlayerSprite(pDude))
        {
            PLAYER *pPlayer = &gPlayer[pDude->type-kDudePlayer1];
            if (a3 > pPlayer->blindEffect)
            {
                pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect+a2, a3);
                return 1;
            }
        }
    }
    return 0;
}

void SpidBiteSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    dx += Random2(2000);
    dy += Random2(2000);
    int dz = Random2(2000);
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    if (IsPlayerSprite(pTarget)) {
        
        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
        if (hit == 3 && IsPlayerSprite(&sprite[gHitInfo.hitsprite])) {
            dz += pTarget->z - pSprite->z;
            PLAYER *pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
            switch (pSprite->type) {
                case kDudeSpiderBrown:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    if (IsPlayerSprite(pTarget) && !pPlayer->godMode && powerupCheck(pPlayer, kPwUpDeathMask) <= 0 && Chance(0x4000))
                        powerupActivate(pPlayer, kPwUpDeliriumShroom);
                    break;
                case kDudeSpiderRed:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    if (Chance(0x5000)) sub_70D30(pXTarget, 4, 16);
                    break;
                case kDudeSpiderBlack:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    sub_70D30(pXTarget, 8, 16);
                    break;
                case kDudeSpiderMother: {
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);

                    dx += Random2(2000);
                    dy += Random2(2000);
                    dz += Random2(2000);
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    sub_70D30(pXTarget, 8, 16);
                }
                    break;
            }
        }

    }
}

void SpidJumpSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    dx += Random2(200);
    dy += Random2(200);
    int dz = Random2(200);
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    if (IsPlayerSprite(pTarget)) {
        dz += pTarget->z-pSprite->z;
        switch (pSprite->type) {
            case kDudeSpiderBrown:
            case kDudeSpiderRed:
            case kDudeSpiderBlack:
                actor->xvel() = IntToFixed(dx);
                actor->yvel() = IntToFixed(dy);
                actor->zvel() = IntToFixed(dz);
                break;
        }
    }
}

void sub_71370(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    
    spritetype *pSpawn = NULL;
    if (IsPlayerSprite(pTarget) && pDudeExtraE->xval2 < 10) {
        
        if (nDist < 0x1a00 && nDist > 0x1400 && abs(pSprite->ang-nAngle) < pDudeInfo->periphery)
            pSpawn = actSpawnDude(pSprite, kDudeSpiderRed, pSprite->clipdist, 0);
        else if (nDist < 0x1400 && nDist > 0xc00 && abs(pSprite->ang-nAngle) < pDudeInfo->periphery)
            pSpawn = actSpawnDude(pSprite, kDudeSpiderBrown, pSprite->clipdist, 0);
        else if (nDist < 0xc00 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
            pSpawn = actSpawnDude(pSprite, kDudeSpiderBrown, pSprite->clipdist, 0);
        
        if (pSpawn) {
            pDudeExtraE->xval2++;
            pSpawn->owner = pSprite->index;
            gKillMgr.AddNewKill(1);
        }
    }

}

static void spidThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void spidThinkGoto(DBloodActor* actor)
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
        aiNewState(actor, &spidSearch);
    aiThinkTarget(actor);
}

static void spidThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &spidGoto);
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
        aiNewState(actor, &spidSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(actor, &spidSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist) {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum)) {
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery) {
                aiSetTarget(pXSprite, pXSprite->target);
                
                switch (pSprite->type) {
                    case kDudeSpiderRed:
                        if (nDist < 0x399 && abs(nDeltaAngle) < 85)
                            aiNewState(actor, &spidBite);
                        break;
                    case kDudeSpiderBrown:
                    case kDudeSpiderBlack:
                        if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
                            aiNewState(actor, &spidJump);
                        else if (nDist < 0x399 && abs(nDeltaAngle) < 85)
                            aiNewState(actor, &spidBite);
                        break;
                    case kDudeSpiderMother:
                        if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
                            aiNewState(actor, &spidJump);
                        else if (Chance(0x8000))
                            aiNewState(actor, &spid13A92C);
                        break;
                }

                return;
            }
        }
    }

    aiNewState(actor, &spidGoto);
    pXSprite->target = -1;
}

END_BLD_NS
