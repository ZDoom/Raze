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
#include "nnexts.h"
#include "ai.h"
#include "eventq.h"


BEGIN_BLD_NS
#ifdef NOONE_EXTENSIONS
enum
{
    kGenDudeDefaultSeq = 11520,
    kGenDudeMaxSlaves = 7,
    kGenDudeTransformStatus = -222,
    kGenDudeUpdTimeRate = 10,
    kGenDudeMaxMeleeDist = 2048,
    kGenDudeMinDispesion = 200,
    kGenDudeMaxDispersion = 3500,
    kGenDudeKlabsAng = 56
};

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

enum {
kGenDudeWeaponNone          = -1,
kGenDudeWeaponHitscan       = 0,
kGenDudeWeaponMissile       = 1,
kGenDudeWeaponThrow         = 2,
kGenDudeWeaponSummon        = 3,
kGenDudeWeaponKamikaze      = 4,
kGenDudeWeaponMax              , 
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
extern AISTATE genDudeDodgeShorterL;
extern AISTATE genDudeDodgeShorterD;
extern AISTATE genDudeDodgeShorterW;
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
extern AISTATE genDudeSearchShortL;
extern AISTATE genDudeSearchShortW;

struct GENDUDESND
{
    int defaultSndId;
    int randomRange;
    int sndIdOffset;  // relative to data3
    bool aiPlaySound; // false = sfxStart3DSound();
    bool interruptable;
};

extern const GENDUDESND gCustomDudeSnd[];

// temporary, until normal DUDEEXTRA gets refactored
struct GENDUDEEXTRA 
{
    unsigned short initVals[3];             // xrepeat, yrepeat, clipdist
    unsigned short availDeaths[kDamageMax]; // list of seqs with deaths for each damage type
    unsigned int moveSpeed;
    unsigned int fireDist;          // counts from sprite size
    unsigned int throwDist;         // counts from sprite size
    unsigned short curWeapon;       // data1 duplicate to avoid potential problems when changing data dynamically
    unsigned short weaponType;
    unsigned short baseDispersion;
    unsigned short slaveCount;              // how many dudes is summoned
    //unsigned short incarnationsCount;
    signed short nLifeLeech;        // spritenum of dropped dude's leech
    signed short slave[kGenDudeMaxSlaves];  // index of the ones dude is summon
    signed short dmgControl[kDamageMax];    // depends of current weapon, drop armor item, sprite yrepeat and surface type
    bool updReq[kGenDudePropertyMax]; // update requests
    union
    {
        struct
        {
            bool sndPlaying : 1;                        // indicate if sound of AISTATE currently playing
            bool forcePunch : 1;                        // indicate if there is no fire trigger in punch state seq
            bool isMelee : 1;
            bool canBurn : 1;                           // can turn in Burning dude or not
            bool canElectrocute : 1;
            bool canAttack : 1;
            bool canRecoil : 1;
            bool canWalk : 1;
            bool canDuck : 1;
            bool canSwim : 1;
            bool canFly : 1;
        };
        int flags;
    };
};

extern GENDUDEEXTRA gGenDudeExtra[kMaxSprites];

GENDUDEEXTRA* genDudeExtra(spritetype* pGenDude);
XSPRITE* getNextIncarnation(XSPRITE* pXSprite);
void killDudeLeech(spritetype* pLeech);
void removeLeech(spritetype* pLeech, bool delSprite = true);
void removeDudeStuff(spritetype* pSprite);
spritetype* leechIsDropped(spritetype* pSprite);
bool spriteIsUnderwater(spritetype* pSprite, bool oldWay = false);
bool playGenDudeSound(spritetype* pSprite, int mode);
void aiGenDudeMoveForward(DBloodActor* actor);
void aiGenDudeChooseDirection(spritetype* pSprite, XSPRITE* pXSprite, int a3, int aXvel = -1, int aYvel = -1);
void aiGenDudeNewState(spritetype* pSprite, AISTATE* pAIState);
int getGenDudeMoveSpeed(spritetype* pSprite, int which, bool mul, bool shift);
int checkAttackState(DBloodActor* actor);
bool doExplosion(spritetype* pSprite, int nType);
spritetype* genDudeSpawn(XSPRITE* pXSource, spritetype* pSprite, int nDist);
void genDudeTransform(spritetype* pSprite);
void dudeLeechOperate(spritetype* pSprite, XSPRITE* pXSprite, EVENT a3);
int getDodgeChance(spritetype* pSprite);
int getRecoilChance(spritetype* pSprite);
bool dudeIsMelee(XSPRITE* pXSprite);
void updateTargetOfSlaves(spritetype* pSprite);
void updateTargetOfLeech(spritetype* pSprite);
bool canSwim(spritetype* pSprite);
bool canDuck(spritetype* pSprite);
bool canWalk(spritetype* pSprite);
short inDodge(AISTATE* aiState);
bool inIdle(AISTATE* aiState);
bool inAttack(AISTATE* aiState);
short inRecoil(AISTATE* aiState);
short inSearch(AISTATE* aiState);
short inChase(AISTATE* aiState);
short inDuck(AISTATE* aiState);
int genDudeSeqStartId(XSPRITE* pXSprite);
int getRangeAttackDist(spritetype* pSprite, int minDist = 3000, int maxDist = 80000);
int getDispersionModifier(spritetype* pSprite, int minDisp, int maxDisp);
void scaleDamage(XSPRITE* pXSprite);
bool genDudePrepare(spritetype* pSprite, int propId);
void genDudeUpdate(spritetype* pSprite);
void genDudePostDeath(spritetype* pSprite, DAMAGE_TYPE damageType, int damage);
void aiGenDudeInitSprite(spritetype* pSprite, XSPRITE* pXSprite);
#endif
END_BLD_NS
