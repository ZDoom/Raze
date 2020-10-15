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
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sound.h"

BEGIN_BLD_NS

static void SlashSeqCallback(int, int);
static void StompSeqCallback(int, int);
static void MorphToBeast(spritetype *, XSPRITE *);
static void beastThinkSearch(spritetype *, XSPRITE *);
static void beastThinkGoto(spritetype *, XSPRITE *);
static void beastThinkChase(spritetype *, XSPRITE *);
static void beastThinkSwimGoto(spritetype *, XSPRITE *);
static void beastThinkSwimChase(spritetype *, XSPRITE *);
static void beastMoveForward(spritetype *, XSPRITE *);
static void sub_628A0(spritetype *, XSPRITE *);
static void sub_62AE0(spritetype *, XSPRITE *);
static void sub_62D7C(spritetype *, XSPRITE *);

static int nSlashClient = seqRegisterClient(SlashSeqCallback);
static int nStompClient = seqRegisterClient(StompSeqCallback);

AISTATE beastIdle = {kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastChase = {kAiStateChase, 8, -1, 0, NULL, beastMoveForward, beastThinkChase, NULL };
AISTATE beastDodge = { kAiStateMove, 8, -1, 60, NULL, aiMoveDodge, NULL, &beastChase };
AISTATE beastGoto = { kAiStateMove, 8, -1, 600, NULL, beastMoveForward, beastThinkGoto, &beastIdle };
AISTATE beastSlash = { kAiStateChase, 6, nSlashClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastStomp = { kAiStateChase, 7, nStompClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastSearch = { kAiStateSearch, 8, -1, 120, NULL, beastMoveForward, beastThinkSearch, &beastIdle };
AISTATE beastRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastSwimIdle = {kAiStateIdle, 9, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastSwimChase = { kAiStateChase, 9, -1, 0, NULL, sub_628A0, beastThinkSwimChase, NULL };
AISTATE beastSwimDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &beastSwimChase };
AISTATE beastSwimGoto = { kAiStateMove, 9, -1, 600, NULL, beastMoveForward, beastThinkSwimGoto, &beastSwimIdle };
AISTATE beastSwimSearch = { kAiStateSearch, 9, -1, 120, NULL, beastMoveForward, beastThinkSearch, &beastSwimIdle };
AISTATE beastSwimSlash = { kAiStateChase, 9, nSlashClient, 0, NULL, NULL, beastThinkSwimChase, &beastSwimChase };
AISTATE beastSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &beastSwimDodge };
AISTATE beastMorphToBeast = { kAiStateOther, -1, -1, 0, MorphToBeast, NULL, NULL, &beastIdle };
AISTATE beastMorphFromCultist = { kAiStateOther, 2576, -1, 0, NULL, NULL, NULL, &beastMorphToBeast };
AISTATE beast138FB4 = { kAiStateOther, 9, -1, 120, NULL, sub_62AE0, beastThinkSwimChase, &beastSwimChase };
AISTATE beast138FD0 = { kAiStateOther, 9, -1, 0, NULL, sub_62D7C, beastThinkSwimChase, &beastSwimChase };
AISTATE beast138FEC = { kAiStateOther, 9, -1, 120, NULL, aiMoveTurn, NULL, &beastSwimChase };

static void SlashSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    spritetype *pTarget = &sprite[pXSprite->target];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    // Correct ?
    int dz = pSprite->z-pTarget->z;
    dx += Random3(4000-700*gGameOptions.nDifficulty);
    dy += Random3(4000-700*gGameOptions.nDifficulty);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_13);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_13);
    actFireVector(pSprite, 0, 0, dx, dy, dz, VECTOR_TYPE_13);
    sfxPlay3DSound(pSprite, 9012+Random(2), -1, 0);
}

static void StompSeqCallback(int, int nXSprite)
{
    uint8_t vb8[(kMaxSectors+7)>>3];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int dx = CosScale16(pSprite->ang);
    int dy = SinScale16(pSprite->ang);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    int vc = 400;
    int nSector = pSprite->sectnum;
    int v1c = 5+2*gGameOptions.nDifficulty;
    int v10 = 25+30*gGameOptions.nDifficulty;
    gAffectedSectors[0] = -1;
    gAffectedXWalls[0] = -1;
    GetClosestSpriteSectors(nSector, x, y, vc, gAffectedSectors, vb8, gAffectedXWalls);
    char v4 = 0;
    int v34 = -1;
    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
    actHitcodeToData(hit, &gHitInfo, &v34, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (hit == 3 && v34 >= 0)
    {
        if (sprite[v34].statnum == kStatDude)
            v4 = 0;
    }
    vc <<= 4;
    int nSprite2;
    StatIterator it1(kStatDude);
    while ((nSprite2 = it1.NextIndex()) >= 0)
    {
        if (nSprite != nSprite2 || v4)
        {
            spritetype *pSprite2 = &sprite[nSprite2];
            if (pSprite2->extra > 0 && pSprite2->extra < kMaxXSprites)
            {
                if (pSprite2->type == kDudeBeast)
                    continue;
                if (pSprite2->flags&32)
                    continue;
                if (TestBitString(vb8, pSprite2->sectnum) && CheckProximity(pSprite2, x, y, z, nSector, vc))
                {
                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    if (klabs(bottom-sector[nSector].floorz) == 0)
                    {
                        int dx = klabs(pSprite->x-pSprite2->x);
                        int dy = klabs(pSprite->y-pSprite2->y);
                        int nDist2 = ksqrt(dx*dx + dy*dy);
                        if (nDist2 <= vc)
                        {
                            int nDamage;
                            if (!nDist2)
                                nDamage = v1c + v10;
                            else
                                nDamage = v1c + ((vc-nDist2)*v10)/vc;
                            if (IsPlayerSprite(pSprite2))
                                gPlayer[pSprite2->type-kDudePlayer1].quakeEffect += nDamage*4;
                            actDamageSprite(nSprite, pSprite2, DAMAGE_TYPE_0, nDamage<<4);
                        }
                    }
                }
            }
        }
    }
    it1.Reset(kStatThing);
    while ((nSprite2 = it1.NextIndex()) >= 0)
    {
        spritetype *pSprite2 = &sprite[nSprite2];
        if (pSprite2->flags&32)
            continue;
        if (TestBitString(vb8, pSprite2->sectnum) && CheckProximity(pSprite2, x, y, z, nSector, vc))
        {
            XSPRITE *pXSprite = &xsprite[pSprite2->extra];
            if (pXSprite->locked)
                continue;
            int dx = klabs(pSprite->x-pSprite2->x);
            int dy = klabs(pSprite->y-pSprite2->y);
            int nDist2 = ksqrt(dx*dx + dy*dy);
            if (nDist2 <= vc)
            {
                int nDamage;
                if (!nDist2)
                    nDamage = v1c + v10;
                else
                    nDamage = v1c + ((vc-nDist2)*v10)/vc;
                if (IsPlayerSprite(pSprite2))
                    gPlayer[pSprite2->type-kDudePlayer1].quakeEffect += nDamage*4;
                actDamageSprite(nSprite, pSprite2, DAMAGE_TYPE_0, nDamage<<4);
            }
        }
    }
    sfxPlay3DSound(pSprite, 9015+Random(2), -1, 0);
}

static void MorphToBeast(spritetype *pSprite, XSPRITE *pXSprite)
{
    actHealDude(pXSprite, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
    pSprite->type = kDudeBeast;
}

static void beastThinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(pSprite, pXSprite);
}

static void beastThinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
    {
        if (pXSector && pXSector->Underwater)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void beastThinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
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
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        XSECTOR *pXSector;
        int nXSector = sector[pSprite->sectnum].extra;
        if (nXSector > 0)
            pXSector = &xsector[nXSector];
        else
            pXSector = NULL;
        if (pXSector && pXSector->Underwater)
            aiNewState(pSprite, pXSprite, &beastSwimSearch);
        else
            aiNewState(pSprite, pXSprite, &beastSearch);
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
                int nXSprite = sprite[pXSprite->reference].extra;
                gDudeSlope[nXSprite] = divscale(pTarget->z-pSprite->z, nDist, 10);
                if (nDist < 0x1400 && nDist > 0xa00 && klabs(nDeltaAngle) < 85 && (pTarget->flags&2)
                    && IsPlayerSprite(pTarget) && Chance(0x8000))
                {
                    XSECTOR *pXSector;
                    int nXSector = sector[pSprite->sectnum].extra;
                    if (nXSector > 0)
                        pXSector = &xsector[nXSector];
                    else
                        pXSector = NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    if (pXTarget->health > gPlayerTemplate[0].startHealth/2)
                    {
                        switch (hit)
                        {
                        case -1:
                            if (!pXSector || !pXSector->Underwater)
                                aiNewState(pSprite, pXSprite, &beastStomp);
                            break;
                        case 3:
                            if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                            {
                                if (!pXSector || !pXSector->Underwater)
                                    aiNewState(pSprite, pXSprite, &beastStomp);
                            }
                            else
                            {
                                if (pXSector && pXSector->Underwater)
                                    aiNewState(pSprite, pXSprite, &beastSwimDodge);
                                else
                                    aiNewState(pSprite, pXSprite, &beastDodge);
                            }
                            break;
                        default:
                            if (!pXSector || !pXSector->Underwater)
                                aiNewState(pSprite, pXSprite, &beastStomp);
                            break;
                        }
                    }
                }
                if (nDist < 921 && klabs(nDeltaAngle) < 28)
                {
                    XSECTOR *pXSector;
                    int nXSector = sector[pSprite->sectnum].extra;
                    if (nXSector > 0)
                        pXSector = &xsector[nXSector];
                    else
                        pXSector = NULL;
                    int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
                    switch (hit)
                    {
                    case -1:
                        if (pXSector && pXSector->Underwater)
                            aiNewState(pSprite, pXSprite, &beastSwimSlash);
                        else
                            aiNewState(pSprite, pXSprite, &beastSlash);
                        break;
                    case 3:
                        if (pSprite->type != sprite[gHitInfo.hitsprite].type)
                        {
                            if (pXSector && pXSector->Underwater)
                                aiNewState(pSprite, pXSprite, &beastSwimSlash);
                            else
                                aiNewState(pSprite, pXSprite, &beastSlash);
                        }
                        else
                        {
                            if (pXSector && pXSector->Underwater)
                                aiNewState(pSprite, pXSprite, &beastSwimDodge);
                            else
                                aiNewState(pSprite, pXSprite, &beastDodge);
                        }
                        break;
                    default:
                        if (pXSector && pXSector->Underwater)
                            aiNewState(pSprite, pXSprite, &beastSwimSlash);
                        else
                            aiNewState(pSprite, pXSprite, &beastSlash);
                        break;
                    }
                }
            }
            return;
        }
    }

    XSECTOR *pXSector;
    int nXSector = sector[pSprite->sectnum].extra;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    else
        pXSector = NULL;
    if (pXSector && pXSector->Underwater)
        aiNewState(pSprite, pXSprite, &beastSwimGoto);
    else
        aiNewState(pSprite, pXSprite, &beastGoto);
    pXSprite->target = -1;
}

static void beastThinkSwimGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(pSprite, pXSprite, &beastSwimSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void beastThinkSwimChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &beastSwimGoto);
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
        aiNewState(pSprite, pXSprite, &beastSwimSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(pSprite, pXSprite, &beastSwimSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist)
    {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = pDudeInfo->eyeHeight+pSprite->z;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
        {
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pXSprite->target);
                if (nDist < 0x400 && klabs(nDeltaAngle) < 85)
                    aiNewState(pSprite, pXSprite, &beastSwimSlash);
                else
                {
                    aiPlay3DSound(pSprite, 9009+Random(2), AI_SFX_PRIORITY_1, -1);
                    aiNewState(pSprite, pXSprite, &beast138FD0);
                }
            }
        }
        else
            aiNewState(pSprite, pXSprite, &beast138FD0);
        return;
    }
    aiNewState(pSprite, pXSprite, &beastSwimGoto);
    pXSprite->target = -1;
}

static void beastMoveForward(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (klabs(nAng) > 341)
        return;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nDist = approxDist(dx, dy);
    if (nDist <= 0x400 && Random(64) < 32)
        return;
    xvel[nSprite] += mulscale30(pDudeInfo->frontSpeed, Cos(pSprite->ang));
    yvel[nSprite] += mulscale30(pDudeInfo->frontSpeed, Sin(pSprite->ang));
}

static void sub_628A0(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
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
    if (Random(64) < 32 && nDist <= 0x400)
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
        t1 += nAccel>>2;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
}

static void sub_62AE0(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    spritetype *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
    int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
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
    int dz = z2 - z;
    int nDist = approxDist(dx, dy);
    if (Chance(0x600) && nDist <= 0x400)
        return;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int vx = xvel[nSprite];
    int vy = yvel[nSprite];
    int t1 = dmulscale30(vx, nCos, vy, nSin);
    int t2 = dmulscale30(vx, nSin, -vy, nCos);
    t1 += nAccel;
    xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
    yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    zvel[nSprite] = -dz;
}

static void sub_62D7C(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    spritetype *pTarget = &sprite[pXSprite->target];
    int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
    int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
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
    int dz = (z2 - z)<<3;
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
    zvel[nSprite] = dz;
}

END_BLD_NS
