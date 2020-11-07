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
#include "bloodactor.h"

BEGIN_BLD_NS

static void aiPodSearch(DBloodActor* actor);
static void aiPodMove(DBloodActor* actor);
static void aiPodChase(DBloodActor* actor);

AISTATE podIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE podMove = { kAiStateMove, 7, -1, 3600, NULL, aiMoveTurn, aiPodMove, &podSearch };
AISTATE podSearch = { kAiStateSearch, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, &podSearch };
AISTATE podStartChase = { kAiStateChase, 8, nPodStartChaseClient, 600, NULL, NULL, NULL, &podChase };
AISTATE podRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &podChase };
AISTATE podChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };
AISTATE tentacleIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tentacle13A6A8 = { kAiStateOther, 7, dword_279B3C, 0, NULL, NULL, NULL, &tentacle13A6C4 };
AISTATE tentacle13A6C4 = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacle13A6E0 = { kAiStateOther, 8, dword_279B40, 0, NULL, NULL, NULL, &tentacle13A6FC };
AISTATE tentacle13A6FC = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleIdle };
AISTATE tentacleMove = { kAiStateOther, 8, -1, 3600, NULL, aiMoveTurn, aiPodMove, &tentacleSearch };
AISTATE tentacleSearch = { kAiStateOther, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, NULL };
AISTATE tentacleStartChase = { kAiStateOther, 6, nTentacleStartSearchClient, 120, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };

void sub_6FF08(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    sfxPlay3DSound(&sprite[nSprite], 2503, -1, 0);
}

void sub_6FF54(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    sfxPlay3DSound(&sprite[nSprite], 2500, -1, 0);
}

void podAttack(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    ///assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites)) {
        Printf(PRINT_HIGH, "pXSprite->target >= 0 && pXSprite->target < kMaxSprites");
        return;
    }
    
    spritetype *pTarget = &sprite[pXSprite->target];

    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int x = pTarget->x-pSprite->x;
    int y = pTarget->y-pSprite->y;
    int dz = pTarget->z-pSprite->z;
    x += Random2(1000);
    y += Random2(1000);
    int nDist = approxDist(x, y);
    int nDist2 = nDist / 540;
    spritetype *pMissile = NULL;
    switch (pSprite->type)
    {
    case kDudePodGreen:
        dz += 8000;
        if (pDudeInfo->seeDist*0.1 < nDist)
        {
            if (Chance(0x8000))
                sfxPlay3DSound(pSprite, 2474, -1, 0);
            else
                sfxPlay3DSound(pSprite, 2475, -1, 0);
            pMissile = actFireThing(pSprite, 0, -8000, dz/128-14500, kThingPodGreenBall, (nDist2<<23)/120);
        }
        if (pMissile)
            seqSpawn(68, 3, pMissile->extra, -1);
        break;
    case kDudePodFire:
        dz += 8000;
        if (pDudeInfo->seeDist*0.1 < nDist)
        {
            sfxPlay3DSound(pSprite, 2454, -1, 0);
            pMissile = actFireThing(pSprite, 0, -8000, dz/128-14500, kThingPodFireBall, (nDist2<<23)/120);
        }
        if (pMissile)
            seqSpawn(22, 3, pMissile->extra, -1);
        break;
    }
    for (int i = 0; i < 4; i++)
        sub_746D4(pSprite, 240);
}

void sub_70284(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    sfxPlay3DSound(pSprite, 2502, -1, 0);
    int nDist, nBurn;
    DAMAGE_TYPE dmgType;
    switch (pSprite->type) {
        case kDudeTentacleGreen:
        default: // ???
            nBurn = 0;
            dmgType = DAMAGE_TYPE_2;
            nDist = 50;
            break;
        case kDudeTentacleFire: // ???
            nBurn = (gGameOptions.nDifficulty*120)>>2;
            dmgType = DAMAGE_TYPE_3;
            nDist = 75;
            break;
    }
    sub_2A620(nSprite, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, nDist, 1, 5*(1+gGameOptions.nDifficulty), dmgType, 2, nBurn, 0, 0);
}

static void aiPodSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void aiPodMove(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery) {
        switch (pSprite->type) {
            case kDudePodGreen:
            case kDudePodFire:
                aiNewState(actor, &podSearch);
                break;
            case kDudeTentacleGreen:
            case kDudeTentacleFire:
                aiNewState(actor, &tentacleSearch);
                break;
        }
    }
    aiThinkTarget(actor);
}

static void aiPodChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1) {
        switch (pSprite->type) {
            case kDudePodGreen:
            case kDudePodFire:
                aiNewState(actor, &podMove);
                break;
            case kDudeTentacleGreen:
            case kDudeTentacleFire:
                aiNewState(actor, &tentacleMove);
                break;
        }
        return;
    }
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    ///assert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites)) {
        Printf(PRINT_HIGH, "pXSprite->target >= 0 && pXSprite->target < kMaxSprites");
        return;
    }
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0) {
        
        switch (pSprite->type) {
            case kDudePodGreen:
            case kDudePodFire:
                aiNewState(actor, &podSearch);
                break;
            case kDudeTentacleGreen:
            case kDudeTentacleFire:
                aiNewState(actor, &tentacleSearch);
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
                if (klabs(nDeltaAngle) < 85 && pTarget->type != kDudePodGreen && pTarget->type != kDudePodFire) {
                    switch (pSprite->type) {
                        case kDudePodGreen:
                        case kDudePodFire:
                            aiNewState(actor, &podStartChase);
                            break;
                        case kDudeTentacleGreen:
                        case kDudeTentacleFire:
                            aiNewState(actor, &tentacleStartChase);
                            break;
                    }
                }
                return;
            }
        }
    }
    
    switch (pSprite->type) {
        case kDudePodGreen:
        case kDudePodFire:
            aiNewState(actor, &podMove);
            break;
        case kDudeTentacleGreen:
        case kDudeTentacleFire:
            aiNewState(actor, &tentacleMove);
            break;
    }
    pXSprite->target = -1;
}

END_BLD_NS
