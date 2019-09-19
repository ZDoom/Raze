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

enum DAMAGE_TYPE {
    DAMAGE_TYPE_0 = 0,
    DAMAGE_TYPE_1, // Flame
    DAMAGE_TYPE_2,
    DAMAGE_TYPE_3,
    DAMAGE_TYPE_4,
    DAMAGE_TYPE_5,
    DAMAGE_TYPE_6, // Tesla
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
    VECTOR_TYPE_22,
    kVectorMax,
};

struct THINGINFO
{
    short at0; // health
    short at2; // mass
    unsigned char at4; // clipdist
    short at5; // flags
    int at7; // elasticity
    int atb; // damage resistance
    short atf; // cstat
    short at11; // picnum
    char at13; // shade
    unsigned char at14; // pal
    unsigned char at15; // xrepeat
    unsigned char at16; // yrepeat
    int at17[7]; // damage
    int allowThrow; // By NoOne: indicates if kGDXCustomDude can throw it
};

struct AMMOITEMDATA
{
    short at0;
    short picnum; // startHealth
    char shade; // mass
    char at5;
    unsigned char xrepeat; // at6
    unsigned char yrepeat; // at7
    short at8;
    unsigned char ata;
    unsigned char atb;
};

struct WEAPONITEMDATA
{
    short at0;
    short picnum; // startHealth
    char shade; // mass
    char at5;
    unsigned char xrepeat; // at6
    unsigned char yrepeat; // at7
    short at8;
    short ata;
    short atc;
};

struct ITEMDATA
{
    short at0; // unused?
    short picnum; // startHealth
    char shade; // mass
    char at5; // unused?
    unsigned char xrepeat; // at6
    unsigned char yrepeat; // at7
    short at8;
};

struct MissileType
{
    short picnum;
    int at2; // speed
    int at6; // angle
    unsigned char ata; // xrepeat
    unsigned char atb; // yrepeat
    char atc; // shade
    unsigned char atd; // clipdist
    int fireSound[2]; // By NoOne: predefined fire sounds. used by kGDXCustomDude, but can be used for something else.
};

struct EXPLOSION
{
    unsigned char at0;
    char at1; // dmg
    char at2; // dmg rnd
    int at3; // radius
    int at7;
    int atb;
    int atf;
    int at13;
    int at17;
};

struct VECTORDATA_at1d {
    FX_ID at0;
    FX_ID at1;
    FX_ID at2;
    int at3;
};

struct VECTORDATA {
    DAMAGE_TYPE at0;
    int at1; // damage
    int at5;
    int maxDist; // range
    int atd;
    int at11; // burn
    int at15; // blood splats
    int at19; // blood splat chance
    VECTORDATA_at1d at1d[15];
    int fireSound[2]; // By NoOne: predefined fire sounds. used by kGDXCustomDude, but can be used for something else.
};

struct SPRITEHIT {
    int hit, ceilhit, florhit;
};

extern AMMOITEMDATA gAmmoItemData[];
extern WEAPONITEMDATA gWeaponItemData[];
extern ITEMDATA gItemData[];
extern MissileType missileInfo[];
extern EXPLOSION explodeInfo[];
extern THINGINFO thingInfo[];
extern VECTORDATA gVectorData[];

extern SPRITEHIT gSpriteHit[];

extern int gDudeDrag;
extern short gAffectedSectors[kMaxSectors];
extern short gAffectedXWalls[kMaxXWalls];

inline void GetSpriteExtents(spritetype *pSprite, int *top, int *bottom)
{
    *top = *bottom = pSprite->z;
    if ((pSprite->cstat & 0x30) != 0x20)
    {
        int height = tilesiz[pSprite->picnum].y;
        int center = height / 2 + picanm[pSprite->picnum].yofs;
        *top -= (pSprite->yrepeat << 2)*center;
        *bottom += (pSprite->yrepeat << 2)*(height - center);
    }
}


inline bool IsPlayerSprite(spritetype *pSprite)
{
    if (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8)
        return 1;
    return 0;
}

inline bool IsDudeSprite(spritetype *pSprite)
{
    if (pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
        return 1;
    return 0;
}

inline void actBurnSprite(int nSource, XSPRITE *pXSprite, int nTime)
{
    pXSprite->burnTime = ClipHigh(pXSprite->burnTime + nTime, sprite[pXSprite->reference].statnum == 6 ? 2400 : 1200);
    pXSprite->burnSource = nSource;
}

bool IsItemSprite(spritetype *pSprite);
bool IsWeaponSprite(spritetype *pSprite);
bool IsAmmoSprite(spritetype *pSprite);
bool IsUnderwaterSector(int nSector);
int actSpriteOwnerToSpriteId(spritetype *pSprite);
void actPropagateSpriteOwner(spritetype *pTarget, spritetype *pSource);
int actSpriteIdToOwnerId(int nSprite);
int actOwnerIdToSpriteId(int nSprite);
bool actTypeInSector(int nSector, int nType);
void actAllocateSpares(void);
void actInit(void);
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
void actImpactMissile(spritetype *pMissile, int a2);
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
spritetype* DropRandomPickupObject(spritetype* pSprite, short prevItem);
spritetype* spawnRandomDude(spritetype* pSprite);
int GetDataVal(spritetype* pSprite, int data);
int my_random(int a, int b);
int GetRandDataVal(int *rData, spritetype* pSprite);
bool sfxPlayMissileSound(spritetype* pSprite, int missileId);
bool sfxPlayVectorSound(spritetype* pSprite, int vectorId);
spritetype* actSpawnCustomDude(spritetype* pSprite, int nDist);
int getDudeMassBySpriteSize(spritetype* pSprite);
bool ceilIsTooLow(spritetype* pSprite);