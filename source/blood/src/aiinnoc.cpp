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

BEGIN_BLD_NS

static void thinkSearch(spritetype *, XSPRITE *);
static void thinkGoto(spritetype *, XSPRITE *);
static void thinkChase(spritetype *, XSPRITE *);

AISTATE innocentIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE innocentSearch = { kAiStateSearch, 6, -1, 1800, NULL, aiMoveForward, thinkSearch, &innocentIdle };
AISTATE innocentChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE innocentRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentGoto = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, thinkGoto, &innocentIdle };

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
        aiNewState(pSprite, pXSprite, &innocentSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &innocentGoto);
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
        aiNewState(pSprite, pXSprite, &innocentSearch);
        return;
    }
    if (IsPlayerSprite(pTarget))
    {
        aiNewState(pSprite, pXSprite, &innocentSearch);
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
                if (nDist < 0x666 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &innocentIdle);
                return;
            }
        }
    }

    aiPlay3DSound(pSprite, 7000+Random(6), AI_SFX_PRIORITY_1, -1);
    aiNewState(pSprite, pXSprite, &innocentGoto);
    pXSprite->target = -1;
}


END_BLD_NS
