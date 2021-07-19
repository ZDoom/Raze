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
#include "raze_sound.h"

#include "blood.h"

#ifdef NOONE_EXTENSIONS


BEGIN_BLD_NS
static void ThrowThing(DBloodActor*, bool);
static void unicultThinkSearch(DBloodActor*);
static void unicultThinkGoto(DBloodActor*);
static void unicultThinkChase(DBloodActor*);
static void forcePunch(DBloodActor*);

AISTATE genDudeIdleL = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE genDudeIdleW = { kAiStateIdle, 13, -1, 0, NULL, NULL, aiThinkTarget, NULL };
// ---------------------
AISTATE genDudeSearchL = { kAiStateSearch, 9, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchW = { kAiStateSearch, 13, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeSearchShortL = { kAiStateSearch, 9, -1, 200, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchShortW = { kAiStateSearch, 13, -1, 200, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeSearchNoWalkL = { kAiStateSearch, 0, -1, 600, NULL, aiMoveTurn, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchNoWalkW = { kAiStateSearch, 13, -1, 600, NULL, aiMoveTurn, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeGotoL = { kAiStateMove, 9, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkGoto, &genDudeIdleL };
AISTATE genDudeGotoW = { kAiStateMove, 13, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkGoto, &genDudeIdleW };
// ---------------------
AISTATE genDudeDodgeL = { kAiStateMove, 9, -1, 90, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeD = { kAiStateMove, 14, -1, 90, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeW = { kAiStateMove, 13, -1, 90, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeDodgeShortL = { kAiStateMove, 9, -1, 60, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeShortD = { kAiStateMove, 14, -1, 60, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeShortW = { kAiStateMove, 13, -1, 60, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeDodgeShorterL = { kAiStateMove, 9, -1, 20, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeShorterD = { kAiStateMove, 14, -1, 20, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeShorterW = { kAiStateMove, 13, -1, 20, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeChaseL = { kAiStateChase, 9, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
AISTATE genDudeChaseD = { kAiStateChase, 14, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
AISTATE genDudeChaseW = { kAiStateChase, 13, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
// ---------------------
AISTATE genDudeChaseNoWalkL = { kAiStateChase, 0, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
AISTATE genDudeChaseNoWalkD = { kAiStateChase, 14, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
AISTATE genDudeChaseNoWalkW = { kAiStateChase, 13, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
// ---------------------
AISTATE genDudeFireL = { kAiStateChase, 6, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireL };
AISTATE genDudeFireD = { kAiStateChase, 8, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireD };
AISTATE genDudeFireW = { kAiStateChase, 8, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireW };
// ---------------------z
AISTATE genDudeRecoilL = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseL };
AISTATE genDudeRecoilD = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseD };
AISTATE genDudeRecoilW = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseW };
AISTATE genDudeRecoilTesla = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &genDudeDodgeShortL };
// ---------------------
AISTATE genDudeThrow = { kAiStateChase, 7, nGenDudeThrow1, 0, NULL, NULL, NULL, &genDudeChaseL };
AISTATE genDudeThrow2 = { kAiStateChase, 7, nGenDudeThrow2, 0, NULL, NULL, NULL, &genDudeChaseL };
// ---------------------
AISTATE genDudePunch = { kAiStateChase,10, nGenDudePunch, 0, NULL, NULL, forcePunch, &genDudeChaseL };
// ---------------------

const GENDUDESND gCustomDudeSnd[] = {
    { 1003, 2, 0, true, false   },      // spot sound
    { 1013, 2, 2, true, true    },      // pain sound
    { 1018, 2, 4, false, true   },      // death sound
    { 1031, 2, 6, true, true    },      // burning state sound
    { 1018, 2, 8, false, true   },      // explosive death or end of burning state sound
    { 4021, 2, 10, true, false  },	    // target of dude is dead
    { 1005, 2, 12, true, false  },	    // chase sound
    { -1, 0, 14, false, true    },	    // weapon attack
    { -1, 0, 15, false, true    },	    // throw attack
    { -1, 0, 16, false, true    },	    // melee attack
    { 9008, 0, 17, false, false },      // transforming in other dude
};

// for kModernThingThrowableRock
short gCustomDudeDebrisPics[6] = {
    
    2406, 2280, 2185, 2155, 2620, 3135

};

GENDUDEEXTRA gGenDudeExtra[kMaxSprites]; // savegame handling in ai.cpp

static void forcePunch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (gGenDudeExtra[pSprite->index].forcePunch && seqGetStatus(3, pSprite->extra) == -1)
        punchCallback(0, actor);
}

/*bool sameFamily(spritetype* pDude1, spritetype* pDude2) {

    return true;
}*/

static bool genDudeAdjustSlope(DBloodActor* actor, int dist, int weaponType, int by = 64) 
{
    spritetype* pSprite = &actor->s();
    XSPRITE* pXSprite = &actor->x();
    if (spriRangeIsFine(pXSprite->target)) 
    {
        int fStart = 0; 
        int fEnd = 0; 
        GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
        unsigned int clipMask = (weaponType == kGenDudeWeaponMissile) ? CLIPMASK0 : CLIPMASK1;

        for (int i = -8191; i < 8192; i += by) 
        {
            HitScan(pSprite, pSprite->z, CosScale16(pSprite->ang), SinScale16(pSprite->ang), i, clipMask, dist);
            if (!fStart && pXSprite->target == gHitInfo.hitsprite) fStart = i;
            else if (fStart && pXSprite->target != gHitInfo.hitsprite) 
            { 
                fEnd = i; 
                break; 
            }
        }

        if (fStart != fEnd) 
        {
            if (weaponType == kGenDudeWeaponHitscan)
            {
                actor->dudeSlope = fStart - ((fStart - fEnd) >> 2);
            }
            else if (weaponType == kGenDudeWeaponMissile) 
            {
                const MissileType* pMissile = &missileInfo[pExtra->curWeapon - kMissileBase];
                actor->dudeSlope = (fStart - ((fStart - fEnd) >> 2)) - (pMissile->clipDist << 1);
            }

            return true;
        }
    }

    return false;

}

GENDUDEEXTRA* genDudeExtra(spritetype* pGenDude) {
    return &gGenDudeExtra[pGenDude->index];
}

void genDudeUpdate(spritetype* pSprite) {
    GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
    for (int i = 0; i < kGenDudePropertyMax; i++) {
        if (pExtra->updReq[i]) genDudePrepare(pSprite, i);
    }
}

void punchCallback(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();
    if (pXSprite->target != -1) 
    {
        int nZOffset1 = getDudeInfo(pSprite->type)->eyeHeight * pSprite->yrepeat << 2;
        int nZOffset2 = 0;
        
        spritetype* pTarget = &sprite[pXSprite->target];
        if(IsDudeSprite(pTarget))
            nZOffset2 = getDudeInfo(pTarget->type)->eyeHeight * pTarget->yrepeat << 2;

        int dx = CosScale16(pSprite->ang);
        int dy = SinScale16(pSprite->ang);
        int dz = nZOffset1 - nZOffset2;

        if (!playGenDudeSound(pSprite, kGenDudeSndAttackMelee))
            sfxPlay3DSound(pSprite, 530, 1, 0);

        actFireVector(pSprite, 0, 0, dx, dy, dz,kVectorGenDudePunch);
    }
}

void genDudeAttack1(int, DBloodActor* actor)
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();

    if (pXSprite->target < 0) return;

    int dx, dy, dz;
    xvel[pSprite->index] = yvel[pSprite->index] = 0;
    
    GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
    short dispersion = pExtra->baseDispersion;
    if (inDuck(pXSprite->aiState))
        dispersion = ClipLow(dispersion >> 1, kGenDudeMinDispesion);

    if (pExtra->weaponType == kGenDudeWeaponHitscan) {

        dx = CosScale16(pSprite->ang); dy = SinScale16(pSprite->ang); dz = actor->dudeSlope;
        // dispersal modifiers here in case if non-melee enemy
        if (!dudeIsMelee(pXSprite)) {
            dx += Random3(dispersion); dy += Random3(dispersion); dz += Random3(dispersion);
        }

        actFireVector(pSprite, 0, 0, dx, dy, dz,(VECTOR_TYPE)pExtra->curWeapon);
        if (!playGenDudeSound(pSprite, kGenDudeSndAttackNormal))
            sfxPlayVectorSound(pSprite, pExtra->curWeapon);
            
    } else if (pExtra->weaponType == kGenDudeWeaponSummon) {

        spritetype* pSpawned = NULL; int dist = pSprite->clipdist << 4; 
        if (pExtra->slaveCount <= gGameOptions.nDifficulty) {
            if ((pSpawned = actSpawnDude(pSprite, pExtra->curWeapon, dist + Random(dist), 0)) != NULL) {
                pSpawned->owner = pSprite->index;

                if (xspriRangeIsFine(pSpawned->extra)) {
                    xsprite[pSpawned->extra].target = pXSprite->target;
                    if (pXSprite->target > -1)
                        aiActivateDude(&bloodActors[pSpawned->index]);
                }

                gKillMgr.AddNewKill(1);
                pExtra->slave[pExtra->slaveCount++] = pSpawned->index;
                if (!playGenDudeSound(pSprite, kGenDudeSndAttackNormal))
                    sfxPlay3DSoundCP(pSprite, 379, 1, 0, 0x10000 - Random3(0x3000));
            }
        }

    } else if (pExtra->weaponType == kGenDudeWeaponMissile) {

        dx = CosScale16(pSprite->ang); dy = SinScale16(pSprite->ang); dz = actor->dudeSlope;

        // dispersal modifiers here
        dx += Random3(dispersion); dy += Random3(dispersion); dz += Random3(dispersion >> 1);

        actFireMissile(pSprite, 0, 0, dx, dy, dz, pExtra->curWeapon);
        if (!playGenDudeSound(pSprite, kGenDudeSndAttackNormal))
            sfxPlayMissileSound(pSprite, pExtra->curWeapon);
    }
}

void ThrowCallback1(int, DBloodActor* actor)
{
    ThrowThing(actor, true);
}

void ThrowCallback2(int, DBloodActor* actor)
{
    ThrowThing(actor, false);
}

static void ThrowThing(DBloodActor* actor, bool impact) 
{
    XSPRITE* pXSprite = &actor->x();
    spritetype* pSprite = &actor->s();

    if (!(pXSprite->target >= 0 && pXSprite->target < kMaxSprites))
        return;

    spritetype * pTarget = &sprite[pXSprite->target];
    if (!(pTarget->type >= kDudeBase && pTarget->type < kDudeMax))
        return;

    short curWeapon = gGenDudeExtra[sprite[pXSprite->reference].index].curWeapon;
    short weaponType = gGenDudeExtra[sprite[pXSprite->reference].index].weaponType;
    if (weaponType != kGenDudeWeaponThrow) return;

    const THINGINFO* pThinkInfo = &thingInfo[curWeapon - kThingBase];
    if (!gThingInfoExtra[curWeapon - kThingBase].allowThrow) return;
    else if (!playGenDudeSound(pSprite, kGenDudeSndAttackThrow))
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
    if ((pThing = actFireThing_(pSprite, 0, 0, (dz / 128) - zThrow, curWeapon, DivScale(dist / 540, 120, 23))) == NULL) return;
    else if (pThinkInfo->picnum < 0 && pThing->type != kModernThingThrowableRock) pThing->picnum = 0;
            
    pThing->owner = pSprite->index;
            
    switch (curWeapon) {
        case kThingNapalmBall:
            pThing->xrepeat = pThing->yrepeat = 24;
            xsprite[pThing->extra].data4 = 3 + gGameOptions.nDifficulty;
            impact = true;
            break;
        case kModernThingThrowableRock:
            pThing->picnum  = gCustomDudeDebrisPics[Random(5)];
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
            else pXThing->health = ((pThinkInfo->startHealth << 4) * gGameOptions.nDifficulty) >> 1;

            sfxPlay3DSound(pSprite, 490, -1, 0);

            pXThing->data3 = 512 / (gGameOptions.nDifficulty + 1);
            pThing->cstat &= ~CSTAT_SPRITE_BLOCK;
            pThing->pal = 6;
            pThing->clipdist = 0;
            pXThing->target = pTarget->index;
            pXThing->Proximity = true;
            pXThing->stateTimer = 1;
                
            gGenDudeExtra[pSprite->index].nLifeLeech = pThing->index;
            evPost(pThing->index, 3, 80, kCallbackLeechStateTimer);
            return;
    }

    if (impact == true && dist <= 7680) xsprite[pThing->extra].Impact = true;
    else {
        xsprite[pThing->extra].Impact = false;
        evPost(pThing->index, 3, 120 * Random(2) + 120, kCmdOn);
    }
}

static void unicultThinkSearch(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    // TO DO: if can't see the target, but in fireDist range - stop moving and look around
    
    //viewSetSystemMessage("IN SEARCH");
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    sub_5F15C(pSprite, pXSprite);
}

static void unicultThinkGoto(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }

    int dx = pXSprite->targetX - pSprite->x;
    int dy = pXSprite->targetY - pSprite->y;
    int nAngle = getangle(dx, dy);

    aiChooseDirection(pSprite, pXSprite, nAngle);

    // if reached target, change to search mode
    if (approxDist(dx, dy) < 5120 && abs(pSprite->ang - nAngle) < getDudeInfo(pSprite->type)->periphery) {
        if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeSearchW);
        else aiGenDudeNewState(pSprite, &genDudeSearchL);
    }
    aiThinkTarget(actor);
}

static void unicultThinkChase(DBloodActor* actor)
{
    auto const pXSprite = &actor->x();
    auto const pSprite = &actor->s();
    if (pSprite->type < kDudeBase || pSprite->type >= kDudeMax) return;
    else if (pXSprite->target < 0 || pXSprite->target >= kMaxSprites) {
        if(spriteIsUnderwater(pSprite,false)) aiGenDudeNewState(pSprite, &genDudeGotoW);
        else aiGenDudeNewState(pSprite, &genDudeGotoL);
        return;
    } else {
        
        genDudeUpdate(pSprite);

    }

    spritetype* pTarget = &sprite[pXSprite->target];
    XSPRITE* pXTarget = (!IsDudeSprite(pTarget) || !xspriRangeIsFine(pTarget->extra)) ? NULL : &xsprite[pTarget->extra];

    if (pXTarget == NULL) {  // target lost
        if(spriteIsUnderwater(pSprite,false)) aiGenDudeNewState(pSprite, &genDudeSearchShortW);
        else aiGenDudeNewState(pSprite, &genDudeSearchShortL);
        pXSprite->target = -1;
        return;

    } else if (pXTarget->health <= 0) { // target is dead
        PLAYER* pPlayer = NULL;
        if ((!IsPlayerSprite(pTarget)) || ((pPlayer = getPlayerById(pTarget->type)) != NULL && pPlayer->fraggerId == pSprite->index)) {
            playGenDudeSound(pSprite, kGenDudeSndTargetDead);
            if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeSearchShortW);
            else aiGenDudeNewState(pSprite, &genDudeSearchShortL);
        } 
        else if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeGotoW);
        else aiGenDudeNewState(pSprite, &genDudeGotoL);
        pXSprite->target = -1;
        return;
    }
    
    // check target
    int dx = pTarget->x - pSprite->x; int dy = pTarget->y - pSprite->y;
    int dist = ClipLow((int)approxDist(dx, dy), 1);

    // quick hack to prevent spinning around or changing attacker's sprite angle on high movement speeds
    // when attacking the target. It happens because vanilla function takes in account x and y velocity, 
    // so i use fake velocity with fixed value and pass it as argument.
    int xvelocity = xvel[pSprite->index]; int yvelocity = yvel[pSprite->index];
    if (inAttack(pXSprite->aiState))
       xvelocity = yvelocity = ClipLow(pSprite->clipdist >> 1, 1);

    //aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    aiGenDudeChooseDirection(pSprite, pXSprite, getangle(dx, dy), xvelocity, yvelocity);

    GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index];
    if (!pExtra->canAttack) {
        if (pExtra->canWalk) aiSetTarget(pXSprite, pSprite->index);
        if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeGotoW);
        else aiGenDudeNewState(pSprite, &genDudeGotoL);
        return;
    } else if (IsPlayerSprite(pTarget)) {
        PLAYER* pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
        if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)  {
            if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeSearchShortW);
            else aiGenDudeNewState(pSprite, &genDudeSearchShortL);
            pXSprite->target = -1;
            return;
        }
    }
    
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    int losAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
    int eyeAboveZ = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;

    if (dist > pDudeInfo->seeDist || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
        pSprite->x, pSprite->y, pSprite->z - eyeAboveZ, pSprite->sectnum)) {

        if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeSearchW);
        else aiGenDudeNewState(pSprite, &genDudeSearchL);
        pXSprite->target = -1;
        return;
    }

    // is the target visible?
    if (dist < pDudeInfo->seeDist && abs(losAngle) <= pDudeInfo->periphery) {

        if ((PlayClock & 64) == 0 && Chance(0x3000) && !spriteIsUnderwater(pSprite, false))
            playGenDudeSound(pSprite, kGenDudeSndChasing);

        actor->dudeSlope = DivScale(pTarget->z - pSprite->z, dist, 10);

        short curWeapon = gGenDudeExtra[pSprite->index].curWeapon; short weaponType = gGenDudeExtra[pSprite->index].weaponType;
        spritetype* pLeech = leechIsDropped(pSprite); const VECTORDATA* meleeVector = &gVectorData[22];
        if (weaponType == kGenDudeWeaponThrow) {
            if (abs(losAngle) < kAng15) {
                if (!gThingInfoExtra[curWeapon - kThingBase].allowThrow) {
                    if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                    else aiGenDudeNewState(pSprite, &genDudeChaseL);
                    return;

                } else if (dist < 12264 && dist > 7680 && !spriteIsUnderwater(pSprite, false) && curWeapon != kModernThingEnemyLifeLeech) {
                    int pHit = HitScan(pSprite, pSprite->z, dx, dy, 0, 16777280, 0);
                    switch (pHit) {
                        case 0:
                        case 4:
                            return;
                        default:
                            aiGenDudeNewState(pSprite, &genDudeThrow);
                            return;
                    }

                } else if (dist > 4072 && dist <= 11072 && !spriteIsUnderwater(pSprite, false) && pSprite->owner != (kMaxSprites - 1)) {
                    switch (curWeapon) {
                        case kModernThingEnemyLifeLeech: {
                            if (pLeech == NULL) {
                                aiGenDudeNewState(pSprite, &genDudeThrow2);
                                genDudeThrow2.nextState = &genDudeDodgeShortL;
                                return;
                            }

                            XSPRITE* pXLeech = &xsprite[pLeech->extra];
                            int ldist = aiFightGetTargetDist(pTarget, pDudeInfo, pLeech);
                            if (ldist > 3 || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
                                pLeech->x, pLeech->y, pLeech->z, pLeech->sectnum) || pXLeech->target == -1) {

                                aiGenDudeNewState(pSprite, &genDudeThrow2);
                                genDudeThrow2.nextState = &genDudeDodgeShortL;

                            } else {
                                
                                genDudeThrow2.nextState = &genDudeChaseL;
                                if (dist > 5072 && Chance(0x5000)) {
                                    if (!canDuck(pSprite) || Chance(0x4000)) aiGenDudeNewState(pSprite, &genDudeDodgeShortL);
                                    else aiGenDudeNewState(pSprite, &genDudeDodgeShortD);
                                } else {
                                    aiGenDudeNewState(pSprite, &genDudeChaseL);
                                }

                            }
                        }
                        return;
                        case kModernThingThrowableRock:
                            if (Chance(0x4000)) aiGenDudeNewState(pSprite, &genDudeThrow2);
                            else playGenDudeSound(pSprite, kGenDudeSndTargetSpot);
                            return;
                        default:
                            aiGenDudeNewState(pSprite, &genDudeThrow2);
                            return;
                    }

                } else if (dist <= meleeVector->maxDist) {

                    if (spriteIsUnderwater(pSprite, false)) {
                        if (Chance(0x9000)) aiGenDudeNewState(pSprite, &genDudePunch);
                        else aiGenDudeNewState(pSprite, &genDudeDodgeW);

                    }
                    else if (Chance(0x9000)) aiGenDudeNewState(pSprite, &genDudePunch);
                    else aiGenDudeNewState(pSprite, &genDudeDodgeL);
                    return;

                } else {
                    int state = checkAttackState(&bloodActors[pXSprite->reference]);
                    if (state == 1) aiGenDudeNewState(pSprite, &genDudeChaseW);
                    else if (state == 2) {
                        if (Chance(0x5000)) aiGenDudeNewState(pSprite, &genDudeChaseD);
                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                    }
                    else  aiGenDudeNewState(pSprite, &genDudeChaseL);
                    return;
                }
            }

        } else {

            int vdist; int mdist; int defDist;
            defDist = vdist = mdist = gGenDudeExtra[pSprite->index].fireDist;

            if (weaponType == kGenDudeWeaponHitscan) {
                if ((vdist = gVectorData[curWeapon].maxDist) <= 0)
                    vdist = defDist;

            } else if (weaponType == kGenDudeWeaponSummon) {

                // don't attack slaves
                if (pXSprite->target >= 0 && sprite[pXSprite->target].owner == pSprite->index) {
                    aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                    return;
                } else if (gGenDudeExtra[pSprite->index].slaveCount > gGameOptions.nDifficulty || dist < meleeVector->maxDist) {
                    if (dist <= meleeVector->maxDist) {
                        aiGenDudeNewState(pSprite, &genDudePunch);
                        return;
                    } else {
                        int state = checkAttackState(&bloodActors[pXSprite->reference]);
                        if (state == 1) aiGenDudeNewState(pSprite, &genDudeChaseW);
                        else if (state == 2) aiGenDudeNewState(pSprite, &genDudeChaseD);
                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                        return;
                    }
                }

            } else if (weaponType == kGenDudeWeaponMissile) {
                // special handling for flame, explosive and life leech missiles
                int state = checkAttackState(&bloodActors[pXSprite->reference]);
                switch (curWeapon) {
                    case kMissileLifeLeechRegular:
                        // pickup life leech if it was thrown previously
                        if (pLeech != NULL) removeLeech(pLeech);
                        mdist = 1500;
                        break;
                    case kMissileFlareAlt:
                        mdist = 2500;
                        fallthrough__;
                    case kMissileFireball:
                    case kMissileFireballNapam:
                    case kMissileFireballCerberus:
                    case kMissileFireballTchernobog:
                        if (mdist == defDist) mdist = 3000;
                        if (dist > mdist || pXSprite->locked == 1) break;
                        else if (dist <= meleeVector->maxDist && Chance(0x9000))
                            aiGenDudeNewState(pSprite, &genDudePunch);
                        else if (state == 1) aiGenDudeNewState(pSprite, &genDudeChaseW);
                        else if (state == 2) aiGenDudeNewState(pSprite, &genDudeChaseD);
                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                        return;
                    case kMissileFlameSpray:
                    case kMissileFlameHound:
                        //viewSetSystemMessage("%d", pXTarget->burnTime);
                        if (spriteIsUnderwater(pSprite, false)) {
                            if (dist > meleeVector->maxDist) aiGenDudeNewState(pSprite, &genDudeChaseW);
                            else if (Chance(0x8000)) aiGenDudeNewState(pSprite, &genDudePunch);
                            else aiGenDudeNewState(pSprite, &genDudeDodgeShortW);
                            return;
                        } else if (dist <= 4000 && pXTarget->burnTime >= 2000 && pXTarget->burnSource == pSprite->index) {
                            if (dist > meleeVector->maxDist) aiGenDudeNewState(pSprite, &genDudeChaseL);
                            else aiGenDudeNewState(pSprite, &genDudePunch);
                            return;
                    }
                        vdist = 3500 + (gGameOptions.nDifficulty * 400);
                    break;
                }
            } else if (weaponType == kGenDudeWeaponKamikaze) {
                int nType = curWeapon - kTrapExploder; const EXPLOSION* pExpl = &explodeInfo[nType];
                if (CheckProximity(pSprite, pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pExpl->radius >> 1)) {
                    xvel[pSprite->index] = zvel[pSprite->index] = yvel[pSprite->index] = 0;
                    if (doExplosion(pSprite, nType) && pXSprite->health > 0)
                            actDamageSprite(pSprite->index, pSprite, kDamageExplode, 65535);
                }
                return;
                }

            int state = checkAttackState(&bloodActors[pXSprite->reference]);
            int kAngle = (dudeIsMelee(pXSprite) || dist <= kGenDudeMaxMeleeDist) ? pDudeInfo->periphery : kGenDudeKlabsAng;

            if (dist < vdist && abs(losAngle) < kAngle) {
                if (pExtra->canWalk) {
                    int objDist = -1; int targetDist = -1; int hit = -1;
                    if (weaponType == kGenDudeWeaponHitscan)
                        hit = HitScan(pSprite, pSprite->z, CosScale16(pSprite->ang), SinScale16(pSprite->ang), actor->dudeSlope, CLIPMASK1, dist);
                    else if (weaponType == kGenDudeWeaponMissile)
                        hit = HitScan(pSprite, pSprite->z, CosScale16(pSprite->ang), SinScale16(pSprite->ang), actor->dudeSlope, CLIPMASK0, dist);
                    
                    if (hit >= 0) {
                        targetDist = dist - (pTarget->clipdist << 2);
                        objDist = approxDist(gHitInfo.hitx - pSprite->x, gHitInfo.hity - pSprite->y);
                    }

                    if (pXSprite->target != gHitInfo.hitsprite && targetDist > objDist) {
                        walltype* pHWall = NULL; XWALL* pXHWall = NULL;
                        spritetype* pHSprite = NULL; XSPRITE* pXHSprite = NULL;
                        bool hscn = false; bool blck = false; bool failed = false;

                        switch (hit) {
                        case 3:
                            pHSprite = &sprite[gHitInfo.hitsprite];
                            if (xspriRangeIsFine(pHSprite->extra)) pXHSprite = &xsprite[pHSprite->extra];
                            hscn = (pHSprite->cstat & CSTAT_SPRITE_BLOCK_HITSCAN); blck = (pHSprite->cstat & CSTAT_SPRITE_BLOCK);
                            break;
                        case 0:
                        case 4:
                            pHWall = &wall[gHitInfo.hitwall];
                            if (xwallRangeIsFine(pHWall->extra)) pXHWall = &xwall[pHWall->extra];
                            hscn = (pHWall->cstat & CSTAT_WALL_BLOCK_HITSCAN); blck = (pHWall->cstat & CSTAT_WALL_BLOCK);
                            break;
                        }

                        switch (hit) {
                        case 0:
                            //if (hit == 0) viewSetSystemMessage("WALL HIT %d", gHitInfo.hitwall);
                            fallthrough__;
                        case 1:
                            //if (hit == 1) viewSetSystemMessage("CEIL HIT %d", gHitInfo.hitsect);
                            fallthrough__;
                        case 2:
                            //if (hit == 2) viewSetSystemMessage("FLOOR HIT %d", gHitInfo.hitsect);
                            if (weaponType != kGenDudeWeaponMissile && genDudeAdjustSlope(actor, dist, weaponType) 
                                && dist < (int)(6000 + Random(2000)) && pExtra->baseDispersion < kGenDudeMaxDispersion >> 1) break;

                            else if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                            else aiGenDudeNewState(pSprite, &genDudeChaseL);
                            return;
                        case 3:
                            if (pHSprite->statnum == kStatFX || pHSprite->statnum == kStatProjectile || pHSprite->statnum == kStatDebris)
                                break;
                            if (IsDudeSprite(pHSprite) && (weaponType != kGenDudeWeaponHitscan || hscn)) {
                                // dodge a bit in sides
                                if (pXHSprite->target != pSprite->index) {
                                    if (pExtra->baseDispersion < 1024 && weaponType != kGenDudeWeaponMissile) {
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeDodgeShorterW);
                                        else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pSprite, &genDudeDodgeShorterD);
                                        else aiGenDudeNewState(pSprite, &genDudeDodgeShorterL);
                                    }
                                    else if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeDodgeShortW);
                                    else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pSprite, &genDudeDodgeShortD);
                                    else aiGenDudeNewState(pSprite, &genDudeDodgeShortL);

                                    switch (pHSprite->type) {
                                        case kDudeModernCustom: // and make dude which could be hit to dodge too
                                            if (!dudeIsMelee(pXHSprite) && Chance(dist << 4)) {
                                                if (!inAttack(pXHSprite->aiState)) {
                                                    if (spriteIsUnderwater(pHSprite)) aiGenDudeNewState(pHSprite, &genDudeDodgeShorterW);
                                                    else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pHSprite, &genDudeDodgeShorterD);
                                                    else aiGenDudeNewState(pHSprite, &genDudeDodgeShorterL);

                                                    // preferable in opposite sides
                                                    if (Chance(0x8000)) {
                                                        if (pXSprite->dodgeDir == 1) pXHSprite->dodgeDir = -1;
                                                        else if (pXSprite->dodgeDir == -1) pXHSprite->dodgeDir = 1;
                                                    }
                                                    break;
                                                }
                                                if (pSprite->x < pHSprite->x) 
                                                {
                                                    if (Chance(0x9000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = -1;
                                                    else pXSprite->dodgeDir = 1;
                                                }
                                                else 
                                                {
                                                    if (Chance(0x9000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = 1;
                                                    else pXSprite->dodgeDir = -1;
                                                }
                                            }
                                            break;
                                        default:
                                            if (pSprite->x < pHSprite->x) 
                                            {
                                                if (Chance(0x9000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = -1;
                                                else pXSprite->dodgeDir = 1;
                                            } 
                                            else 
                                            {
                                                if (Chance(0x9000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = 1;
                                                else pXSprite->dodgeDir = -1;
                                            }
                                            break;
                                    }
                                    return;
                                }
                                break;
                            } 
                            else if (weaponType == kGenDudeWeaponHitscan && hscn) 
                            {
                                if (genDudeAdjustSlope(actor, dist, weaponType)) break;
                                VectorScan(pSprite, 0, 0, CosScale16(pSprite->ang), SinScale16(pSprite->ang), actor->dudeSlope, dist, 1);
                                if (pXSprite->target == gHitInfo.hitsprite) break;
                                
                                bool immune = nnExtIsImmune(pHSprite, gVectorData[curWeapon].dmgType);
                                if (!(pXHSprite != NULL && (!immune || (immune && pHSprite->statnum == kStatThing && pXHSprite->Vector)) && !pXHSprite->locked)) 
                                {
                                    if ((approxDist(gHitInfo.hitx - pSprite->x, gHitInfo.hity - pSprite->y) <= 1500 && !blck)
                                        || (dist <= (int)(pExtra->fireDist / ClipLow(Random(4), 1)))) 
                                    {
                                        //viewSetSystemMessage("GO CHASE");
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                                        return;

                                    }

                                    int wd1 = picWidth(pHSprite->picnum, pHSprite->xrepeat);
                                    int wd2 = picWidth(pSprite->picnum, pSprite->xrepeat);
                                    if (wd1 < (wd2 << 3)) {
                                        //viewSetSystemMessage("OBJ SIZE: %d   DUDE SIZE: %d", wd1, wd2);
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeDodgeShorterW);
                                        else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pSprite, &genDudeDodgeShorterD);
                                        else aiGenDudeNewState(pSprite, &genDudeDodgeShorterL);

                                        if (pSprite->x < pHSprite->x) {
                                            if (Chance(0x3000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = -1;
                                            else pXSprite->dodgeDir = 1;
                                        } else {
                                            if (Chance(0x3000) && pTarget->x > pHSprite->x) pXSprite->dodgeDir = 1;
                                            else pXSprite->dodgeDir = -1;
                                        }

                                        if (((gSpriteHit[pSprite->extra].hit & 0xc000) == 0x8000) || ((gSpriteHit[pSprite->extra].hit & 0xc000) == 0xc000)) {
                                            if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                            else aiGenDudeNewState(pSprite, &genDudeChaseL);
                                            pXSprite->goalAng = Random(kAng360);
                                            //viewSetSystemMessage("WALL OR SPRITE TOUCH");
                                        }

                                    } else {
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                                        //viewSetSystemMessage("TOO BIG OBJECT TO DODGE!!!!!!!!");
                                    }
                                    return;
                                }
                                break;
                            }
                            fallthrough__;
                        case 4:
                            if (hit == 4 && weaponType == kGenDudeWeaponHitscan && hscn) {
                                bool masked = (pHWall->cstat & CSTAT_WALL_MASKED);
                                if (masked) VectorScan(pSprite, 0, 0, CosScale16(pSprite->ang), SinScale16(pSprite->ang), actor->dudeSlope, dist, 1);

                                //viewSetSystemMessage("WALL VHIT: %d", gHitInfo.hitwall);
                                if ((pXSprite->target != gHitInfo.hitsprite) && (pHWall->type != kWallGib || !masked || pXHWall == NULL || !pXHWall->triggerVector || pXHWall->locked)) {
                                    if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                    else aiGenDudeNewState(pSprite, &genDudeChaseL);
                                    return;
                                }
                            } else if (hit >= 3 && weaponType == kGenDudeWeaponMissile && blck) {
                                switch (curWeapon) {
                                case kMissileLifeLeechRegular:
                                case kMissileTeslaAlt:
                                case kMissileFlareAlt:
                                case kMissileFireball:
                                case kMissileFireballNapam:
                                case kMissileFireballCerberus:
                                case kMissileFireballTchernobog: {
                                    // allow attack if dude is far from object, but target is close to it
                                    int dudeDist = approxDist(gHitInfo.hitx - pSprite->x, gHitInfo.hity - pSprite->y);
                                    int targetDist = approxDist(gHitInfo.hitx - pTarget->x, gHitInfo.hity - pTarget->y);
                                    if (dudeDist < mdist) {
                                        //viewSetSystemMessage("DUDE CLOSE TO OBJ: %d, MDIST: %d", dudeDist, mdist);
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                        else aiGenDudeNewState(pSprite, &genDudeChaseL);
                                        return;
                                    } else if (targetDist <= mdist >> 1) {
                                        //viewSetSystemMessage("TARGET CLOSE TO OBJ: %d, MDIST: %d", targetDist, mdist >> 1);
                                        break;
                                    }
                                    fallthrough__;
                                }
                                default:
                                    //viewSetSystemMessage("DEF HIT: %d, MDIST: %d", hit, mdist);
                                    if (hit == 4) failed = (pHWall->type != kWallGib || pXHWall == NULL || !pXHWall->triggerVector || pXHWall->locked);
                                    else if (hit == 3 && (failed = (pHSprite->statnum != kStatThing || pXHSprite == NULL || pXHSprite->locked)) == false) {
                                        // check also for damage resistance (all possible damages missile can use)
                                        for (int i = 0; i < kDmgMax; i++) {
                                            if (gMissileInfoExtra[curWeapon - kMissileBase].dmgType[i] && (failed = nnExtIsImmune(pHSprite, i)) == false)
                                                break;
                                        }
                                    }

                                    if (failed) {
                                        if (spriteIsUnderwater(pSprite)) aiGenDudeNewState(pSprite, &genDudeSearchW);
                                        else aiGenDudeNewState(pSprite, &genDudeSearchL);
                                        return;
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                
                aiSetTarget(pXSprite, pXSprite->target);
                switch (state) {
                    case 1:
                        aiGenDudeNewState(pSprite, &genDudeFireW);
                        pXSprite->aiState->nextState = &genDudeFireW;
                        break;
                    case 2:
                        aiGenDudeNewState(pSprite, &genDudeFireD);
                        pXSprite->aiState->nextState = &genDudeFireD;
                        break;
                    default:
                        aiGenDudeNewState(pSprite, &genDudeFireL);
                        pXSprite->aiState->nextState = &genDudeFireL;
                        break;
                }


            } else {

                if (seqGetID(3, pSprite->extra) == pXSprite->data2 + ((state < 3) ? 8 : 6)) {
                    if (state == 1) pXSprite->aiState->nextState = &genDudeChaseW;
                    else if (state == 2) pXSprite->aiState->nextState = &genDudeChaseD;
                    else pXSprite->aiState->nextState = &genDudeChaseL;

                } else if (state == 1 && pXSprite->aiState != &genDudeChaseW && pXSprite->aiState != &genDudeFireW) {
                    aiGenDudeNewState(pSprite, &genDudeChaseW);
                    pXSprite->aiState->nextState = &genDudeFireW;

                } else if (state == 2 && pXSprite->aiState != &genDudeChaseD && pXSprite->aiState != &genDudeFireD) {
                    aiGenDudeNewState(pSprite, &genDudeChaseD);
                    pXSprite->aiState->nextState = &genDudeFireD;

                } else if (pXSprite->aiState != &genDudeChaseL && pXSprite->aiState != &genDudeFireL) {
                    aiGenDudeNewState(pSprite, &genDudeChaseL);
                    pXSprite->aiState->nextState = &genDudeFireL;
                }

            }
        }
    }
}


int checkAttackState(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    if (dudeIsPlayingSeq(pSprite, 14) || spriteIsUnderwater(pSprite,false))
    {
        if ( !dudeIsPlayingSeq(pSprite, 14) || spriteIsUnderwater(pSprite,false))
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

///// For gen dude
int getGenDudeMoveSpeed(spritetype* pSprite,int which, bool mul, bool shift) {
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
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
    
void aiGenDudeMoveForward(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index];
    int maxTurn = pDudeInfo->angSpeed * 4 >> 4;


    if (pExtra->canFly) {
        int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
        int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
        pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
        int nAccel = pDudeInfo->frontSpeed << 2;
        if (abs(nAng) > 341)
            return;
        if (pXSprite->target == -1)
            pSprite->ang = (pSprite->ang + 256) & 2047;
        int dx = pXSprite->targetX - pSprite->x;
        int dy = pXSprite->targetY - pSprite->y;
         int nDist = approxDist(dx, dy);
        if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
            return;
        int nCos = Cos(pSprite->ang);
        int nSin = Sin(pSprite->ang);
        int vx = xvel[pSprite->index];
        int vy = yvel[pSprite->index];
        int t1 = DMulScale(vx, nCos, vy, nSin, 30);
        int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
        if (pXSprite->target == -1)
            t1 += nAccel;
        else
            t1 += nAccel >> 1;
        xvel[pSprite->index] = DMulScale(t1, nCos, t2, nSin, 30);
        yvel[pSprite->index] = DMulScale(t1, nSin, -t2, nCos, 30);
    } else {
    int dang = ((kAng180 + pXSprite->goalAng - pSprite->ang) & 2047) - kAng180;
    pSprite->ang = ((pSprite->ang + ClipRange(dang, -maxTurn, maxTurn)) & 2047);

    // don't move forward if trying to turn around
        if (abs(dang) > kAng60)
        return;

    int sin = Sin(pSprite->ang);
    int cos = Cos(pSprite->ang);

        int frontSpeed = gGenDudeExtra[pSprite->index].moveSpeed;
        xvel[pSprite->index] += MulScale(cos, frontSpeed, 30);
        yvel[pSprite->index] += MulScale(sin, frontSpeed, 30);
    }
}

void aiGenDudeChooseDirection(spritetype* pSprite, XSPRITE* pXSprite, int a3, int xvel, int yvel) {
    if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
        return;
    }
    
    // TO-DO: Take in account if sprite is flip-x, so enemy select correct angle

    int vc = ((a3 + 1024 - pSprite->ang) & 2047) - 1024;
    int t1 = DMulScale(xvel, Cos(pSprite->ang), yvel, Sin(pSprite->ang), 30);
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

void aiGenDudeNewState(spritetype* pSprite, AISTATE* pAIState) {
    if (!xspriRangeIsFine(pSprite->extra)) {
        Printf(PRINT_HIGH, "!xspriRangeIsFine(pSprite->extra)");
        return;
    }

    XSPRITE* pXSprite = &xsprite[pSprite->extra];

    // redirect dudes which cannot walk to non-walk states
    if (!gGenDudeExtra[pSprite->index].canWalk) {
    
        if (pAIState == &genDudeDodgeL || pAIState == &genDudeDodgeShortL || pAIState == &genDudeDodgeShorterL) 
            pAIState = &genDudeRecoilL;

        else if (pAIState == &genDudeDodgeD || pAIState == &genDudeDodgeShortD || pAIState == &genDudeDodgeShorterD)
            pAIState = &genDudeRecoilD;

        else if (pAIState == &genDudeDodgeW || pAIState == &genDudeDodgeShortW || pAIState == &genDudeDodgeShorterW)
            pAIState = &genDudeRecoilW;

        else if (pAIState == &genDudeSearchL || pAIState == &genDudeSearchShortL) 
            pAIState = &genDudeSearchNoWalkL;

        else if (pAIState == &genDudeSearchW || pAIState == &genDudeSearchShortW) 
            pAIState = &genDudeSearchNoWalkW;

        else if (pAIState == &genDudeGotoL) pAIState = &genDudeIdleL;
        else if (pAIState == &genDudeGotoW) pAIState = &genDudeIdleW;
        else if (pAIState == &genDudeChaseL) pAIState = &genDudeChaseNoWalkL;
        else if (pAIState == &genDudeChaseD) pAIState = &genDudeChaseNoWalkD;
        else if (pAIState == &genDudeChaseW) pAIState = &genDudeChaseNoWalkW;
        else if (pAIState == &genDudeRecoilTesla) {
    
            if (spriteIsUnderwater(pSprite, false)) pAIState = &genDudeRecoilW;
            else pAIState = &genDudeRecoilL;

        }

    }

    if (!gGenDudeExtra[pSprite->index].canRecoil) {
        if (pAIState == &genDudeRecoilL || pAIState == &genDudeRecoilD) pAIState = &genDudeIdleL;
        else if (pAIState == &genDudeRecoilW) pAIState = &genDudeIdleW;
    }
    
    pXSprite->stateTimer = pAIState->stateTicks; pXSprite->aiState = pAIState;
    
    int stateSeq = pXSprite->data2 + pAIState->seqId;
    if (pAIState->seqId >= 0 && getSequence(stateSeq)) {
        seqSpawn(stateSeq, 3, pSprite->extra, pAIState->funcId);
    }

    if (pAIState->enterFunc)
        pAIState->enterFunc(&bloodActors[pXSprite->reference]);
}

    
bool playGenDudeSound(spritetype* pSprite, int mode) {
    
    if (mode < kGenDudeSndTargetSpot || mode >= kGenDudeSndMax) return false;
    const GENDUDESND* sndInfo =& gCustomDudeSnd[mode]; bool gotSnd = false;
    short sndStartId = xsprite[pSprite->extra].sysData1; int rand = sndInfo->randomRange;
    int sndId = (sndStartId <= 0) ? sndInfo->defaultSndId : sndStartId + sndInfo->sndIdOffset;
    GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);

    // let's check if there same sounds already played by other dudes
    // so we won't get a lot of annoying screams in the same time and ensure sound played in it's full length (if not interruptable)
    if (pExtra->sndPlaying && !sndInfo->interruptable) {
#if 0
        for (int i = 0; i < 256; i++) {
            if (Bonkle[i].atc <= 0) continue;
            for (int a = 0; a < rand; a++) {
                if (sndId + a == Bonkle[i].atc) {
                    if (Bonkle[i].at0 <= 0) {
                        pExtra->sndPlaying = false;
                        break;
                    }
                    return true;
                }
            }
        }
#endif

        pExtra->sndPlaying = false;
        
    }

    if (sndId < 0) return false;
    else if (sndStartId <= 0) { sndId += Random(rand); gotSnd = true; }
    else {

        // Let's try to get random snd
        int maxRetries = 5;
        while (maxRetries-- > 0) {
            int random = Random(rand);
            if (!soundEngine->FindSoundByResID(sndId + random)) continue;
            sndId = sndId + random;
            gotSnd = true;
            break;
        }

        // If no success in getting random snd, get first existing one
        if (gotSnd == false) {
            int maxSndId = sndId + rand;
            while (sndId++ < maxSndId) {
                if (!soundEngine->FindSoundByResID(sndId)) continue;
                gotSnd = true;
                break;
            }
        }

    }

    if (gotSnd == false) return false;
    else if (sndInfo->aiPlaySound) aiPlay3DSound(pSprite, sndId, AI_SFX_PRIORITY_2, -1);
    else sfxPlay3DSound(pSprite, sndId, -1, 0);
    
    pExtra->sndPlaying = true;
    return true;
}
    
    
bool spriteIsUnderwater(spritetype* pSprite, bool oldWay) {
    return ((sector[pSprite->sectnum].extra >= 0 && xsector[sector[pSprite->sectnum].extra].Underwater)
        || (oldWay && (xsprite[pSprite->extra].medium == kMediumWater || xsprite[pSprite->extra].medium == kMediumGoo)));
}

spritetype* leechIsDropped(spritetype* pSprite) {
    short nLeech = gGenDudeExtra[pSprite->index].nLifeLeech;
    if (nLeech >= 0 && nLeech < kMaxSprites) return &sprite[nLeech];
    return NULL;

}
    
void removeDudeStuff(spritetype* pSprite) {
    int nSprite;
    StatIterator it(kStatThing);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        if (sprite[nSprite].owner != pSprite->index) continue;
        switch (sprite[nSprite].type) {
            case kThingArmedProxBomb:
            case kThingArmedRemoteBomb:
            case kModernThingTNTProx:
                sprite[nSprite].type = kSpriteDecoration;
                actPostSprite(sprite[nSprite].index, kStatFree);
                break;
            case kModernThingEnemyLifeLeech:
                killDudeLeech(&sprite[nSprite]);
                break;
        }
    }

    it.Reset(kStatDude);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        if (sprite[nSprite].owner != pSprite->index) continue;
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
        actDamageSprite(pLeech->owner, pLeech, kDamageExplode, 65535);
        sfxPlay3DSoundCP(pLeech, 522, -1, 0, 60000);

        if (pLeech->owner >= 0 && pLeech->owner < kMaxSprites)
            gGenDudeExtra[sprite[pLeech->owner].index].nLifeLeech = -1;
    }
}
    
XSPRITE* getNextIncarnation(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != 3 || rxBucket[i].index == pXSprite->reference)
            continue;
        
        if (sprite[rxBucket[i].index].statnum == kStatInactive)
                    return &xsprite[sprite[rxBucket[i].index].extra];
        }
    return NULL;
}

bool dudeIsMelee(XSPRITE* pXSprite) {
    return gGenDudeExtra[sprite[pXSprite->reference].index].isMelee;
}

void scaleDamage(XSPRITE* pXSprite) {

    short curWeapon = gGenDudeExtra[sprite[pXSprite->reference].index].curWeapon;
    short weaponType = gGenDudeExtra[sprite[pXSprite->reference].index].weaponType;
    signed short* curScale = gGenDudeExtra[sprite[pXSprite->reference].index].dmgControl;
    for (int i = 0; i < kDmgMax; i++)
        curScale[i] = getDudeInfo(kDudeModernCustom)->startDamage[i];

    switch (weaponType) {
        // just copy damage resistance of dude that should be summoned
        case kGenDudeWeaponSummon:
            for (int i = 0; i < kDmgMax; i++)
                curScale[i] = getDudeInfo(curWeapon)->startDamage[i];
            break;
        // these does not like the explosions and burning
        case kGenDudeWeaponKamikaze:
            curScale[kDmgBurn] = curScale[kDmgExplode] = curScale[kDmgElectric] = 1024;
            break;
        case kGenDudeWeaponMissile:
        case kGenDudeWeaponThrow:
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
                    curScale[kDmgSpirit] -= 32;
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
                    curScale[kDmgExplode] -= 32;
                    break;
                case kMissileLifeLeechRegular:
                    curScale[kDmgBurn] = 60 + Random(4);
                    fallthrough__;
                case kThingDroppedLifeLeech:
                case kModernThingEnemyLifeLeech:
                    curScale[kDmgSpirit] = 32 + Random(18);
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
                    curScale[kDmgBurn] -= 32;
                    curScale[kDmgExplode] = 32;
                    curScale[kDmgFall] = 65 + Random(15);
                    break;
                case kMissileTeslaAlt:
                case kMissileTeslaRegular:
                    curScale[kDmgElectric] = 32 + Random(8);
                    break;
            }
            break;

    }

    // add resistance if have an armor item to drop
    if (pXSprite->dropMsg >= kItemArmorAsbest && pXSprite->dropMsg <= kItemArmorSuper) {
        switch (pXSprite->dropMsg) {
            case kItemArmorAsbest:
                curScale[kDmgBurn] = 0;
                curScale[kDmgExplode] -= 30;
                break;
            case kItemArmorBasic:
                curScale[kDmgBurn] -= 15;
                curScale[kDmgExplode] -= 15;
                curScale[kDmgBullet] -= 15;
                curScale[kDmgSpirit] -= 15;
                break;
            case kItemArmorBody:
                curScale[kDmgBullet] -= 30;
                break;
            case kItemArmorFire:
                curScale[kDmgBurn] -= 30;
                curScale[kDmgExplode] -= 30;
                break;
            case kItemArmorSpirit:
                curScale[kDmgSpirit] -= 30;
                break;
            case kItemArmorSuper:
                curScale[kDmgBurn] -= 60;
                curScale[kDmgExplode] -= 60;
                curScale[kDmgBullet] -= 60;
                curScale[kDmgSpirit] -= 60;
                break;
        }
    }

    // take in account yrepeat of sprite
    short yrepeat = sprite[pXSprite->reference].yrepeat;
    if (yrepeat < 64) {
        for (int i = 0; i < kDmgMax; i++) curScale[i] += (64 - yrepeat);
    } else if (yrepeat > 64) {
        for (int i = 0; i < kDmgMax; i++) curScale[i] -= ((yrepeat - 64) >> 2);
    }

    // take surface type into account
    int surfType = tileGetSurfType(sprite[pXSprite->reference].index + 0xc000);
    switch (surfType) {
        case 1:  // stone
            curScale[kDmgFall] = 0;
            curScale[kDmgBullet] -= 200;
            curScale[kDmgBurn] -= 100;
            curScale[kDmgExplode] -= 80;
            curScale[kDmgChoke] += 30;
            curScale[kDmgElectric] += 20;
            break;
        case 2:  // metal
            curScale[kDmgFall] = 16;
            curScale[kDmgBullet] -= 128;
            curScale[kDmgBurn] -= 90;
            curScale[kDmgExplode] -= 55;
            curScale[kDmgChoke] += 20;
            curScale[kDmgElectric] += 30;
            break;
        case 3:  // wood 
            curScale[kDmgBullet] -= 10;
            curScale[kDmgBurn] += 50;
            curScale[kDmgExplode] += 40;
            curScale[kDmgChoke] += 10;
            curScale[kDmgElectric] -= 60;
            break;
        case 5:  // water
        case 6:  // dirt
        case 7:  // clay
        case 13: // goo
            curScale[kDmgFall] = 8;
            curScale[kDmgBullet] -= 20;
            curScale[kDmgBurn] -= 200;
            curScale[kDmgExplode] -= 60;
            curScale[kDmgChoke] = 0;
            curScale[kDmgElectric] += 40;
            break;
        case 8:  // snow
        case 9:  // ice
            curScale[kDmgFall] = 8;
            curScale[kDmgBullet] -= 20;
            curScale[kDmgBurn] -= 100;
            curScale[kDmgExplode] -= 50;
            curScale[kDmgChoke] = 0;
            curScale[kDmgElectric] += 40;
            break;
        case 10: // leaves
        case 12: // plant
            curScale[kDmgFall] = 0;
            curScale[kDmgBullet] -= 10;
            curScale[kDmgBurn] += 70;
            curScale[kDmgExplode] += 50;
            break;
        case 11: // cloth
            curScale[kDmgFall] = 8;
            curScale[kDmgBullet] -= 10;
            curScale[kDmgBurn] += 30;
            curScale[kDmgExplode] += 20;
            break;
        case 14: // lava
            curScale[kDmgBurn] = 0;
            curScale[kDmgExplode] = 0;
            curScale[kDmgChoke] += 30;
            break;
    }

    // finally, scale dmg for difficulty
    for (int i = 0; i < kDmgMax; i++) 
        curScale[i] = MulScale(DudeDifficulty[gGameOptions.nDifficulty], ClipLow(curScale[i], 1), 8);
    
    //short* dc = curScale;
    //if (pXSprite->rxID == 788)
        //viewSetSystemMessage("0: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d", dc[0], dc[1], dc[2], dc[3], dc[4], dc[5], dc[6]);
}

int getDispersionModifier(spritetype* pSprite, int minDisp, int maxDisp) 
{
    // the faster fire rate, the less frames = more dispersion
    Seq* pSeq = getSequence(xsprite[pSprite->extra].data2 + 6); 
    int disp = 1;
    if (pSeq != nullptr) 
    {
        int nFrames = pSeq->nFrames; int ticks = pSeq->ticksPerFrame; int shots = 0;
        for (int i = 0; i <= pSeq->nFrames; i++) {
            if (pSeq->frames[i].trigger) shots++;
        }
        
        disp = (((shots * 1000) / nFrames) / ticks) * 20;
        if (gGameOptions.nDifficulty > 0)
            disp /= gGameOptions.nDifficulty;

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
            Seq* pSeq = getSequence(seqId);
            if (pSeq) {
                picnum = seqGetTile(&pSeq->frames[0]);
            }
        }
        
        dist = tileHeight(picnum) << 8;
        if (yrepeat < 64) dist -= (64 - yrepeat) * mul;
        else if (yrepeat > 64) dist += (yrepeat - 64) * (mul / 3);
    }
    
    dist = ClipRange(dist, minDist, maxDist);
    //viewSetSystemMessage("DIST: %d, SPRHEIGHT: %d: YREPEAT: %d PIC: %d", dist, tileHeight(pSprite->picnum), yrepeat, picnum);
    return dist;
}

int getBaseChanceModifier(int baseChance) {
    return ((gGameOptions.nDifficulty > 0) ? baseChance - (0x0500 * gGameOptions.nDifficulty) : baseChance);
}

int getRecoilChance(spritetype* pSprite) {
    XSPRITE* pXSprite = &xsprite[pSprite->extra]; int mass = getSpriteMassBySize(pSprite);
    int baseChance = (!dudeIsMelee(pXSprite) ? 0x8000 : 0x4000);
    baseChance = getBaseChanceModifier(baseChance) + pXSprite->data3;
    
    int chance = ((baseChance / mass) << 7);
    return chance;
}

int getDodgeChance(spritetype* pSprite) {
    XSPRITE* pXSprite = &xsprite[pSprite->extra]; int mass = getSpriteMassBySize(pSprite);
    int baseChance = (!dudeIsMelee(pXSprite) ? 0x6000 : 0x1000);
    baseChance = getBaseChanceModifier(baseChance) + pXSprite->data3;

    int chance = ((baseChance / mass) << 7);
    return chance;

}

void dudeLeechOperate(spritetype* pSprite, XSPRITE* pXSprite, EVENT event)
{
    if (event.cmd == kCmdOff) {
        actPostSprite(pSprite->index, kStatFree);
        return;
    }

    int nTarget = pXSprite->target;
    if (spriRangeIsFine(nTarget) && nTarget != pSprite->owner) {
        spritetype* pTarget = &sprite[nTarget];
        if (pTarget->statnum == kStatDude && !(pTarget->flags & 32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites && !pXSprite->stateTimer) {
            
            if (IsPlayerSprite(pTarget)) {
                PLAYER* pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
                if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0) return;
            }
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            int nType = pTarget->type - kDudeBase;
            DUDEINFO* pDudeInfo = &dudeInfo[nType];
            int z1 = (top - pSprite->z) - 256;
            int x = pTarget->x; int y = pTarget->y; int z = pTarget->z;
            int nDist = approxDist(x - pSprite->x, y - pSprite->y);
            
            if (nDist != 0 && cansee(pSprite->x, pSprite->y, top, pSprite->sectnum, x, y, z, pTarget->sectnum)) {
                int t = DivScale(nDist, 0x1aaaaa, 12);
                x += (xvel[nTarget] * t) >> 12;
                y += (yvel[nTarget] * t) >> 12;
                int angBak = pSprite->ang;
                pSprite->ang = getangle(x - pSprite->x, y - pSprite->y);
                int dx = CosScale16(pSprite->ang);
                int dy = SinScale16(pSprite->ang);
                int tz = pTarget->z - (pTarget->yrepeat * pDudeInfo->aimHeight) * 4;
                int dz = DivScale(tz - top - 256, nDist, 10);
                int nMissileType = kMissileLifeLeechAltNormal + (pXSprite->data3 ? 1 : 0);
                int t2;
                
                if (!pXSprite->data3) t2 = 120 / 10;
                else t2 = (3 * 120) / 10;

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
    spritetype* pExplosion = actSpawnSprite_(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, kStatExplosion, true);
    if (pExplosion->extra < 0 || pExplosion->extra >= kMaxXSprites) 
        return false;

    int nSeq = 4; int nSnd = 304; const EXPLOSION* pExpl = &explodeInfo[nType];
    
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

// this function allows to spawn new custom dude and inherit spawner settings,
// so custom dude can have different weapons, hp and so on...
spritetype* genDudeSpawn(XSPRITE* pXSource, spritetype* pSprite, int nDist) {

    spritetype* pSource = &sprite[pXSource->reference]; 
    spritetype* pDude = actSpawnSprite(pSprite, kStatDude); XSPRITE* pXDude = &xsprite[pDude->extra];

    int x, y, z = pSprite->z, nAngle = pSprite->ang, nType = kDudeModernCustom;

    if (nDist > 0) {
        
        x = pSprite->x + mulscale30r(Cos(nAngle), nDist);
        y = pSprite->y + mulscale30r(Sin(nAngle), nDist);

    } else {
        
        x = pSprite->x;
        y = pSprite->y;

    }

    pDude->type = nType; pDude->ang = nAngle;
    vec3_t pos = { x, y, z }; setsprite(pDude->index, &pos);
    pDude->cstat |= 0x1101; pDude->clipdist = dudeInfo[nType - kDudeBase].clipdist;

    // inherit weapon, seq and sound settings.
    pXDude->data1 = pXSource->data1;
    pXDude->data2 = pXSource->data2;
    pXDude->sysData1 = pXSource->data3; // move sndStartId from data3 to sysData1
    pXDude->data3 = 0;

    // spawn seq
    seqSpawn(genDudeSeqStartId(pXDude), 3, pDude->extra, -1);

    // inherit movement speed.
    pXDude->busyTime = pXSource->busyTime;

    // inherit clipdist?
    if (pSource->clipdist > 0)
        pDude->clipdist = pSource->clipdist;

    // inherit custom hp settings
    if (pXSource->data4 <= 0) pXDude->health = dudeInfo[nType - kDudeBase].startHealth << 4;
    else pXDude->health = ClipRange(pXSource->data4 << 4, 1, 65535);


    if (pSource->flags & kModernTypeFlag1) {
        switch (pSource->type) {
            case kModernCustomDudeSpawn:
                //inherit pal?
                if (pDude->pal <= 0) pDude->pal = pSource->pal;

                // inherit spawn sprite trigger settings, so designer can count monsters.
                pXDude->txID = pXSource->txID;
                pXDude->command = pXSource->command;
                pXDude->triggerOn = pXSource->triggerOn;
                pXDude->triggerOff = pXSource->triggerOff;

                // inherit drop items
                pXDude->dropMsg = pXSource->dropMsg;

                // inherit required key so it can be dropped
                pXDude->key = pXSource->key;

                // inherit dude flags
                pXDude->dudeDeaf = pXSource->dudeDeaf;
                pXDude->dudeGuard = pXSource->dudeGuard;
                pXDude->dudeAmbush = pXSource->dudeAmbush;
                pXDude->dudeFlag4 = pXSource->dudeFlag4;
                pXDude->unused1 = pXSource->unused1;
                break;
        }
    }

    // inherit sprite size (useful for seqs with zero repeats)
    if (pSource->flags & kModernTypeFlag2) {
        pDude->xrepeat = pSource->xrepeat;
        pDude->yrepeat = pSource->yrepeat;
    }

    gKillMgr.AddNewKill(1);
    aiInitSprite(pDude);
    return pDude;
}

void genDudeTransform(spritetype* pSprite) {
    
    if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites)) {
        Printf(PRINT_HIGH, "pSprite->extra >= 0 && pSprite->extra < kMaxXSprites");
        return;
    }
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    XSPRITE* pXIncarnation = getNextIncarnation(pXSprite);
    if (pXIncarnation == NULL) {
        if (pXSprite->sysData1 == kGenDudeTransformStatus) pXSprite->sysData1 = 0;
        trTriggerSprite(pSprite->index, pXSprite, kCmdOff);
        return;
    }
    
    spritetype* pIncarnation = &sprite[pXIncarnation->reference];
    pXSprite->key = pXSprite->dropMsg = pXSprite->locked = 0;

    // save incarnation's going on and off options
    bool triggerOn = pXIncarnation->triggerOn;
    bool triggerOff = pXIncarnation->triggerOff;

    // then remove it from incarnation so it will not send the commands
    pXIncarnation->triggerOn = false;
    pXIncarnation->triggerOff = false;

    // trigger dude death before transform
    trTriggerSprite(pSprite->index, pXSprite, kCmdOff);

    pSprite->type = pSprite->inittype = pIncarnation->type;
    pSprite->flags = pIncarnation->flags;
    pSprite->pal = pIncarnation->pal;
    pSprite->shade = pIncarnation->shade;
    pSprite->clipdist = pIncarnation->clipdist;
    pSprite->xrepeat = pIncarnation->xrepeat;
    pSprite->yrepeat = pIncarnation->yrepeat;

    pXSprite->txID = pXIncarnation->txID;
    pXSprite->command = pXIncarnation->command;
    pXSprite->triggerOn = triggerOn;
    pXSprite->triggerOff = triggerOff;
    pXSprite->busyTime = pXIncarnation->busyTime;
    pXSprite->waitTime = pXIncarnation->waitTime;

    // inherit respawn properties
    pXSprite->respawn = pXIncarnation->respawn;
    pXSprite->respawnPending = pXIncarnation->respawnPending;

    pXSprite->burnTime = 0;
    pXSprite->burnSource = -1;

    pXSprite->data1 = pXIncarnation->data1;
    pXSprite->data2 = pXIncarnation->data2;

    pXSprite->sysData1 = pXIncarnation->data3;  // soundBase id
    pXSprite->sysData2 = pXIncarnation->data4;  // start hp

    pXSprite->dudeGuard = pXIncarnation->dudeGuard;
    pXSprite->dudeDeaf = pXIncarnation->dudeDeaf;
    pXSprite->dudeAmbush = pXIncarnation->dudeAmbush;
    pXSprite->dudeFlag4 = pXIncarnation->dudeFlag4;
    pXSprite->unused1 = pXIncarnation->unused1;

    pXSprite->dropMsg = pXIncarnation->dropMsg;
    pXSprite->key = pXIncarnation->key;

    pXSprite->locked = pXIncarnation->locked;
    pXSprite->Decoupled = pXIncarnation->Decoupled;

    // clear drop items of the incarnation
    pXIncarnation->key = pXIncarnation->dropMsg = 0;

    // set hp
    if (pXSprite->sysData2 <= 0) pXSprite->health = dudeInfo[pSprite->type - kDudeBase].startHealth << 4;
    else pXSprite->health = ClipRange(pXSprite->sysData2 << 4, 1, 65535);

    int seqId = dudeInfo[pSprite->type - kDudeBase].seqStartID;
    switch (pSprite->type) {
        case kDudePodMother: // fake dude
        case kDudeTentacleMother: // fake dude
            break;
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
            seqId = genDudeSeqStartId(pXSprite);
            genDudePrepare(pSprite, kGenDudePropertyMass);
            fallthrough__; // go below
        default:
            seqSpawn(seqId, 3, pSprite->extra, -1);

            // save target
            int target = pXSprite->target;

            // re-init sprite
            aiInitSprite(pSprite);

            // try to restore target
            if (target == -1) aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
            else aiSetTarget(pXSprite, target);

            // finally activate it
            aiActivateDude(&bloodActors[pXSprite->reference]);

            break;
    }
    pXIncarnation->triggerOn = triggerOn;
    pXIncarnation->triggerOff = triggerOff;

    /*// remove the incarnation in case if non-locked
    if (pXIncarnation->locked == 0) {
        pXIncarnation->txID = pIncarnation->type = 0;
        actPostSprite(pIncarnation->index, kStatFree);
        // or restore triggerOn and off options
    } else {
        pXIncarnation->triggerOn = triggerOn;
        pXIncarnation->triggerOff = triggerOff;
    }*/
}


void updateTargetOfLeech(spritetype* pSprite) {
    if (!(pSprite->extra >= 0 && pSprite->extra < kMaxXSprites)) {
        Printf(PRINT_HIGH, "pSprite->extra >= 0 && pSprite->extra < kMaxXSprites");
        return;
    }
    
    
    spritetype* pLeech = leechIsDropped(pSprite);
    if (pLeech == NULL || pLeech->extra < 0) gGenDudeExtra[pSprite->index].nLifeLeech = -1;
    else if (xsprite[pSprite->extra].target != xsprite[pLeech->extra].target) {
        XSPRITE* pXDude = &xsprite[pSprite->extra]; XSPRITE* pXLeech = &xsprite[pLeech->extra];
        if (pXDude->target < 0 && spriRangeIsFine(pXLeech->target)) {
            aiSetTarget(pXDude, pXLeech->target);
            if (inIdle(pXDude->aiState))
                aiActivateDude(&bloodActors[pXDude->reference]);
        } else {
            pXLeech->target = pXDude->target;
        }
    }
}

void updateTargetOfSlaves(spritetype* pSprite) {
    if (!xspriRangeIsFine(pSprite->extra)) {
        Printf(PRINT_HIGH, "!xspriRangeIsFine(pSprite->extra)");
        return;
    }
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    GENDUDEEXTRA* pExtra = genDudeExtra(pSprite); short* slave = pExtra->slave;
    spritetype* pTarget = (pXSprite->target >= 0 && IsDudeSprite(&sprite[pXSprite->target])) ? &sprite[pXSprite->target] : NULL;
    XSPRITE* pXTarget = (pTarget != NULL && xspriRangeIsFine(pTarget->extra) && xsprite[pTarget->extra].health > 0) ? &xsprite[pTarget->extra] : NULL;

    int newCnt = pExtra->slaveCount;
    for (int i = 0; i <= gGameOptions.nDifficulty; i++) {
        if (spriRangeIsFine(slave[i])) {
            spritetype* pSlave = &sprite[slave[i]];
            if (!IsDudeSprite(pSlave) || !xspriRangeIsFine(pSlave->extra) || xsprite[pSlave->extra].health < 0) {
                slave[i] = pSlave->owner = -1; newCnt--;
                continue;
            }

            XSPRITE* pXSlave = &xsprite[pSlave->index];
            if (pXTarget != NULL) {
                if (pXSprite->target != pXSlave->target) aiSetTarget(pXSlave, pXSprite->target);
                // check if slave have proper target
                if (!spriRangeIsFine(pXSlave->target) || sprite[pXSlave->target].owner == pSprite->index)
                    aiSetTarget(pXSlave, pSprite->x, pSprite->y, pSprite->z);
            } else {
                aiSetTarget(pXSlave, pSprite->x, pSprite->y, pSprite->z); // try return to master
            }
        } 
    }
    
    pExtra->slaveCount = newCnt;
}

short inDodge(AISTATE* aiState) {
    if (aiState == &genDudeDodgeL) return 1;
    else if (aiState == &genDudeDodgeD) return 2;
    else if (aiState == &genDudeDodgeW) return 3;
    else if (aiState == &genDudeDodgeShortL) return 4;
    else if (aiState == &genDudeDodgeShortD) return 5;
    else if (aiState == &genDudeDodgeShortW) return 6;
    else if (aiState == &genDudeDodgeShorterL) return 7;
    else if (aiState == &genDudeDodgeShorterD) return 8;
    else if (aiState == &genDudeDodgeShorterW) return 9;
    return 0;

}

bool inIdle(AISTATE* aiState) {
    return (aiState == &genDudeIdleW || aiState == &genDudeIdleL);
}

bool inAttack(AISTATE* aiState) {
    return (aiState == &genDudeFireL || aiState == &genDudeFireW
        || aiState == &genDudeFireD || aiState == &genDudeThrow || aiState == &genDudeThrow2 || aiState == &genDudePunch);
}

short inSearch(AISTATE* aiState) {
    if (aiState->stateType == kAiStateSearch) return 1;
    return 0;
}

short inChase(AISTATE* aiState) {
    if (aiState == &genDudeChaseL) return 1;
    else if (aiState == &genDudeChaseD) return 2;
    else if (aiState == &genDudeChaseW) return 3;
    else if (aiState == &genDudeChaseNoWalkL) return 4;
    else if (aiState == &genDudeChaseNoWalkD) return 5;
    else if (aiState == &genDudeChaseNoWalkW) return 6;
    else return 0;
}

short inRecoil(AISTATE* aiState) {
    if (aiState == &genDudeRecoilL || aiState == &genDudeRecoilTesla) return 1;
    else if (aiState == &genDudeRecoilD) return 2;
    else if (aiState == &genDudeRecoilW) return 3;
    else return 0;
}

short inDuck(AISTATE* aiState) {
    if (aiState == &genDudeFireD) return 1;
    else if (aiState == &genDudeChaseD) return 2;
    else if (aiState == &genDudeChaseNoWalkD) return 3;
    else if (aiState == &genDudeRecoilD) return 4;
    else if (aiState == &genDudeDodgeShortD) return 5;
    return 0;
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

int genDudeSeqStartId(XSPRITE* pXSprite) {
    if (genDudePrepare(&sprite[pXSprite->reference], kGenDudePropertyStates)) return pXSprite->data2;
    else return kGenDudeDefaultSeq;
}

bool genDudePrepare(spritetype* pSprite, int propId) {
    if (!spriRangeIsFine(pSprite->index)) {
        Printf(PRINT_HIGH, "!spriRangeIsFine(pSprite->index)");
        return false;
    } else if (!xspriRangeIsFine(pSprite->extra)) {
        Printf(PRINT_HIGH, "!xspriRangeIsFine(pSprite->extra)");
        return false;
    } else if (pSprite->type != kDudeModernCustom) {
        Printf(PRINT_HIGH, "pSprite->type != kDudeModernCustom");
        return false;
    } else if (propId < kGenDudePropertyAll || propId >= kGenDudePropertyMax) {
        viewSetSystemMessage("Unknown custom dude #%d property (%d)", pSprite->index, propId);
        return false;
    }
    
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index]; pExtra->updReq[propId] = false;
    
    switch (propId) {
        case kGenDudePropertyAll:
        case kGenDudePropertyInitVals:
            pExtra->moveSpeed = getGenDudeMoveSpeed(pSprite, 0, true, false);
            pExtra->initVals[0] = pSprite->xrepeat;
            pExtra->initVals[1] = pSprite->yrepeat;
            pExtra->initVals[2] = pSprite->clipdist;
            if (propId) break;
            fallthrough__;

        case kGenDudePropertyWeapon: {
            pExtra->curWeapon = pXSprite->data1;
            switch (pXSprite->data1) {
                case VECTOR_TYPE_19: pExtra->curWeapon = kVectorBullet; break;
                case kMissileUnused: pExtra->curWeapon = kMissileArcGargoyle; break;
                case kThingDroppedLifeLeech: pExtra->curWeapon = kModernThingEnemyLifeLeech; break;
            }

            pExtra->canAttack = false;
            if (pExtra->curWeapon > 0 && getSequence(pXSprite->data2 + kGenDudeSeqAttackNormalL))
                pExtra->canAttack = true;
            
            pExtra->weaponType = kGenDudeWeaponNone;
            if (pExtra->curWeapon > 0 && pExtra->curWeapon < kVectorMax) pExtra->weaponType = kGenDudeWeaponHitscan;
            else if (pExtra->curWeapon >= kDudeBase && pExtra->curWeapon < kDudeMax) pExtra->weaponType = kGenDudeWeaponSummon;
            else if (pExtra->curWeapon >= kMissileBase && pExtra->curWeapon < kMissileMax) pExtra->weaponType = kGenDudeWeaponMissile;
            else if (pExtra->curWeapon >= kThingBase && pExtra->curWeapon < kThingMax) pExtra->weaponType = kGenDudeWeaponThrow;
            else if (pExtra->curWeapon >= kTrapExploder && pExtra->curWeapon < (kTrapExploder + kExplodeMax) - 1) 
                pExtra->weaponType = kGenDudeWeaponKamikaze;
            
            pExtra->isMelee = false;
            if (pExtra->weaponType == kGenDudeWeaponKamikaze) pExtra->isMelee = true;
            else if (pExtra->weaponType == kGenDudeWeaponHitscan) {
                if (gVectorData[pExtra->curWeapon].maxDist > 0 && gVectorData[pExtra->curWeapon].maxDist <= kGenDudeMaxMeleeDist)
                    pExtra->isMelee = true;
            }

            if (propId) break;
            fallthrough__;

        }
        case kGenDudePropertyDmgScale:
            scaleDamage(pXSprite);
            if (propId) break;
            fallthrough__;

        case kGenDudePropertyMass: {
            // to ensure mass gets updated, let's clear all cache
            SPRITEMASS* pMass = &gSpriteMass[pSprite->extra];
            pMass->seqId = pMass->picnum = pMass->xrepeat = pMass->yrepeat = pMass->clipdist = 0;
            pMass->mass = pMass->airVel = pMass->fraction = 0;
            getSpriteMassBySize(pSprite);
            if (propId) break;
            fallthrough__;
        }
        case kGenDudePropertyAttack:
            pExtra->fireDist = getRangeAttackDist(pSprite, 3000, 45000);
            pExtra->throwDist = pExtra->fireDist; // temp
            pExtra->baseDispersion = getDispersionModifier(pSprite, 200, 3500);
            if (propId) break;
            fallthrough__;

        case kGenDudePropertyStates: {

            pExtra->canFly = false;

            // check the animation
            int seqStartId = -1;
            if (pXSprite->data2 <= 0) seqStartId = pXSprite->data2 = getDudeInfo(pSprite->type)->seqStartID;
            else seqStartId = pXSprite->data2;

            for (int i = seqStartId; i < seqStartId + kGenDudeSeqMax; i++) {
                switch (i - seqStartId) {
                    case kGenDudeSeqIdleL:
                    case kGenDudeSeqDeathDefault:
                    case kGenDudeSeqAttackNormalL:
                    case kGenDudeSeqAttackThrow:
                    case kGenDudeSeqAttackPunch:
                    {
                        Seq* pSeq = getSequence(i);
                        if (!pSeq) 
                        {
                            pXSprite->data2 = getDudeInfo(pSprite->type)->seqStartID;
                            viewSetSystemMessage("No SEQ animation id %d found for custom dude #%d!", i, pSprite->index);
                            viewSetSystemMessage("SEQ base id: %d", seqStartId);
                        } 
                        else if ((i - seqStartId) == kGenDudeSeqAttackPunch) 
                        {
                            pExtra->forcePunch = true; // required for those who don't have fire trigger in punch seq and for default animation
                            for (int i = 0; i < pSeq->nFrames; i++) {
                                if (!pSeq->frames[i].trigger) continue;
                                pExtra->forcePunch = false;
                                break;
                            }
                        }
                        break;
                    }
                    case kGenDudeSeqDeathExplode:
                        pExtra->availDeaths[kDmgExplode] = !!getSequence(i);
                        break;
                    case kGenDudeSeqBurning:
                        pExtra->canBurn = !!getSequence(i);
                        break;
                    case kGenDudeSeqElectocuted:
                        pExtra->canElectrocute = !!getSequence(i);
                        break;
                    case kGenDudeSeqRecoil:
                        pExtra->canRecoil = !!getSequence(i);
                        break;
                    case kGenDudeSeqMoveL: {
                        bool oldStatus = pExtra->canWalk;
                        pExtra->canWalk = !!getSequence(i);
                        if (oldStatus != pExtra->canWalk) {
                            if (!spriRangeIsFine(pXSprite->target)) 
                            {
                                if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeIdleW);
                                else aiGenDudeNewState(pSprite, &genDudeIdleL);
                            }
                            else if (pExtra->canWalk) 
                            {
                                if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeChaseW);
                                else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pSprite, &genDudeChaseD);
                                else aiGenDudeNewState(pSprite, &genDudeChaseL);
                            } 
                            else 
                            {
                                if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeChaseNoWalkW);
                                else if (inDuck(pXSprite->aiState)) aiGenDudeNewState(pSprite, &genDudeChaseNoWalkD);
                                else aiGenDudeNewState(pSprite, &genDudeChaseNoWalkL);
                            }
                        }
                        break;
                    }
                    case kGenDudeSeqAttackNormalDW:
                        pExtra->canDuck = (getSequence(i) && getSequence(seqStartId + 14));
                        pExtra->canSwim = (getSequence(i) && getSequence(seqStartId + 13)
                            && getSequence(seqStartId + 17));
                        break;
                    case kGenDudeSeqDeathBurn1: {
                        bool seq15 = getSequence(i); 
                        bool seq16 = getSequence(seqStartId + 16);
                        if (seq15 && seq16) pExtra->availDeaths[kDmgBurn] = 3;
                        else if (seq16) pExtra->availDeaths[kDmgBurn] = 2;
                        else if (seq15) pExtra->availDeaths[kDmgBurn] = 1;
                        else pExtra->availDeaths[kDmgBurn] = 0;
                        break;
                    }
                    case kGenDudeSeqMoveW:
                    case kGenDudeSeqMoveD:
                    case kGenDudeSeqDeathBurn2:
                    case kGenDudeSeqIdleW:
                        break;
                    case kGenDudeSeqReserved3:
                    case kGenDudeSeqReserved4:
                    case kGenDudeSeqReserved5:
                    case kGenDudeSeqReserved6:
                    case kGenDudeSeqReserved7:
                    case kGenDudeSeqReserved8:
                        /*if (getSequence(i)) {
                            viewSetSystemMessage("Found reserved SEQ animation (%d) for custom dude #%d!", i, pSprite->index);
                            viewSetSystemMessage("Using reserved animation is not recommended.");
                            viewSetSystemMessage("SEQ base id: %d", seqStartId);
                        }*/
                        break;
                }
            }
            if (propId) break;
            fallthrough__;
        }
        case kGenDudePropertyLeech:
            pExtra->nLifeLeech = -1;
            if (pSprite->owner != kMaxSprites - 1) {
                int nSprite;
                StatIterator it(kStatThing);
                while ((nSprite = it.NextIndex()) >= 0)
                {
                    if (sprite[nSprite].owner == pSprite->index && sprite[nSprite].type == kModernThingEnemyLifeLeech) {
                        pExtra->nLifeLeech = nSprite;
                        break;
                    }
                }
            }
            if (propId) break;
            fallthrough__;

        case kGenDudePropertySlaves:
        {
            pExtra->slaveCount = 0; memset(pExtra->slave, -1, sizeof(pExtra->slave));
            int nSprite;
            StatIterator it(kStatDude);
            while ((nSprite = it.NextIndex()) >= 0)
            {
                if (sprite[nSprite].owner != pSprite->index) continue;
                else if (!IsDudeSprite(&sprite[nSprite]) || !xspriRangeIsFine(sprite[nSprite].extra) || xsprite[sprite[nSprite].extra].health <= 0) {
                    sprite[nSprite].owner = -1;
                    continue;
                }

                pExtra->slave[pExtra->slaveCount++] = nSprite;
                if (pExtra->slaveCount > gGameOptions.nDifficulty)
                    break;
            }
            if (propId) break;
            fallthrough__;
        }
        case kGenDudePropertySpriteSize: {
            if (seqGetStatus(3, pSprite->extra) == -1)
                seqSpawn(pXSprite->data2 + pXSprite->aiState->seqId, 3, pSprite->extra, -1);

            // make sure dudes aren't in the floor or ceiling
            int zTop, zBot; GetSpriteExtents(pSprite, &zTop, &zBot);
            if (!(sector[pSprite->sectnum].ceilingstat & 0x0001))
                pSprite->z += ClipLow(sector[pSprite->sectnum].ceilingz - zTop, 0);
            if (!(sector[pSprite->sectnum].floorstat & 0x0001))
                pSprite->z += ClipHigh(sector[pSprite->sectnum].floorz - zBot, 0);

            pSprite->clipdist = ClipRange((pSprite->xrepeat + pSprite->yrepeat) >> 1, 4, 120);           
            if (propId) break;
            }
        }

    return true;
}

void genDudePostDeath(spritetype* pSprite, DAMAGE_TYPE damageType, int damage) {
    if (damageType == kDamageExplode) {
        DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
        for (int i = 0; i < 3; i++)
            if (pDudeInfo->nGibType[i] > -1)
                GibSprite(pSprite, (GIBTYPE)pDudeInfo->nGibType[i], NULL, NULL);

        for (int i = 0; i < 4; i++)
            fxSpawnBlood(pSprite, damage);
    }
    
    gKillMgr.AddKill(pSprite);

    pSprite->type = kThingBloodChunks;
    actPostSprite(pSprite->index, kStatThing);
}

void aiGenDudeInitSprite(spritetype* pSprite, XSPRITE* pXSprite) {
    switch (pSprite->type) {
        case kDudeModernCustom: {
            DUDEEXTRA_at6_u1* pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
            pDudeExtraE->xval3 = pDudeExtraE->xval1 = 0;
            aiGenDudeNewState(pSprite, &genDudeIdleL);
            break;
        }
        case kDudeModernCustomBurning:
            aiGenDudeNewState(pSprite, &genDudeBurnGoto);
            pXSprite->burnTime = 1200;
            break;
    }
    
    pSprite->flags = 15;
    return;
}

END_BLD_NS
#endif
