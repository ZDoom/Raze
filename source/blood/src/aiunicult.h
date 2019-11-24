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
#pragma once
#include "ai.h"
#include "eventq.h"

BEGIN_BLD_NS
#define kGenDudeDefaultSeq 11520
#define kGenDudeMaxSlaves 7
#define kGenDudeTransformStatus -222
#define kGenDudeUpdTimeRate 10
#define kGenDudeMaxMeleeDist 2048

enum {
kGenDudeSeqIdleL            = 0,
kGenDudeSeqDeathDefault     = 1,
kGenDudeSeqDeathExplode     = 2,
kGenDudeSeqBurning          = 3,
kGenDudeSeqElectocuted      = 4,
kGenDudeSeqRecoil           = 5,
kGenDudeSeqAttackNormalL    = 6,
kGenDudeSeqAttackThrow      = 7,
kGenDudeSeqAttackNormalDW   = 8,
kGenDudeSeqMoveL            = 9,
kGenDudeSeqAttackPunch      = 10,
kGenDudeSeqReserved1        = 11,
kGenDudeSeqReserved2        = 12,
kGenDudeSeqMoveW            = 13,
kGenDudeSeqMoveD            = 14,
kGenDudeSeqDeathBurn1       = 15,
kGenDudeSeqDeathBurn2       = 16,
kGenDudeSeqIdleW            = 17,
kGenDudeSeqTransform        = 18,
kGenDudeSeqReserved3        = 19,
kGenDudeSeqReserved4        = 20,
kGenDudeSeqReserved5        = 21,
kGenDudeSeqReserved6        = 22,
kGenDudeSeqReserved7        = 23,
kGenDudeSeqReserved8        = 24,
kGenDudeSeqMax                  ,
};

enum {
kGenDudeSndTargetSpot       = 0,
kGenDudeSndGotHit           = 1,
kGenDudeSndDeathNormal      = 2,
kGenDudeSndBurning          = 3,
kGenDudeSndDeathExplode     = 4,
kGenDudeSndTargetDead       = 5,
kGenDudeSndChasing          = 6,
kGenDudeSndAttackNormal     = 7,
kGenDudeSndAttackThrow      = 8,
kGenDudeSndAttackMelee      = 9,
kGenDudeSndTransforming     = 10,
kGenDudeSndMax                  ,
};

enum {
kGenDudePropertyAll         = 0,
kGenDudePropertyWeapon      = 1,
kGenDudePropertyDmgScale    = 2,
kGenDudePropertyMass        = 3,
kGenDudePropertyAttack      = 4,
kGenDudePropertyStates      = 5,
kGenDudePropertyLeech       = 6,
kGenDudePropertySlaves      = 7,
kGenDudePropertySpriteSize  = 8,
kGenDudePropertyInitVals    = 9,
kGenDudePropertyMax            ,
};

extern AISTATE genDudeIdleL;
extern AISTATE genDudeIdleW;
extern AISTATE genDudeSearchL;
extern AISTATE genDudeSearchW;
extern AISTATE genDudeGotoL;
extern AISTATE genDudeGotoW;
extern AISTATE genDudeDodgeL;
extern AISTATE genDudeDodgeD;
extern AISTATE genDudeDodgeW;
extern AISTATE genDudeDodgeShortL;
extern AISTATE genDudeDodgeShortD;
extern AISTATE genDudeDodgeShortW;
extern AISTATE genDudeChaseL;
extern AISTATE genDudeChaseD;
extern AISTATE genDudeChaseW;
extern AISTATE genDudeFireL;
extern AISTATE genDudeFireD;
extern AISTATE genDudeFireW;
extern AISTATE genDudeRecoilL;
extern AISTATE genDudeRecoilD;
extern AISTATE genDudeRecoilW;
extern AISTATE genDudeThrow;
extern AISTATE genDudeThrow2;
extern AISTATE genDudePunch;
extern AISTATE genDudeRecoilTesla;
extern AISTATE genDudeSearchNoWalkL;
extern AISTATE genDudeSearchNoWalkW;
extern AISTATE genDudeChaseNoWalkL;
extern AISTATE genDudeChaseNoWalkD;
extern AISTATE genDudeChaseNoWalkW;

struct GENDUDESND
{
    int defaultSndId;
    int randomRange;
    int sndIdOffset;  // relative to data3
    bool aiPlaySound; // false = sfxStart3DSound();
    bool interruptable;
};

extern GENDUDESND gCustomDudeSnd[];

// temporary, until normal DUDEEXTRA gets refactored
struct GENDUDEEXTRA {
    unsigned int fireDist;          // counts from sprite size
    unsigned int throwDist;         // counts from sprite size
    unsigned int frontSpeed;
    unsigned short curWeapon;       // data1 duplicate to avoid potential problems when changing data dynamically
    unsigned short baseDispersion;
    signed short nLifeLeech;        // spritenum of dropped dude's leech
    short slaveCount;
    short slave[kGenDudeMaxSlaves]; // index of the ones dude is summon
    short dmgControl[kDamageMax];           // depends of current weapon, drop armor item, sprite yrepeat and surface type
    short availDeaths[kDamageMax];          // list of seqs with deaths for each damage type
    short initVals[3];                      // xrepeat, yrepeat, clipdist
    bool forcePunch;                        // indicate if there is no fire trigger in punch state seq
    bool updReq[kGenDudePropertyMax]; // update requests
    bool sndPlaying;                        // indicate if sound of AISTATE currently playing
    bool isMelee;
    bool canBurn;                           // can turn in Burning dude or not
    bool canElectrocute;
    bool canAttack;
    bool canRecoil;
    bool canWalk;
    bool canDuck;
    bool canSwim;
    bool canFly;
};

extern GENDUDEEXTRA gGenDudeExtra[];

inline GENDUDEEXTRA* genDudeExtra(spritetype* pSprite) {
    return &gGenDudeExtra[pSprite->index];
}

XSPRITE* getNextIncarnation(XSPRITE* pXSprite);
void killDudeLeech(spritetype* pLeech);
void removeLeech(spritetype* pLeech, bool delSprite = true);
void removeDudeStuff(spritetype* pSprite);
spritetype* leechIsDropped(spritetype* pSprite);
bool spriteIsUnderwater(spritetype* pSprite, bool oldWay = false);
bool playGenDudeSound(spritetype* pSprite, int mode, bool forceInterrupt = false);
void aiGenDudeMoveForward(spritetype* pSprite, XSPRITE* pXSprite);
void aiGenDudeChooseDirection(spritetype* pSprite, XSPRITE* pXSprite, int a3, int aXvel = -1, int aYvel = -1);
void aiGenDudeNewState(spritetype* pSprite, AISTATE* pAIState);
int getGenDudeMoveSpeed(spritetype* pSprite, int which, bool mul, bool shift);
bool TargetNearThing(spritetype* pSprite, int thingType);
int checkAttackState(spritetype* pSprite, XSPRITE* pXSprite);
bool doExplosion(spritetype* pSprite, int nType);
void dudeLeechOperate(spritetype* pSprite, XSPRITE* pXSprite, EVENT a3);
int getDodgeChance(spritetype* pSprite);
int getRecoilChance(spritetype* pSprite);
bool dudeIsMelee(XSPRITE* pXSprite);
void updateTargetOfSlaves(spritetype* pSprite);
void updateTargetOfLeech(spritetype* pSprite);
bool canSwim(spritetype* pSprite);
bool canDuck(spritetype* pSprite);
bool canWalk(spritetype* pSprite);
bool inDodge(AISTATE* aiState);
bool inIdle(AISTATE* aiState);
bool inAttack(AISTATE* aiState);
short inRecoil(AISTATE* aiState);
short inSearch(AISTATE* aiState);
short inDuck(AISTATE* aiState);
int genDudeSeqStartId(XSPRITE* pXSprite);
int getRangeAttackDist(spritetype* pSprite, int minDist = 1200, int maxDist = 80000);
int getDispersionModifier(spritetype* pSprite, int minDisp, int maxDisp);
void scaleDamage(XSPRITE* pXSprite);
bool genDudePrepare(spritetype* pSprite, int propId = kGenDudePropertyAll);
void genDudeUpdate(spritetype* pSprite);
void genDudeProcess(spritetype* pSprite, XSPRITE* pXSprite);

END_BLD_NS
