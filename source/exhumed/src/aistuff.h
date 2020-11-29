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

BEGIN_PS_NS

// anims

struct Anim
{
    short nSeq;
    short field_2;
    short field_4;
    short nSprite;
};

extern Anim AnimList[];
extern uint8_t AnimFlags[];

void InitAnims();
void DestroyAnim(int nAnim);
int BuildAnim(int nSprite, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag);
short GetAnimSprite(short nAnim);

void FuncAnim(int, int, int);
void BuildExplosion(short nSprite);
int BuildSplash(int nSprite, int nSector);

// anubis

void InitAnubis();
int BuildAnubis(int nSprite, int x, int y, int z, int nSector, int nAngle, uint8_t bIsDrummer);
void FuncAnubis(int a, int b, int c);

// bubbles

void InitBubbles();
void BuildBubbleMachine(int nSprite);
void DoBubbleMachines();
void DoBubbles(int nPlayer);
void FuncBubble(int, int, int);

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

void InitBullets();
short GrabBullet();
void DestroyBullet(short nRun);
int MoveBullet(short nBullet);
void SetBulletEnemy(short nBullet, short nEnemy);
int BuildBullet(short nSprite, int nType, int ebx, int ecx, int val1, int nAngle, int val2, int val3);
void IgniteSprite(int nSprite);
void FuncBullet(int, int, int);
void BackUpBullet(int *x, int *y, short nAngle);

// fish

void InitFishes();
int BuildFish(int nSprite, int x, int y, int z, int nSector, int nAngle);
void FuncFish(int, int, int);
void FuncFishLimb(int a, int b, int c);

// grenade

enum { kMaxGrenades = 50 };

void InitGrenades();
int BuildGrenade(int nPlayer);
void DestroyGrenade(short nGrenade);
int ThrowGrenade(short nPlayer, int edx, int ebx, int ecx, int push1);
void FuncGrenade(int, int, int);

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

void BuildItemAnim(short nSprite);
void DestroyItemAnim(short nSprite);
void ItemFlash();
void FillItems(short nPlayer);
void UseItem(short nPlayer, short nItem);
void UseCurItem(short nPlayer);
int GrabItem(short nPlayer, short nItem);
void DropMagic(short nSprite);
void InitItems();
void StartRegenerate(short nSprite);
void DoRegenerates();

// lavadude

void InitLava();
int BuildLava(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
int BuildLavaLimb(int nSprite, int edx, int ebx);
void FuncLavaLimb(int, int, int);
void FuncLava(int, int, int);

// lighting

extern short nFlashDepth;

void InitLights();
void AddFlash(short nSector, int x, int y, int z, int val);
void SetTorch(int nPlayer, int bTorchOnOff);
void UndoFlashes();
void DoLights();
void AddFlow(int nSprite, int nSpeed, int b);
void BuildFlash(short nPlayer, short nSector, int nVal);
void AddGlow(short nSector, int nVal);
void AddFlicker(short nSector, int nVal);

extern short bTorch;

// lion

void InitLion();
int BuildLion(short nSprite, int x, int y, int z, short nSector, short nAngle);
void FuncLion(int, int, int);

// move

// 16 bytes
struct BlockInfo
{
    int x;
    int y;
    int field_8;
    short nSprite;
};
extern BlockInfo sBlockInfo[];

extern int hihit;
extern short nChunkSprite[];
extern short nBodySprite[];

signed int lsqrt(int a1);
void MoveThings();
void ResetMoveFifo();
void InitChunks();
void InitPushBlocks();
void Gravity(short nSprite);
short UpdateEnemy(short *nEnemy);
int MoveCreature(short nSprite);
int MoveCreatureWithCaution(int nSprite);
void WheresMyMouth(int nPlayer, int *x, int *y, int *z, short *sectnum);
int GetSpriteHeight(int nSprite);
int GrabBody();
int GrabBodyGunSprite();
void CreatePushBlock(int nSector);
void FuncCreatureChunk(int a, int, int nRun);
int FindPlayer(int nSprite, int nDistance);
int BuildCreatureChunk(int nVal, int nPic);
void BuildNear(int x, int y, int walldist, int nSector);
int BelowNear(short nSprite);
int PlotCourseToSprite(int nSprite1, int nSprite2);
void CheckSectorFloor(short nSector, int z, int *x, int *y);
int GetAngleToSprite(int nSprite1, int nSprite2);
int GetWallNormal(short nWall);
int GetUpAngle(short nSprite1, int nVal, short nSprite2, int ecx);
void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel);
int AngleChase(int nSprite, int nSprite2, int ebx, int ecx, int push1);
void SetQuake(short nSprite, int nVal);

// mummy

enum { kMaxMummies = 150 };

void InitMummy();
int BuildMummy(int val, int x, int y, int z, int nSector, int nAngle);
void FuncMummy(int nSector, int edx, int nRun);

// object

enum
{
	kMaxPoints	= 1024,
	kMaxSlides	= 128,
	kMaxElevs	= 1024
};

enum kStatus
{
    kStatDestructibleSprite = 97,
    kStatAnubisDrum,
    kStatExplodeTrigger = 141,
    kStatExplodeTarget = 152
};

extern short nSmokeSparks;
extern short nDronePitch;
extern int lFinaleStart;
extern short nFinaleSpr;

void InitObjects();
void InitElev();
void InitPoint();
void InitSlide();
void InitWallFace();
void DoDrips();
void DoMovingSects();
void DoFinale();
void PostProcess();

void FuncElev(int, int, int);
void FuncWallFace(int, int, int);
void FuncSlide(int, int, int);
void FuncObject(int, int, int);
void FuncTrap(int, int, int);
void FuncEnergyBlock(int, int, int);
void FuncSpark(int, int, int);
void SnapBobs(short nSectorA, short nSectorB);
short FindWallSprites(short nSector);
void AddMovingSector(int nSector, int edx, int ebx, int ecx);
int BuildWallSprite(int nSector);
void ProcessTrailSprite(int nSprite, int nLotag, int nHitag);
void AddSectorBob(int nSector, int nHitag, int bx);
int BuildObject(int const nSprite, int nOjectType, int nHitag);
int BuildArrow(int nSprite, int nVal);
int BuildFireBall(int nSprite, int a, int b);
void BuildDrip(int nSprite);
int BuildEnergyBlock(short nSector);
int BuildElevC(int arg1, int nChannel, int nSector, int nWallSprite, int arg5, int arg6, int nCount, ...);
int BuildElevF(int nChannel, int nSector, int nWallSprite, int arg_4, int arg_5, int nCount, ...);
int BuildWallFace(short nChannel, short nWall, int nCount, ...);
int BuildSlide(int nChannel, int edx, int ebx, int ecx, int arg1, int arg2, int arg3);

// queen

void InitQueens();
int BuildQueen(int nSprite, int x, int y, int z, int nSector, int nAngle, int nVal);
void FuncQueenEgg(int, int, int);
void FuncQueenHead(int, int, int);
void FuncQueen(int, int, int);

// ra

struct RA
{
    short nAction;
    short nFrame;
    short nRun;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short nPlayer;
};

// ra
extern RA Ra[];

void FreeRa(short nPlayer);
int BuildRa(short nPlayer);
void InitRa();
void MoveRaToEnemy(short nPlayer);
void FuncRa(int, int, int);

// rat

void InitRats();
void SetRatVel(short nSprite);
int BuildRat(short nSprite, int x, int y, int z, short nSector, int nAngle);
int FindFood(short nSprite);
void FuncRat(int a, int b, int nRun);

// rex

void InitRexs();
int BuildRex(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncRex(int, int, int);

// roach

void InitRoachs();
int BuildRoach(int nType, int nSprite, int x, int y, int z, short nSector, int angle);
void FuncRoach(int a, int nDamage, int nRun);

// runlist

enum
{
	kMaxRuns		= 25600,
	kMaxChannels	= 4096
};

struct RunStruct
{
    union
    {
        int nMoves;
        struct
        {
            short nVal;
            short nRef;
        };
    };

    short _4;
    short _6;
};

struct RunChannel
{
    short a;
    short b;
    short c;
    short d;
};

typedef void(*AiFunc)(int, int, int nRun);

extern RunStruct RunData[kMaxRuns];
extern RunChannel sRunChannels[kMaxChannels];
extern short NewRun;
extern int nRadialOwner;
extern short nRadialSpr;

void runlist_InitRun();

int runlist_GrabRun();
int runlist_FreeRun(int nRun);
int runlist_AddRunRec(int a, int b);
int runlist_HeadRun();
void runlist_InitChan();
void runlist_ChangeChannel(int eax, short dx);
void runlist_ReadyChannel(short eax);
void runlist_ProcessSectorTag(int nSector, int nLotag, int nHitag);
int runlist_AllocChannel(int a);
void runlist_DoSubRunRec(int RunPtr);
void runlist_SubRunRec(int RunPtr);
void runlist_ProcessWallTag(int nWall, short nLotag, short nHitag);
int runlist_CheckRadialDamage(short nSprite);
void runlist_RadialDamageEnemy(short nSprite, short nDamage, short nRadius);
void runlist_DamageEnemy(int nSprite, int nSprite2, short nDamage);
void runlist_SignalRun(int NxtPtr, int edx);

void runlist_CleanRunRecs();
void runlist_ExecObjects();

// scorp

void InitScorp();
int BuildScorp(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncScorp(int, int, int);

// set

void InitSets();
int BuildSet(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncSoul(int, int, int);
void FuncSet(int, int, int);

// snake

enum { kSnakeSprites = 8 }; // or rename to kSnakeParts?

// 32bytes
struct Snake
{
    short nEnemy;	 // nRun
    short nSprites[kSnakeSprites];

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
int BuildSnake(short nPlayer, short zVal);
void FuncSnake(int, int, int);

// spider

void InitSpider();
int BuildSpider(int nSprite, int x, int y, int z, short nSector, int nAngle);
void FuncSpider(int a, int b, int nRun);

// switch

enum
{
	kMaxLinks		= 1024,
	kMaxSwitches	= 1024
};

void InitLink();
void InitSwitch();

void FuncSwReady(int, int, int);
void FuncSwPause(int, int, int);
void FuncSwStepOn(int, int, int);
void FuncSwNotOnPause(int, int, int);
void FuncSwPressSector(int, int, int);
void FuncSwPressWall(int, int, int);

int BuildSwPause(int nChannel, int nLink, int ebx);
int BuildSwNotOnPause(int nChannel, int nLink, int nSector, int ecx);
int BuildLink(int nCount, ...);
int BuildSwPressSector(int nChannel, int nLink, int nSector, int ecx);
int BuildSwStepOn(int nChannel, int nLink, int nSector);
int BuildSwReady(int nChannel, short nLink);

int BuildSwPressWall(short nChannel, short nLink, short nWall);

// wasp

int WaspCount();

void InitWasps();
int BuildWasp(short nSprite, int x, int y, int z, short nSector, short nAngle);
void FuncWasp(int eax, int edx, int nRun);







enum { kMessageMask = 0x7F0000 };
inline int GrabTimeSlot(int nVal) { return -1; }

END_PS_NS

