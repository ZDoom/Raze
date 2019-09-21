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

static void GDXCultistAttack1(int, int);
static void punchCallback(int, int);
static void ThrowCallback1(int, int);
static void ThrowCallback2(int, int);
static void ThrowThing(int, bool);
static void thinkSearch(spritetype*, XSPRITE*);
static void thinkGoto(spritetype*, XSPRITE*);
static void thinkChase(spritetype*, XSPRITE*);
static void forcePunch(spritetype*, XSPRITE*);
static void thinkTransform(spritetype*, XSPRITE*);

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

        if (!sfxPlayGDXGenDudeSound(pSprite, 9))
            sfxPlay3DSound(pSprite, 530, 1, 0);

        actFireVector(pSprite, 0, 0, dx, dy, dz,VECTOR_TYPE_22);
    }
}

static void GDXCultistAttack1(int, int nXIndex) {
    XSPRITE* pXSprite = &xsprite[nXIndex];
    int nSprite = pXSprite->reference;
    spritetype* pSprite = &sprite[nSprite];
    int dx, dy, dz; int weapon = pXSprite->data1;
    if (weapon >= 0 && weapon < kVectorMax) {
        
        int vector = pXSprite->data1;
        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = gDudeSlope[nXIndex];

        VECTORDATA* pVectorData = &gVectorData[vector];
        int vdist = pVectorData->maxDist;
            
        // dispersal modifiers here in case if non-melee enemy
        if (vdist <= 0 || vdist > 1280) {
            dx += Random3(3000 - 1000 * gGameOptions.nDifficulty);
            dy += Random3(3000 - 1000 * gGameOptions.nDifficulty);
            dz += Random3(1000 - 500 * gGameOptions.nDifficulty);
        }

        actFireVector(pSprite, 0, 0, dx, dy, dz,(VECTOR_TYPE)vector);
        if (!sfxPlayGDXGenDudeSound(pSprite,7))
            sfxPlayVectorSound(pSprite,vector);
            
    } else if (weapon >= kDudeBase && weapon < kDudeMax) {

        spritetype* pSpawned = NULL; int dist = pSprite->clipdist * 6;
        if ((pSpawned = actSpawnDude(pSprite, weapon, dist, 0)) == NULL)
            return;

        gDudeExtra[pSprite->extra].at6.u1.at4++;
        pSpawned->owner = nSprite;
        pSpawned->x += dist + (Random3(dist));
        if (pSpawned->extra > -1) {
            xsprite[pSpawned->extra].target = pXSprite->target;
            if (pXSprite->target > -1)
                aiActivateDude(pSpawned, &xsprite[pSpawned->extra]);
        }
        gKillMgr.sub_263E0(1);
        
        if (!sfxPlayGDXGenDudeSound(pSprite, 7))
            sfxPlay3DSoundCP(pSprite, 379, 1, 0, 0x10000 - Random3(0x3000));
        
        if (Chance(0x5500)) {
            int state = checkAttackState(pSprite, pXSprite);
            switch (state) {
            case 1:
                aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);
                break;
            case 2:
                aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeD);
                break;
            default:
                aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                break;
            }
        }

    } else if (weapon >= kMissileBase && weapon < kMissileMax) {

        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = gDudeSlope[nXIndex];

        // dispersal modifiers here
        dx += Random3(3000 - 1000 * gGameOptions.nDifficulty);
        dy += Random3(3000 - 1000 * gGameOptions.nDifficulty);
        dz += Random3(1000 - 500 * gGameOptions.nDifficulty);

        actFireMissile(pSprite, 0, 0, dx, dy, dz, weapon);
        if (!sfxPlayGDXGenDudeSound(pSprite,7))
            sfxPlayMissileSound(pSprite, weapon);
    }
}

static void ThrowCallback1(int, int nXIndex) {
    ThrowThing(nXIndex, true);
}

static void ThrowCallback2(int, int nXIndex) {
    ThrowThing(nXIndex, true);
}

static void ThrowThing(int nXIndex, bool impact) {
    XSPRITE* pXSprite = &xsprite[nXIndex];
    int nSprite = pXSprite->reference;
    spritetype* pSprite = &sprite[nSprite];

    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites))
        return;

    spritetype * pTarget = &sprite[pXSprite->target];
    if (!(pTarget->type >= kDudeBase && pTarget->type < kDudeMax))
        return;


    int thingType = pXSprite->data1;
    if (thingType >= kThingBase && thingType < kThingMax) {

        THINGINFO* pThinkInfo = &thingInfo[thingType - kThingBase];
        if (pThinkInfo->allowThrow) {

            if (!sfxPlayGDXGenDudeSound(pSprite, 8))
                sfxPlay3DSound(pSprite, 455, -1, 0);

            int dx = pTarget->x - pSprite->x;
            int dy = pTarget->y - pSprite->y;
            int dz = pTarget->z - pSprite->z;

            int dist = approxDist(dx, dy);	int zThrow = 14500;
            spritetype* pThing = NULL; spritetype* pLeech = NULL; XSPRITE* pXLeech = NULL;
            if (thingType == kGDXThingCustomDudeLifeLeech) {
                if ((pLeech = leechIsDropped(pSprite)) != NULL) {
                    // pickup life leech before throw it again
                    pXLeech = &xsprite[pLeech->extra];
                    removeLeech(pLeech);
                }

                zThrow = 5000;
            }

            pThing = actFireThing(pSprite, 0, 0, (dz / 128) - zThrow, thingType, divscale(dist / 540, 120, 23));
            if (pThing == NULL) return;

            if (pThinkInfo->at11 < 0 && pThing->type != kGDXThingThrowableRock) pThing->picnum = 0;
            pThing->owner = pSprite->xvel;
            switch (thingType) {
            case 428:
                impact = true;
                pThing->xrepeat = 24;
                pThing->yrepeat = 24;
                xsprite[pThing->extra].data4 = 3 + gGameOptions.nDifficulty;
                break;

            case kGDXThingThrowableRock:
                int sPics[6];
                sPics[0] = 2406;	sPics[1] = 2280;
                sPics[2] = 2185;	sPics[3] = 2155;
                sPics[4] = 2620;	sPics[5] = 3135;

                pThing->picnum = sPics[Random(5)];
                pThing->pal = 5;
                pThing->xrepeat = 24 + Random(42);
                pThing->yrepeat = 24 + Random(42);
                pThing->cstat |= 0x0001;

                if (Chance(0x5000)) pThing->cstat |= 0x0004;
                if (Chance(0x5000)) pThing->cstat |= 0x0008;

                if (pThing->xrepeat > 60) xsprite[pThing->extra].data1 = 43;
                else if (pThing->xrepeat > 40) xsprite[pThing->extra].data1 = 33;
                else if (pThing->xrepeat > 30) xsprite[pThing->extra].data1 = 23;
                else xsprite[pThing->extra].data1 = 12;

                impact = false;
                return;
            case 400:
            case 401:
            case 420:
                impact = false;
                break;
            case kGDXThingTNTProx:
                xsprite[pThing->extra].state = 0;
                xsprite[pThing->extra].Proximity = true;
                return;
            case 431:
            case kGDXThingCustomDudeLifeLeech:
                XSPRITE* pXThing = &xsprite[pThing->extra];
                if (pLeech != NULL) pXThing->health = pXLeech->health;
                else pXThing->health = 300 * gGameOptions.nDifficulty;

                sfxPlay3DSound(pSprite, 490, -1, 0);

                if (gGameOptions.nDifficulty <= 2) pXThing->data3 = 32700;
                else pXThing->data3 = Random(10);
                pThing->pal = 6;
                pXThing->target = pTarget->xvel;
                pXThing->Proximity = true;
                pXThing->stateTimer = 1;
                evPost(pThing->xvel, 3, 80, CALLBACK_ID_20);
                return;
            }

            if (impact == true && dist <= 7680) xsprite[pThing->extra].Impact = true;
            else {
                xsprite[pThing->extra].Impact = false;
                evPost(pThing->xvel, 3, 120 * Random(2) + 120, COMMAND_ID_1);
            }
            return;
        }

    }
}

static void thinkSearch( spritetype* pSprite, XSPRITE* pXSprite )
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    sub_5F15C(pSprite, pXSprite);
        
}

static void thinkGoto( spritetype* pSprite, XSPRITE* pXSprite )
{
    int dx, dy, dist;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO* pDudeInfo = &dudeInfo[pSprite->lotag - kDudeBase];

    dx = pXSprite->targetX - pSprite->x;
    dy = pXSprite->targetY - pSprite->y;

    int nAngle = getangle(dx, dy);
    dist = approxDist(dx, dy);

    aiChooseDirection(pSprite, pXSprite, nAngle);

    // if reached target, change to search mode
    if (dist < 5120 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery ) {
        if(spriteIsUnderwater(pSprite,false))
            aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
        else
            aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
    }
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase( spritetype* pSprite, XSPRITE* pXSprite )
{

    if (pXSprite->target == -1) {
        if(spriteIsUnderwater(pSprite,false)) aiNewState(pSprite, pXSprite, &GDXGenDudeGotoW);
        else aiNewState(pSprite, pXSprite, &GDXGenDudeGotoL);
        return;
    } dassert(pXSprite->target < kMaxSprites);
    
    
    int dx, dy, dist; DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
    
    spritetype* pTarget = &sprite[pXSprite->target];
    XSPRITE* pXTarget = (!IsDudeSprite(pTarget) || pTarget->extra < 0) ? NULL : &xsprite[pTarget->extra];

    // check target
    dx = pTarget->x - pSprite->x;
    dy = pTarget->y - pSprite->y;

    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));

    if ( pXTarget == NULL || pXTarget->health <= 0 )
    {
        // target is dead
        if(spriteIsUnderwater(pSprite,false)) 
            aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
        else {
            aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
            sfxPlayGDXGenDudeSound(pSprite,5);
        }
        return;
    }

    if (IsPlayerSprite(pTarget))
    {
        PLAYER* pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
        if (powerupCheck( pPlayer, 13 ) > 0)  {
            if(spriteIsUnderwater(pSprite,false)) aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
            else aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
            return;
        }
    }
    
    dist = (int) approxDist(dx, dy); if (dist == 0) dist = 1;
    int eyeAboveZ = pDudeInfo->eyeHeight * pSprite->yrepeat << 2;
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

        if (pXSprite->target < 0) aiSetTarget(pXSprite, pXSprite->target);
        if (((int)gFrameClock & 64) == 0 && Chance(0x3000) && !spriteIsUnderwater(pSprite, false))
            sfxPlayGDXGenDudeSound(pSprite, 6);

        gDudeSlope[sprite[pXSprite->reference].extra] = (int)divscale(pTarget->z - pSprite->z, dist, 10);

        spritetype* pLeech = NULL;	VECTORDATA* meleeVector = &gVectorData[22];
        if (pXSprite->data1 >= kThingBase && pXSprite->data1 < kThingMax) {
            if (pXSprite->data1 == 431) pXSprite->data1 = kGDXThingCustomDudeLifeLeech;
            if ((pLeech = leechIsDropped(pSprite)) != NULL && xsprite[pLeech->extra].target != pXSprite->target)
                xsprite[pLeech->extra].target = pXSprite->target;

            if (klabs(losAngle) < kAng15) {
                if (dist < 12264 && dist > 7680 && !spriteIsUnderwater(pSprite, false) && pXSprite->data1 != kGDXThingCustomDudeLifeLeech) {
                    int pHit = HitScan(pSprite, pSprite->z, dx, dy, 0, 16777280, 0);
                    switch (pHit) {
                    case 0:
                    case 4:
                        return;
                    default:
                        aiNewState(pSprite, pXSprite, &GDXGenDudeThrow);
                        return;
                    }

                }
                else if (dist > 4072 && dist <= 9072 && !spriteIsUnderwater(pSprite, false) && pSprite->owner != (kMaxSprites - 1)) {
                    switch (pXSprite->data1) {
                        case kGDXThingCustomDudeLifeLeech:
                        {
                            if (pLeech == NULL) {
                                aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                                GDXGenDudeThrow2.at18 = &GDXGenDudeDodgeL;
                                return;
                            }

                            XSPRITE* pXLeech = &xsprite[pLeech->extra];
                            int ldist = getTargetDist(pTarget, pDudeInfo, pLeech);
                            //System.out.println("LDIST: "+ldist);
                            if (ldist > 3 || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
                                pLeech->x, pLeech->y, pLeech->z, pLeech->sectnum) || pXLeech->target == -1) {

                                aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                                GDXGenDudeThrow2.at18 = &GDXGenDudeDodgeL;

                            }
                            else {
                                GDXGenDudeThrow2.at18 = &GDXGenDudeChaseL;
                                if (pXLeech->target != pXSprite->target) pXLeech->target = pXSprite->target;
                                if (dist > 5072 && Chance(0x5000)) {
                                    if (!canDuck(pSprite) || Chance(0x4000)) aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                                    else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeD);
                                }
                                else {
                                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                                }
                            }
                        }
                        return;
                        case kGDXThingThrowableRock:
                            if (Chance(0x4000)) aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                            else sfxPlayGDXGenDudeSound(pSprite, 0);
                            return;
                        default:
                            aiNewState(pSprite, pXSprite, &GDXGenDudeThrow2);
                            return;
                    }

                }
                else if (dist <= meleeVector->maxDist) {

                    if (spriteIsUnderwater(pSprite, false)) {
                        if (Chance(0x9000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);

                    }
                    else if (Chance(0x9000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                    else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeL);
                    return;

                }
                else {
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

        }
        else {

            int defDist = 17920; int vdist = defDist;

            if (pXSprite->data1 > 0 && pXSprite->data1 < kVectorMax) {

                switch (pXSprite->data1) {
                case 19:
                    pXSprite->data1 = 2;
                    break;
                }

                VECTORDATA* pVectorData = &gVectorData[pXSprite->data1];
                vdist = pVectorData->maxDist;
                if (vdist <= 0 || vdist > defDist)
                    vdist = defDist;

            }
            else if (pXSprite->data1 >= kDudeBase && pXSprite->data1 < kDudeMax) {

                if (gDudeExtra[pSprite->extra].at6.u1.at4 > 0) {
                    updateTargetOfSlaves(pSprite);
                    if (pXSprite->target >= 0 && sprite[pXSprite->target].owner == pSprite->xvel) {
                        aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                        return;
                    }
                }

                int state = checkAttackState(pSprite, pXSprite);
                if (gDudeExtra[pSprite->extra].at6.u1.at4 <= gGameOptions.nDifficulty && dist > meleeVector->maxDist) {
                    vdist = (vdist / 2) + Random(vdist / 2);
                }
                else if (dist <= meleeVector->maxDist) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                    return;
                }
                else {

                    if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                    else if (state == 2) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                    else aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                    return;
                }



            }
            else if (pXSprite->data1 >= kMissileBase && pXSprite->data1 < kMissileMax) {
                // special handling for flame, explosive and life leech missiles
                int state = checkAttackState(pSprite, pXSprite);
                int mdist = (pXSprite->data1 != 303) ? 3000 : 2500;
                switch (pXSprite->data1) {
                case 315:
                    // pickup life leech if it was thrown previously
                    if ((pLeech = leechIsDropped(pSprite)) != NULL) removeLeech(pLeech);
                    break;
                case 303:
                case 305:
                case 312:
                case 313:
                case 314:
                    if (dist > mdist || pXSprite->locked == 1) break;
                    else if (dist <= meleeVector->maxDist && Chance(0x9000))
                        aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                    else if (state == 1) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                    else if (state == 2) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                    else aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);
                    return;


                case 304:
                case 308:
                    if (spriteIsUnderwater(pSprite, false)) {
                        if (dist > meleeVector->maxDist) aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                        else if (Chance(0x8000)) aiNewState(pSprite, pXSprite, &GDXGenDudePunch);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeW);
                        return;
                    }

                    vdist = 4200; if (((int)gFrameClock & 16) == 0) vdist += Random(800);
                    break;
                }

            }
            else if (pXSprite->data1 >= 459 && pXSprite->data1 < (459 + kExplodeMax) - 1) {

                int nType = pXSprite->data1 - 459; EXPLOSION* pExpl = &explodeInfo[nType];
                if (pExpl != NULL && CheckProximity(pSprite, pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pExpl->radius / 2)
                    && doExplosion(pSprite, nType)) {

                    pXSprite->health = 1;
                    actDamageSprite(pSprite->xvel, pSprite, DAMAGE_TYPE_3, 65535);

                    xvel[pSprite->xvel] = 0;
                    zvel[pSprite->xvel] = 0;
                    yvel[pSprite->xvel] = 0;

                }
                return;

                // scared dude - no weapon. Still can punch you sometimes.	
            }
            else {

                int state = checkAttackState(pSprite, pXSprite);
                if (Chance(0x0500) && !spriteIsUnderwater(pSprite, false))
                    sfxPlayGDXGenDudeSound(pSprite, 6);

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
            if (dist < vdist && /*klabs(losAngle) < 32*/ klabs(losAngle) < kAng5) {

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

            }
            else {
                
                if (seqGetID(3, pSprite->extra) == pXSprite->data2 + (state < 3) ? 8 : 6) {
                    if (state == 1) pXSprite->aiState->at18 = &GDXGenDudeChaseW;
                    else if (state == 2) pXSprite->aiState->at18 = &GDXGenDudeChaseD;
                    else pXSprite->aiState->at18 = &GDXGenDudeChaseL;
                }
                else if (state == 1 && pXSprite->aiState != &GDXGenDudeChaseW && pXSprite->aiState != &GDXGenDudeFireW) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
                    pXSprite->aiState->at18 = &GDXGenDudeFireW;

                }
                else if (state == 2 && pXSprite->aiState != &GDXGenDudeChaseD && pXSprite->aiState != &GDXGenDudeFireD) {
                    aiNewState(pSprite, pXSprite, &GDXGenDudeChaseD);
                    pXSprite->aiState->at18 = &GDXGenDudeFireD;

                }
                else if (pXSprite->aiState != &GDXGenDudeChaseL && pXSprite->aiState != &GDXGenDudeFireL) {
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

/*bool sub_5BDA8(spritetype* pSprite, int nSeq)
{
    if (pSprite->statnum == 6 && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
    {
        DUDEINFO* pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
        if (seqGetID(3, pSprite->extra) == pDudeInfo->seqStartID + nSeq && seqGetStatus(3, pSprite->extra) >= 0)
            return true;
    }
    return false;
}

bool sub_57901(spritetype* pSprite, int nSeqID) {
    if ( pSprite->statnum == 6 )
    {
        if ( IsDudeSprite(pSprite) )
        {
            SEQINST* pSeqInst = GetInstance(3, pSprite->extra); Seq* pSeq = pSeqInst->pSequence;
            if ( pSeq == pSEQs.get(xsprite[pSprite->extra].data2 + nSeqID) && seqGetStatus(3, pSprite->extra) >= 0 )
                return true;
        }
    }
    return false;
}*/

bool TargetNearThing(spritetype* pSprite, int thingType) {
    for ( int nSprite = headspritesect[pSprite->sectnum]; nSprite >= 0; nSprite = nextspritesect[nSprite] )
    {
        // check for required things or explosions in the same sector as the target
        if ( sprite[nSprite].type == thingType || sprite[nSprite].statnum == 2 )
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
            
        //System.err.println(pXSprite.busyTime);
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

    int frontSpeed = getGenDudeMoveSpeed(pSprite,0,true,false);
    xvel[pSprite->xvel] += mulscale(cos, frontSpeed, 30);
    yvel[pSprite->xvel] += mulscale(sin, frontSpeed, 30);
}
    
bool sfxPlayGDXGenDudeSound(spritetype* pSprite, int mode) {
    
    if (mode < 0 || mode >= kMaxGenDudeSndMode) return false;
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
        if (xsprite[pSprite->extra].medium == 1 || xsprite[pSprite->extra].medium == 2)
            return true;
        return false;
    }
        
    if (sector[pSprite->sectnum].extra < 0) return false;
    else if (xsector[sector[pSprite->sectnum].extra].Underwater)
        return true;
        
    return false;
}
    
spritetype* leechIsDropped(spritetype* pSprite) {
    for (int nSprite = headspritestat[4]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].lotag == kGDXThingCustomDudeLifeLeech && sprite[nSprite].owner == pSprite->xvel)
            return &sprite[nSprite];
    }
        
    return NULL;
        
}
    
void removeDudeStuff(spritetype* pSprite) {
    for (short nSprite = headspritestat[4]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].owner != pSprite->xvel) continue;
        switch (sprite[nSprite].lotag) {
        case 401:
        case 402:
        case 433:
            sprite[nSprite].lotag = 0;
            actPostSprite(sprite[nSprite].xvel, kStatFree);
            break;
        case kGDXThingCustomDudeLifeLeech:
            killDudeLeech(&sprite[nSprite]);
            break;
        }
    }

    for (short nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
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
        if (delSprite) {
            pLeech->type = 0;
            actPostSprite(pLeech->index, kStatFree);
        }
    }
}
    
void killDudeLeech(spritetype* pLeech) {
    if (pLeech != NULL) {
        actDamageSprite(pLeech->owner, pLeech, DAMAGE_TYPE_3, 65535);
        sfxPlay3DSoundCP(pLeech, 522, -1, 0, 60000);
    }
}
    
XSPRITE* getNextIncarnation(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != 3 || rxBucket[i].index == pXSprite->reference)
            continue;
        
        switch (sprite[rxBucket[i].index].statnum) {
            case 6:
            case 7: // inactive (ambush) dudes
                if (xsprite[sprite[rxBucket[i].index].extra].health > 0)
                    return &xsprite[sprite[rxBucket[i].index].extra];
        }

    }
    return NULL;
}

bool dudeIsMelee(XSPRITE* pXSprite) {
    int meleeDist = 2048; int vdist = meleeDist;
    if (pXSprite->data1 >= 0 && pXSprite->data1 < kVectorMax) {
        int vector = pXSprite->data1; if (vector <= 0) vector = 2;
        VECTORDATA pVectorData = gVectorData[vector];
        vdist = pVectorData.maxDist;

        if (vdist > 0 && vdist <= meleeDist)
            return true;

    } else {

        if (pXSprite->data1 >= 459 && pXSprite->data1 < (459 + kExplodeMax) - 1)
            return true;

        /*switch (pXSprite->data1) {
            case 304:
            case 308:
                return true;
            default:
                return false;
        }*/
    }

    return false;
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

void dudeLeechOperate(spritetype* pSprite, XSPRITE* pXSprite, EVENT a3)
{
    if (a3.cmd == COMMAND_ID_0) {
        actPostSprite(pSprite->xvel, kStatFree);
        return;
    }
    
    int nTarget = pXSprite->target;
    if (nTarget >= 0 && nTarget < kMaxSprites) {
        spritetype* pTarget = &sprite[nTarget];
        if (pTarget->statnum == 6 && !(pTarget->hitag & 32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites && !pXSprite->stateTimer)
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
                int nMissileType = 316 + (pXSprite->data3 ? 1 : 0);
                int t2;
                
                if (!pXSprite->data3) t2 = 120 / 10.0;
                else t2 = (3 * 120) / 10.0;

                spritetype * pMissile = actFireMissile(pSprite, 0, z1, dx, dy, dz, nMissileType);
                if (pMissile)
                {
                    pMissile->owner = pSprite->owner;
                    pXSprite->stateTimer = 1;
                    evPost(pSprite->index, 3, t2, CALLBACK_ID_20);
                    pXSprite->data3 = ClipLow(pXSprite->data3 - 1, 0);
                }
                pSprite->ang = angBak;
            }
        }
        
    }
}

bool doExplosion(spritetype* pSprite, int nType) {
    spritetype* pExplosion = actSpawnSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 2, true);
    int nSeq = 4; int nSnd = 304; EXPLOSION* pExpl = &explodeInfo[nType];

    pExplosion->yrepeat = pExpl->at0;
    pExplosion->xrepeat = pExpl->at0;
    pExplosion->lotag = nType;
    pExplosion->cstat |= CSTAT_SPRITE_INVISIBLE | CSTAT_SPRITE_ALIGNMENT_SLAB;
    pExplosion->owner = pSprite->xvel;

    if (pExplosion->extra >= 0) {
        xsprite[pExplosion->extra].target = 0;
        xsprite[pExplosion->extra].data1 = pExpl->atf;
        xsprite[pExplosion->extra].data2 = pExpl->at13;
        xsprite[pExplosion->extra].data3 = pExpl->at17;


        if (nType == 0) { nSeq = 3; nSnd = 303; pExplosion->z = pSprite->z; }
        else if (nType == 2) { nSeq = 4; nSnd = 305; }
        else if (nType == 3) { nSeq = 9; nSnd = 307; }
        else if (nType == 4) { nSeq = 5; nSnd = 307; }
        else if (nType <= 6) { nSeq = 4; nSnd = 303; }
        else if (nType == 7) { nSeq = 4; nSnd = 303; }


        if (fileExistsRFF(nSeq, "SEQ")) seqSpawn(nSeq, 3, pExplosion->extra, -1);
        sfxPlay3DSound(pExplosion, nSnd, -1, 0);

        return true;
    }

    return false;
}


void updateTargetOfSlaves(spritetype* pSprite) {
    for (short nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        if (sprite[nSprite].owner != pSprite->xvel || sprite[nSprite].extra < 0 || !IsDudeSprite(&sprite[nSprite])) continue;
        else if (xsprite[pSprite->extra].target != xsprite[sprite[nSprite].extra].target
            && IsDudeSprite(&sprite[xsprite[pSprite->extra].target])) {
            aiSetTarget(&xsprite[sprite[nSprite].extra], xsprite[pSprite->extra].target);
        }

        if (xsprite[sprite[nSprite].extra].target >= 0) {
            // don't attack mates
            if (sprite[xsprite[sprite[nSprite].extra].target].owner == sprite[nSprite].owner)
                aiSetTarget(&xsprite[sprite[nSprite].extra], pSprite->x, pSprite->y, pSprite->z);
        }

        if (!isActive(sprite[nSprite].xvel) && xsprite[sprite[nSprite].extra].target >= 0)
            aiActivateDude(&sprite[nSprite], &xsprite[sprite[nSprite].extra]);
    }

    return;
}

bool canSwim(spritetype* pSprite) {
    return (gSysRes.Lookup(xsprite[pSprite->extra].data2 + 8, "SEQ") && gSysRes.Lookup(xsprite[pSprite->extra].data2 + 13, "SEQ")
        && gSysRes.Lookup(xsprite[pSprite->extra].data2 + 17, "SEQ"));
}


bool canDuck(spritetype* pSprite) {
    return (gSysRes.Lookup(xsprite[pSprite->extra].data2 + 8, "SEQ") && gSysRes.Lookup(xsprite[pSprite->extra].data2 + 14, "SEQ"));

}

bool CDCanMove(spritetype* pSprite) {
    return (gSysRes.Lookup(xsprite[pSprite->extra].data2 + 9, "SEQ") && gSysRes.Lookup(xsprite[pSprite->extra].data2 + 13, "SEQ")
        && gSysRes.Lookup(xsprite[pSprite->extra].data2 + 14, "SEQ"));

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
        for (int i = seqStartId; i <= seqStartId + 19; i++) {

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
                continue;
            }

            if (!gSysRes.Lookup(i, "SEQ")) {
                pXSprite->data2 = dudeInfo[sprite[pXSprite->reference].type - kDudeBase].seqStartID;
                return pXSprite->data2;
            }

        }

    }  else {
        pXSprite->data2 = seqStartId;
    }

    return seqStartId;
}
//////////
