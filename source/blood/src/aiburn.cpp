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
#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "common_game.h"

#include "actor.h"
#include "ai.h"
#include "aiburn.h"
#include "blood.h"
#include "db.h"
#include "dude.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void BurnSeqCallback(int, int);
static void thinkSearch(spritetype*, XSPRITE*);
static void thinkGoto(spritetype*, XSPRITE*);
static void thinkChase(spritetype*, XSPRITE*);

static int nBurnClient = seqRegisterClient(BurnSeqCallback);

AISTATE cultistBurnIdle = { kAiStateIdle, 3, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE cultistBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE cultistBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &cultistBurnSearch };
AISTATE cultistBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, &cultistBurnSearch };
AISTATE cultistBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &cultistBurnChase };

AISTATE zombieABurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE zombieABurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &zombieABurnSearch };
AISTATE zombieABurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, NULL };
AISTATE zombieABurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &zombieABurnChase };

AISTATE zombieFBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE zombieFBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &zombieFBurnSearch };
AISTATE zombieFBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, NULL };
AISTATE zombieFBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &zombieFBurnChase };

AISTATE innocentBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE innocentBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &zombieFBurnSearch };
AISTATE innocentBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, NULL };
AISTATE innocentBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &zombieFBurnChase };

AISTATE beastBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE beastBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &beastBurnSearch };
AISTATE beastBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, &beastBurnSearch };
AISTATE beastBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &beastBurnChase };

AISTATE tinycalebBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE tinycalebBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &tinycalebBurnSearch };
AISTATE tinycalebBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, &tinycalebBurnSearch };
AISTATE tinycalebBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &tinycalebBurnChase };

AISTATE GDXGenDudeBurnIdle = { kAiStateIdle, 3, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE GDXGenDudeBurnChase = { kAiStateChase, 3, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE GDXGenDudeBurnGoto = { kAiStateMove, 3, -1, 3600, NULL, aiMoveForward, thinkGoto, &GDXGenDudeBurnSearch };
AISTATE GDXGenDudeBurnSearch = { kAiStateSearch, 3, -1, 3600, NULL, aiMoveForward, thinkSearch, &GDXGenDudeBurnSearch };
AISTATE GDXGenDudeBurnAttack = { kAiStateChase, 3, nBurnClient, 120, NULL, NULL, NULL, &GDXGenDudeBurnChase };

static void BurnSeqCallback(int, int)
{
}

static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
    {
        switch (pSprite->type)
        {
        case 240:
            aiNewState(pSprite, pXSprite, &cultistBurnSearch);
            break;
        case 241:
            aiNewState(pSprite, pXSprite, &zombieABurnSearch);
            break;
        case 242:
            aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
            break;
        case 239:
            aiNewState(pSprite, pXSprite, &innocentBurnSearch);
            break;
        case 253:
            aiNewState(pSprite, pXSprite, &beastBurnSearch);
            break;
        case 252:
            aiNewState(pSprite, pXSprite, &tinycalebBurnSearch);
            break;
        case kGDXGenDudeBurning:
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnSearch);
            break;
        }
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        switch (pSprite->type)
        {
        case 240:
            aiNewState(pSprite, pXSprite, &cultistBurnGoto);
            break;
        case 241:
            aiNewState(pSprite, pXSprite, &zombieABurnGoto);
            break;
        case 242:
            aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
            break;
        case 239:
            aiNewState(pSprite, pXSprite, &innocentBurnGoto);
            break;
        case 253:
            aiNewState(pSprite, pXSprite, &beastBurnGoto);
            break;
        case 252:
            aiNewState(pSprite, pXSprite, &tinycalebBurnGoto);
            break;
        case kGDXGenDudeBurning:
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnGoto);
            break;
        }
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        switch (pSprite->type)
        {
        case 240:
            aiNewState(pSprite, pXSprite, &cultistBurnSearch);
            break;
        case 241:
            aiNewState(pSprite, pXSprite, &zombieABurnSearch);
            break;
        case 242:
            aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
            break;
        case 239:
            aiNewState(pSprite, pXSprite, &innocentBurnSearch);
            break;
        case 253:
            aiNewState(pSprite, pXSprite, &beastBurnSearch);
            break;
        case 252:
            aiNewState(pSprite, pXSprite, &tinycalebBurnSearch);
            break;
        case kGDXGenDudeBurning:
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnSearch);
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
                if (nDist < 0x333 && klabs(nDeltaAngle) < 85)
                {
                    switch (pSprite->type)
                    {
                    case 240:
                        aiNewState(pSprite, pXSprite, &cultistBurnAttack);
                        break;
                    case 241:
                        aiNewState(pSprite, pXSprite, &zombieABurnAttack);
                        break;
                    case 242:
                        aiNewState(pSprite, pXSprite, &zombieFBurnAttack);
                        break;
                    case 239:
                        aiNewState(pSprite, pXSprite, &innocentBurnAttack);
                        break;
                    case 253:
                        aiNewState(pSprite, pXSprite, &beastBurnAttack);
                        break;
                    case 252:
                        aiNewState(pSprite, pXSprite, &tinycalebBurnAttack);
                        break;
                    case kGDXGenDudeBurning:
                        aiNewState(pSprite, pXSprite, &GDXGenDudeBurnSearch);
                        break;
                    }
                }
                return;
            }
        }
    }
    
    switch (pSprite->type)
    {
    case 240:
        aiNewState(pSprite, pXSprite, &cultistBurnGoto);
        break;
    case 241:
        aiNewState(pSprite, pXSprite, &zombieABurnGoto);
        break;
    case 242:
        aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
        break;
    case 239:
        aiNewState(pSprite, pXSprite, &innocentBurnGoto);
        break;
    case 253:
        aiNewState(pSprite, pXSprite, &beastBurnGoto);
        break;
    case 252:
        aiNewState(pSprite, pXSprite, &tinycalebBurnGoto);
        break;
    case kGDXGenDudeBurning:
        aiNewState(pSprite, pXSprite, &GDXGenDudeBurnSearch);
        break;
    }
    pXSprite->target = -1;
}
