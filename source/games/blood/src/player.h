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
#include "actor.h"
#include "build.h"
#include "common_game.h"
#include "globals.h"
#include "db.h"
#include "dude.h"
#include "levels.h"
#include "qav.h"
#include "coreplayer.h"

BEGIN_BLD_NS

// life modes of the player
enum
{
	kModeHuman = 0,
	kModeBeast = 1,
	kModeHumanShrink = 2,
	kModeHumanGrown = 3,
	kModeMax = 4,
};

// postures
enum
{
	kPostureStand = 0,
	kPostureSwim = 1,
	kPostureCrouch = 2,
	kPostureMax = 3,
};

struct PACKINFO
{
	bool isActive; // is active (0/1)
	int curAmount = 0; // remaining percent
};

struct POSTURE
{
	double frontAccel;
	double sideAccel;
	double backAccel;
	int pace[2];
	double bobV;
	double bobH;
	double swayV;
	double swayH;
	double eyeAboveZ;
	double weaponAboveZ;
	double xOffset;
	double zOffset;
	double normalJumpZ;
	double pwupJumpZ;
};

extern POSTURE gPostureDefaults[kModeMax][kPostureMax];

struct BloodPlayer final : public CorePlayer
{
	DBloodActor* actor;
	DUDEINFO* pDudeInfo;
	uint8_t             newWeapon;
	int                 weaponQav;
	int                 qavCallback;
	bool                isRunning;
	int                 posture;   // stand, crouch, swim
	int                 sceneQav;  // by NoOne: used to keep qav id
	double              bobPhase;
	int                 bobAmp;
	double              bobHeight;
	double              bobWidth;
	double              obobHeight;
	double              obobWidth;
	int                 swayAmp;
	double              swayHeight;
	double              swayWidth;
	double              oswayHeight;
	double              oswayWidth;
	int                 nPlayer;  // Connect id
	int                 lifeMode;
	double              zView;
	double              ozView;
	double              zViewVel;
	double              zWeapon;
	double              ozWeapon;
	double              zWeaponVel;
	double              slope;
	bool                isUnderwater;
	bool                hasKey[8];
	int8_t              hasFlag;
	TObjPtr<DBloodActor*>        ctfFlagState[2];
	int                 damageControl[7];
	int8_t              curWeapon;
	int8_t              nextWeapon;
	int                 weaponTimer;
	int                 weaponState;
	int                 weaponAmmo;  //rename
	bool                hasWeapon[kWeapMax];
	int                 weaponMode[kWeapMax];
	int                 weaponOrder[2][kWeapMax];
	//int               at149[14];
	int                 ammoCount[12];
	bool                qavLoop;
	int                 qavLastTick;
	int                 qavTimer;
	int                 fuseTime;
	int                 throwTime;
	double              throwPower;
	DVector3            aim;  // world
	DVector3            relAim;  // relative
	TObjPtr<DBloodActor*>        aimTarget;  // aim target sprite
	int                 aimTargetsCount;
	TObjPtr<DBloodActor*>        aimTargets[16];
	int                 deathTime;
	int                 pwUpTime[kMaxPowerUps];
	int                 fragCount;
	int                 fragInfo[8];
	int                 teamId;
	TObjPtr<DBloodActor*>        fragger;
	int                 underwaterTime;
	int                 bubbleTime;
	int                 restTime;
	int                 kickPower;
	int                 laughCount;
	bool                godMode;
	bool                fallScream;
	bool                cantJump;
	int                 packItemTime;  // pack timer
	int                 packItemId;    // pack id 1: diving suit, 2: crystal ball, 3: beast vision 4: jump boots
	PACKINFO            packSlots[5];  // at325 [1]: diving suit, [2]: crystal ball, [3]: beast vision [4]: jump boots
	int                 armor[3];      // armor
	//int               at342;
	//int               at346;
	TObjPtr<DBloodActor*>        voodooTarget;
	int                 voodooTargets;  // --> useless
	int                 voodooVar1;     // --> useless
	int                 vodooVar2;      // --> useless
	int                 flickerEffect;
	int                 tiltEffect;
	int                 visibility;
	int                 painEffect;
	int                 blindEffect;
	int                 chokeEffect;
	int                 handTime;
	bool                hand;  // if true, there is hand start choking the player
	int                 pickupEffect;
	bool                flashEffect;  // if true, reduce pPlayer->visibility counter
	int                 quakeEffect;
	int                 player_par;
	int                 nWaterPal;
	POSTURE             pPosture[kModeMax][kPostureMax];

	inline DBloodActor* GetActor()
	{
		return actor;
	}
};

struct AMMOINFO
{
	int max;
	int8_t vectorType;
};

struct POWERUPINFO
{
	int16_t picno;
	bool pickupOnce;
	int bonusTime;
	int maxTime;
	FTextureID textureID() const { return tileGetTextureID(picno); }
};

void playerResetPosture(BloodPlayer* pPlayer);

extern BloodPlayer PlayerArray[kMaxPlayers];

inline BloodPlayer* getPlayer(int index)
{
	return &PlayerArray[index];
}

extern bool gBlueFlagDropped;
extern bool gRedFlagDropped;

extern int team_score[kMaxPlayers];
extern int team_ticker[kMaxPlayers];
extern AMMOINFO gAmmoInfo[];
extern POWERUPINFO gPowerUpInfo[kMaxPowerUps];

bool IsTargetTeammate(BloodPlayer* pSourcePlayer, DBloodActor* target);
int powerupCheck(BloodPlayer* pPlayer, int nPowerUp);
bool powerupActivate(BloodPlayer* pPlayer, int nPowerUp);
void powerupDeactivate(BloodPlayer* pPlayer, int nPowerUp);
void powerupSetState(BloodPlayer* pPlayer, int nPowerUp, bool bState);
void powerupProcess(BloodPlayer* pPlayer);
void powerupClear(BloodPlayer* pPlayer);
int packItemToPowerup(int nPack);
int powerupToPackItem(int nPowerUp);
bool packAddItem(BloodPlayer* pPlayer, unsigned int nPack);
int packCheckItem(BloodPlayer* pPlayer, int nPack);
bool packItemActive(BloodPlayer* pPlayer, int nPack);
void packUseItem(BloodPlayer* pPlayer, int nPack);
void packPrevItem(BloodPlayer* pPlayer);
void packNextItem(BloodPlayer* pPlayer);
bool playerSeqPlaying(BloodPlayer* pPlayer, int nSeq);
void playerSetRace(BloodPlayer* pPlayer, int nLifeMode);
void playerSetGodMode(BloodPlayer* pPlayer, bool bGodMode);
void playerResetInertia(BloodPlayer* pPlayer);
void playerCorrectInertia(BloodPlayer* pPlayer, const DVector3& oldpos);
void playerStart(int nPlayer, int bNewLevel = 0);
void playerReset(BloodPlayer* pPlayer);
void playerInit(int nPlayer, unsigned int a2);
void CheckPickUp(BloodPlayer* pPlayer);
void ProcessInput(BloodPlayer* pPlayer);
void playerProcess(BloodPlayer* pPlayer);
DBloodActor* playerFireMissile(BloodPlayer* pPlayer, double xyoff, const DVector3& vec, int nType);
DBloodActor* playerFireThing(BloodPlayer* pPlayer, double xyoff, double zvel, int thingType, double nSpeed);
void playerFrag(BloodPlayer* pKiller, BloodPlayer* pVictim);
int playerDamageArmor(BloodPlayer* pPlayer, DAMAGE_TYPE nType, int nDamage);
int playerDamageSprite(DBloodActor* nSource, BloodPlayer* pPlayer, DAMAGE_TYPE nDamageType, int nDamage);
int UseAmmo(BloodPlayer* pPlayer, int nAmmoType, int nDec);
void voodooTarget(BloodPlayer* pPlayer);
void playerLandingSound(BloodPlayer* pPlayer);
void PlayerSurvive(int, DBloodActor*);

END_BLD_NS
