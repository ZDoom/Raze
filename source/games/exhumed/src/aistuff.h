//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#include "compat.h"
#include "freelistarray.h"
#include "exhumedactor.h"


BEGIN_PS_NS


// anims

void InitAnims();
void DestroyAnim(DExhumedActor* nAnim);
DExhumedActor* BuildAnim(DExhumedActor* actor, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag);

void FuncAnim(int, int, int, int);
void BuildExplosion(DExhumedActor* actor);
void BuildSplash(DExhumedActor* actor, int nSector);


// anubis

void BuildAnubis(DExhumedActor* nSprite, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer);
void FuncAnubis(int, int a, int b, int c);

// bubbles

void BuildBubbleMachine(DExhumedActor* nSprite);
void DoBubbleMachines();
void DoBubbles(int nPlayer);
void FuncBubble(int, int, int, int);

// bullet

// 32 bytes
struct bulletInfo
{
    short nDamage; // 0
    short field_2; // 2
    int field_4;   // 4
    short field_8; // 8
    short nSeq; // 10
    short field_C; // 12
    short nFlags;
    short nRadius; // damage radius
    short xyRepeat;
};

extern bulletInfo BulletInfo[];

extern short nRadialBullet;
extern short lasthitsect;
extern int lasthitz;
extern int lasthitx;
extern int lasthity;
extern TArray<DExhumedActor*> EnergyBlocks;

void InitBullets();
int GrabBullet();
void DestroyBullet(short nRun);
int MoveBullet(short nBullet);
void SetBulletEnemy(short nBullet, DExhumedActor* nEnemy);
DExhumedActor* BuildBullet(DExhumedActor* pActor, int nType, int val1, int nAngle, DExhumedActor* pTarget, int val3);

void IgniteSprite(DExhumedActor* nSprite);
void FuncBullet(int, int, int, int);
void BackUpBullet(int *x, int *y, short nAngle);

// fish

void BuildFish(DExhumedActor* nSprite, int x, int y, int z, int nSector, int nAngle);
void FuncFish(int, int, int, int);
void FuncFishLimb(int a, int b, int c);

// grenade

enum { kMaxGrenades = 50 };

void BuildGrenade(int nPlayer);
void DestroyGrenade(short nGrenade);
void ThrowGrenade(short nPlayer, int edx, int ebx, int ecx, int push1);
void FuncGrenade(int, int, int, int);

// gun

enum { kMaxWeapons = 7 };

enum
{
    kWeaponSword = 0,
    kWeaponPistol,
    kWeaponM60,
    kWeaponFlamer,
    kWeaponGrenade,
    kWeaponStaff,
    kWeaponRing,
    kWeaponMummified
};

struct Weapon
{
    short nSeq;
    short b[12]; // seq offsets?
    short nAmmoType;
    short c;
    short d; // default or min ammo? or ammo used per 'shot' ?
    short bFireUnderwater;
//	short pad[15];
};

extern Weapon WeaponInfo[];
extern short nTemperature[];

void RestoreMinAmmo(short nPlayer);
void FillWeapons(short nPlayer);
void ResetPlayerWeapons(short nPlayer);
void InitWeapons();
void SetNewWeapon(short nPlayer, short nWeapon);
void SetNewWeaponImmediate(short nPlayer, short nWeapon);
void SetNewWeaponIfBetter(short nPlayer, short nWeapon);
void SelectNewWeapon(short nPlayer);
void StopFiringWeapon(short nPlayer);
void FireWeapon(short nPlayer);
void CheckClip(short nPlayer);
void MoveWeapons(short nPlayer);
void DrawWeapons(double smooth);

// items

enum
{
    kItemHeart = 0,
    kItemInvincibility,
    kItemTorch,
    kItemDoubleDamage,
    kItemInvisibility,
    kItemMask,
};

extern short nItemMagic[];

void BuildItemAnim(DExhumedActor* nSprite);
void ItemFlash();
void FillItems(short nPlayer);
void UseItem(short nPlayer, short nItem);
void UseCurItem(short nPlayer);
int GrabItem(short nPlayer, short nItem);
void DropMagic(DExhumedActor* actor);
void InitItems();
void StartRegenerate(DExhumedActor* nSprite);
void DoRegenerates();

// lavadude

void BuildLava(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
DExhumedActor* BuildLavaLimb(DExhumedActor* nSprite, int edx, int ebx);
void FuncLavaLimb(int, int, int, int);
void FuncLava(int, int, int, int);

// lighting

extern short nFlashDepth;

void InitLights();
void AddFlash(short nSector, int x, int y, int z, int val);
void SetTorch(int nPlayer, int bTorchOnOff);
void UndoFlashes();
void DoLights();
void AddFlow(int nSprite, int nSpeed, int b, int ang = -1);
void BuildFlash(short nPlayer, short nSector, int nVal);
void AddGlow(short nSector, int nVal);
void AddFlicker(short nSector, int nVal);

extern short bTorch;

// lion

void BuildLion(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle);
void FuncLion(int, int, int, int);

// move

// 16 bytes
struct BlockInfo
{
    DExhumedActor* pActor;
    int x;
    int y;
    int field_8;
};
extern BlockInfo sBlockInfo[];

extern Collision hiHit;
extern DExhumedActor* nChunkSprite[];
extern DExhumedActor* nBodySprite[];

signed int lsqrt(int a1);
void MoveThings();
void ResetMoveFifo();
void InitChunks();
void InitPushBlocks();
void Gravity(DExhumedActor* actor);
DExhumedActor* UpdateEnemy(DExhumedActor** ppEnemy);
Collision MoveCreature(DExhumedActor* nSprite);
Collision MoveCreatureWithCaution(DExhumedActor* actor);
void WheresMyMouth(int nPlayer, vec3_t* pos, short *sectnum);
int GetActorHeight(DExhumedActor* nSprite);
DExhumedActor* insertActor(int, int);
DExhumedActor* GrabBody();
DExhumedActor* GrabBodyGunSprite();
void CreatePushBlock(int nSector);
void FuncCreatureChunk(int a, int, int nRun);
DExhumedActor* FindPlayer(DExhumedActor* nSprite, int nDistance, bool dontengage = false);

DExhumedActor* BuildCreatureChunk(DExhumedActor* pSrc, int nPic, bool bSpecial = false);
void BuildNear(int x, int y, int walldist, int nSector);
int PlotCourseToSprite(DExhumedActor* nSprite1, DExhumedActor* nSprite2);
void CheckSectorFloor(short nSector, int z, int *x, int *y);
int GetAngleToSprite(DExhumedActor* nSprite1, DExhumedActor* nSprite2);
int GetWallNormal(short nWall);
int GetUpAngle(DExhumedActor* nSprite1, int nVal, DExhumedActor* nSprite2, int ecx);
void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel);
Collision AngleChase(DExhumedActor* nSprite, DExhumedActor* nSprite2, int ebx, int ecx, int push1);
void SetQuake(DExhumedActor* nSprite, int nVal);

// mummy

enum { kMaxMummies = 150 };

void BuildMummy(DExhumedActor* val, int x, int y, int z, int nSector, int nAngle);
void FuncMummy(int nSector, int edx, int nRun);

// object

enum kStatus
{
    kStatDestructibleSprite = 97,
    kStatAnubisDrum,
    kStatExplodeTrigger = 141,
    kStatExplodeTarget = 152,
    kStatBubbleMachine = 1022,

};

extern short nSmokeSparks;
extern short nDronePitch;
extern int lFinaleStart;
extern DExhumedActor* pFinaleSpr;

void InitObjects();
void InitElev();
void InitPoint();
void InitSlide();
void InitWallFace();
void DoDrips();
void DoMovingSects();
void DoFinale();
void PostProcess();

void FuncElev(int, int, int, int);
void FuncWallFace(int, int, int, int);
void FuncSlide(int, int, int, int);
void FuncObject(int, int, int, int);
void FuncTrap(int, int, int, int);
void FuncEnergyBlock(int, int, int, int);
void FuncSpark(int, int, int, int);
void SnapBobs(short nSectorA, short nSectorB);
DExhumedActor* FindWallSprites(short nSector);
void AddMovingSector(int nSector, int edx, int ebx, int ecx);
void ProcessTrailSprite(DExhumedActor* nSprite, int nLotag, int nHitag);
void AddSectorBob(int nSector, int nHitag, int bx);
DExhumedActor* BuildObject(DExhumedActor* nSprite, int nOjectType, int nHitag);
int BuildArrow(DExhumedActor* nSprite, int nVal);
int BuildFireBall(DExhumedActor*, int a, int b);
void BuildDrip(DExhumedActor* nSprite);
DExhumedActor* BuildEnergyBlock(short nSector);
int BuildElevC(int arg1, int nChannel, int nSector, DExhumedActor* nWallSprite, int arg5, int arg6, int nCount, ...);
int BuildElevF(int nChannel, int nSector, DExhumedActor* nWallSprite, int arg_4, int arg_5, int nCount, ...);
int BuildWallFace(short nChannel, short nWall, int nCount, ...);
int BuildSlide(int nChannel, int edx, int ebx, int ecx, int arg1, int arg2, int arg3);

// queen

void InitQueens();
void BuildQueen(DExhumedActor* nSprite, int x, int y, int z, int nSector, int nAngle, int nVal);
void FuncQueenEgg(int, int, int, int);
void FuncQueenHead(int, int, int, int);
void FuncQueen(int, int, int, int);

// ra

struct RA
{
	DExhumedActor* pActor;
	DExhumedActor* pTarget;
	
    short nAction;
    short nFrame;
    short nRun;
    short field_A;
    short field_C;
    short nPlayer;
};

// ra
extern RA Ra[];

void FreeRa(short nPlayer);
void BuildRa(short nPlayer);
void InitRa();
void MoveRaToEnemy(short nPlayer);
void FuncRa(int, int, int, int);

// rat

void InitRats();
void SetRatVel(short nSprite);
void BuildRat(DExhumedActor* nSprite, int x, int y, int z, short nSector, int nAngle);
int FindFood(short nSprite);
void FuncRat(int a, int, int b, int nRun);

// rex

void BuildRex(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncRex(int, int, int, int);

// roach

void BuildRoach(int nType, DExhumedActor* nSprite, int x, int y, int z, short nSector, int angle);
void FuncRoach(int a, int, int nDamage, int nRun);

// runlist

enum
{
	kMaxRuns		= 25600,
	kMaxChannels	= 4096
};

struct RunStruct
{
    int nAIType;                // todo later: replace this with an AI pointer
    int nObjIndex;              // If object is a non-actor / not refactored yet.
    DExhumedActor* pObjActor;   // If object is an actor
    short next;
    short prev;
};

struct RunChannel
{
    short a;
    short b;
    short c;
    short d;
};


struct RunListEvent
{
    int nMessage;
    int nParam;                 // mostly the player, sometimes the channel list
    int nObjIndex;
    DExhumedActor* pObjActor;
    tspritetype* pTSprite;      // for the draw event
    DExhumedActor* pOtherActor;      // for the damage event, radialSpr for radial damage - owner will not be passed as it can be retrieved from this.
    int nDamage, nRun;

    int nRadialDamage;          // Radial damage needs a bit more info.
    int nDamageRadius;
    DExhumedActor* pRadialActor;

    bool isRadialEvent() const { return nMessage == 1; }
};

struct ExhumedAI
{
    //virtual ~ExhumedAI() = default;
    virtual void ProcessChannel(RunListEvent* ev) {}
    virtual void Tick(RunListEvent* ev) {}
    virtual void Process(RunListEvent* ev) {}
    virtual void Use(RunListEvent* ev) {}
    virtual void TouchFloor(RunListEvent* ev) {}
    virtual void LeaveSector(RunListEvent* ev) {}
    virtual void EnterSector(RunListEvent* ev) {}
    virtual void Damage(RunListEvent* ev) {}
    virtual void Draw(RunListEvent* ev) {}
    virtual void RadialDamage(RunListEvent* ev) {}
};

struct AIAnim : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AIAnubis : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
};

struct AIBubble : ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

class AIBullet : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AIFish : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

class AIFishLimb : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AIGrenade : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AILavaDude : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AILavaDudeLimb : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AILion : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AICreatureChunk : public ExhumedAI
{
    virtual void Tick(RunListEvent* ev) override;
};

struct AIMummy : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIElev : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Tick(RunListEvent* ev) override;
};

struct AIWallFace : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
};

struct AISlide : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Tick(RunListEvent* ev) override;
};

struct AITrap : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Tick(RunListEvent* ev) override;
};

struct AISpark : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
};

struct AIEnergyBlock : public ExhumedAI
{
    virtual void Damage(RunListEvent* ev) override;
    virtual void RadialDamage(RunListEvent* ev) override;
};

struct AIObject : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIPlayer : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIQueenEgg : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIQueenHead : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIQueen : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIRa : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AIRat : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIRex : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIRoach : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIScorp : public ExhumedAI
{
    void Effect(RunListEvent* ev, DExhumedActor* nTarget, int mode);
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AISet : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AISoul : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
};

struct AISnake : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
};

struct AISpider : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AIWasp : public ExhumedAI
{
    void Tick(RunListEvent* ev) override;
    void Damage(RunListEvent* ev) override;
    void Draw(RunListEvent* ev) override;
    void RadialDamage(RunListEvent* ev) override;
};

struct AISWReady : public ExhumedAI
{
    void Process(RunListEvent* ev) override;
};

struct AISWPause : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Tick(RunListEvent* ev) override;
    void Process(RunListEvent* ev) override;
};

struct AISWStepOn : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void TouchFloor(RunListEvent* ev) override;
};

struct AISWNotOnPause : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Tick(RunListEvent* ev) override;
    void Process(RunListEvent* ev) override;
    void TouchFloor(RunListEvent* ev) override;
};

struct AISWPressSector : public ExhumedAI
{
    void ProcessChannel(RunListEvent* ev) override;
    void Use(RunListEvent* ev) override;
};

struct AISWPressWall : public ExhumedAI
{
    void Process(RunListEvent* ev) override;
    void Use(RunListEvent* ev) override;
};


typedef void(*AiFunc)(int, int, int, int nRun);

extern FreeListArray<RunStruct, kMaxRuns> RunData;

extern RunChannel sRunChannels[kMaxChannels];
extern short NewRun;

void runlist_InitRun();

int runlist_GrabRun();
int runlist_FreeRun(int nRun);
int runlist_AddRunRec(int index, int object, int aitype);
int runlist_AddRunRec(int index, DExhumedActor* object, int aitype);
int runlist_AddRunRec(int index, RunStruct* other);
int runlist_HeadRun();
void runlist_InitChan();
void runlist_ChangeChannel(int eax, short dx);
void runlist_ReadyChannel(short eax);
void runlist_ProcessSectorTag(int nSector, int nLotag, int nHitag);
int runlist_AllocChannel(int a);
void runlist_DoSubRunRec(int RunPtr);
void runlist_SubRunRec(int RunPtr);
void runlist_ProcessWallTag(int nWall, short nLotag, short nHitag);
int runlist_CheckRadialDamage(DExhumedActor* actor);
void runlist_RadialDamageEnemy(DExhumedActor* nSprite, short nSprite2, short nDamage);
void runlist_DamageEnemy(DExhumedActor* nSprite, DExhumedActor* nSprite2, short nDamage);
void runlist_SignalRun(int NxtPtr, int edx, void(ExhumedAI::* func)(RunListEvent*), RunListEvent* ev = nullptr);

void runlist_CleanRunRecs();
void runlist_ExecObjects();

// scorp

void BuildScorp(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncScorp(int, int, int, int);

// set

void BuildSet(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncSoul(int, int, int, int);
void FuncSet(int, int, int, int);

// snake

enum { kSnakeSprites = 8 }; // or rename to kSnakeParts?

// 32bytes
struct Snake
{
    DExhumedActor* pEnemy;	 // nRun
    DExhumedActor* pSprites[kSnakeSprites];

	short nCountdown;
    short sC;
    short nRun;

    char c[8];
    short sE;
    short nSnakePlayer;
};

enum { kMaxSnakes = 50 };

extern FreeListArray<Snake, kMaxSnakes> SnakeList;

void InitSnakes();
short GrabSnake();
void BuildSnake(short nPlayer, short zVal);
void FuncSnake(int, int, int, int);

// spider

DExhumedActor* BuildSpider(DExhumedActor* nSprite, int x, int y, int z, short nSector, int nAngle);
void FuncSpider(int a, int, int b, int nRun);

// switch

void InitLink();
void InitSwitch();

void FuncSwReady(int, int, int, int);
void FuncSwPause(int, int, int, int);
void FuncSwStepOn(int, int, int, int);
void FuncSwNotOnPause(int, int, int, int);
void FuncSwPressSector(int, int, int, int);
void FuncSwPressWall(int, int, int, int);

std::pair<int, int> BuildSwPause(int nChannel, int nLink, int ebx);
std::pair<int, int> BuildSwNotOnPause(int nChannel, int nLink, int nSector, int ecx);
int BuildLink(int nCount, ...);
std::pair<int, int> BuildSwPressSector(int nChannel, int nLink, int nSector, int ecx);
std::pair<int, int> BuildSwStepOn(int nChannel, int nLink, int nSector);
std::pair<int, int> BuildSwReady(int nChannel, short nLink);

std::pair<int, int> BuildSwPressWall(short nChannel, short nLink, short nWall);

// wasp

DExhumedActor* BuildWasp(DExhumedActor* nSprite, int x, int y, int z, short nSector, short nAngle, bool bEggWasp);
void FuncWasp(int eax, int, int edx, int nRun);







enum { kMessageMask = 0x7F0000 };
inline int GrabTimeSlot(int nVal) { return -1; }
inline int dmgAdjust(int dmg, int fact = 2) { return dmg; }
inline bool easy() { return false; }

END_PS_NS

