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

static void ghostThinkTarget(DBloodActor *);
static void ghostThinkSearch(DBloodActor *);
static void ghostThinkGoto(DBloodActor *);
static void ghostMoveDodgeUp(DBloodActor *);
static void ghostMoveDodgeDown(DBloodActor *);
static void ghostThinkChase(DBloodActor *);
static void ghostMoveForward(DBloodActor *);
static void ghostMoveSlow(DBloodActor *);
static void ghostMoveSwoop(DBloodActor *);
static void ghostMoveFly(DBloodActor *);


AISTATE ghostIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, ghostThinkTarget, NULL };
AISTATE ghostChase = { kAiStateChase, 0, -1, 0, NULL, ghostMoveForward, ghostThinkChase, &ghostIdle };
AISTATE ghostGoto = { kAiStateMove, 0, -1, 600, NULL, ghostMoveForward, ghostThinkGoto, &ghostIdle };
AISTATE ghostSlash = { kAiStateChase, 6, nGhostSlashClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostThrow = { kAiStateChase, 6, nGhostThrowClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostBlast = { kAiStateChase, 6, nGhostBlastClient, 120, NULL, ghostMoveSlow, NULL, &ghostChase };
AISTATE ghostRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostSearch = { kAiStateSearch, 0, -1, 120, NULL, ghostMoveForward, ghostThinkSearch, &ghostIdle };
AISTATE ghostSwoop = { kAiStateOther, 0, -1, 120, NULL, ghostMoveSwoop, ghostThinkChase, &ghostChase };
AISTATE ghostFly = { kAiStateMove, 0, -1, 0, NULL, ghostMoveFly, ghostThinkChase, &ghostChase };
AISTATE ghostTurn = { kAiStateMove, 0, -1, 120, NULL, aiMoveTurn, NULL, &ghostChase };
AISTATE ghostDodgeUp = { kAiStateMove, 0, -1, 60, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeDown = { kAiStateMove, 0, -1, 120, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };

void ghostSlashSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pSprite->yrepeat*pDudeInfo->eyeHeight)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->eyeHeight)<<2;
    int dz = height-height2;
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    sfxPlay3DSound(pSprite, 1406, 0, 0);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_12);
    int r1 = Random(50);
    int r2 = Random(50);
    actFireVector(pSprite, 0, 0, dx+r2, dy-r1, dz, VECTOR_TYPE_12);
    r1 = Random(50);
    r2 = Random(50);
    actFireVector(pSprite, 0, 0, dx-r2, dy+r1, dz, VECTOR_TYPE_12);
}

void ghostThrowSeqCallback(int, DBloodActor* actor)
{
    actFireThing(&actor->s(), 0, 0, actor->dudeSlope - 7500, kThingBone, 0xeeeee);
}

void ghostBlastSeqCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    wrand(); // ???
    spritetype *pTarget = &sprite[pXSprite->target];
    int height = (pSprite->yrepeat*getDudeInfo(pSprite->type)->eyeHeight) << 2;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int x = pSprite->x;
    int y = pSprite->y;
    int z = height;
    TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
    Aim aim;
    aim.dx = CosScale16(pSprite->ang);
    aim.dy = SinScale16(pSprite->ang);
    aim.dz = actor->dudeSlope;
    int nClosest = 0x7fffffff;
    int nSprite2;
    StatIterator it(kStatDude);
    while ((nSprite2 = it.NextIndex()) >= 0)
    {
        spritetype *pSprite2 = &sprite[nSprite2];
        if (pSprite == pSprite2 || !(pSprite2->flags&8))
            continue;
        int x2 = pSprite2->x;
        int y2 = pSprite2->y;
        int z2 = pSprite2->z;
        int nDist = approxDist(x2-x, y2-y);
        if (nDist == 0 || nDist > 0x2800)
            continue;
        if (tt.at10)
        {
            int t = divscale(nDist, tt.at10, 12);
            x2 += (xvel[nSprite2]*t)>>12;
            y2 += (yvel[nSprite2]*t)>>12;
            z2 += (zvel[nSprite2]*t)>>8;
        }
        int tx = x+mulscale30(Cos(pSprite->ang), nDist);
        int ty = y+mulscale30(Sin(pSprite->ang), nDist);
        int tz = z+mulscale(actor->dudeSlope, nDist, 10);
        int tsr = mulscale(9460, nDist, 10);
        int top, bottom;
        GetSpriteExtents(pSprite2, &top, &bottom);
        if (tz-tsr > bottom || tz+tsr < top)
            continue;
        int dx = (tx-x2)>>4;
        int dy = (ty-y2)>>4;
        int dz = (tz-z2)>>8;
        int nDist2 = ksqrt(dx*dx+dy*dy+dz*dz);
        if (nDist2 < nClosest)
        {
            int nAngle = getangle(x2-x, y2-y);
            int nDeltaAngle = ((nAngle-pSprite->ang+1024)&2047)-1024;
            if (klabs(nDeltaAngle) <= tt.at8)
            {
                int tz = pSprite2->z-pSprite->z;
                if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
                {
                    nClosest = nDist2;
                    aim.dx = CosScale16(nAngle);
                    aim.dy = SinScale16(nAngle);
                    aim.dz = divscale(tz, nDist, 10);
                    if (tz > -0x333)
                        aim.dz = divscale(tz, nDist, 10);
                    else if (tz < -0x333 && tz > -0xb33)
                        aim.dz = divscale(tz, nDist, 10)+9460;
                    else if (tz < -0xb33 && tz > -0x3000)
                        aim.dz = divscale(tz, nDist, 10)+9460;
                    else if (tz < -0x3000)
                        aim.dz = divscale(tz, nDist, 10)-7500;
                    else
                        aim.dz = divscale(tz, nDist, 10);
                }
                else
                    aim.dz = divscale(tz, nDist, 10);
            }
        }
    }
    #ifdef NOONE_EXTENSIONS
        // allow fire missile in non-player targets if not a demo
        if (IsPlayerSprite(pTarget) || gModernMap) {
            sfxPlay3DSound(pSprite, 489, 0, 0);
            actFireMissile(pSprite, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
        }
    #else
        if (IsPlayerSprite(pTarget)) {
            sfxPlay3DSound(pSprite, 489, 0, 0);
            actFireMissile(pSprite, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
        }
    #endif
}

static void ghostThinkTarget(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
    if (pDudeExtraE->xval3 && pDudeExtraE->xval2 < 10)
        pDudeExtraE->xval2++;
    else if (pDudeExtraE->xval2 >= 10 && pDudeExtraE->xval3)
    {
        pXSprite->goalAng += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(actor, &ghostTurn);
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
                pDudeExtraE->xval2 = 0;
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                pDudeExtraE->xval2 = 0;
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
        }
    }
}

static void ghostThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(actor);
}

static void ghostThinkGoto(DBloodActor* actor)
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
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(actor, &ghostSearch);
    aiThinkTarget(actor);
}

static void ghostMoveDodgeUp(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    int nSprite = pSprite->index;
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = actor->xvel();
    int dy = actor->yvel();
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
    actor->zvel() = -0x1d555;
}

static void ghostMoveDodgeDown(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    int nSprite = pSprite->index;
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int t2 = dmulscale30(dx, nSin, -dy, nCos);
    if (pXSprite->dodgeDir > 0)
        t2 += pDudeInfo->sideSpeed;
    else
        t2 -= pDudeInfo->sideSpeed;

    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
    actor->zvel() = 0x44444;
}

static void ghostThinkChase(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (pXSprite->target == -1)
    {
        aiNewState(actor, &ghostGoto);
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
    if (pXTarget->health == 0)
    {
        aiNewState(actor, &ghostSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(actor, &ghostSearch);
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
                switch (pSprite->type) {
                case kDudePhantasm:
                    if (nDist < 0x2000 && nDist > 0x1000 && klabs(nDeltaAngle) < 85) {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            aiNewState(actor, &ghostBlast);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudePhantasm)
                                aiNewState(actor, &ghostBlast);
                            break;
                        default:
                            aiNewState(actor, &ghostBlast);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            aiNewState(actor, &ghostSlash);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudePhantasm)
                                aiNewState(actor, &ghostSlash);
                            break;
                        default:
                            aiNewState(actor, &ghostSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
                    {
                        aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1, -1);
                        aiNewState(actor, &ghostSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1, -1);
                    break;
                }
            }
            return;
        }
        else
        {
            aiNewState(actor, &ghostFly);
            return;
        }
    }

    aiNewState(actor, &ghostGoto);
    pXSprite->target = -1;
}

static void ghostMoveForward(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    if (pXSprite->target == -1)
        t1 += nAccel;
    else
        t1 += nAccel>>1;
    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
}

static void ghostMoveSlow(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 = nAccel>>1;
    t2 >>= 1;
    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type) {
        case kDudePhantasm:
            actor->zvel() = 0x44444;
            break;
    }
}

static void ghostMoveSwoop(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel>>1;
    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type) {
        case kDudePhantasm:
            actor->zvel() = t1;
            break;
    }
}

static void ghostMoveFly(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    ///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    if (Chance(0x4000) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = actor->xvel();
    int vy = actor->yvel();
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel>>1;
    actor->xvel() = dmulscale30(t1, nCos, t2, nSin);
    actor->yvel() = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type) {
        case kDudePhantasm:
            actor->zvel() = -t1;
            break;
    }
}

END_BLD_NS
