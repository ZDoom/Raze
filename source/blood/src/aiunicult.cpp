//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

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
#include "aiunicult.h"
#include "blood.h"
#include "db.h"
#include "dude.h"

#include "eventq.h"
#include "globals.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "trig.h"
#include "triggers.h"
#include "endgame.h"
#include "view.h"

BEGIN_BLD_NS

static void GDXCultistAttack1(int, int);
static void punchCallback(int, int);
static void ThrowCallback1(int, int);
static void ThrowCallback2(int, int);
static void ThrowThing(int, bool);
static void thinkSearch(spritetype*, XSPRITE*);
static void thinkGoto(spritetype*, XSPRITE*);
static void thinkChase(spritetype*, XSPRITE*);
static void forcePunch(spritetype*, XSPRITE*);

static int nGDXGenDudeAttack1 = seqRegisterClient(GDXCultistAttack1);
static int nGDXGenDudePunch = seqRegisterClient(punchCallback);
static int nGDXGenDudeThrow1 = seqRegisterClient(ThrowCallback1);
static int nGDXGenDudeThrow2 = seqRegisterClient(ThrowCallback2);

AISTATE GDXGenDudeIdleL = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE GDXGenDudeIdleW = { kAiStateIdle, 13, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE GDXGenDudeSearchL = { kAiStateSearch, 9, -1, 600, NULL, aiGenDudeMoveForward, thinkSearch, &GDXGenDudeIdleL };
AISTATE GDXGenDudeSearchW= { kAiStateSearch, 13, -1, 600, NULL, aiGenDudeMoveForward, thinkSearch, &GDXGenDudeIdleW };
AISTATE GDXGenDudeGotoL = { kAiStateMove, 9, -1, 600, NULL, aiGenDudeMoveForward, thinkGoto, &GDXGenDudeIdleL };
AISTATE GDXGenDudeGotoW = { kAiStateMove, 13, -1, 600, NULL, aiGenDudeMoveForward, thinkGoto, &GDXGenDudeIdleW };
AISTATE GDXGenDudeDodgeL = { kAiStateMove, 9, -1, 90, NULL,	aiMoveDodge,	NULL, &GDXGenDudeChaseL };
AISTATE GDXGenDudeDodgeD = { kAiStateMove, 14, -1, 90, NULL, aiMoveDodge,	NULL, &GDXGenDudeChaseD };
AISTATE GDXGenDudeDodgeW = { kAiStateMove, 13, -1, 90, NULL, aiMoveDodge,	NULL, &GDXGenDudeChaseW };
// Dodge when get damage
AISTATE GDXGenDudeDodgeDmgL = { kAiStateMove, 9, -1, 60, NULL,	aiMoveDodge,	NULL, &GDXGenDudeChaseL };
AISTATE GDXGenDudeDodgeDmgD = { kAiStateMove, 14, -1, 60, NULL, aiMoveDodge,	NULL, &GDXGenDudeChaseD };
AISTATE GDXGenDudeDodgeDmgW = { kAiStateMove, 13, -1, 60, NULL, aiMoveDodge,	NULL, &GDXGenDudeChaseW };
// ---------------------
AISTATE GDXGenDudeChaseL = { kAiStateChase, 9, -1, 0, NULL,	aiGenDudeMoveForward, thinkChase, NULL };
AISTATE GDXGenDudeChaseD = { kAiStateChase, 14, -1, 0, NULL,	aiGenDudeMoveForward, thinkChase, NULL };
AISTATE GDXGenDudeChaseW = { kAiStateChase, 13, -1, 0, NULL,	aiGenDudeMoveForward, thinkChase, NULL };
AISTATE GDXGenDudeFireL = { kAiStateChase, 6, nGDXGenDudeAttack1, 0, NULL, aiMoveTurn, thinkChase, &GDXGenDudeFireL };
AISTATE GDXGenDudeFireD = { kAiStateChase, 8, nGDXGenDudeAttack1, 0, NULL, aiMoveTurn, thinkChase, &GDXGenDudeFireD };
AISTATE GDXGenDudeFireW = { kAiStateChase, 8, nGDXGenDudeAttack1, 0, NULL, aiMoveTurn, thinkChase, &GDXGenDudeFireW };
AISTATE GDXGenDudeRecoilL = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &GDXGenDudeChaseL };
AISTATE GDXGenDudeRecoilD = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &GDXGenDudeChaseD };
AISTATE GDXGenDudeRecoilW = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &GDXGenDudeChaseW };
AISTATE GDXGenDudeThrow = { kAiStateChase, 7, nGDXGenDudeThrow1, 0, NULL, NULL, NULL, &GDXGenDudeChaseL };
AISTATE GDXGenDudeThrow2 = { kAiStateChase, 7, nGDXGenDudeThrow2, 0, NULL, NULL, NULL, &GDXGenDudeChaseL };
AISTATE GDXGenDudeRTesla = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &GDXGenDudeDodgeDmgL };
AISTATE GDXGenDudePunch = { kAiStateChase,10, nGDXGenDudePunch, 0, NULL, NULL, forcePunch, &GDXGenDudeChaseL };

GENDUDESND gCustomDudeSnd[] = {
    { 1003, 2, 0, true   }, // spot sound
    { 1013, 2, 2, true   }, // pain sound
    { 1018, 2, 4, false  }, // death sound
    { 1031, 2, 6, true   }, // burning state sound
    { 1018, 2, 8, false  },	// explosive death or end of burning state sound
    { 4021, 2, 10, true  },	// target of dude is dead
    { 1005, 2, 12, true  },	// chase sound
    { -1, 0, 14, false   },	// weapon attack
    { -1, 0, 15, false   },	// throw attack
    { -1, 0, 16, false   },	// melee attack
    { 9008, 0, 17, false },	// transforming in other dude
};

GENDUDEEXTRA gGenDudeExtra[kMaxSprites];

static void forcePunch(spritetype* pSprite, XSPRITE* pXSprite) {
    
    // Required for those who don't have fire trigger in punch seq and for default animation
    if (pXSprite->data2 != kDefaultAnimationBase) {
        Seq* pSeq = NULL; DICTNODE* hSeq = gSysRes.Lookup(pXSprite->data2 + 10, "SEQ");
        if ((pSeq = (Seq*)gSysRes.Load(hSeq)) != NULL) {
            for (int i = 0; i < pSeq->nFrames; i++)
                if (pSeq->frames[i].at5_5) return;
        }
    }

    if (seqGetStatus(3, pSprite->extra) == -1)
        punchCallback(0,pSprite->extra);

}

void genDudeUpdate(spritetype* pSprite) {
    for (int i = 0; i < kGenDudePropertyMax; i++) {
        if (gGenDudeExtra[pSprite->index].updReq[i])
            genDudePrepare(pSprite, i);
    }
}

static void punchCallback(int, int nXIndex) {
    XSPRITE* pXSprite = &xsprite[nXIndex];
    if (pXSprite->target != -1) {
        int nSprite = pXSprite->reference;
        spritetype* pSprite = &sprite[nSprite];

        int nZOffset1 = dudeInfo[pSprite->type - kDudeBase].eyeHeight * pSprite->yrepeat << 2;
        int nZOffset2 = 0;
        
        spritetype* pTarget = &sprite[pXSprite->target];
        if(IsDudeSprite(pTarget))
            nZOffset2 = dudeInfo[pTarget->type - kDudeBase].eyeHeight * pTarget->yrepeat << 2;

        int dx = Cos(pSprite->ang) >> 16;
        int dy = Sin(pSprite->ang) >> 16;
        int dz = nZOffset1 - nZOffset2;

        if (!sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndAttackMelee))
            sfxPlay3DSound(pSprite, 530, 1, 0);

        actFireVector(pSprite, 0, 0, dx, dy, dz,VECTOR_TYPE_22);
    }
}

static void GDXCultistAttack1(int, int nXIndex) {
    if (!(nXIndex >= 0 && nXIndex < kMaxXSprites)) {
        consoleSysMsg("nXIndex >= 0 && nXIndex < kMaxXSprites");
        return;
    }
    
    XSPRITE* pXSprite = &xsprite[nXIndex]; int nSprite = pXSprite->reference;
    
    if (!(nSprite >= 0 && nSprite < kMaxSprites)) {
        consoleSysMsg("nIndex >= 0 && nIndex < kMaxSprites");
        return;
    }

    spritetype* pSprite = &sprite[nSprite]; int dx, dy, dz;
    xvel[pSprite->index] = yvel[pSprite->index] = 0;

    short curWeapon = gGenDudeExtra[nSprite].curWeapon;
    short disperion = gGenDudeExtra[nSprite].baseDispersion;

    if (curWeapon >= 0 && curWeapon < kVectorMax) {

        dx = Cos(pSprite->ang) >> 16; dy = Sin(pSprite->ang) >> 16; dz = gDudeSlope[nXIndex];

        VECTORDATA* pVectorData = &gVectorData[curWeapon];
        int vdist = pVectorData->maxDist;
            
        // dispersal modifiers here in case if non-melee enemy
        if (vdist <= 0 || vdist > 1280) {
            dx += Random3(disperion); dy += Random3(disperion);
            dz += Random3(disperion);
        }

        actFireVector(pSprite, 0, 0, dx, dy, dz,(VECTOR_TYPE)curWeapon);
        if (!sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndAttackNormal))
            sfxPlayVectorSound(pSprite, curWeapon);
            
    } else if (curWeapon >= kDudeBase && curWeapon < kDudeMax) {

        spritetype* pSpawned = NULL; int dist = pSprite->clipdist << 4; 
        short slaveCnt = gGenDudeExtra[pSprite->index].slaveCount;
        if (slaveCnt <= gGameOptions.nDifficulty && (pSpawned = actSpawnDude(pSprite, curWeapon, dist + Random(dist), 0)) != NULL) {
            
            pSpawned->owner = nSprite;
            
            if (pSpawned->extra > -1) {
                xsprite[pSpawned->extra].target = pXSprite->target;
                if (pXSprite->target > -1)
                    aiActivateDude(pSpawned, &xsprite[pSpawned->extra]);
            }
                
            gKillMgr.sub_263E0(1);
            gGenDudeExtra[pSprite->index].slave[slaveCnt] = pSpawned->index;
            gGenDudeExtra[pSprite->index].slaveCount++;

            if (!sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndAttackNormal))
                sfxPlay3DSoundCP(pSprite, 379, 1, 0, 0x10000 - Random3(0x3000));
        }

    } else if (curWeapon >= kMissileBase && curWeapon < kMissileMax) {

        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = gDudeSlope[nXIndex];

        // dispersal modifiers here
        dx += Random3(disperion); dy += Random3(disperion);
        dz += Random3(disperion >> 1);

        actFireMissile(pSprite, 0, 0, dx, dy, dz, curWeapon);
        if (!sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndAttackNormal))
            sfxPlayMissileSound(pSprite, curWeapon);
    }
}

static void ThrowCallback1(int, int nXIndex) {
    ThrowThing(nXIndex, true);
}

static void ThrowCallback2(int, int nXIndex) {
    ThrowThing(nXIndex, false);
}

static void ThrowThing(int nXIndex, bool impact) {
    XSPRITE* pXSprite = &xsprite[nXIndex]; spritetype* pSprite = &sprite[pXSprite->reference];

    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites))
        return;

    spritetype * pTarget = &sprite[pXSprite->target];
    if (!(pTarget->type >= kDudeBase && pTarget->type < kDudeMax))
        return;

    short curWeapon = gGenDudeExtra[sprite[pXSprite->reference].index].curWeapon;
    if (curWeapon < kThingBase || curWeapon >= kThingMax)
        return;

    THINGINFO* pThinkInfo = &thingInfo[curWeapon - kThingBase];
    if (!pThinkInfo->allowThrow) return;

    if (!sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndAttackThrow))
        sfxPlay3DSound(pSprite, 455, -1, 0);
            
    int zThrow = 14500;
    int dx = pTarget->x - pSprite->x;
    int dy = pTarget->y - pSprite->y;
    int dz = pTarget->z - pSprite->z;
    int dist = approxDist(dx, dy);
    
    spritetype* pLeech = leechIsDropped(pSprite); 
    XSPRITE* pXLeech = (pLeech != NULL) ? &xsprite[pLeech->extra] : NULL;
    
    switch (curWeapon) {
        case kModernThingEnemyLifeLeech:
        case kThingDroppedLifeLeech:
            zThrow = 5000;
            // pickup life leech before throw it again
            if (pLeech != NULL) removeLeech(pLeech);
            break;
    }

    spritetype* pThing = NULL;
    if ((pThing = actFireThing(pSprite, 0, 0, (dz / 128) - zThrow, curWeapon, divscale(dist / 540, 120, 23))) == NULL) return;
    else if (pThinkInfo->picnum < 0 && pThing->type != kModernThingThrowableRock) pThing->picnum = 0;
            
    pThing->owner = pSprite->xvel;
            
    switch (curWeapon) {
        case kThingNapalmBall:
            pThing->xrepeat = pThing->yrepeat = 24;
            xsprite[pThing->extra].data4 = 3 + gGameOptions.nDifficulty;
            impact = true;
            break;
        case kModernThingThrowableRock:
            int sPics[6];
            sPics[0] = 2406;	sPics[1] = 2280;
            sPics[2] = 2185;	sPics[3] = 2155;
            sPics[4] = 2620;	sPics[5] = 3135;

            pThing->picnum = sPics[Random(5)];
            pThing->xrepeat = pThing->yrepeat = 24 + Random(42);
            pThing->cstat |= 0x0001;
            pThing->pal = 5;

            if (Chance(0x5000)) pThing->cstat |= 0x0004;
            if (Chance(0x5000)) pThing->cstat |= 0x0008;

            if (pThing->xrepeat > 60) xsprite[pThing->extra].data1 = 43;
            else if (pThing->xrepeat > 40) xsprite[pThing->extra].data1 = 33;
            else if (pThing->xrepeat > 30) xsprite[pThing->extra].data1 = 23;
            else xsprite[pThing->extra].data1 = 12;
            return;
        case kThingTNTBarrel:
        case kThingArmedProxBomb:
        case kThingArmedSpray:
            impact = false;
            break;
        case kModernThingTNTProx:
            xsprite[pThing->extra].state = 0;
            xsprite[pThing->extra].Proximity = true;
            return;
        case kModernThingEnemyLifeLeech:
            XSPRITE* pXThing = &xsprite[pThing->extra];
            if (pLeech != NULL) pXThing->health = pXLeech->health;
            else pXThing->health = 300 * gGameOptions.nDifficulty;

            sfxPlay3DSound(pSprite, 490, -1, 0);

            if (gGameOptions.nDifficulty <= 2) pXThing->data3 = 32700;
            else pXThing->data3 = Random(10);
            pThing->cstat &= ~CSTAT_SPRITE_BLOCK;
            pThing->pal = 6; 
            pXThing->target = pTarget->xvel;
            pXThing->Proximity = true;
            pXThing->stateTimer = 1;
                
            gGenDudeExtra[pSprite->index].nLifeLeech = pThing->index;
            evPost(pThing->xvel, 3, 80, kCallbackLeechStateTimer);
            return;
    }

    if (impact == true && dist <= 7680) xsprite[pThing->extra].Impact = true;
    else {
        xsprite[pThing->extra].Impact = false;
        evPost(pThing->xvel, 3, 120 * Random(2) + 120, kCmdOn, pXSprite->reference);
    }
}

static void thinkSearch( spritetype* pSprite, XSPRITE* pXSprite ) {
    
    int velocity = ClipLow(pSprite->clipdist / 2, 1);
    aiGenDudeChooseDirection(pSprite, pXSprite, pXSprite->goalAng, velocity, velocity);
    sub_5F15C(pSprite, pXSprite);
        
}

static void thinkGoto(spritetype* pSprite, XSPRITE* pXSprite) {

    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }

    int dx = pXSprite->targetX - pSprite->x;
    int dy = pXSprite->targetY - pSprite->y;
    int nAngle = getangle(dx, dy);

    int velocity = ClipLow(pSprite->clipdist / 2, 1);
    aiGenDudeChooseDirection(pSprite, pXSprite, nAngle, velocity, velocity);

    // if reached target, change to search mode
    if (approxDist(dx, dy) < 5120 && klabs(pSprite->ang - nAngle) < dudeInfo[pSprite->type - kDudeBase].periphery) {
        if (spriteIsUnderwater(pSprite, false)) aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
        else aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase( spritetype* pSprite, XSPRITE* pXSprite ) {

    if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax) return;
    else if (pXSprite->target < 0 || pXSprite->target >= kMaxSprites) {
        if(spriteIsUnderwater(pSprite,false)) aiNewState(pSprite, pXSprite, &GDXGenDudeGotoW);
        else aiNewState(pSprite, pXSprite, &GDXGenDudeGotoL);
        return;
    } else {
        
        genDudeUpdate(pSprite);

    }

    spritetype* pTarget = &sprite[pXSprite->target];
    XSPRITE* pXTarget = (!IsDudeSprite(pTarget) || pTarget->extra < 0) ? NULL : &xsprite[pTarget->extra];


    if (pXTarget == NULL || pXTarget->health <= 0) {
        // target is dead
        if(spriteIsUnderwater(pSprite,false)) aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
        else {
            aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
            sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndTargetDead);
        }
        return;
    }
    
    // check target
    int dx = pTarget->x - pSprite->x; int dy = pTarget->y - pSprite->y;
    int dist = ClipLow((int)approxDist(dx, dy), 1); 

    // quick hack to prevent spinning around or changing attacker's sprite angle on high movement speeds
    // when attacking the target. It happens because vanilla function takes in account x and y velocity, 
    // so i use fake velocity with fixed value and pass it as argument.
    int velocity = ClipLow(pSprite->clipdist / 2, 1);
    aiGenDudeChooseDirection(pSprite, pXSprite, getangle(dx, dy), velocity, velocity);
    // aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));

    if (IsPlayerSprite(pTarget)) {
        PLAYER* pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
        if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)  {
            if(spriteIsUnderwater(pSprite,false)) aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
            else aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
            return;
        }
    }
    
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase]; GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index];
    int eyeAboveZ = pDudeInfo->eyeHeight * pSprite->yrepeat << 2; int seeDist = getSeeDist(pSprite, 51200, 35000, 85000);
    if (dist > pDudeInfo->seeDist || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
        pSprite->x, pSprite->y, pSprite->z - eyeAboveZ, pSprite->sectnum)) {

        if (spriteIsUnderwater(pSprite, false)) aiNewState(pSprite, pXSprite, &GDXGenDudeGotoW);
        else aiNewState(pSprite, pXSprite, &GDXGenDudeGotoL);
        pXSprite->target = -1;
        return;
    }

    int nAngle = getangle(dx, dy);
    int losAngle = ((kAng180 + nAngle - pSprite->ang) & 2047) - kAng180;
    
    // is the target visible?
    if (dist < pDudeInfo->seeDist && klabs(losAngle) <= pDudeInfo->periphery) {

        if (((int)gFrameClock & 64) == 0 && Chance(0x3000) && !spriteIsUnderwater(pSprite, false))
            sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndChasing);

        gDudeSlope[sprite[pXSprite->reference].extra] = (int)divscale(pTarget->z - pSprite->z, dist, 10);
        
        short curWeapon = gGenDudeExtra[pSprite->index].curWeapon;
        spritetype* pLeech = leechIsDropped(pSprite); VECTORDATA* meleeVector = &gVectorData[22];

        if (pExtra->updReq[kGenDudePropertyAttack]) genDudePrepare(pSprite, kGenDudePropertyAttack);

        if (curWeapon >= kThingBase && curWeapon < kThingMax) {
            if (klabs(losAngle) < kAng15) {
                if (dist < 12264 && dist > 7680 && !spriteIsUnderwater(pSprite, false) && curWeapon != kModernThingEnemyLifeLeech) {
                    int pHit = HitScan(pSprite, pSprite->z, dx, dy, 0, 16777280, 0);
                    switch (pHit) {
                        case 0:
                        case 4:
                            return;
                        default:
                            aiNewState(pSprite, pXSprite, &GDXGenDudeThrow);
                            return;
                    }

                } else if (dist > 4072 && dist <= 9072 && !spriteIsUnderwater(pSprite, false) && pSprite->owner != (kMaxSprites - 1)) {
                    switch (curWeapon) {
                        case kModernThingEnemyLifeLeech: {
                            if (pLeech == NULL) {
                                aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                                GDXGenDudeThrow2.at18 = &GDXGenDudeDodgeL;
                                return;
                            }

                            XSPRITE* pXLeech = &xsprite[pLeech->extra];
                            int ldist = getTargetDist(pTarget, pDudeInfo, pLeech);
                            if (ldist > 3 || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
                                pLeech->x, pLeech->y, pLeech->z, pLeech->sectnum) || pXLeech->target == -1) {

                                aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                                GDXGenDudeThrow2.at18 = &GDXGenDudeDodgeL;

                            } else {
                                
                                GDXGenDudeThrow2.at18 = &GDXGenDudeChaseL;
                                if (dist > 5072 && Chance(0x5000)) {
                                    if (!canDuck(pSprite) || Chance(0x4000)) aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                                    else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeD);
                                } else {
                                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                                }

                            }
                        }
                        return;
                        case kModernThingThrowableRock:
                            if (Chance(0x4000)) aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                            else sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndTargetSpot);
                            return;
                        default:
                            aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                            return;
                    }

                } else if (dist <= meleeVector->maxDist) {

                    if (spriteIsUnderwater(pSprite, false)) {
                        if (Chance(0x9000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);

                    }
                    else if (Chance(0x9000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                    else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                    return;

                } else {
                    int state = checkAttackState(pSprite, pXSprite);
                    if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                    else if (state == 2) {
                        if (Chance(0x5000)) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                    }
                    else  aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                    return;
                }
            }

        } else {

            int defDist = gGenDudeExtra[pSprite->index].fireDist; int vdist = defDist;

            if (curWeapon > 0 && curWeapon < kVectorMax) {
                if ((vdist = gVectorData[curWeapon].maxDist) <= 0 || vdist > defDist)
                    vdist = defDist;

            } else if (curWeapon >= kDudeBase && curWeapon < kDudeMax) {

                // don't attack slaves
                if (pXSprite->target >= 0 && sprite[pXSprite->target].owner == pSprite->xvel) {
                    aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                    return;
                } else if (gGenDudeExtra[pSprite->index].slaveCount > gGameOptions.nDifficulty || dist < meleeVector->maxDist) {
                    if (dist <= meleeVector->maxDist) {
                        aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                        return;
                    } else {
                        int state = checkAttackState(pSprite, pXSprite);
                        if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                        else if (state == 2) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                        return;
                    }
                }

            } else if (curWeapon >= kMissileBase && curWeapon < kMissileMax) {
                // special handling for flame, explosive and life leech missiles
                int state = checkAttackState(pSprite, pXSprite);
                int mdist = (curWeapon != kMissileFlareAlt) ? 3000 : 2500;
                switch (curWeapon) {
                    case kMissileLifeLeechRegular:
                        // pickup life leech if it was thrown previously
                        if (pLeech != NULL) removeLeech(pLeech);
                        break;
                    case kMissileFlareAlt:
                    case kMissileFireball:
                    case kMissileFireballNapam:
                    case kMissileFireballCerberus:
                    case kMissileFireballTchernobog:
                        if (dist > mdist || pXSprite->locked == 1) break;
                        else if (dist <= meleeVector->maxDist && Chance(0x9000))
                            aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                        else if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                        else if (state == 2) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                        return;
                    case kMissileFlameSpray:
                    case kMissileFlameHound:
                        if (spriteIsUnderwater(pSprite, false)) {
                            if (dist > meleeVector->maxDist) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                            else if (Chance(0x8000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                            else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);
                            return;
                    }

                    vdist = 4200; if (((int)gFrameClock & 16) == 0) vdist += Random(800);
                    break;
                }
            } else if (curWeapon >= kTrapExploder && curWeapon < (kTrapExploder + kExplodeMax) - 1) {
                int nType = curWeapon - kTrapExploder; EXPLOSION* pExpl = &explodeInfo[nType];
                if (CheckProximity(pSprite, pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pExpl->radius / 2)) {
                    
                    xvel[pSprite->xvel] = zvel[pSprite->xvel] = yvel[pSprite->xvel] = 0;
                    
                    if (doExplosion(pSprite, nType) && pXSprite->health > 0)
                            actDamageSprite(pSprite->xvel, pSprite, DAMAGE_TYPE_3, 65535);

                }
                return;
            // scared dude - no weapon. Still can punch you sometimes.	
            } else {

                int state = checkAttackState(pSprite, pXSprite);
                if (Chance(0x0500) && !spriteIsUnderwater(pSprite, false))
                    sfxPlayGDXGenDudeSound(pSprite, kGenDudeSndChasing);

                if (Chance(0x0200)) {
                    if (dist <= meleeVector->maxDist) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                    else if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);
                    else if (state == 2) aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeD);
                    else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                }
                else if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
                else aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);

                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                return;
            }

            if (dist <= vdist && pXSprite->aiState == &GDXGenDudeChaseD)
                aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);

            int state = checkAttackState(pSprite, pXSprite);

            int kAngle = (dist <= 2048) ? kAngle = pDudeInfo->periphery : kAng5;
            if (dist < vdist && klabs(losAngle) < kAngle) {

                switch (state) {
                    case 1:
                        aiNewState(pSprite, pXSprite, &GDXGenDudeFireW);
                        pXSprite->aiState->at18 = &GDXGenDudeFireW;
                        return;
                    case 2:
                        aiNewState(pSprite, pXSprite, &GDXGenDudeFireD);
                        pXSprite->aiState->at18 = &GDXGenDudeFireD;
                        return;
                    default:
                        aiNewState(pSprite, pXSprite, &GDXGenDudeFireL);
                        pXSprite->aiState->at18 = &GDXGenDudeFireL;
                        return;
                }

            } else {
                
                if (seqGetID(3, pSprite->extra) == pXSprite->data2 + (state < 3) ? 8 : 6) {
                    if (state == 1) pXSprite->aiState->at18 = &GDXGenDudeChaseW;
                    else if (state == 2) pXSprite->aiState->at18 = &GDXGenDudeChaseD;
                    else pXSprite->aiState->at18 = &GDXGenDudeChaseL;

                } else if (state == 1 && pXSprite->aiState != &GDXGenDudeChaseW && pXSprite->aiState != &GDXGenDudeFireW) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                    pXSprite->aiState->at18 = &GDXGenDudeFireW;

                } else if (state == 2 && pXSprite->aiState != &GDXGenDudeChaseD && pXSprite->aiState != &GDXGenDudeFireD) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                    pXSprite->aiState->at18 = &GDXGenDudeFireD;

                } else if (pXSprite->aiState != &GDXGenDudeChaseL && pXSprite->aiState != &GDXGenDudeFireL) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                    pXSprite->aiState->at18 = &GDXGenDudeFireL;
                }

            }
        }
    }
}


int checkAttackState(spritetype* pSprite, XSPRITE* pXSprite) {
    UNREFERENCED_PARAMETER(pXSprite);
    if (sub_5BDA8(pSprite, 14) || spriteIsUnderwater(pSprite,false))
    {
        if ( !sub_5BDA8(pSprite, 14) || spriteIsUnderwater(pSprite,false))
        {
            if (spriteIsUnderwater(pSprite,false))
            {
                return 1; //water
            }
        }
        else
        {
            return 2; //duck
        }
    }
    else
    {
        return 3; //land
    }
    return 0;
}

bool TargetNearThing(spritetype* pSprite, int thingType) {
    for ( int nSprite = headspritesect[pSprite->sectnum]; nSprite >= 0; nSprite = nextspritesect[nSprite] )
    {
        // check for required things or explosions in the same sector as the target
        if ( sprite[nSprite].type == thingType || sprite[nSprite].statnum == kStatExplosion )
            return true; // indicate danger
    }
    return false;
}
    
///// For gen dude
int getGenDudeMoveSpeed(spritetype* pSprite,int which, bool mul, bool shift) {
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    int speed = -1; int step = 2500; int maxSpeed = 146603;
    switch(which){
        case 0:
            speed = pDudeInfo->frontSpeed;
            break;
        case 1:
            speed = pDudeInfo->sideSpeed;
            break;
        case 2:
            speed = pDudeInfo->backSpeed;
            break;
        case 3:
            speed = pDudeInfo->angSpeed;
            break;
        default:
            return -1;
    }
    if (pXSprite->busyTime > 0) speed /=3;
    if (speed > 0 && mul) {
            

        if (pXSprite->busyTime > 0)
            speed += (step * pXSprite->busyTime);
    }
        
    if (shift) speed *= 4 >> 4;
    if (speed > maxSpeed) speed = maxSpeed;
        
    return speed;
}
    
void aiGenDudeMoveForward(spritetype* pSprite, XSPRITE* pXSprite ) {
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    
    int maxTurn = pDudeInfo->angSpeed * 4 >> 4;

    int dang = ((kAng180 + pXSprite->goalAng - pSprite->ang) & 2047) - kAng180;
    pSprite->ang = ((pSprite->ang + ClipRange(dang, -maxTurn, maxTurn)) & 2047);
        
    // don't move forward if trying to turn around
    if ( klabs(dang) > kAng60 )
        return;
        
    int sin = Sin(pSprite->ang);
    int cos = Cos(pSprite->ang);

    int frontSpeed = gGenDudeExtra[pSprite->index].frontSpeed;
    xvel[pSprite->xvel] += mulscale(cos, frontSpeed, 30);
    yvel[pSprite->xvel] += mulscale(sin, frontSpeed, 30);
}

void aiGenDudeChooseDirection(spritetype* pSprite, XSPRITE* pXSprite, int a3, int xvel, int yvel) {
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        consoleSysMsg("pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    
    int vc = ((a3 + 1024 - pSprite->ang) & 2047) - 1024;
    int t1 = dmulscale30(xvel, Cos(pSprite->ang), yvel, Sin(pSprite->ang));
    int vsi = ((t1 * 15) >> 12) / 2; int v8 = (vc >= 0) ? 341 : -341;
    
    if (CanMove(pSprite, pXSprite->target, pSprite->ang + vc, vsi))
        pXSprite->goalAng = pSprite->ang + vc;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang + vc / 2, vsi))
        pXSprite->goalAng = pSprite->ang + vc / 2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang - vc / 2, vsi))
        pXSprite->goalAng = pSprite->ang - vc / 2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang + v8, vsi))
        pXSprite->goalAng = pSprite->ang + v8;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang, vsi))
        pXSprite->goalAng = pSprite->ang;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang - v8, vsi))
        pXSprite->goalAng = pSprite->ang - v8;
    else
        pXSprite->goalAng = pSprite->ang + 341;
    
    pXSprite->dodgeDir = (Chance(0x8000)) ? 1 : -1;

    if (!CanMove(pSprite, pXSprite->target, pSprite->ang + pXSprite->dodgeDir * 512, 512)) {
        pXSprite->dodgeDir = -pXSprite->dodgeDir;
        if (!CanMove(pSprite, pXSprite->target, pSprite->ang + pXSprite->dodgeDir * 512, 512))
            pXSprite->dodgeDir = 0;
    }
}
    
bool sfxPlayGDXGenDudeSound(spritetype* pSprite, int mode) {
    
    if (mode < kGenDudeSndTargetSpot || mode >= kGenDudeSndMax) return false;
    GENDUDESND* sndInfo =& gCustomDudeSnd[mode]; bool gotSnd = false;
    short sndStartId = xsprite[pSprite->extra].sysData1; int rand = sndInfo->randomRange;
    int sndId = (sndStartId <= 0) ? sndInfo->defaultSndId : sndStartId + sndInfo->sndIdOffset;

    if (sndId < 0) return false;
    else if (sndStartId <= 0) { sndId += Random(rand); gotSnd = true; }
    else {

        // Let's try to get random snd
        int maxRetries = 5;
        while (maxRetries-- > 0) {
            int random = Random(rand);
            if (!gSoundRes.Lookup(sndId + random, "SFX")) continue;
            sndId = sndId + random;
            gotSnd = true;
            break;
        }

        // If no success in getting random snd, get first existing one
        if (gotSnd == false) {
            int maxSndId = sndId + rand;
            while (sndId++ <= maxSndId) {
                if (!gSoundRes.Lookup(sndId, "SFX")) continue;
                gotSnd = true;
                break;
            }
        }

    }

    if (gotSnd == false) return false;
    else if (sndInfo->aiPlaySound) aiPlay3DSound(pSprite, sndId, AI_SFX_PRIORITY_2, -1);
    else sfxPlay3DSound(pSprite, sndId, -1, 0);
    return true;
}
    
    
bool spriteIsUnderwater(spritetype* pSprite,bool oldWay) {
    if (oldWay){
        if (xsprite[pSprite->extra].medium == kMediumWater || xsprite[pSprite->extra].medium == kMediumGoo)
            return true;
        return false;
    }
        
    if (sector[pSprite->sectnum].extra < 0) return false;
    else if (xsector[sector[pSprite->sectnum].extra].Underwater)
        return true;
        
    return false;
}

spritetype* leechIsDropped(spritetype* pSprite) {
    short nLeech = gGenDudeExtra[pSprite->index].nLifeLeech;
    if (nLeech >= 0 && nLeech < kMaxSprites) return &sprite[nLeech];
    return NULL;

}
    
void removeDudeStuff(spritetype* pSprite) {
    for (short nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].owner != pSprite->xvel) continue;
        switch (sprite[nSprite].type) {
            case kThingArmedProxBomb:
            case kThingArmedRemoteBomb:
            case kModernThingTNTProx:
                sprite[nSprite].type = kSpriteDecoration;
                actPostSprite(sprite[nSprite].xvel, kStatFree);
                break;
            case kModernThingEnemyLifeLeech:
                killDudeLeech(&sprite[nSprite]);
                break;
        }
    }

    for (short nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].owner != pSprite->xvel) continue;
        actDamageSprite(sprite[nSprite].owner, &sprite[nSprite], (DAMAGE_TYPE) 0, 65535);
    }
}
    
void removeLeech(spritetype* pLeech, bool delSprite) {
    if (pLeech != NULL) {
        spritetype* pEffect = gFX.fxSpawn((FX_ID)52,pLeech->sectnum,pLeech->x,pLeech->y,pLeech->z,pLeech->ang);
        if (pEffect != NULL) {
            pEffect->cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
            pEffect->pal = 6;
            int repeat = 64 + Random(50);
            pEffect->xrepeat = repeat;
            pEffect->yrepeat = repeat;
        }
        
        sfxPlay3DSoundCP(pLeech, 490, -1, 0,60000);
        
        if (pLeech->owner >= 0 && pLeech->owner < kMaxSprites)
            gGenDudeExtra[sprite[pLeech->owner].index].nLifeLeech = -1;

        if (delSprite) {
            pLeech->type = kSpriteDecoration;
            actPostSprite(pLeech->index, kStatFree);
        }


    }
}
    
void killDudeLeech(spritetype* pLeech) {
    if (pLeech != NULL) {
        actDamageSprite(pLeech->owner, pLeech, DAMAGE_TYPE_3, 65535);
        sfxPlay3DSoundCP(pLeech, 522, -1, 0, 60000);

        if (pLeech->owner >= 0 && pLeech->owner < kMaxSprites)
            gGenDudeExtra[sprite[pLeech->owner].index].nLifeLeech = -1;
    }
}
    
XSPRITE* getNextIncarnation(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != 3 || rxBucket[i].index == pXSprite->reference)
            continue;
        
        switch (sprite[rxBucket[i].index].statnum) {
            case kStatDude:
            case kStatInactive: // inactive (ambush) dudes
                if (xsprite[sprite[rxBucket[i].index].extra].health > 0)
                    return &xsprite[sprite[rxBucket[i].index].extra];
        }

    }
    return NULL;
}

bool dudeIsMelee(XSPRITE* pXSprite) {
    return gGenDudeExtra[sprite[pXSprite->reference].index].isMelee;
}

void scaleDamage(XSPRITE* pXSprite) {

    short curWeapon = gGenDudeExtra[sprite[pXSprite->reference].index].curWeapon;
    short* curScale = gGenDudeExtra[sprite[pXSprite->reference].index].dmgControl;
    for (int i = 0; i < kDmgMax; i++)
        curScale[i] = dudeInfo[kDudeModernCustom - kDudeBase].startDamage[i];

    if (curWeapon > 0 && curWeapon < kVectorMax) {

        switch (gVectorData[curWeapon].dmgType) {
            case kDmgFall:
                curScale[kDmgFall] = 64 + Random(64);
                break;
            case kDmgBurn:
                curScale[kDmgBurn] = 64;
                curScale[kDmgExplode] = 82;
                break;
            case kDmgBullet:
                curScale[kDmgBullet] = 82 + Random(28);
                break;
            case kDmgExplode:
                curScale[kDmgBurn] = 82;
                curScale[kDmgExplode] = 64;
                break;
            case kDmgChoke:
                curScale[kDmgChoke] = 16 + Random(16);
                break;
            case kDmgSpirit:
                curScale[kDmgSpirit] = 32 + Random(10);
                break;
            case kDmgElectric:
                curScale[kDmgElectric] = 64 + Random(16);
                break;
        }

    // just copy damage resistance of dude that should be summoned
    } else if (curWeapon >= kDudeBase && curWeapon < kDudeMax) {
        
        for (int i = 0; i < kDmgMax; i++)
            curScale[i] = dudeInfo[curWeapon - kDudeBase].startDamage[i];

    // these does not like the explosions and burning
    } else if (curWeapon >= kTrapExploder && curWeapon < (kTrapExploder + kExplodeMax) - 1) {

        curScale[kDmgBurn] = 255; curScale[kDmgExplode] = 512;

    } else if (curWeapon >= kMissileBase && curWeapon < kThingMax) {

        switch (curWeapon) {
            case kMissileButcherKnife:
                curScale[kDmgBullet] = 100;
                fallthrough__;
            case kMissileEctoSkull:
                curScale[kDmgSpirit] = 32;
                break;
            case kMissileLifeLeechAltNormal:
            case kMissileLifeLeechAltSmall:
            case kMissileArcGargoyle:
                curScale[kDmgSpirit] = 32;
                curScale[kDmgElectric] = 52;
                break;
            case kMissileFlareRegular:
            case kMissileFlareAlt:
            case kMissileFlameSpray:
            case kMissileFlameHound:
            case kThingArmedSpray:
            case kThingPodFireBall:
            case kThingNapalmBall:
                curScale[kDmgBurn] = 32;
                curScale[kDmgExplode] = 50;
                break;
            case kMissileLifeLeechRegular:
            case kThingDroppedLifeLeech:
            case kModernThingEnemyLifeLeech:
                curScale[kDmgSpirit] = 32 + Random(18);
                curScale[kDmgBurn] = 60 + Random(4);
                for (int i = 2; i < kDmgMax; i++) {
                    if (Chance(0x1000) && i != kDmgSpirit)
                        curScale[i] = 48 + Random(32);
                }
                break;
            case kMissileFireball:
            case kMissileFireballNapam:
            case kMissileFireballCerberus:
            case kMissileFireballTchernobog:
                curScale[kDmgBurn] = 50;
                curScale[kDmgExplode] = 32;
                curScale[kDmgFall] = 65 + Random(15);
                break;
            case kThingTNTBarrel:
            case kThingArmedProxBomb:
            case kThingArmedRemoteBomb:
            case kThingArmedTNTBundle:
            case kThingArmedTNTStick:
            case kModernThingTNTProx:
                curScale[kDmgExplode] = 32;
                curScale[kDmgFall] = 65 + Random(15);
                break;
            case kMissileTeslaAlt:
            case kMissileTeslaRegular:
                curScale[kDmgElectric] = 32 + Random(8);
                break;
        }

    }

    // add resistance if have an armor item to drop
    if (pXSprite->dropMsg >= kItemArmorAsbest && pXSprite->dropMsg <= kItemArmorSuper) {
        switch (pXSprite->dropMsg) {
            case kItemArmorAsbest:
                curScale[kDmgBurn] = 0;
                curScale[kDmgExplode] -= 25;
                break;
            case kItemArmorBasic:
                curScale[kDmgBurn] -= 10;
                curScale[kDmgExplode] -= 10;
                curScale[kDmgBullet] -= 10;
                curScale[kDmgSpirit] -= 10;
                break;
            case kItemArmorBody:
                curScale[kDmgBullet] -= 20;
                break;
            case kItemArmorFire:
                curScale[kDmgBurn] -= 10;
                curScale[kDmgExplode] -= 10;
                break;
            case kItemArmorSpirit:
                curScale[kDmgSpirit] -= 20;
                break;
            case kItemArmorSuper:
                curScale[kDmgBurn] -= 20;
                curScale[kDmgExplode] -= 20;
                curScale[kDmgBullet] -= 20;
                curScale[kDmgSpirit] -= 20;
                break;
        }
    }

    // take in account yrepeat of sprite
    short yrepeat = sprite[pXSprite->reference].yrepeat;
    if (yrepeat == 0) {
        for (int i = 0; i < kDmgMax; i++) {
            if (i != kDmgSpirit && i != kDmgChoke) curScale[i] = 500;
        }
    } else if (yrepeat < 64) {
        for (int i = 0; i < kDmgMax; i++) curScale[i] += (64 - yrepeat);
    } else if (yrepeat > 64) {
        for (int i = 0; i < kDmgMax; i++) curScale[i] -= (yrepeat - 64);
    }

    // finally, scale dmg for difficulty
    for (int i = 0; i < kDmgMax; i++) 
        curScale[i] = mulscale8(DudeDifficulty[gGameOptions.nDifficulty], ClipLow(curScale[i], 1));
    
    short* dc = curScale;
    //viewSetSystemMessage("0: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d", dc[0], dc[1], dc[2], dc[3], dc[4], dc[5], dc[6]);
}

int getDispersionModifier(spritetype* pSprite, int minDisp, int maxDisp) {
    // the faster fire rate, the less frames = more dispersion
    Seq* pSeq = NULL; DICTNODE* hSeq = gSysRes.Lookup(xsprite[pSprite->extra].data2 + 6, "SEQ"); int disp = 1;
    if ((pSeq = (Seq*)gSysRes.Load(hSeq)) != NULL) {
        int nFrames = pSeq->nFrames; int ticks = pSeq->at8; int shots = 0;
        for (int i = 0; i <= pSeq->nFrames; i++) {
            if (pSeq->frames[i].at5_5) shots++;
        }
        
        disp = (((shots * 1000) / nFrames) / ticks) * 20;
        if (gGameOptions.nDifficulty > 0)
            disp /= gGameOptions.nDifficulty + 1;

        //viewSetSystemMessage("DISP: %d FRAMES: %d SHOTS: %d TICKS %d", disp, nFrames, shots, ticks);

    }

    return ClipRange(disp, minDisp, maxDisp);
}

// the distance counts from sprite size
int getRangeAttackDist(spritetype* pSprite, int minDist, int maxDist) {
    short yrepeat = pSprite->yrepeat; int dist = 0; int seqId = xsprite[pSprite->extra].data2; 
    short mul = 550; int picnum = pSprite->picnum;
    
    if (yrepeat > 0) {
        if (seqId >= 0) {
            Seq* pSeq = NULL; DICTNODE* hSeq = gSysRes.Lookup(seqId, "SEQ");
            if (hSeq) {
                pSeq = (Seq*)gSysRes.Load(hSeq);
                picnum = seqGetTile(&pSeq->frames[0]);
            }
        }
        dist = tilesiz[picnum].y << 8;
        
        if (yrepeat < 64) dist -= (64 - yrepeat) * mul;
        else if (yrepeat > 64) dist += (yrepeat - 64) * (mul / 3);
    }
    
    dist = ClipRange(dist, minDist, maxDist);
    //viewSetSystemMessage("DIST: %d, SPRHEIGHT: %d: YREPEAT: %d PIC: %d", dist, tilesiz[pSprite->picnum].y, yrepeat, picnum);
    return dist;
}

// the distance counts from sprite size (the bigger sprite, the greater distance)
int getSeeDist(spritetype* pSprite, int startDist, int minDist, int maxDist) {
    short y = tilesiz[pSprite->picnum].y; short yrepeat = pSprite->yrepeat;
    int dist = 0;

    return dist;
}

int getBaseChanceModifier(int baseChance) {
    return ((gGameOptions.nDifficulty > 0) ? baseChance - (0x0300 * gGameOptions.nDifficulty) : baseChance);
}

int getRecoilChance(spritetype* pSprite) {
    int mass = getSpriteMassBySize(pSprite);
    int cumulDmg = 0; int baseChance = getBaseChanceModifier(0x8000);
    if (pSprite->extra >= 0) {
        XSPRITE pXSprite = xsprite[pSprite->extra];
        baseChance += (pXSprite.burnTime / 2);
        cumulDmg = pXSprite.data3;
        if (dudeIsMelee(&pXSprite))
            baseChance = 0x700;
    }

    baseChance += cumulDmg;
    int chance = ((baseChance / mass) << 7);
    return chance;

}

int getDodgeChance(spritetype* pSprite) {
    int mass = getSpriteMassBySize(pSprite); int baseChance = getBaseChanceModifier(0x4000);
    if (pSprite->extra >= 0) {
        XSPRITE pXSprite = xsprite[pSprite->extra];
        baseChance += pXSprite.burnTime;
        if (dudeIsMelee(&pXSprite))
            baseChance = 0x400;
    }

    int chance = ((baseChance / mass) << 7);
    return chance;
}

void dudeLeechOperate(spritetype* pSprite, XSPRITE* pXSprite, EVENT event)
{
    if (event.cmd == kCmdOff) {
        actPostSprite(pSprite->xvel, kStatFree);
        return;
    }
    
    int nTarget = pXSprite->target;
    if (nTarget >= 0 && nTarget < kMaxSprites) {
        spritetype* pTarget = &sprite[nTarget];
        if (pTarget->statnum == kStatDude && !(pTarget->flags & 32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites && !pXSprite->stateTimer)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            int nType = pTarget->type - kDudeBase;
            DUDEINFO* pDudeInfo = &dudeInfo[nType];
            int z1 = (top - pSprite->z) - 256;
            int x = pTarget->x; int y = pTarget->y; int z = pTarget->z;
            int nDist = approxDist(x - pSprite->x, y - pSprite->y);
            
            if (nDist != 0 && cansee(pSprite->x, pSprite->y, top, pSprite->sectnum, x, y, z, pTarget->sectnum))
            {
                int t = divscale(nDist, 0x1aaaaa, 12);
                x += (xvel[nTarget] * t) >> 12;
                y += (yvel[nTarget] * t) >> 12;
                int angBak = pSprite->ang;
                pSprite->ang = getangle(x - pSprite->x, y - pSprite->y);
                int dx = Cos(pSprite->ang) >> 16;
                int dy = Sin(pSprite->ang) >> 16;
                int tz = pTarget->z - (pTarget->yrepeat * pDudeInfo->aimHeight) * 4;
                int dz = divscale(tz - top - 256, nDist, 10);
                int nMissileType = kMissileLifeLeechAltNormal + (pXSprite->data3 ? 1 : 0);
                int t2;
                
                if (!pXSprite->data3) t2 = 120 / 10.0;
                else t2 = (3 * 120) / 10.0;

                spritetype * pMissile = actFireMissile(pSprite, 0, z1, dx, dy, dz, nMissileType);
                if (pMissile)
                {
                    pMissile->owner = pSprite->owner;
                    pXSprite->stateTimer = 1;
                    evPost(pSprite->index, 3, t2, kCallbackLeechStateTimer);
                    pXSprite->data3 = ClipLow(pXSprite->data3 - 1, 0);
                }
                pSprite->ang = angBak;
            }
        }
        
    }
}

bool doExplosion(spritetype* pSprite, int nType) {
    spritetype* pExplosion = actSpawnSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, kStatExplosion, true);
    if (pExplosion->extra < 0 || pExplosion->extra >= kMaxXSprites) 
        return false;

    int nSeq = 4; int nSnd = 304; EXPLOSION* pExpl = &explodeInfo[nType];
    
    pExplosion->type = nType;
    pExplosion->cstat |= CSTAT_SPRITE_INVISIBLE;
    pExplosion->owner = pSprite->index;
    pExplosion->shade = -127;

    pExplosion->yrepeat = pExplosion->xrepeat = pExpl->repeat;

    xsprite[pExplosion->extra].data1 = pExpl->ticks;
    xsprite[pExplosion->extra].data2 = pExpl->quakeEffect;
    xsprite[pExplosion->extra].data3 = pExpl->flashEffect;

    if (nType == 0) { nSeq = 3; nSnd = 303; }
    else if (nType == 2) { nSeq = 4; nSnd = 305; }
    else if (nType == 3) { nSeq = 9; nSnd = 307; }
    else if (nType == 4) { nSeq = 5; nSnd = 307; }
    else if (nType <= 6) { nSeq = 4; nSnd = 303; }
    else if (nType == 7) { nSeq = 4; nSnd = 303; }
    
    seqSpawn(nSeq, 3, pExplosion->extra, -1);
    sfxPlay3DSound(pExplosion, nSnd, -1, 0);

    return true;
}


void updateTargetOfLeech(spritetype* pSprite) {
    if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites)) {
        consoleSysMsg("pSprite->extra >= 0 && pSprite->extra < kMaxXSprites");
        return;
    }

    spritetype* pLeech = leechIsDropped(pSprite);
    if (pLeech == NULL || pLeech->extra < 0) gGenDudeExtra[pSprite->index].nLifeLeech = -1;
    else if (xsprite[pSprite->extra].target != xsprite[pLeech->extra].target)
        xsprite[pLeech->extra].target = xsprite[pSprite->extra].target;
}

void updateTargetOfSlaves(spritetype* pSprite) {
    if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites)) {
        consoleSysMsg("pSprite->extra >= 0 && pSprite->extra < kMaxXSprites");
        return;
    }

    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    
    short* slave = gGenDudeExtra[pSprite->index].slave;
    spritetype* pTarget = (pXSprite->target >= 0 && IsDudeSprite(&sprite[pXSprite->target])) ? &sprite[pXSprite->target] : NULL;
    XSPRITE* pXTarget = (pTarget != NULL && pTarget->extra >= 0 && xsprite[pTarget->extra].health > 0) ? &xsprite[pTarget->extra] : NULL;

    for (int i = 0; i <= gGameOptions.nDifficulty; i++) {
        if (slave[i] >= 0) {
            spritetype* pSlave = &sprite[slave[i]];
            if (!IsDudeSprite(pSlave) || pSlave->extra < 0 || xsprite[pSlave->extra].health < 0) {
                slave[i] = -1;
                continue;
            }
            
            XSPRITE* pXSlave = &xsprite[pSlave->index];
            if (pXTarget == NULL) aiSetTarget(pXSlave, pSprite->x, pSprite->y, pSprite->z); // try return to master
            else if (pXSprite->target != pXSlave->target)
                aiSetTarget(pXSlave, pXSprite->target);

            // check if slave attacking another slave
            if (pXSlave->target >= 0) {
                if (sprite[pXSlave->target].owner == pSprite->index)
                    aiSetTarget(pXSlave, pSprite->x, pSprite->y, pSprite->z); // try return to master

            } else {
                // try return to master
                aiSetTarget(pXSlave, pSprite->x, pSprite->y, pSprite->z);
            }
        } 
    }
}

bool inDodge(AISTATE* aiState) {
    return (aiState == &GDXGenDudeDodgeDmgW || aiState == &GDXGenDudeDodgeDmgL || aiState == &GDXGenDudeDodgeDmgD);
}

bool inIdle(AISTATE* aiState) {
    return (aiState == &GDXGenDudeIdleW || aiState == &GDXGenDudeIdleL);
}

int getSeqStartId(XSPRITE* pXSprite) {
    int seqStartId = dudeInfo[sprite[pXSprite->reference].type - kDudeBase].seqStartID;
    // Store seqStartId in data2 field
    if (pXSprite->data2 > 0) {
        seqStartId = pXSprite->data2;
        // check for full set of animations
        for (int i = seqStartId; i <= seqStartId + 24; i++) {

            // exceptions
            switch (i - seqStartId) {
                case 3:		// burning dude
                case 4: 	// electrocution
                case 8:		// attack u/d
                case 11: 	// reserved
                case 12:	// reserved
                case 13:	// move u
                case 14: 	// move d
                case 16:	// burning death 2
                case 17:	// idle w
                case 18:	// transformation in another dude
                case 19:	// reserved
                case 20:    // reserved
                case 21:    // reserved
                case 22:    // reserved
                case 23:    // reserved
                case 24:    // reserved
                    continue;
            }

            if (!gSysRes.Lookup(i, "SEQ")) {
                pXSprite->data2 = dudeInfo[sprite[pXSprite->reference].type - kDudeBase].seqStartID;
                viewSetSystemMessage("No SEQ animation id %d found for custom dude #%d!", i, sprite[pXSprite->reference].index);
                viewSetSystemMessage("SEQ base id: %d", pXSprite->data2);
                return pXSprite->data2;
            }

        }

    }  else {
        pXSprite->data2 = seqStartId;
    }

    return seqStartId;
}

void genDudePrepare(spritetype* pSprite, int propId) {
    if (!(pSprite->index >= 0 && pSprite->index < kMaxSprites)) {
        consoleSysMsg("pSprite->index >= 0 && pSprite->index < kMaxSprites");
        return;
    } else if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites)) {
        consoleSysMsg("pSprite->extra >= 0 && pSprite->extra < kMaxXSprites");
        return;
    } else if (pSprite->type != kDudeModernCustom) {
        consoleSysMsg("pSprite->type != kDudeModernCustom");
        return;
    }
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra]; GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index];
    switch (propId) {
        case kGenDudePropertyAll:
            pExtra->updReq[propId] = false;
        case kGenDudePropertyWeapon:
            //viewSetSystemMessage("PROPERTY: WEAPONS");
            pExtra->curWeapon = pXSprite->data1;
            switch (pXSprite->data1) {
                case 19:
                    pExtra->curWeapon = 2;
                    break;
                case 310:
                    pExtra->curWeapon = kMissileArcGargoyle;
                    break;
                case kThingDroppedLifeLeech:
                    pExtra->curWeapon = kModernThingEnemyLifeLeech;
                    break;
            }
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertyDamage:
            //viewSetSystemMessage("PROPERTY: DAMAGE SCALE");
            scaleDamage(pXSprite);
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertyMass: {
            //viewSetSystemMessage("PROPERTY: MASS");

            // to ensure mass get's updated, let's clear all cache
            SPRITEMASS* pMass = &gSpriteMass[pSprite->index];
            pMass->seqId = pMass->picnum = pMass->xrepeat = pMass->yrepeat = pMass->clipdist = 0;
            pMass->mass = pMass->airVel = pMass->fraction = 0;

            getSpriteMassBySize(pSprite);
            pExtra->updReq[propId] = false;
            if (propId) break;
        }
        case kGenDudePropertyAttack:
            //viewSetSystemMessage("PROPERTY: ATTACK");
            pExtra->fireDist = getRangeAttackDist(pSprite, 1200, 45000);
            pExtra->throwDist = gGenDudeExtra[pSprite->index].fireDist; // temp
            pExtra->baseDispersion = getDispersionModifier(pSprite, 200, 3500);
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertyStates:
            //viewSetSystemMessage("PROPERTY: STATES");
            pExtra->frontSpeed = getGenDudeMoveSpeed(pSprite, 0, true, false);
            pExtra->canFly = false;
            pExtra->canDuck = (gSysRes.Lookup(pXSprite->data2 + 8, "SEQ") && gSysRes.Lookup(pXSprite->data2 + 14, "SEQ"));
            pExtra->canSwim = (gSysRes.Lookup(pXSprite->data2 + 8, "SEQ") && gSysRes.Lookup(pXSprite->data2 + 13, "SEQ")
                && gSysRes.Lookup(pXSprite->data2 + 17, "SEQ"));
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertyLeech:
            //viewSetSystemMessage("PROPERTY: LEECH");
            pExtra->nLifeLeech = -1;
            if (pSprite->owner != kMaxSprites - 1) {
                for (int nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
                    if (sprite[nSprite].owner == pSprite->index && pSprite->type == kModernThingEnemyLifeLeech) {
                        pExtra->nLifeLeech = nSprite;
                        break;
                    }
                }
            }
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertySlaves:
            //viewSetSystemMessage("PROPERTY: SLAVES");
            pExtra->slaveCount = 0; memset(pExtra->slave, -1, sizeof(pExtra->slave));
            for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
                if (sprite[nSprite].owner != pSprite->xvel) continue;
                else if (!IsDudeSprite(&sprite[nSprite]) || sprite[nSprite].extra < 0 || xsprite[sprite[nSprite].extra].health <= 0) {
                    sprite[nSprite].owner = -1;
                    continue;
                }

                pExtra->slave[pExtra->slaveCount++] = nSprite;
                if (pExtra->slaveCount > gGameOptions.nDifficulty)
                    break;
            }
            pExtra->updReq[propId] = false;
            if (propId) break;
        case kGenDudePropertyMelee: {
            //viewSetSystemMessage("PROPERTY: MELEE");
            short curWeapon = pExtra->curWeapon;
            pExtra->isMelee = false; int meleeDist = 2048;
            if (curWeapon >= kTrapExploder && curWeapon < (kTrapExploder + kExplodeMax) - 1) {
                pExtra->isMelee = true;
            } else if (curWeapon >= 0 && curWeapon < kVectorMax) {
                if (gVectorData[curWeapon].maxDist > 0 && gVectorData[curWeapon].maxDist <= meleeDist)
                    pExtra->isMelee = true;
            }
            pExtra->updReq[propId] = false;
        }
        break;
        default:
            viewSetSystemMessage("Unknown custom dude #%d property (%d)", pSprite->index, propId);
            break;
    }
}

bool canSwim(spritetype* pSprite) {
    return gGenDudeExtra[pSprite->index].canSwim;
}

bool canDuck(spritetype* pSprite) {
    return gGenDudeExtra[pSprite->index].canDuck;
}

bool canWalk(spritetype* pSprite) {
    return gGenDudeExtra[pSprite->index].canWalk;
}

//////////

END_BLD_NS
