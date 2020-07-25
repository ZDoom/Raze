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
#include "nnexts.h"

BEGIN_BLD_NS

static void SlashFSeqCallback(int, int);
static void ThrowFSeqCallback(int, int);
static void BlastSSeqCallback(int, int);
static void ThrowSSeqCallback(int, int);
static void thinkTarget(spritetype *, XSPRITE *);
static void thinkSearch(spritetype *, XSPRITE *);
static void thinkGoto(spritetype *, XSPRITE *);
static void MoveDodgeUp(spritetype *, XSPRITE *);
static void MoveDodgeDown(spritetype *, XSPRITE *);
static void thinkChase(spritetype *, XSPRITE *);
static void entryFStatue(spritetype *, XSPRITE *);
static void entrySStatue(spritetype *, XSPRITE *);
static void MoveForward(spritetype *, XSPRITE *);
static void MoveSlow(spritetype *, XSPRITE *);
static void MoveSwoop(spritetype *, XSPRITE *);
static void MoveFly(spritetype *, XSPRITE *);
static void playStatueBreakSnd(spritetype*,XSPRITE*);

static int nSlashFClient = seqRegisterClient(SlashFSeqCallback);
static int nThrowFClient = seqRegisterClient(ThrowFSeqCallback);
static int nThrowSClient = seqRegisterClient(ThrowSSeqCallback);
static int nBlastSClient = seqRegisterClient(BlastSSeqCallback);

AISTATE gargoyleFIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, thinkTarget, NULL };
AISTATE gargoyleStatueIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE gargoyleFChase = { kAiStateChase, 0, -1, 0, NULL, MoveForward, thinkChase, &gargoyleFIdle };
AISTATE gargoyleFGoto = { kAiStateMove, 0, -1, 600, NULL, MoveForward, thinkGoto, &gargoyleFIdle };
AISTATE gargoyleFSlash = { kAiStateChase, 6, nSlashFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFThrow = { kAiStateChase, 6, nThrowFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleSThrow = { kAiStateChase, 6, nThrowSClient, 120, NULL, MoveForward, NULL, &gargoyleFChase };
AISTATE gargoyleSBlast = { kAiStateChase, 7, nBlastSClient, 60, NULL, MoveSlow, NULL, &gargoyleFChase };
AISTATE gargoyleFRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFSearch = { kAiStateSearch, 0, -1, 120, NULL, MoveForward, thinkSearch, &gargoyleFIdle };
AISTATE gargoyleFMorph2 = { kAiStateOther, -1, -1, 0, entryFStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleFMorph = { kAiStateOther, 6, -1, 0, NULL, NULL, NULL, &gargoyleFMorph2 };
AISTATE gargoyleSMorph2 = { kAiStateOther, -1, -1, 0, entrySStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleSMorph = { kAiStateOther, 6, -1, 0, NULL, NULL, NULL, &gargoyleSMorph2 };
AISTATE gargoyleSwoop = { kAiStateOther, 0, -1, 120, NULL, MoveSwoop, thinkChase, &gargoyleFChase };
AISTATE gargoyleFly = { kAiStateMove, 0, -1, 120, NULL, MoveFly, thinkChase, &gargoyleFChase };
AISTATE gargoyleTurn = { kAiStateMove, 0, -1, 120, NULL, aiMoveTurn, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeUp = { kAiStateMove, 0, -1, 60, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, MoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeDown = { kAiStateMove, 0, -1, 120, NULL, MoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, MoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, MoveDodgeDown, NULL, &gargoyleFChase };

AISTATE statueFBreakSEQ = { kAiStateOther, 5, -1, 0, entryFStatue, NULL, playStatueBreakSnd, &gargoyleFMorph2};
AISTATE statueSBreakSEQ = { kAiStateOther, 5, -1, 0, entrySStatue, NULL, playStatueBreakSnd, &gargoyleSMorph2};

static void playStatueBreakSnd(spritetype* pSprite, XSPRITE* pXSprite) {
    UNREFERENCED_PARAMETER(pXSprite);
    aiPlay3DSound(pSprite, 313, AI_SFX_PRIORITY_1, -1);
}

static void SlashFSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEINFO *pDudeInfoT = getDudeInfo(pTarget->type);
    int height = (pSprite->yrepeat*pDudeInfo->eyeHeight)<<2;
    int height2 = (pTarget->yrepeat*pDudeInfoT->eyeHeight)<<2;
    int dz = height-height2;
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_13);
    int r1 = Random(50);
    int r2 = Random(50);
    actFireVector(pSprite, 0, 0, dx+r2, dy-r1, dz, VECTOR_TYPE_13);
    r1 = Random(50);
    r2 = Random(50);
    actFireVector(pSprite, 0, 0, dx-r2, dy+r1, dz, VECTOR_TYPE_13);
}

static void ThrowFSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    actFireThing(&sprite[nSprite], 0, 0, gDudeSlope[nXSprite]-7500, kThingBone, 0xeeeee);
}

static void BlastSSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    wrand(); // ???
    spritetype *pTarget = &sprite[pXSprite->target];
    int height = (pSprite->yrepeat*getDudeInfo(pSprite->type)->eyeHeight) << 2;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int UNUSED(nDist) = approxDist(dx, dy);
    int UNUSED(nAngle) = getangle(dx, dy);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = height;
    TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
    Aim aim;
    aim.dx = Cos(pSprite->ang)>>16;
    aim.dy = Sin(pSprite->ang)>>16;
    aim.dz = gDudeSlope[nXSprite];
    int nClosest = 0x7fffffff;
    for (short nSprite2 = headspritestat[kStatDude]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
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
        int tz = z+mulscale(gDudeSlope[nXSprite], nDist, 10);
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
                    aim.dx = Cos(nAngle)>>16;
                    aim.dy = Sin(nAngle)>>16;
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
        // allow to fire missile in non-player targets
        if (IsPlayerSprite(pTarget) || gModernMap) {
            actFireMissile(pSprite, -120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
            actFireMissile(pSprite, 120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
        }
    #else
        if (IsPlayerSprite(pTarget)) {
            actFireMissile(pSprite, -120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
            actFireMissile(pSprite, 120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
        }
    #endif

}

static void ThrowSSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    actFireThing(pSprite, 0, 0, gDudeSlope[nXSprite]-7500, kThingBone, Chance(0x6000) ? 0x133333 : 0x111111);
}

static void thinkTarget(spritetype *pSprite, XSPRITE *pXSprite)
{
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
    if (pDudeExtraE->at8 && pDudeExtraE->at4 < 10)
        pDudeExtraE->at4++;
    else if (pDudeExtraE->at4 >= 10 && pDudeExtraE->at8)
    {
        pXSprite->goalAng += 256;
        POINT3D *pTarget = &baseSprite[pSprite->index];
        aiSetTarget(pXSprite, pTarget->x, pTarget->y, pTarget->z);
        aiNewState(pSprite, pXSprite, &gargoyleTurn);
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
                pDudeExtraE->at4 = 0;
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(pSprite, pXSprite);
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                pDudeExtraE->at4 = 0;
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
    sub_5F15C(pSprite, pXSprite);
}

static void thinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void MoveDodgeUp(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
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
    zvel[nSprite] = -0x1d555;
}

static void MoveDodgeDown(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
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
        aiNewState(pSprite, pXSprite, &gargoyleFGoto);
        return;
    }
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    ///dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites)) {
        consoleSysMsg("pXSprite->target >= 0 && pXSprite->target < kMaxSprites");
        return;
    }
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(pSprite, pXSprite, &gargoyleFSearch);
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
                case kDudeGargoyleFlesh:
                    if (nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1408, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeGargoyleStone)
                            {
                                sfxPlay3DSound(pSprite, 1408, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1408, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFThrow);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1406, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeGargoyleStone)
                            {
                                sfxPlay3DSound(pSprite, 1406, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1406, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0xa00)
                    {
                        aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                        aiNewState(pSprite, pXSprite, &gargoyleSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                    break;
                case kDudeGargoyleStone:
                    if (nDist < 0x1800 && nDist > 0xc00 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            sfxPlay3DSound(pSprite, 1457, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeGargoyleFlesh)
                            {
                                sfxPlay3DSound(pSprite, 1457, 0, 0);
                                aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            }
                            break;
                        default:
                            sfxPlay3DSound(pSprite, 1457, 0, 0);
                            aiNewState(pSprite, pXSprite, &gargoyleSBlast);
                            break;
                        }
                    }
                    else if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    {
                        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                        switch (hit)
                        {
                        case -1:
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        case 0:
                        case 4:
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type && sprite[gHitInfo.hitsprite].type != kDudeGargoyleFlesh)
                                aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        default:
                            aiNewState(pSprite, pXSprite, &gargoyleFSlash);
                            break;
                        }
                    }
                    else if ((height2-height > 0x2000 || floorZ-bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
                    {
                        if (pSprite->type == kDudeGargoyleFlesh)
                            aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
                        else
                            aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
                        aiNewState(pSprite, pXSprite, &gargoyleSwoop);
                    }
                    else if ((height2-height < 0x2000 || floorZ-bottom < 0x2000) && klabs(nDeltaAngle) < 85)
                        aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
                    break;
                }
            }
            return;
        }
        else
        {
            aiNewState(pSprite, pXSprite, &gargoyleFly);
            return;
        }
    }

    aiNewState(pSprite, pXSprite, &gargoyleFGoto);
    pXSprite->target = -1;
}

static void entryFStatue(spritetype *pSprite, XSPRITE *pXSprite)
{
    DUDEINFO *pDudeInfo = &dudeInfo[6];
    actHealDude(pXSprite, pDudeInfo->startHealth, pDudeInfo->startHealth);
    pSprite->type = kDudeGargoyleFlesh;
}

static void entrySStatue(spritetype *pSprite, XSPRITE *pXSprite)
{
    DUDEINFO *pDudeInfo = &dudeInfo[7];
    actHealDude(pXSprite, pDudeInfo->startHealth, pDudeInfo->startHealth);
    pSprite->type = kDudeGargoyleStone;
}

static void MoveForward(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
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
    int UNUSED(nAngle) = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
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

static void MoveSlow(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
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
    int UNUSED(nAngle) = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 = nAccel>>1;
    t2 >>= 1;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    switch (pSprite->type) { 
        case kDudeGargoyleFlesh:
            zvel[nSprite] = 0x44444;
            break;
        case kDudeGargoyleStone:
            zvel[nSprite] = 0x35555;
            break;
    }
}

static void MoveSwoop(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
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
    int UNUSED(nAngle) = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
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
    switch (pSprite->type) {
        case kDudeGargoyleFlesh:
            zvel[nSprite] = t1;
            break;
        case kDudeGargoyleStone:
            zvel[nSprite] = t1;
            break;
    }
}

static void MoveFly(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    ///dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
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
    int UNUSED(nAngle) = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    if (Chance(0x4000) && nDist <= 0x400)
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
    switch (pSprite->type) {
        case kDudeGargoyleFlesh:
            zvel[nSprite] = -t1;
            break;
        case kDudeGargoyleStone:
            zvel[nSprite] = -t1;
            break;
    }
    klabs(zvel[nSprite]);
}

END_BLD_NS
