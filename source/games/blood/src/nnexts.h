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
	kModernTypeFlag32 = 0x0020,
	kModernTypeFlag64 = 0x0040,
	kModernTypeFlag128 = 0x0080,
	kModernTypeFlag256 = 0x0100,

	kMaxRandomizeRetries = 16,
	kCondRange = 100,
};

constexpr int kPercFull = 100;


enum {
	kListEndDefault     = -1,

	kListSKIP = 0,
	kListOK = 1,
	kListREMOVE = 2,
};
enum
{
	kPatrolStateSize = 50,
	kPatrolAlarmHearDist = 10000,
	kMaxPatrolSpotValue = 500,
	kMinPatrolTurnDelay = 8,
	kPatrolTurnDelayRange = 20,

	kDudeFlagStealth = 0x0001,
	kDudeFlagCrouch = 0x0002,

	kEffectGenCallbackBase = 200,
	kTriggerSpriteScreen = 0x0001,
	kTriggerSpriteAim = 0x0002,

	kMinAllowedPowerup = kPwUpFeatherFall,
	kMaxAllowedPowerup = kMaxPowerUps
};

constexpr double kPatrolAlarmSeeDistSq = 625 * 625;
constexpr double kSlopeDist = 0x20;
constexpr double kMaxPatrolVelocity = FixedToFloat(500000); // ~7.63
constexpr double kMaxPatrolCrouchVelocity = (kMaxPatrolVelocity / 2);


// modern statnums
enum {
	kStatModernBase = 20,
	kStatModernDudeTargetChanger = kStatModernBase,
	kStatModernCondition = 21,
	kStatModernEventRedirector = 22,
	kStatModernPlayerLinker = 23,
	kStatModernBrokenDudeLeech = 24,
	kStatModernQavScene = 25,
	kStatModernWindGen = 26,
	kStatModernStealthRegion = 27,
	kStatModernTmp = 39,
	kStatModernMax = 40,
};

// modern sprite types
enum {
	kModernStealthRegion = 16,
	kModernCustomDudeSpawn = 24,
	kModernRandomTX = 25,
	kModernSequentialTX = 26,
	kModernSeqSpawner = 27,
	kModernObjPropertiesChanger = 28,
	kModernObjPicnumChanger = 29,
	kModernObjSizeChanger = 31,
	kModernDudeTargetChanger = 33,
	kModernSectorFXChanger = 34,
	kModernObjDataChanger = 35,
	kModernSpriteDamager = 36,
	kModernObjDataAccumulator = 37,
	kModernEffectSpawner = 38,
	kModernWindGenerator = 39,
	kModernRandom = 40,
	kModernRandom2 = 80,
	kItemModernMapLevel = 150,  // once picked up, draws whole minimap
	kDudeModernCustom = kDudeVanillaMax,
	kModernThingTNTProx = 433, // detects only players
	kModernThingThrowableRock = 434, // does small damage if hits target
	kModernThingEnemyLifeLeech = 435, // the same as normal, except it aims in specified target only
	kModernPlayerControl = 500, /// WIP
	kModernCondition = 501, /// WIP, sends command only if specified conditions == true
	kModernConditionFalse = 502, /// WIP, sends command only if specified conditions != true
	kModernSlopeChanger = 504,
	kModernVelocityChanger = 506,
	kGenModernMissileUniversal = 704,
	kGenModernSound = 708,
};

// type of random
enum {
	kRandomizeItem = 0,
	kRandomizeDude = 1,
	kRandomizeTX = 2,
};

// type of object
enum {
	OBJ_WALL = 0,
	OBJ_SPRITE = 3,
	OBJ_SECTOR = 6,
};

enum {
	kPatrolMoveForward = 0,
	kPatrolMoveBackward = 1,
};

// - STRUCTS ------------------------------------------------------------------

struct SPRITEMASS { // sprite mass info for getSpriteMassBySize();
	int seqId;
	FTextureID texid; // mainly needs for moving debris
	DVector2 scale;
	int16_t airVel; // mainly needs for moving debris
	double clipDist; // mass multiplier
	int mass;
	int fraction; // mainly needs for moving debris
};

struct EXPLOSION_EXTRA
{
	uint8_t seq;
	uint16_t snd;
	bool ground;
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

struct PATROL_FOUND_SOUNDS {

	FSoundID snd;
	int max;
	int cur;

};

struct QAVSCENE { // this one stores qavs anims that can be played by trigger
	DBloodActor* initiator = nullptr;  // index of sprite which triggered qav scene
	QAV* qavResrc = nullptr;
	short dummy = -1;
};
struct TRPLAYERCTRL { // this one for controlling the player using triggers (movement speed, jumps and other stuff)
	QAVSCENE qavScene;
};

inline bool rngok(int val, int rngA, int rngB) { return (val >= rngA && val < rngB); }
inline bool irngok(int val, int rngA, int rngB) { return (val >= rngA && val <= rngB); }
extern uint8_t gModernMap;
inline bool mapRev1() { return (gModernMap == 1); }
inline bool mapRev2() { return (gModernMap == 2); }

// - VARIABLES ------------------------------------------------------------------
extern bool gTeamsSpawnUsed;
extern bool gEventRedirectsUsed;
extern ZONE gStartZoneTeam1[kMaxPlayers];
extern ZONE gStartZoneTeam2[kMaxPlayers];
extern const THINGINFO_EXTRA gThingInfoExtra[kThingMax];
extern const VECTORINFO_EXTRA gVectorInfoExtra[kVectorMax];
extern const MISSILEINFO_EXTRA gMissileInfoExtra[kMissileMax];
extern const EXPLOSION_EXTRA gExplodeExtra[kExplosionMax];
extern const DUDEINFO_EXTRA gDudeInfoExtra[kDudeMax];
extern TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];
extern AISTATE genPatrolStates[kPatrolStateSize];

inline TArray<TObjPtr<DBloodActor*>> gProxySpritesList;
inline TArray<TObjPtr<DBloodActor*>> gSightSpritesList;
inline TArray<TObjPtr<DBloodActor*>> gPhysSpritesList;
inline TArray<TObjPtr<DBloodActor*>> gFlwSpritesList;
inline TArray<TObjPtr<DBloodActor*>> gImpactSpritesList;

// - FUNCTIONS ------------------------------------------------------------------
bool xsprIsFine(DBloodActor* pSpr);
bool nnExtEraseModernStuff(DBloodActor* actor);
void nnExtInitModernStuff(TArray<DBloodActor*>& actors);
void nnExtProcessSuperSprites(void);
bool nnExtIsImmune(DBloodActor* pSprite, int dmgType, int minScale = 16);
int nnExtRandom(int a, int b);
void nnExtResetGlobals();
void nnExtTriggerObject(EventObject& eob, int command, DBloodActor* initiator);
//  -------------------------------------------------------------------------   //
DBloodActor* randomSpawnDude(DBloodActor* sourceactor, DBloodActor* origin, double dist, double zadd);
void sfxPlayMissileSound(DBloodActor* pSprite, int missileId);
void sfxPlayVectorSound(DBloodActor* pSprite, int vectorId);
//  -------------------------------------------------------------------------   //
void debrisBubble(DBloodActor* nSprite);
void debrisMove(DBloodActor* act);
void debrisConcuss(DBloodActor* nOwner, int listIndex, const DVector3& pos, int dmg);
//  -------------------------------------------------------------------------   //
void aiSetGenIdleState(DBloodActor*);

// triggers related
//  -------------------------------------------------------------------------   //
int aiFightGetTargetDist(DBloodActor* pSprite, DUDEINFO* pDudeInfo, DBloodActor* pTarget);
double aiFightGetFineTargetDist(DBloodActor* actor, DBloodActor* target);
bool aiFightDudeCanSeeTarget(DBloodActor* pXDude, DUDEINFO* pDudeInfo, DBloodActor* pTarget);
bool aiFightDudeIsAffected(DBloodActor* pXDude);
bool aiFightMatesHaveSameTarget(DBloodActor* leaderactor, DBloodActor* targetactor, int allow);
void aiFightActivateDudes(int rx);
//  -------------------------------------------------------------------------   //
void useSlopeChanger(DBloodActor* sourceactor, int objType, sectortype* pSect, DBloodActor* objActor);
void damageSprites(DBloodActor* pXSource, DBloodActor* pSprite);
void useRandomItemGen(DBloodActor* pSource);
void useUniMissileGen(DBloodActor* sourceactor, DBloodActor* actor);
void useSoundGen(DBloodActor* sourceactor, DBloodActor* actor);
void useIncDecGen(DBloodActor* sourceactor, int objType, sectortype* destSect, walltype* destWall, DBloodActor* objactor);
void useDataChanger(DBloodActor* sourceactor, int objType, sectortype* pSector, walltype* pWall, DBloodActor* objActor);
void useSectorLightChanger(DBloodActor* pXSource, sectortype* pSector);
void useTargetChanger(DBloodActor* sourceactor, DBloodActor* actor);
void usePictureChanger(DBloodActor* sourceactor, int objType, sectortype*, walltype*, DBloodActor* objActor);
void useSequentialTx(DBloodActor* pXSource, COMMAND_ID cmd, bool setState, DBloodActor* initiator);
void useRandomTx(DBloodActor* sourceactor, COMMAND_ID cmd, bool setState, DBloodActor* initiator);
void useDudeSpawn(DBloodActor* pXSource, DBloodActor* pSprite);
void useCustomDudeSpawn(DBloodActor* pXSource, DBloodActor* pSprite);
void useVelocityChanger(DBloodActor* pXSource, sectortype* sect, DBloodActor* causerID, DBloodActor* pSprite);
void seqTxSendCmdAll(DBloodActor* pXSource, DBloodActor* nIndex, COMMAND_ID cmd, bool modernSend, DBloodActor* initiator);
//  -------------------------------------------------------------------------   //
void trPlayerCtrlLink(DBloodActor* pXSource, DBloodPlayer* pPlayer, bool checkCondition);
void trPlayerCtrlStopScene(DBloodPlayer* pPlayer);
//  -------------------------------------------------------------------------   //
void modernTypeTrigger(int type, sectortype* sect, walltype* wal, DBloodActor* actor, EVENT& event);
bool modernTypeOperateSector(sectortype* pSector, const EVENT& event);
bool modernTypeOperateSprite(DBloodActor*, EVENT& event);
bool modernTypeOperateWall(walltype* pWall, const EVENT& event);
void modernTypeSendCommand(DBloodActor* nSprite, int channel, COMMAND_ID command, DBloodActor* initiator);
//  -------------------------------------------------------------------------   //
QAV* playerQavSceneLoad(int qavId);
void playerQavSceneProcess(DBloodPlayer* pPlayer, QAVSCENE* pQavScene);
void playerQavScenePlay(DBloodPlayer* pPlayer);
void playerQavSceneDraw(DBloodPlayer* pPlayer, int shade, double xpos, double ypos, int palnum, DAngle angle);
void playerQavSceneReset(DBloodPlayer* pPlayer);
//  -------------------------------------------------------------------------   //
void callbackMissileBurst(DBloodActor* actor);
void callbackMakeMissileBlocking(DBloodActor* actor);
void callbackGenDudeUpdate(DBloodActor* actor);
//  -------------------------------------------------------------------------   //
DBloodPlayer* getPlayerById(int id);
bool IsBurningDude(DBloodActor* pSprite);
bool IsKillableDude(DBloodActor* pSprite);
bool isActive(DBloodActor* nSprite);
int getDataFieldOfObject(EventObject& eob, int dataIndex);
int getDataFieldOfObject(int objType, sectortype* sect, walltype* wal, DBloodActor* actor, int dataIndex);
bool setDataValueOfObject(int objType, sectortype* sect, walltype* wal, DBloodActor* objActor, int dataIndex, int value);
bool incDecGoalValueIsReached(DBloodActor* actor);
int getSpriteMassBySize(DBloodActor* pSprite);
bool ceilIsTooLow(DBloodActor* pSprite);
void levelEndLevelCustom(int nLevel);
DBloodActor* evrListRedirectors(int objType, sectortype*, walltype*, DBloodActor* objActor, DBloodActor* pXRedir, int* tx);
void seqSpawnerOffSameTx(DBloodActor* actor);
void triggerTouchSprite(DBloodActor* pSprite, DBloodActor* nHSprite);
void triggerTouchWall(DBloodActor* pSprite, walltype* nHWall);
void killEvents(int nRx, int nCmd);
void changeSpriteAngle(DBloodActor* pSpr, DAngle nAng);
//  -------------------------------------------------------------------------   //
void aiPatrolSetMarker(DBloodActor* actor);
void aiPatrolThink(DBloodActor* actor);
void aiPatrolStop(DBloodActor* actor, DBloodActor* targetactor, bool alarm = false);
void aiPatrolAlarmFull(DBloodActor* actor, DBloodActor* targetactor, bool chain);
void aiPatrolAlarmLite(DBloodActor* actor, DBloodActor* targetactor);
void aiPatrolState(DBloodActor* pSprite, int state);
void aiPatrolMove(DBloodActor* actor);
DBloodActor* aiPatrolMarkerBusy(DBloodActor* except, DBloodActor* marker);
bool aiPatrolMarkerReached(DBloodActor*);
bool aiPatrolGetPathDir(DBloodActor* actor, DBloodActor* marker);
void aiPatrolFlagsMgr(DBloodActor* sourceactor, DBloodActor* destactor, bool copy, bool init);
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
bool readyForCrit(DBloodActor* pHunter, DBloodActor* pVictim);
void clampSprite(DBloodActor* actor, int which = 3);
int getSpritesNearWalls(int nSrcSect, int* spriOut, int nMax, int nDist);
bool isMovableSector(int nType);
bool isMovableSector(sectortype* pSect);
void killEffectGenCallbacks(DBloodActor* actor);
bool isOnRespawn(DBloodActor* pSpr);
void nnExtOffsetPos(const DVector3& opos, DAngle nAng, DVector3& pos);
void actPropagateSpriteOwner(DBloodActor* pShot, DBloodActor* pSpr);
int nnExtDudeStartHealth(DBloodActor* pSpr, int nHealth);
void nnExtSprScaleSet(DBloodActor* actor, int nScale);
struct Seq;
bool seqCanOverride(Seq* pSeq, int nFrame, bool* xrp, bool* yrp, bool* plu);
inline unsigned int perc2val(unsigned int reqPerc, unsigned int val) { return (val * reqPerc) / 100; }
inline int perc2val(int reqPerc, int val) { return (val * reqPerc) / 100; }
inline double perc2val(int reqPerc, double val) { return (val * reqPerc) / 100; }
void nnExtScaleVelocity(DBloodActor* pSpr, double nVel, const DVector3& dv, int which = 0x03);
void nnExtScaleVelocityRel(DBloodActor* pSpr, double nVel, const DVector3& dv, int which = 0x03);
enum GIBTYPE;
int nnExtGibSprite(DBloodActor* pSpr, TArray<DBloodActor*>& pOut, GIBTYPE nGibType, DVector3* pPos, DVector3* pVel);
void useGibObject(DBloodActor* pXSource, DBloodActor* pSpr);
void useDripGenerator(DBloodActor* pXSource, DBloodActor* pSprite);
int nnExtGetStartHealth(DBloodActor* actor);
int nnExtResAddExternalFiles();


inline bool valueIsBetween(int val, int min, int max)
{
	return (val > min && val < max);
}


inline int EVTIME2TICKS(int x) { return ((x * 120) / 10); }

////////////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
////////////////////////////////////////////////////////////////////////////////////
END_BLD_NS

#endif
