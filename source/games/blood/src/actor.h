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

BEGIN_BLD_NS

enum DAMAGE_TYPE {
	kDamageFall = 0,
	kDamageBurn,
	kDamageBullet,
	kDamageExplode,
	kDamageDrown,
	kDamageSpirit,
	kDamageTesla,
	kDamageMax = 7,
};

enum VECTOR_TYPE {
	kVectorTine = 0,
	kVectorShell,
	kVectorBullet,
	kVectorTommyAP,
	kVectorShellAP,
	kVectorTommyRegular,
	kVectorBatBite,
	kVectorBoneelBite,
	kVectorGillBite,
	kVectorBeastSlash,
	kVectorAxe,
	kVectorCleaver,
	kVectorGhost,
	kVectorGargSlash,
	kVectorCerberusHack,
	kVectorHoundBite,
	kVectorRatBite,
	kVectorSpiderBite,
	VECTOR_TYPE_18,
	VECTOR_TYPE_19,
	kVectorTchernobogBurn,
	kVectorVoodoo10,
	#ifdef NOONE_EXTENSIONS
	kVectorGenDudePunch,
	#endif
	kVectorMax,
};

struct EXPLOSION
{
	uint8_t repeat;
	uint8_t dmg;
	uint8_t dmgRng;
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

	double fMaxDist() const { return maxDist * maptoworld; }
};

extern const EXPLOSION explodeInfo[];
extern const VECTORDATA gVectorData[];

const int gDudeDrag = 0x2a00;



#ifdef POLYMER
void actAddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio);
void actDoLight(int spriteNum);
#endif

bool IsUnderwaterSector(sectortype* pSector);
void actInit(TArray<DBloodActor*>& actors);
void actWallBounceVector(DBloodActor* actor, walltype* pWall, double factor);
DVector4 actFloorBounceVector(DBloodActor* actor, double oldz, sectortype* pSector, double factor);
void actRadiusDamage(DBloodActor* source, const DVector3& pos, sectortype* pSector, int nDist, int a7, int a8, DAMAGE_TYPE a9, int a10, int a11);
DBloodActor *actDropObject(DBloodActor *pSprite, int nType);
bool actHealDude(DBloodActor* pXDude, int a2, int a3);
void actKillDude(DBloodActor* a1, DBloodActor* pSprite, DAMAGE_TYPE a3, int a4);
int actDamageSprite(DBloodActor* pSource, DBloodActor* pTarget, DAMAGE_TYPE damageType, int damage);
void actHitcodeToData(int a1, HitInfo *pHitInfo, DBloodActor **actor, walltype **a7 = nullptr);
void actAirDrag(DBloodActor *pSprite, fixed_t drag);
void actExplodeSprite(DBloodActor *pSprite);
void actActivateGibObject(DBloodActor *actor);
void actProcessSprites(void);
DBloodActor* actSpawnSprite(sectortype* pSector, const DVector3& pos, int nStat, bool setextra, PClass* cls = nullptr, int type = 0);
DBloodActor* actSpawnDude(DBloodActor* pSource, int nType, double dist);
DBloodActor * actSpawnSprite(DBloodActor *pSource, int nStat, PClass* cls = nullptr, int type = 0);
DBloodActor* actSpawnThing(sectortype* pSector, const DVector3& pos, int nThingType);

DBloodActor* actFireThing(DBloodActor* actor, double xyoff, double zoff, double zvel, int thingType, double nSpeed);
DBloodActor* actFireMissile(DBloodActor* actor, double xyoff, double zoff, DVector3 dc, int nType);

bool IsBurningDude(DBloodActor* pSprite);
void actBurnSprite(DBloodActor* pSource, DBloodActor* pTarget, int nTime);

bool isGrown(DBloodActor* pSprite);
bool isShrunk(DBloodActor* pSprite);
bool ceilIsTooLow(DBloodActor* actor);

int actGetRespawnTime(DBloodActor *pSprite);
bool actCheckRespawn(DBloodActor *pSprite);

void actFireVector(DBloodActor* shooter, double offset, double zoffset, DVector3 dv, VECTOR_TYPE vectorType, double nRange = -1);
void actPostSprite(DBloodActor* actor, int status);
void actPostProcess(void);
void actOnHit(DBloodActor *actor, Collision& hit);
void callActorFunction(VMFunction* funcID, DBloodActor* actor);


extern const int16_t DudeDifficulty[];


bool IsUnderwaterSector(sectortype* pSector);

// route state, seq and event callbacks through the scripting interface.
// this needs to work with incomplete data, so avoid the asserting macros.
#define DEF_ANIMATOR(func) \
    void func(DBloodActor*); \
    DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, func, func) \
    { \
        auto self = (DBloodActor *)(param[0].a); \
        func(self); \
		return 0; \
    }
#define AF(func) DBloodActor_##func##_VMPtr

void callActorFunction(VMFunction* callback, DBloodActor* actor);

#define xx(n) inline PClassActor* n##Class;
#include "classnames.h"
#undef xx

void RegisterClasses();


END_BLD_NS
