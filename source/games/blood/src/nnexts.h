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

enum
{
    // CONSTANTS
    // additional non-thing proximity, sight and physics sprites 
    kMaxSuperXSprites = 512,
    kMaxTrackingConditions = 64,
    kMaxTracedObjects = 32, // per one tracking condition

    // additional physics attributes for debris sprites
    kPhysDebrisFloat = 0x0008, // *debris* slowly goes up and down from it's position
    kPhysDebrisFly = 0x0010, // *debris* affected by negative gravity (fly instead of falling)
    kPhysDebrisSwim = 0x0020, // *debris* can swim underwater (instead of drowning)
    kPhysDebrisTouch = 0x0040, // *debris* can be moved via touch
    kPhysDebrisVector = 0x0400, // *debris* can be affected by vector weapons
    kPhysDebrisExplode = 0x0800, // *debris* can be affected by explosions

    // *modern types only hitag*
    kModernTypeFlag0 = 0x0000,
    kModernTypeFlag1 = 0x0001,
    kModernTypeFlag2 = 0x0002,
    kModernTypeFlag3 = 0x0003,
    kModernTypeFlag4 = 0x0004,
    kModernTypeFlag8 = 0x0008,
    kModernTypeFlag16 = 0x0010,

    kMaxRandomizeRetries = 16,
    kPercFull = 100,
    kCondRange = 100,
};

enum
{
    kPatrolStateSize = 42,
    kPatrolAlarmSeeDist = 10000,
    kPatrolAlarmHearDist = 10000,
    kMaxPatrolVelocity = 500000,
    kMaxPatrolCrouchVelocity = (kMaxPatrolVelocity >> 1),
    kMaxPatrolSpotValue = 500,
    kMinPatrolTurnDelay = 8,
    kPatrolTurnDelayRange = 20,

    kDudeFlagStealth    = 0x0001,
    kDudeFlagCrouch     = 0x0002,

    kSlopeDist = 0x20,
    kEffectGenCallbackBase = 200,
    kTriggerSpriteScreen = 0x0001,
    kTriggerSpriteAim    = 0x0002,

    kMinAllowedPowerup = kPwUpFeatherFall,
    kMaxAllowedPowerup = kMaxPowerUps
};

// modern statnums
enum {
kStatModernBase                     = 20,
kStatModernDudeTargetChanger        = kStatModernBase,
kStatModernCondition                = 21,
kStatModernEventRedirector          = 22,
kStatModernPlayerLinker             = 23,
kStatModernBrokenDudeLeech          = 24,
kStatModernQavScene                 = 25,
kStatModernWindGen                  = 26,
kStatModernStealthRegion            = 27,
kStatModernTmp                      = 39,
kStatModernMax                      = 40,
};

// modern sprite types
enum {
kModernStealthRegion                = 16,
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
kModernSlopeChanger                 = 504,
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
kCondGameBase                       = 0,
kCondGameMax                        = 50,
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

enum {
kPatrolMoveForward                  = 0,
kPatrolMoveBackward                 = 1,
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
    int idlgseqofs : 6;             // used for patrol
    int mvegseqofs : 6;             // used for patrol
    int idlwseqofs : 6;             // used for patrol
    int mvewseqofs : 6;             // used for patrol
    int idlcseqofs : 6;             // used for patrol
    int mvecseqofs : 6;             // used for patrol
    
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

struct PATROL_FOUND_SOUNDS {

    int snd;
    int max;
    int cur;

};

struct CONDITION_TYPE_NAMES {

    int rng1;
    int rng2;
    char name[32];

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
extern AISTATE genPatrolStates[kPatrolStateSize];


// - INLINES -------------------------------------------------------------------
inline bool xsprIsFine(spritetype* pSpr) {
    return (pSpr && xspriRangeIsFine(pSpr->extra) && !(pSpr->flags & kHitagFree) && !(pSpr->flags & kHitagRespawn));
}
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
spritetype* randomSpawnDude(XSPRITE* pXSource, spritetype* pSprite, int a3, int a4);
int GetDataVal(spritetype* pSprite, int data);
int randomGetDataValue(XSPRITE* pXSprite, int randType);
void sfxPlayMissileSound(spritetype* pSprite, int missileId);
void sfxPlayVectorSound(spritetype* pSprite, int vectorId);
//  -------------------------------------------------------------------------   //
int debrisGetIndex(int nSprite);
int debrisGetFreeIndex(void);
void debrisBubble(int nSprite);
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
void useSlopeChanger(XSPRITE* pXSource, int objType, int objIndex);
void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector);
void useEffectGen(XSPRITE* pXSource, spritetype* pSprite);
void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index);
void damageSprites(XSPRITE* pXSource, spritetype* pSprite);
void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite);
void useObjResizer(XSPRITE* pXSource, short objType, int objIndex);
void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource);
void useUniMissileGen(XSPRITE* pXSource, spritetype* pSprite);
void useSoundGen(XSPRITE* pXSource, spritetype* pSprite);
void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex);
void useDataChanger(XSPRITE* pXSource, int objType, int objIndex);
void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector);
void useTargetChanger(XSPRITE* pXSource, spritetype* pSprite);
void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex);
void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex);
void useSequentialTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
void useRandomTx(XSPRITE* pXSource, COMMAND_ID cmd, bool setState);
void useDudeSpawn(XSPRITE* pXSource, spritetype* pSprite);
void useCustomDudeSpawn(XSPRITE* pXSource, spritetype* pSprite);
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
bool modernTypeOperateSector(int nSector, sectortype* pSector, XSECTOR* pXSector, EVENT event);
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
void playerQavSceneDraw(PLAYER* pPlayer, int a2, double a3, double a4, int a5, double smoothratio);
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
//  -------------------------------------------------------------------------   //
void aiPatrolSetMarker(spritetype* pSprite, XSPRITE* pXSprite);
void aiPatrolThink(DBloodActor* actor);
void aiPatrolStop(spritetype* pSprite, int target, bool alarm = false);
void aiPatrolAlarmFull(spritetype* pSprite, XSPRITE* pXTarget, bool chain);
void aiPatrolAlarmLite(spritetype* pSprite, XSPRITE* pXTarget);
void aiPatrolState(spritetype* pSprite, int state);
void aiPatrolMove(DBloodActor* actor);
int aiPatrolMarkerBusy(int nExcept, int nMarker);
bool aiPatrolMarkerReached(spritetype* pSprite, XSPRITE* pXSprite);
bool aiPatrolGetPathDir(XSPRITE* pXSprite, XSPRITE* pXMarker);
void aiPatrolFlagsMgr(spritetype* pSource, XSPRITE* pXSource, spritetype* pDest, XSPRITE* pXDest, bool copy, bool init);
void aiPatrolRandGoalAng(DBloodActor* actor);
void aiPatrolTurn(DBloodActor* actor);
inline int aiPatrolGetVelocity(int speed, int value) {
    return (value > 0) ? ClipRange((speed / 3) + (2500 * value), 0, 0x47956) : speed;
}

inline bool aiPatrolWaiting(AISTATE* pAiState) {
    return (pAiState && pAiState->stateType >= kAiStatePatrolWaitL && pAiState->stateType <= kAiStatePatrolWaitW);
}

inline bool aiPatrolMoving(AISTATE* pAiState) {
    return (pAiState && pAiState->stateType >= kAiStatePatrolMoveL && pAiState->stateType <= kAiStatePatrolMoveW);
}

inline bool aiPatrolTurning(AISTATE* pAiState) {
    return (pAiState && pAiState->stateType >= kAiStatePatrolTurnL && pAiState->stateType <= kAiStatePatrolTurnW);
}

inline bool aiInPatrolState(AISTATE* pAiState) {
    return (pAiState && pAiState->stateType >= kAiStatePatrolBase && pAiState->stateType < kAiStatePatrolMax);
}

inline bool aiInPatrolState(int nAiStateType) {
    return (nAiStateType >= kAiStatePatrolBase && nAiStateType < kAiStatePatrolMax);
}
//  -------------------------------------------------------------------------   //
bool readyForCrit(spritetype* pHunter, spritetype* pVictim);
int sectorInMotion(int nSector);
void clampSprite(spritetype* pSprite, int which = 0x03);
#endif

////////////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
////////////////////////////////////////////////////////////////////////////////////
END_BLD_NS

