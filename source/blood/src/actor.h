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
#pragma once
#include "build.h"
#include "common_game.h"
#include "blood.h"
#include "db.h"
#include "fx.h"
#include "gameutil.h"

BEGIN_BLD_NS

enum DAMAGE_TYPE {
    DAMAGE_TYPE_0 = 0,
    DAMAGE_TYPE_1, // Flame
    DAMAGE_TYPE_2,
    DAMAGE_TYPE_3,
    DAMAGE_TYPE_4,
    DAMAGE_TYPE_5,
    DAMAGE_TYPE_6, // Tesla
    kDamageMax = 7,
};

enum VECTOR_TYPE {
    VECTOR_TYPE_0 = 0,
    VECTOR_TYPE_1,
    VECTOR_TYPE_2,
    VECTOR_TYPE_3,
    VECTOR_TYPE_4,
    VECTOR_TYPE_5,
    VECTOR_TYPE_6,
    VECTOR_TYPE_7,
    VECTOR_TYPE_8,
    VECTOR_TYPE_9,
    VECTOR_TYPE_10,
    VECTOR_TYPE_11,
    VECTOR_TYPE_12,
    VECTOR_TYPE_13,
    VECTOR_TYPE_14,
    VECTOR_TYPE_15,
    VECTOR_TYPE_16,
    VECTOR_TYPE_17,
    VECTOR_TYPE_18,
    VECTOR_TYPE_19,
    VECTOR_TYPE_20,
    VECTOR_TYPE_21,
    #ifdef NOONE_EXTENSIONS
    kVectorGenDudePunch,
    #endif
    kVectorMax,
};

struct THINGINFO
{
    short startHealth;
    short mass;
    unsigned char clipdist;
    short flags;
    int elastic; // elasticity
    int dmgResist; // damage resistance
    short cstat;
    short picnum;
    char shade;
    unsigned char pal;
    unsigned char xrepeat; // xrepeat
    unsigned char yrepeat; // yrepeat
    int dmgControl[kDamageMax]; // damage
};

struct AMMOITEMDATA
{
    short cstat;
    short picnum;
    char shade;
    char pal;
    unsigned char xrepeat;
    unsigned char yrepeat;
    short count;
    unsigned char type;
    unsigned char weaponType;
};

struct WEAPONITEMDATA
{
    short cstat;
    short picnum;
    char shade;
    char pal;
    unsigned char xrepeat;
    unsigned char yrepeat;
    short type;
    short ammoType;
    short count;
};

struct ITEMDATA
{
    short cstat;
    short picnum;
    char shade;
    char pal;
    unsigned char xrepeat;
    unsigned char yrepeat;
    short packSlot;
};

struct MissileType
{
    short picnum;
    int velocity;
    int angleOfs;
    unsigned char xrepeat;
    unsigned char yrepeat;
    char shade;
    unsigned char clipDist;
};

struct EXPLOSION
{
    unsigned char repeat;
    char dmg;
    char dmgRng;
    int radius;
    int dmgType;
    int burnTime;
    int ticks;
    int quakeEffect;
    int flashEffect;
};

struct SURFHIT {
    FX_ID fx1;
    FX_ID fx2;
    FX_ID fx3;
    int fxSnd;
};

struct VECTORDATA {
    DAMAGE_TYPE dmgType;
    int dmg; // damage
    int impulse;
    int maxDist;
    int fxChance;
    int burnTime; // burn
    int bloodSplats; // blood splats
    int splatChance; // blood splat chance
    SURFHIT surfHit[15];
};

extern const AMMOITEMDATA gAmmoItemData[];
extern const WEAPONITEMDATA gWeaponItemData[];
extern const ITEMDATA gItemData[];
extern const MissileType missileInfo[];
extern const EXPLOSION explodeInfo[];
extern const THINGINFO thingInfo[];
extern VECTORDATA gVectorData[];

const int gDudeDrag = 0x2a00;

template<typename T> bool IsPlayerSprite(T const * const pSprite)
{
    return pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8;
}

template<typename T> bool IsDudeSprite(T const * const pSprite)
{
    return pSprite->type >= kDudeBase && pSprite->type < kDudeMax;
}

template<typename T> bool IsItemSprite(T const * const pSprite)
{
    return pSprite->type >= kItemBase && pSprite->type < kItemMax;
}

template<typename T> bool IsWeaponSprite(T const * const pSprite)
{
    return pSprite->type >= kItemWeaponBase && pSprite->type < kItemWeaponMax;
}

template<typename T> bool IsAmmoSprite(T const * const pSprite)
{
    return pSprite->type >= kItemAmmoBase && pSprite->type < kItemAmmoMax;
}

inline void actBurnSprite(int nSource, XSPRITE *pXSprite, int nTime)
{
    pXSprite->burnTime = ClipHigh(pXSprite->burnTime + nTime, sprite[pXSprite->reference].statnum == kStatDude ? 2400 : 1200);
    pXSprite->burnSource = nSource;
}

#ifdef POLYMER
void actAddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio);
void actDoLight(int spriteNum);
#endif

bool IsUnderwaterSector(int nSector);
int actSpriteOwnerToSpriteId(spritetype *pSprite);
void actPropagateSpriteOwner(spritetype *pTarget, spritetype *pSource);
int actSpriteIdToOwnerId(int nSprite);
int actOwnerIdToSpriteId(int nSprite);
bool actTypeInSector(int nSector, int nType);
void actAllocateSpares(void);
void actInit(bool bSaveLoad);
void ConcussSprite(int a1, spritetype *pSprite, int x, int y, int z, int a6);
int actWallBounceVector(int *x, int *y, int nWall, int a4);
int actFloorBounceVector(int *x, int *y, int *z, int nSector, int a5);
void sub_2A620(int nSprite, int x, int y, int z, int nSector, int nDist, int a7, int a8, DAMAGE_TYPE a9, int a10, int a11, int a12, int a13);
void sub_2AA94(spritetype *pSprite, XSPRITE *pXSprite);
spritetype *actSpawnFloor(spritetype *pSprite);
spritetype *actDropAmmo(spritetype *pSprite, int nType);
spritetype *actDropWeapon(spritetype *pSprite, int nType);
spritetype *actDropItem(spritetype *pSprite, int nType);
spritetype *actDropKey(spritetype *pSprite, int nType);
spritetype *actDropFlag(spritetype *pSprite, int nType);
spritetype *actDropObject(spritetype *pSprite, int nType);
bool actHealDude(XSPRITE *pXDude, int a2, int a3);
void actKillDude(int a1, spritetype *pSprite, DAMAGE_TYPE a3, int a4);
int actDamageSprite(int nSource, spritetype *pSprite, DAMAGE_TYPE a3, int a4);
void actHitcodeToData(int a1, HITINFO *pHitInfo, int *a3, spritetype **a4, XSPRITE **a5, int *a6, walltype **a7, XWALL **a8, int *a9, sectortype **a10, XSECTOR **a11);
void actImpactMissile(spritetype *pMissile, int hitCode);
void actKickObject(spritetype *pSprite1, spritetype *pSprite2);
void actTouchFloor(spritetype *pSprite, int nSector);
void ProcessTouchObjects(spritetype *pSprite, int nXSprite);
void actAirDrag(spritetype *pSprite, int a2);
int MoveThing(spritetype *pSprite);
void MoveDude(spritetype *pSprite);
int MoveMissile(spritetype *pSprite);
void actExplodeSprite(spritetype *pSprite);
void actActivateGibObject(spritetype *pSprite, XSPRITE *pXSprite);
bool IsUnderWater(spritetype *pSprite);
void actProcessSprites(void);
spritetype * actSpawnSprite(int nSector, int x, int y, int z, int nStat, char a6);
spritetype *actSpawnDude(spritetype *pSource, short nType, int a3, int a4);
spritetype * actSpawnSprite(spritetype *pSource, int nStat);
spritetype * actSpawnThing(int nSector, int x, int y, int z, int nThingType);
spritetype * actFireThing(spritetype *pSprite, int a2, int a3, int a4, int thingType, int a6);
spritetype* actFireMissile(spritetype *pSprite, int a2, int a3, int a4, int a5, int a6, int nType);
int actGetRespawnTime(spritetype *pSprite);
bool actCheckRespawn(spritetype *pSprite);
bool actCanSplatWall(int nWall);
void actFireVector(spritetype *pShooter, int a2, int a3, int a4, int a5, int a6, VECTOR_TYPE vectorType);
void actPostSprite(int nSprite, int nStatus);
void actPostProcess(void);
void MakeSplash(spritetype *pSprite, XSPRITE *pXSprite);
void actBuildMissile(spritetype* pMissile, int nXSprite, int nSprite);

extern const int DudeDifficulty[];

END_BLD_NS
