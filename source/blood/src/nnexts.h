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
#include "warp.h"

BEGIN_BLD_NS

// CONSTANTS
// additional non-thing proximity, sight and physics sprites 
#define kMaxSuperXSprites 128

// additional physics attributes for debris sprites
#define kPhysDebrisFly 0x0008 // *debris* affected by negative gravity (fly instead of falling, DO NOT mess with kHitagAutoAim)
#define kPhysDebrisSwim 0x0016 // *debris* can swim underwater (instead of drowning)
#define kPhysDebrisVector 0x0400 // *debris* can be affected by vector weapons
#define kPhysDebrisExplode 0x0800 // *debris* can be affected by explosions

// *modern types only hitag*
#define kModernTypeFlag0 0x0
#define kModernTypeFlag1 0x1
#define kModernTypeFlag2 0x2
#define kModernTypeFlag3 0x3

#define kMaxRandomizeRetries 16

// modern sprite types
enum {
kStatModernDudeTargetChanger        = 20,
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
};

struct TRPLAYERCTRL { // this one for controlling the player using triggers (movement speed, jumps and other stuff)
    QAVSCENE qavScene;
};

// - VARIABLES ------------------------------------------------------------------
extern bool gModernMap;
extern bool gTeamsSpawnUsed;
extern ZONE gStartZoneTeam1[kMaxPlayers];
extern ZONE gStartZoneTeam2[kMaxPlayers];
extern THINGINFO_EXTRA gThingInfoExtra[kThingMax];
extern VECTORINFO_EXTRA gVectorInfoExtra[kVectorMax];
extern MISSILEINFO_EXTRA gMissileInfoExtra[kMissileMax];
extern TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];
extern SPRITEMASS gSpriteMass[kMaxXSprites];
extern short gProxySpritesList[kMaxSuperXSprites];
extern short gSightSpritesList[kMaxSuperXSprites];
extern short gPhysSpritesList[kMaxSuperXSprites];
extern short gProxySpritesCount;
extern short gSightSpritesCount;
extern short gPhysSpritesCount;

// - FUNCTIONS ------------------------------------------------------------------
bool nnExtEraseModernStuff(spritetype* pSprite, XSPRITE* pXSprite);
void nnExtInitModernStuff(bool bSaveLoad);
void nnExtProcessSuperSprites(void);
bool nnExtIsImmune(spritetype* pSprite, int dmgType, int minScale = 16);
int nnExtRandom(int a, int b);
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
bool aiFightIsAnnoyingUnit(spritetype* pDude);
bool aiFightUnitCanFly(spritetype* pDude);
bool aiFightIsMeleeUnit(spritetype* pDude);
bool aiFightDudeIsAffected(XSPRITE* pXDude);
bool aiFightMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow);
bool aiFightGetDudesForBattle(XSPRITE* pXSprite);
inline bool aiFightIsMateOf(XSPRITE* pXDude, XSPRITE* pXSprite) {
    return (pXDude->rxID == pXSprite->rxID);
}
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
//  -------------------------------------------------------------------------   //
void trPlayerCtrlLink(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetRace(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlStartScene(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlStopScene(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetMoveSpeed(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetJumpHeight(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetScreenEffect(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlSetLookAngle(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlEraseStuff(XSPRITE* pXSource, PLAYER* pPlayer);
void trPlayerCtrlGiveStuff(XSPRITE* pXSource, PLAYER* pPlayer, TRPLAYERCTRL* pCtrl);
void trPlayerCtrlUsePackItem(XSPRITE* pXSource, PLAYER* pPlayer);
//  -------------------------------------------------------------------------   //
void modernTypeTrigger(int type, int nDest, EVENT event);
char modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState);
bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event);
bool modernTypeLinkSprite(spritetype* pSprite, XSPRITE* pXSprite, EVENT event);
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
void playerQavSceneDraw(PLAYER* pPlayer, int a2, int a3, int a4, int a5);
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
#endif

////////////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
////////////////////////////////////////////////////////////////////////////////////
END_BLD_NS
