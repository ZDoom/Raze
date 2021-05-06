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

static void innocThinkSearch(DBloodActor *);
static void innocThinkGoto(DBloodActor *);
static void innocThinkChase(DBloodActor *);

AISTATE innocentIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE innocentSearch = { kAiStateSearch, 6, -1, 1800, NULL, aiMoveForward, innocThinkSearch, &innocentIdle };
AISTATE innocentChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, innocThinkChase, NULL };
AISTATE innocentRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentGoto = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, innocThinkGoto, &innocentIdle };

static void innocThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void innocThinkGoto(DBloodActor* actor)
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
        aiNewState(actor, &innocentSearch);
    aiThinkTarget(actor);
}

static void innocThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &innocentGoto);
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
        aiNewState(actor, &innocentSearch);
        return;
    }
    if (IsPlayerSprite(pTarget))
    {
        aiNewState(actor, &innocentSearch);
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
                if (nDist < 0x666 && abs(nDeltaAngle) < 85)
                    aiNewState(actor, &innocentIdle);
                return;
            }
        }
    }

    aiPlay3DSound(pSprite, 7000+Random(6), AI_SFX_PRIORITY_1, -1);
    aiNewState(actor, &innocentGoto);
    pXSprite->target = -1;
}


END_BLD_NS
