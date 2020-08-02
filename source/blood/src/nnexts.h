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


////////////////////////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
/////////////////////////////////////////////////////////////////////////


#pragma once
#include "common_game.h"
#ifdef NOONE_EXTENSIONS
#include "eventq.h"
#include "qav.h"
#include "actor.h"
#include "dude.h"
#include "player.h"

BEGIN_BLD_NS

// CONSTANTS
// additional non-thing proximity, sight and physics sprites 
#define kMaxSuperXSprites 128
#define kMaxTrackingConditions 64
#define kMaxTracedObjects 32 // per one tracking condition

// additional physics attributes for debris sprites
#define kPhysDebrisFly 0x0008 // *debris* affected by negative gravity (fly instead of falling, DO NOT mess with kHitagAutoAim)
#define kPhysDebrisSwim 0x0016 // *debris* can swim underwater (instead of drowning)
#define kPhysDebrisVector 0x0400 // *debris* can be affected by vector weapons
#define kPhysDebrisExplode 0x0800 // *debris* can be affected by explosions

// *modern types only hitag*
#define kModernTypeFlag0 0x0000
#define kModernTypeFlag1 0x0001
#define kModernTypeFlag2 0x0002
#define kModernTypeFlag3 0x0003
#define kModernTypeFlag4 0x0004

#define kMaxRandomizeRetries 16
#define kPercFull 100
#define kCondRange 100



// modern statnums
enum {
kStatModernBase                     = 20,
kStatModernDudeTargetChanger        = kStatModernBase,
kStatModernCondition                = 21,
kStatModernEventRedirector          = 22,
kStatModernPlayerLinker             = 23,
kStatModernBrokenDudeLeech          = 24,
kStatModernQavScene                 = 25,
kStatModernTmp                      = 39,
kStatModernMax                      = 40,
};

// modern sprite types
enum {
kModernCustomDudeSpawn              = 24,
kModernRandomTX                     = 25,
kModernSequentialTX                 = 26,
kModernSeqSpawner                   = 27,
kModernObjPropertiesChanger         = 28,
kModernObjPicnumChanger             = 29,
kModernObjSizeChanger               = 31,
kModernDudeTargetChanger            = 33,
kModernSectorFXChanger              = 34,
kModernObjDataChanger               = 35,
kModernSpriteDamager                = 36,
kModernObjDataAccumulator           = 37,
kModernEffectSpawner                = 38,
kModernWindGenerator                = 39,
kModernRandom                       = 40,
kModernRandom2                      = 80,
kItemShroomGrow                     = 129,
kItemShroomShrink                   = 130,
kItemModernMapLevel                 = 150,  // once picked up, draws whole minimap
kDudeModernCustom                   = kDudeVanillaMax,
kDudeModernCustomBurning            = 255,
kModernThingTNTProx                 = 433, // detects only players
kModernThingThrowableRock           = 434, // does small damage if hits target
kModernThingEnemyLifeLeech          = 435, // the same as normal, except it aims in specified target only
kModernPlayerControl                = 500, /// WIP
kModernCondition                    = 501, /// WIP, sends command only if specified conditions == true
kModernConditionFalse               = 502, /// WIP, sends command only if specified conditions != true
kGenModernMissileUniversal          = 704,
kGenModernSound                     = 708,
};

// type of random
enum {
kRandomizeItem                      = 0,
kRandomizeDude                      = 1,
kRandomizeTX                        = 2,
};

// type of object
enum {
OBJ_WALL                            = 0,
OBJ_SPRITE                          = 3,
OBJ_SECTOR                          = 6,
};

enum {
kCondMixedBase                      = 100,
kCondMixedMax                       = 200,
kCondWallBase                       = 200,
kCondWallMax                        = 300,
kCondSectorBase                     = 300,
kCondSectorMax                      = 400,
kCondPlayerBase                     = 400,
kCondPlayerMax                      = 450,
kCondDudeBase                       = 450,
kCondDudeMax                        = 500,
kCondSpriteBase                     = 500,
kCondSpriteMax                      = 600,
};

enum {
kCondSerialSector                   = 100000,
kCondSerialWall                     = 200000,
kCondSerialSprite                   = 300000,
kCondSerialMax                      = 400000,
};

// - STRUCTS ------------------------------------------------------------------
struct SPRITEMASS { // sprite mass info for getSpriteMassBySize();
    int seqId;
    short picnum; // mainly needs for moving debris
    short xrepeat;
    short yrepeat;
    short clipdist; // mass multiplier
    int mass;
    short airVel; // mainly needs for moving debris
    int fraction; // mainly needs for moving debris
};

struct QAVSCENE { // this one stores qavs anims that can be played by trigger
    short index = -1;  // index of sprite which triggered qav scene
    QAV* qavResrc = NULL;
    short dummy = -1;
};

struct THINGINFO_EXTRA {
    bool allowThrow; // indicates if kDudeModernCustom can throw it
};

struct VECTORINFO_EXTRA {
    int fireSound[2]; // predefined fire sounds. used by kDudeModernCustom, but can be used for something else.
};

struct MISSILEINFO_EXTRA {
    int fireSound[2]; // predefined fire sounds. used by kDudeModernCustom, but can be used for something else.
    bool dmgType[kDamageMax]; // list of damages types missile can use
    bool allowImpact; // allow to trigger object with Impact flag enabled with this missile
};

struct DUDEINFO_EXTRA {
    bool flying;    // used by kModernDudeTargetChanger (ai fight)
    bool melee;     // used by kModernDudeTargetChanger (ai fight)
    bool annoying;  // used by kModernDudeTargetChanger (ai fight)
};

struct TRPLAYERCTRL { // this one for controlling the player using triggers (movement speed, jumps and other stuff)
    QAVSCENE qavScene;
};

struct OBJECTS_TO_TRACK {
    signed int type:     3;
    unsigned int index:  16;
    unsigned int cmd:    8;
};

struct TRCONDITION {
    signed   int xindex:    16;
    unsigned int length:    8;
    OBJECTS_TO_TRACK obj[kMaxTracedObjects];
};



// - VARIABLES ------------------------------------------------------------------
extern bool gModernMap;
extern bool gTeamsSpawnUsed;
extern bool gEventRedirectsUsed;
extern ZONE gStartZoneTeam1[kMaxPlayers];
extern ZONE gStartZoneTeam2[kMaxPlayers];
extern THINGINFO_EXTRA gThingInfoExtra[kThingMax];
extern VECTORINFO_EXTRA gVectorInfoExtra[kVectorMax];
extern MISSILEINFO_EXTRA gMissileInfoExtra[kMissileMax];
extern DUDEINFO_EXTRA gDudeInfoExtra[kDudeMax];
extern TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];
extern SPRITEMASS gSpriteMass[kMaxXSprites];
extern TRCONDITION gCondition[kMaxTrackingConditions];
extern short gProxySpritesList[kMaxSuperXSprites];
extern short gSightSpritesList[kMaxSuperXSprites];
extern short gPhysSpritesList[kMaxSuperXSprites];
extern short gImpactSpritesList[kMaxSuperXSprites];
extern short gProxySpritesCount;
extern short gSightSpritesCount;
extern short gPhysSpritesCount;
extern short gImpactSpritesCount;
extern short gTrackingCondsCount;

// - FUNCTIONS ------------------------------------------------------------------
bool nnExtEraseModernStuff(spritetype* pSprite, XSPRITE* pXSprite);
void nnExtInitModernStuff(bool bSaveLoad);
void nnExtProcessSuperSprites(void);
bool nnExtIsImmune(spritetype* pSprite, int dmgType, int minScale = 16);
int nnExtRandom(int a, int b);
void nnExtResetGlobals();
void nnExtTriggerObject(int objType, int objIndex, int command);
//  -------------------------------------------------------------------------   //
spritetype* randomDropPickupObject(spritetype* pSprite, short prevItem);
spritetype* randomSpawnDude(spritetype* pSprite);
int GetDataVal(spritetype* pSprite, int data);
int randomGetDataValue(XSPRITE* pXSprite, int randType);
void sfxPlayMissileSound(spritetype* pSprite, int missileId);
void sfxPlayVectorSound(spritetype* pSprite, int vectorId);
//  -------------------------------------------------------------------------   //
int debrisGetIndex(int nSprite);
int debrisGetFreeIndex(void);
void debrisMove(int listIndex);
void debrisConcuss(int nOwner, int listIndex, int x, int y, int z, int dmg);
//  -------------------------------------------------------------------------   //
void aiSetGenIdleState(spritetype* pSprite, XSPRITE* pXSprite);

// triggers related
//  -------------------------------------------------------------------------   //
int aiFightGetTargetDist(spritetype* pSprite, DUDEINFO* pDudeInfo, spritetype* pTarget);
int aiFightGetFineTargetDist(spritetype* pSprite, spritetype* pTarget);
bool aiFightDudeCanSeeTarget(XSPRITE* pXDude, DUDEINFO* pDudeInfo, spritetype* pTarget);
bool aiFightUnitCanFly(spritetype* pDude);
bool aiFightIsMeleeUnit(spritetype* pDude);
bool aiFightDudeIsAffected(XSPRITE* pXDude);
bool aiFightMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow);
bool aiFightGetDudesForBattle(XSPRITE* pXSprite);
bool aiFightIsMateOf(XSPRITE* pXDude, XSPRITE* pXSprite);
void aiFightAlarmDudesInSight(spritetype* pSprite, int max);
void aiFightActivateDudes(int rx);
void aiFightFreeTargets(int nSprite);
void aiFightFreeAllTargets(XSPRITE* pXSource);
spritetype* aiFightGetTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode);
spritetype* aiFightTargetIsPlayer(XSPRITE* pXSprite);
spritetype* aiFightGetMateTargets(XSPRITE* pXSprite);
//  -------------------------------------------------------------------------   //
void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector);
void useEffectGen(XSPRITE* pXSource, spritetype* pSprite);
void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index);
void useSpriteDamager(XSPRITE* pXSource, spritetype* pSprite);
void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite);
void useObjResizer(XSPRITE* pXSource, short objType, int objIndex);
void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource);
void useUniMissileGen(int, int nXSprite);
void useSoundGen(spritetype* pSource, XSPRITE* pXSource);
void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex);
void useDataChanger(XSPRITE* pXSource, int objType, int objIndex);
void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector);
void useTargetChanger(XSPRITE* pXSource, spritetype* pSprite);
void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex);
void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex);
void useSequentialTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
void useRandomTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
bool txIsRanged(XSPRITE* pXSource);
void seqTxSendCmdAll(XSPRITE* pXSource, int nIndex, COMMAND_ID cmd, bool modernSend);
//  -------------------------------------------------------------------------   //
void trPlayerCtrlLink(XSPRITE* pXSource, PLAYER* pPlayer, bool checkCondition);
void trPlayerCtrlSetRace(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlStartScene(XSPRITE* pXSource, PLAYER* pPlayer, bool force);
void trPlayerCtrlStopScene(PLAYER* pPlayer);
void trPlayerCtrlSetMoveSpeed(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetJumpHeight(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetScreenEffect(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetLookAngle(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlEraseStuff(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlGiveStuff(XSPRITE* pXSource, PLAYER* pPlayer, TRPLAYERCTRL* pCtrl);
void trPlayerCtrlUsePackItem(XSPRITE* pXSource, PLAYER* pPlayer, int evCmd);
//  -------------------------------------------------------------------------   //
void modernTypeTrigger(int type, int nDest, EVENT event);
char modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event);
bool modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event);
void modernTypeSendCommand(int nSprite, int channel, COMMAND_ID command);
//  -------------------------------------------------------------------------   //
bool playerSizeShrink(PLAYER* pPlayer, int divider);
bool playerSizeGrow(PLAYER* pPlayer, int multiplier);
bool playerSizeReset(PLAYER* pPlayer);
void playerDeactivateShrooms(PLAYER* pPlayer);
//  -------------------------------------------------------------------------   //
QAV* playerQavSceneLoad(int qavId);
void playerQavSceneProcess(PLAYER* pPlayer, QAVSCENE* pQavScene);
void playerQavScenePlay(PLAYER* pPlayer);
void playerQavSceneDraw(PLAYER* pPlayer, int a2, double a3, double a4, int a5, int basepal);
void playerQavSceneReset(PLAYER* pPlayer);
//  -------------------------------------------------------------------------   //
void callbackUniMissileBurst(int nSprite);
void callbackMakeMissileBlocking(int nSprite);
void callbackGenDudeUpdate(int nSprite);
//  -------------------------------------------------------------------------   //
PLAYER* getPlayerById(short id);
bool isGrown(spritetype* pSprite);
bool isShrinked(spritetype* pSprite);
bool valueIsBetween(int val, int min, int max);
bool IsBurningDude(spritetype* pSprite);
bool IsKillableDude(spritetype* pSprite);
bool isActive(int nSprite);
int getDataFieldOfObject(int objType, int objIndex, int dataIndex);
bool setDataValueOfObject(int objType, int objIndex, int dataIndex, int value);
bool incDecGoalValueIsReached(XSPRITE* pXSprite);
void windGenStopWindOnSectors(XSPRITE* pXSource);
int getSpriteMassBySize(spritetype* pSprite);
bool ceilIsTooLow(spritetype* pSprite);
void levelEndLevelCustom(int nLevel);
int useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT event);
bool condPush(XSPRITE* pXSprite, int objType, int objIndex);
bool condRestore(XSPRITE* pXSprite);
bool condCmp(int val, int arg1, int arg2, int comOp);
bool condCmpne(int arg1, int arg2, int comOp);
void condError(XSPRITE* pXCond, const char* pzFormat, ...);
bool condCheckMixed(XSPRITE* pXCond, EVENT event, int cmpOp, bool PUSH);
bool condCheckSector(XSPRITE* pXCond, int cmpOp, bool PUSH);
bool condCheckWall(XSPRITE* pXCond, int cmpOp, bool PUSH);
bool condCheckSprite(XSPRITE* pXCond, int cmpOp, bool PUSH);
bool condCheckPlayer(XSPRITE* pXCond, int cmpOp, bool PUSH);
bool condCheckDude(XSPRITE* pXCond, int cmpOp, bool PUSH);
void condUpdateObjectIndex(int objType, int oldIndex, int newIndex);
XSPRITE* evrListRedirectors(int objType, int objXIndex, XSPRITE* pXRedir, int* tx);
XSPRITE* evrIsRedirector(int nSprite);
int listTx(XSPRITE* pXRedir, int tx);
void seqSpawnerOffSameTx(XSPRITE* pXSource);
#endif

////////////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
////////////////////////////////////////////////////////////////////////////////////
END_BLD_NS
