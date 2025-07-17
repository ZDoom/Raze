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


///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////
#include "ns.h"

#ifdef NOONE_EXTENSIONS
#include <random>
#include "blood.h"
#include "savegamehelp.h"

BEGIN_BLD_NS

bool gAllowTrueRandom = false;
bool gEventRedirectsUsed = false;

CVARD(Bool, nnext_showconditionsprites, false, 0, "makes kModernCondition sprites visable")




short gEffectGenCallbacks[] = {

	kCallbackFXFlameLick,
	kCallbackFXFlareSpark,
	kCallbackFXFlareSparkLite,
	kCallbackFXZombieSpurt,
	kCallbackFXBloodSpurt,
	kCallbackFXArcSpark,
	kCallbackFXTeslaAlt,

};


TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];

TArray<TRCONDITION> gConditions;

std::default_random_engine gStdRandom;

const VECTORINFO_EXTRA gVectorInfoExtra[] = {
	1207,1207,      1001,1001,      4001,4002,
	431,431,        1002,1002,      359,359,
	521,521,        513,513,        499,499,
	9012,9014,      1101,1101,      1207,1207,
	499,495,        495,496,        9013,499,
	1307,1308,      499,499,        499,499,
	499,499,        499,499,        351,351,
	0,0,            357,499
};

const MISSILEINFO_EXTRA gMissileInfoExtra[] = {
	1207, 1207,    false, false, false, false, false, true, false,     true,
	420, 420,      false, true, true, false, false, false, false,      true,
	471, 471,      false, false, false, false, false, false, true,     false,
	421, 421,      false, true, false, true, false, false, false,      false,
	1309, 351,     false, true, false, false, false, false, false,     true,
	480, 480,      false, true, false, true, false, false, false,      false,
	470, 470,      false, false, false, false, false, false, true,     true,
	489, 490,      false, false, false, false, false, true, false,     true,
	462, 351,      false, true, false, false, false, false, false,     true,
	1203, 172,     false, false, true, false, false, false, false,     true,
	0,0,           false, false, true, false, false, false, false,     true,
	1457, 249,     false, false, false, false, false, true, false,     true,
	480, 489,      false, true, false, true, false, false, false,      false,
	480, 489,      false, false, false, true, false, false, false,     false,
	480, 489,      false, false, false, true, false, false, false,     false,
	491, 491,      true, true, true, true, true, true, true,           true,
	520, 520,      false, false, false, false, false, true, false,     true,
	520, 520,      false, false, false, false, false, true, false,     true,
};

const THINGINFO_EXTRA gThingInfoExtra[] = {
	true,   true,   true,   false,  false,
	false,  false,  false,  false,  false,
	false,  false,  false,  false,  false,
	true,   false,  false,  true,   true,
	true,   true,   false,  false,  false,
	false,  false,  true,   true,   true,
	true,   true,   true,   true,   true,
	true,
};

const DUDEINFO_EXTRA gDudeInfoExtra[] = {

	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 200
	{ false,  false,  0, 9, 13, 13, 17, 14 },       // 201
	{ false,  false,  0, 9, 13, 13, 17, 14 },       // 202
	{ false,  true,   0, 8, 0, 8, -1, -1 },         // 203
	{ false,  false,  0, 8, 0, 8, -1, -1 },         // 204
	{ false,  true,   1, -1, -1, -1, -1, -1 },      // 205
	{ true,   true,   0, 0, 0, 0, -1, -1 },         // 206
	{ true,   false,  0, 0, 0, 0, -1, -1 },         // 207
	{ true,   false,  1, -1, -1, -1, -1, -1 },      // 208
	{ true,   false,  1, -1, -1, -1, -1, -1 },      // 209
	{ true,   true,   0, 0, 0, 0, -1, -1 },         // 210
	{ false,  true,   0, 8, 0, 8, -1, -1 },         // 211
	{ false,  true,   0, 6, 0, 6, -1, -1 },         // 212
	{ false,  true,   0, 7, 0, 7, -1, -1 },         // 213
	{ false,  true,   0, 7, 0, 7, -1, -1 },         // 214
	{ false,  true,   0, 7, 0, 7, -1, -1 },         // 215
	{ false,  true,   0, 7, 0, 7, -1, -1 },         // 216
	{ false,  true,   0, 9, 10, 10, -1, -1 },       // 217
	{ false,  true,   0, 0, 0, 0, -1, -1 },         // 218
	{ true,  false,   7, 7, 7, 7, -1, -1 },         // 219
	{ false,  true,   0, 7, 0, 7, -1, -1 },         // 220
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 221
	{ false,  true,   -1, -1, -1, -1, -1, -1 },     // 222
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 223
	{ false,  true,   -1, -1, -1, -1, -1, -1 },     // 224
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 225
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 226
	{ false,  false,  0, 7, 0, 7, -1, -1 },         // 227
	{ false,  false,  0, 7, 0, 7, -1, -1 },         // 228
	{ false,  false,  0, 8, 0, 8, -1, -1 },         // 229
	{ false,  false,  0, 9, 13, 13, 17, 14 },       // 230
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 231
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 232
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 233
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 234
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 235
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 236
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 237
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 238
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 239
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 240
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 241
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 242
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 243
	{ false,  true,   -1, -1, -1, -1, -1, -1 },     // 244
	{ false,  true,   0, 6, 0, 6, -1, -1 },         // 245
	{ false,  false,  0, 9, 13, 13, 17, 14 },       // 246
	{ false,  false,  0, 9, 13, 13, 14, 14 },       // 247
	{ false,  false,  0, 9, 13, 13, 14, 14 },       // 248
	{ false,  false,  0, 9, 13, 13, 17, 14 },       // 249
	{ false,  true,   0, 6, 8, 8, 10, 9 },          // 250
	{ false,  true,   0, 8, 9, 9, 11, 10 },         // 251
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 252
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 253
	{ false,  false,  0, 9, 17, 13, 17, 14 },       // 254
	{ false,  false,  -1, -1, -1, -1, -1, -1 },     // 255

};


AISTATE genPatrolStates[] = {

	//-------------------------------------------------------------------------------

	{ kAiStatePatrolWaitL, 0, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitL, 7, -1, 0, NULL, NULL, aiPatrolThink, NULL },

	{ kAiStatePatrolMoveL, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveL, 8, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveL, 0, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveL, 6, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveL, 7, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

	{ kAiStatePatrolTurnL, 9, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnL, 8, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnL, 0, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnL, 6, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnL, 7, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },

	//-------------------------------------------------------------------------------

	{ kAiStatePatrolWaitW, 0, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitW, 10, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitW, 13, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitW, 17, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitW, 8, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitW, 9, -1, 0, NULL, NULL, aiPatrolThink, NULL },

	{ kAiStatePatrolMoveW, 0, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 10, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 13, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 8, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 7, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveW, 6, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

	{ kAiStatePatrolTurnW, 0, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 10, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 13, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 8, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 9, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 7, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnW, 6, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },

	//-------------------------------------------------------------------------------

	{ kAiStatePatrolWaitC, 17, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitC, 11, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitC, 10, -1, 0, NULL, NULL, aiPatrolThink, NULL },
	{ kAiStatePatrolWaitC, 14, -1, 0, NULL, NULL, aiPatrolThink, NULL },

	{ kAiStatePatrolMoveC, 14, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveC, 10, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },
	{ kAiStatePatrolMoveC, 9, -1, 0, NULL, aiPatrolMove, aiPatrolThink, NULL },

	{ kAiStatePatrolTurnC, 14, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnC, 10, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },
	{ kAiStatePatrolTurnC, 9, -1, 0, aiPatrolRandGoalAng, aiPatrolTurn, aiPatrolThink, NULL },

	//-------------------------------------------------------------------------------

};

CONDITION_TYPE_NAMES gCondTypeNames[7] = {

	{kCondGameBase,     kCondGameMax,   "Game"},
	{kCondMixedBase,    kCondMixedMax,  "Mixed"},
	{kCondWallBase,     kCondWallMax,   "Wall"},
	{kCondSectorBase,   kCondSectorMax, "Sector"},
	{kCondPlayerBase,   kCondPlayerMax, "Player"},
	{kCondDudeBase,     kCondDudeMax,   "Enemy"},
	{kCondSpriteBase,   kCondSpriteMax, "Sprite"},

};

// for actor.cpp
//-------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DBloodActor* nnExtSpawnDude(DBloodActor* sourceactor, DBloodActor* origin, int nType, double dist, double zadd)
{
	DBloodActor* pDudeActor = nullptr;

	if (nType < kDudeBase || nType >= kDudeMax || (pDudeActor = actSpawnSprite(origin, kStatDude)) == NULL)
		return NULL;

	DAngle angle = origin->spr.Angles.Yaw;
	auto pos = origin->spr.pos.plusZ(zadd);

	if (dist >= 0)
	{
		pos += angle.ToVector() * dist;
	}

	SetActor(pDudeActor, pos);

	pDudeActor->spr.type = nType;
	pDudeActor->spr.Angles.Yaw = angle;

	pDudeActor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
	pDudeActor->clipdist = getDudeInfo(nType)->fClipdist();

	pDudeActor->xspr.respawn = 1;
	pDudeActor->xspr.health = getDudeInfo(nType)->startHealth << 4;

	if (fileSystem.FindResource(getDudeInfo(nType)->seqStartID, "SEQ"))
		seqSpawn(getDudeInfo(nType)->seqStartID, pDudeActor, -1);

	// add a way to inherit some values of spawner by dude.
	if (sourceactor->spr.flags & kModernTypeFlag1) {

		//inherit pal?
		if (pDudeActor->spr.pal <= 0)
			pDudeActor->spr.pal = sourceactor->spr.pal;

		// inherit spawn sprite trigger settings, so designer can count monsters.
		pDudeActor->xspr.txID = sourceactor->xspr.txID;
		pDudeActor->xspr.command = sourceactor->xspr.command;
		pDudeActor->xspr.triggerOn = sourceactor->xspr.triggerOn;
		pDudeActor->xspr.triggerOff = sourceactor->xspr.triggerOff;

		// inherit drop items
		pDudeActor->xspr.dropMsg = sourceactor->xspr.dropMsg;

		// inherit dude flags
		pDudeActor->xspr.dudeDeaf = sourceactor->xspr.dudeDeaf;
		pDudeActor->xspr.dudeGuard = sourceactor->xspr.dudeGuard;
		pDudeActor->xspr.dudeAmbush = sourceactor->xspr.dudeAmbush;
		pDudeActor->xspr.dudeFlag4 = sourceactor->xspr.dudeFlag4;
		pDudeActor->xspr.unused1 = sourceactor->xspr.unused1;

	}

	aiInitSprite(pDudeActor);

	if (AllowedKillType(pDudeActor)) Level.addKillCount();

	bool burning = IsBurningDude(pDudeActor);
	if (burning) {
		pDudeActor->xspr.burnTime = 10;
		pDudeActor->SetTarget(nullptr);
	}

	if ((burning || (sourceactor->spr.flags & kModernTypeFlag3)) && !pDudeActor->xspr.dudeFlag4)
		aiActivateDude(pDudeActor);

	return pDudeActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool nnExtIsImmune(DBloodActor* actor, int dmgType, int minScale)
{
	if (dmgType >= kDmgFall && dmgType < kDmgMax && actor->hasX() && actor->xspr.locked != 1)
	{
		if (actor->spr.type >= kThingBase && actor->spr.type < kThingMax)
		{
			return (thingInfo[actor->spr.type - kThingBase].dmgControl[dmgType] <= minScale);
		}
		else if (actor->IsDudeActor())
		{
			if (actor->IsPlayerActor()) return (getPlayer(actor->spr.type - kDudePlayer1)->damageControl[dmgType]);
			else if (actor->spr.type == kDudeModernCustom) return (actor->genDudeExtra.dmgControl[dmgType] <= minScale);
			else return (getDudeInfo(actor->spr.type)->damageVal[dmgType] <= minScale);
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool nnExtEraseModernStuff(DBloodActor* actor)
{
	bool erased = false;
	switch (actor->spr.type) {
		// erase all modern types if the map is not extended
	case kModernSpriteDamager:
	case kModernCustomDudeSpawn:
	case kModernRandomTX:
	case kModernSequentialTX:
	case kModernSeqSpawner:
	case kModernObjPropertiesChanger:
	case kModernObjPicnumChanger:
	case kModernObjSizeChanger:
	case kModernDudeTargetChanger:
	case kModernSectorFXChanger:
	case kModernObjDataChanger:
	case kModernObjDataAccumulator:
	case kModernEffectSpawner:
	case kModernWindGenerator:
	case kModernPlayerControl:
	case kModernCondition:
	case kModernConditionFalse:
	case kModernSlopeChanger:
	case kModernStealthRegion:
		actor->spr.type = kSpriteDecoration;
		erased = true;
		break;
	case kItemModernMapLevel:
	case kDudeModernCustom:
	case kDudeModernCustomBurning:
	case kModernThingTNTProx:
	case kModernThingEnemyLifeLeech:
		actor->spr.type = kSpriteDecoration;
		ChangeActorStat(actor, kStatDecoration);
		erased = true;
		break;
		// also erase some modernized vanilla types which was not active
	case kMarkerWarpDest:
		if (actor->spr.statnum == kStatMarker) break;
		actor->spr.type = kSpriteDecoration;
		erased = true;
		break;
	}

	if (actor->xspr.Sight)
	{
		actor->xspr.Sight = false; // it does not work in vanilla at all
		erased = true;
	}

	if (actor->xspr.Proximity)
	{
		// proximity works only for things and dudes in vanilla
		switch (actor->spr.statnum)
		{
		case kStatThing:
		case kStatDude:
			break;
		default:
			actor->xspr.Proximity = false;
			erased = true;
			break;
		}
	}

	return erased;
}

//---------------------------------------------------------------------------
//

//
//---------------------------------------------------------------------------

void nnExtTriggerObject(EventObject& eob, int command, DBloodActor* initiator)
{
	if (eob.isSector())
	{
		trTriggerSector(eob.sector(), command, initiator);
	}
	else if (eob.isWall())
	{
		trTriggerWall(eob.wall(), command, initiator);
	}
	else if (eob.isActor())
	{
		auto objActor = eob.actor();
		if (!objActor || !objActor->hasX()) return;
		trTriggerSprite(objActor, command, initiator);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void nnExtResetGlobals()
{
	gAllowTrueRandom = gEventRedirectsUsed = false;

	// reset counters
	gProxySpritesCount = gSightSpritesCount = gPhysSpritesCount = gImpactSpritesCount = 0;

	// fill arrays with negative values to avoid index 0 situation
	memset(gSightSpritesList, 0, sizeof(gSightSpritesList));
	memset(gProxySpritesList, 0, sizeof(gProxySpritesList));
	memset(gPhysSpritesList, 0, sizeof(gPhysSpritesList));
	memset(gImpactSpritesList, 0, sizeof(gImpactSpritesList));
	gConditions.Clear();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(nnext_ifshow)
{
	if (!isBlood()) return;
	BloodSpriteIterator it;
	int cnt = 0;
	while (auto actor = it.Next())
	{
		if (actor->spr.type == kModernCondition || actor->spr.type == kModernConditionFalse)
		{
			if (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE)
			{
				actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
				cnt++;
			}
		}
	}
	
	if (cnt <= 0)
		Printf("No condition sprites found!\n");

	Printf("%d sprites are visible now.\n", cnt);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void nnExtInitModernStuff(TArray<DBloodActor*>& actors)
{
	nnExtResetGlobals();

	// use true random only for single player mode, otherwise use Blood's default one.
	if (gGameOptions.nGameType == 0 && !VanillaMode())
	{
		gStdRandom.seed(std::random_device()());

		// since true random is not working if compiled with old mingw versions, we should
		// check if it works in game and if not - switch to using in-game random function.
		for (int i = kMaxRandomizeRetries; i >= 0; i--)
		{
			std::uniform_int_distribution<int> dist_a_b(0, 100);
			if (gAllowTrueRandom || i <= 0) break;
			else if (dist_a_b(gStdRandom) != 0)
				gAllowTrueRandom = true;
		}
	}

	for (auto actor : actors)
	{
		if (!actor->exists() || !actor->hasX()) continue;

		switch (actor->spr.type) {
		case kModernRandomTX:
		case kModernSequentialTX:
			if (actor->xspr.command == kCmdLink) gEventRedirectsUsed = true;
			break;
		case kDudeModernCustom:
		case kDudeModernCustomBurning:
			getSpriteMassBySize(actor); // create mass cache
			break;
		case kModernCondition:
		case kModernConditionFalse:
			if (!actor->xspr.rxID && actor->xspr.data1 > kCondGameMax) condError(actor, "\nThe condition must have RX ID!\nSPRITE #%d", actor->GetIndex());
			else if (!actor->xspr.txID && !actor->spr.flags)
			{
				Printf(PRINT_HIGH, "The condition must have TX ID or hitag to be set: RX ID %d, SPRITE #%d", actor->xspr.rxID, actor->GetIndex());
			}
			break;
		}

		// auto set going On and going Off if both are empty
		if (actor->xspr.txID && !actor->xspr.triggerOn && !actor->xspr.triggerOff)
			actor->xspr.triggerOn = actor->xspr.triggerOff = true;

		// copy custom start health to avoid overwrite by kThingBloodChunks
		if (actor->IsDudeActor())
			actor->xspr.sysData2 = actor->xspr.data4;

		// check reserved statnums
		if (actor->spr.statnum >= kStatModernBase && actor->spr.statnum < kStatModernMax)
		{
			bool sysStat = true;
			switch (actor->spr.statnum)
			{
			case kStatModernStealthRegion:
				sysStat = (actor->spr.type != kModernStealthRegion);
				break;
			case kStatModernDudeTargetChanger:
				sysStat = (actor->spr.type != kModernDudeTargetChanger);
				break;
			case kStatModernCondition:
				sysStat = (actor->spr.type != kModernCondition && actor->spr.type != kModernConditionFalse);
				break;
			case kStatModernEventRedirector:
				sysStat = (actor->spr.type != kModernRandomTX && actor->spr.type != kModernSequentialTX);
				break;
			case kStatModernWindGen:
				sysStat = (actor->spr.type != kModernWindGenerator);
				break;
			case kStatModernPlayerLinker:
			case kStatModernQavScene:
				sysStat = (actor->spr.type != kModernPlayerControl);
				break;
			}

			if (sysStat)
				I_Error("Sprite statnum %d on sprite #%d is in a range of reserved (%d - %d)!", actor->spr.statnum, actor->GetIndex(), kStatModernBase, kStatModernMax);
		}

		switch (actor->spr.type)
		{
		case kModernRandomTX:
		case kModernSequentialTX:
			if (actor->xspr.command != kCmdLink) break;
			// add statnum for faster redirects search
			ChangeActorStat(actor, kStatModernEventRedirector);
			break;
		case kModernWindGenerator:
			actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
			ChangeActorStat(actor, kStatModernWindGen);
			break;
		case kModernDudeTargetChanger:
		case kModernObjDataAccumulator:
		case kModernRandom:
		case kModernRandom2:
		case kModernStealthRegion:
			actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
			actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			switch (actor->spr.type)
			{
				// stealth regions for patrolling enemies
			case kModernStealthRegion:
				ChangeActorStat(actor, kStatModernStealthRegion);
				break;
				// add statnum for faster dude searching
			case kModernDudeTargetChanger:
				ChangeActorStat(actor, kStatModernDudeTargetChanger);
				if (actor->xspr.busyTime <= 0) actor->xspr.busyTime = 5;
				actor->xspr.command = kCmdLink;
				break;
				// remove kStatItem status from random item generators
			case kModernRandom:
			case kModernRandom2:
				ChangeActorStat(actor, kStatDecoration);
				actor->xspr.sysData1 = actor->xspr.command; // save the command so spawned item can inherit it
				actor->xspr.command = kCmdLink;  // generator itself can't send commands
				break;
			}
			break;
		case kModernThingTNTProx:
			actor->xspr.Proximity = true;
			break;
		case kDudeModernCustom:
		{
			if (actor->xspr.txID <= 0) break;
			int found = 0;
			BloodStatIterator it(kStatDude);
			while (DBloodActor* iactor = it.Next())
			{
				if (iactor->xspr.rxID != actor->xspr.txID) continue;
				else if (found) I_Error("\nCustom dude (TX ID %d):\nOnly one incarnation allowed per channel!", actor->xspr.txID);
				ChangeActorStat(iactor, kStatInactive);
				found++;
			}
			break;
		}
		case kDudePodMother:
		case kDudeTentacleMother:
			actor->xspr.state = 1;
			break;
		case kModernPlayerControl:
			switch (actor->xspr.command)
			{
			case kCmdLink:
			{
				if (actor->xspr.data1 < 0 || actor->xspr.data1 > kMaxPlayers)
					I_Error("\nPlayer Control (SPRITE #%d):\nPlayer out of a range (data1 = %d)", actor->GetIndex(), actor->xspr.data1);

				//if (numplayers < actor->xspr.data1)
				//I_Error("\nPlayer Control (SPRITE #%d):\n There is no player #%d", actor->GetIndex(), actor->xspr.data1);

				if (actor->xspr.rxID && actor->xspr.rxID != kChannelLevelStart)
					I_Error("\nPlayer Control (SPRITE #%d) with Link command should have no RX ID!", actor->GetIndex());

				if (actor->xspr.txID && actor->xspr.txID < kChannelUser)
					I_Error("\nPlayer Control (SPRITE #%d):\nTX ID should be in range of %d and %d!", actor->GetIndex(), kChannelUser, kChannelMax);

				// only one linker per player allowed
				BloodStatIterator it(kStatModernPlayerLinker);
				while (auto iactor = it.Next())
				{
					if (actor != iactor && actor->xspr.data1 == iactor->xspr.data1)
						I_Error("\nPlayer Control (SPRITE #%d):\nPlayer %d already linked with different player control sprite #%d!", actor->GetIndex(), actor->xspr.data1, iactor->GetIndex());
				}
				actor->xspr.sysData1 = -1;
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
				ChangeActorStat(actor, kStatModernPlayerLinker);
				break;
			}
			case 67: // play qav animation
				if (actor->xspr.txID >= kChannelUser && !actor->xspr.waitTime) actor->xspr.waitTime = 1;
				ChangeActorStat(actor, kStatModernQavScene);
				break;
			}
			break;
		case kModernCondition:
		case kModernConditionFalse:
			if (actor->xspr.busyTime > 0)
			{
				if (actor->xspr.waitTime > 0)
				{
					actor->xspr.busyTime = ClipHigh(actor->xspr.busyTime + ((actor->xspr.waitTime * 120) / 10), 4095); actor->xspr.waitTime = 0;
					Printf(PRINT_HIGH, "Summing busyTime and waitTime for tracking condition #%d, RX ID %d. Result = %d ticks", actor->GetIndex(), actor->xspr.rxID, actor->xspr.busyTime);
				}
				actor->xspr.busy = actor->xspr.busyTime;
			}

			if (actor->xspr.waitTime && actor->xspr.command >= kCmdNumberic)
				condError(actor, "Delay is not available when using numberic commands (%d - %d)", kCmdNumberic, 255);

			actor->xspr.Decoupled = false; // must go through operateSprite always
			actor->xspr.Sight = actor->xspr.Impact = actor->xspr.Touch = false;
			actor->xspr.Proximity = actor->xspr.Push = actor->xspr.Vector = false;
			actor->xspr.state = actor->xspr.restState = 0;

			if (gModernMap == 2 && actor->xspr.triggerOn && !actor->xspr.triggerOff)
				actor->spr.flags |= kModernTypeFlag64;
			else
				actor->spr.flags &= ~kModernTypeFlag64;

			actor->xspr.triggerOn = actor->xspr.triggerOff = false;

			actor->xspr.TargetPos = {-1, -1, -1};

			actor->xspr.sysData2 = -1;
			actor->SetTarget(nullptr);
			ChangeActorStat(actor, kStatModernCondition);
			auto oldStat = actor->spr.cstat;
			actor->spr.cstat = 0;

			if (oldStat & CSTAT_SPRITE_BLOCK)
				actor->spr.cstat |= CSTAT_SPRITE_BLOCK;

			if (oldStat & CSTAT_SPRITE_MOVE_FORWARD) actor->spr.cstat |= CSTAT_SPRITE_MOVE_FORWARD;
			else if (oldStat & CSTAT_SPRITE_MOVE_REVERSE) actor->spr.cstat |= CSTAT_SPRITE_MOVE_REVERSE;

			if (!nnext_showconditionsprites)
				actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			break;
		}

		// the following trigger flags are senseless to have together
		if ((actor->xspr.Touch && (actor->xspr.Proximity || actor->xspr.Sight) && actor->xspr.DudeLockout)
			|| (actor->xspr.Touch && actor->xspr.Proximity && !actor->xspr.Sight)) actor->xspr.Touch = false;

		if (actor->xspr.Proximity && actor->xspr.Sight && actor->xspr.DudeLockout)
			actor->xspr.Proximity = false;

		// very quick fix for floor sprites with Touch trigger flag if their Z is equals sector florz / ceilgz
		if (actor->insector() && actor->xspr.Touch && (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) {
			if (actor->spr.pos.Z == actor->sector()->floorz) actor->spr.pos.Z -= zmaptoworld;
			else if (actor->spr.pos.Z == actor->sector()->ceilingz) actor->spr.pos.Z += zmaptoworld;
		}

		// make Proximity flag work not just for dudes and things...
		if (actor->xspr.Proximity && gProxySpritesCount < kMaxSuperXSprites)
		{
			switch (actor->spr.statnum)
			{
			case kStatFX:           case kStatExplosion:            case kStatItem:
			case kStatPurge:        case kStatSpares:               case kStatFlare:
			case kStatInactive:     case kStatFree:                 case kStatMarker:
			case kStatThing:        case kStatDude:
			case kStatModernPlayerLinker:
				break;
			default:
				gProxySpritesList[gProxySpritesCount++] = actor;
				if (gProxySpritesCount == kMaxSuperXSprites)
					I_Error("Max (%d) *additional* Proximity sprites reached!", kMaxSuperXSprites);
				break;
			}
		}

		// make Sight, Screen, Aim flags work not just for dudes and things...
		if ((actor->xspr.Sight || actor->xspr.unused3) && gSightSpritesCount < kMaxSuperXSprites)
		{
			switch (actor->spr.statnum)
			{
			case kStatFX:           case kStatExplosion:            case kStatItem:
			case kStatPurge:        case kStatSpares:               case kStatFlare:
			case kStatInactive:     case kStatFree:                 case kStatMarker:
			case kStatModernPlayerLinker:
				break;
			default:
				gSightSpritesList[gSightSpritesCount++] = actor;
				if (gSightSpritesCount == kMaxSuperXSprites)
					I_Error("Max (%d) Sight sprites reached!", kMaxSuperXSprites);
				break;
			}
		}

		// make Impact flag work for sprites that affected by explosions...
		if (actor->xspr.Impact && gImpactSpritesCount < kMaxSuperXSprites)
		{
			switch (actor->spr.statnum)
			{
			case kStatFX:           case kStatExplosion:            case kStatItem:
			case kStatPurge:        case kStatSpares:               case kStatFlare:
			case kStatInactive:     case kStatFree:                 case kStatMarker:
			case kStatModernPlayerLinker:
				break;
			default:
				gImpactSpritesList[gImpactSpritesCount++] = actor;
				if (gImpactSpritesCount == kMaxSuperXSprites)
					I_Error("Max (%d) *additional* Impact sprites reached!", kMaxSuperXSprites);
				break;
			}
		}
	}

	// collect objects for tracking conditions
	BloodStatIterator it2(kStatModernCondition);
	while (auto iactor = it2.Next())
	{
		if (iactor->xspr.busyTime <= 0 || iactor->xspr.isTriggered) continue;

		TRCONDITION* pCond = &gConditions[gConditions.Reserve(1)];

		for (auto iactor2 : actors)
		{
			if (!iactor->exists() || !iactor2->hasX() || iactor2->xspr.txID != iactor->xspr.rxID || iactor2 == iactor)
				continue;

			switch (iactor2->spr.type)
			{
			case kSwitchToggle: // exceptions
			case kSwitchOneWay: // exceptions
				continue;
			}

			if (iactor2->spr.type == kModernCondition || iactor2->spr.type == kModernConditionFalse)
				condError(iactor, "Tracking condition always must be first in condition sequence!");

			pCond->objects.Reserve(1);
			pCond->objects.Last().obj = EventObject(iactor2);
			pCond->objects.Last().cmd = (uint8_t)iactor2->xspr.command;
		}

		for (auto& sect : sector)
		{
			if (!sect.hasX() || sect.xs().txID != iactor->xspr.rxID) continue;

			pCond->objects.Reserve(1);
			pCond->objects.Last().obj = EventObject(&sect);
			pCond->objects.Last().cmd = sect.xs().command;
		}

		for (auto& wal : wall)
		{
			if (!wal.hasX() || wal.xw().txID != iactor->xspr.rxID)
				continue;

			switch (wal.type) {
			case kSwitchToggle: // exceptions
			case kSwitchOneWay: // exceptions
				continue;
			}

			pCond->objects.Reserve(1);
			pCond->objects.Last().obj = EventObject(&wal);
			pCond->objects.Last().cmd = wal.xw().command;
		}

		if (iactor->xspr.data1 > kCondGameMax && pCond->objects.Size() == 0)
			Printf(PRINT_HIGH, "No objects to track found for condition #%d, RXID: %d!", iactor->GetIndex(), iactor->xspr.rxID);

		pCond->actor = iactor;
	}
}


// The following functions required for random event features
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int nnExtRandom(int a, int b)
{
	if (!gAllowTrueRandom) return Random(((b + 1) - a)) + a;
	// used for better randomness in single player
	std::uniform_int_distribution<int> dist_a_b(a, b);
	return dist_a_b(gStdRandom);
}

static int GetDataVal(DBloodActor* actor, int data)
{
	if (!actor->hasX()) return -1;

	switch (data) {
	case 0: return actor->xspr.data1;
	case 1: return actor->xspr.data2;
	case 2: return actor->xspr.data3;
	case 3: return actor->xspr.data4;
	}

	return -1;
}

//---------------------------------------------------------------------------
//
// tries to get random data field of sprite
//
//---------------------------------------------------------------------------

static int randomGetDataValue(DBloodActor* actor, int randType)
{
	if (actor == NULL || !actor->hasX()) return -1;
	int random = 0; int bad = 0; int maxRetries = kMaxRandomizeRetries;

	int rData[4];
	rData[0] = actor->xspr.data1; rData[2] = actor->xspr.data3;
	rData[1] = actor->xspr.data2; rData[3] = actor->xspr.data4;
	// randomize only in case if at least 2 data fields fits.
	for (int i = 0; i < 4; i++)
	{
		switch (randType) {
		case kRandomizeItem:
			if (rData[i] >= kItemWeaponBase && rData[i] < kItemMax) break;
			else bad++;
			break;
		case kRandomizeDude:
			if (rData[i] >= kDudeBase && rData[i] < kDudeMax) break;
			else bad++;
			break;
		case kRandomizeTX:
			if (rData[i] > kChannelZero && rData[i] < kChannelUserMax) break;
			else bad++;
			break;
		default:
			bad++;
			break;
		}
	}

	if (bad < 3)
	{
		// try randomize few times
		while (maxRetries > 0)
		{
			random = nnExtRandom(0, 3);
			if (rData[random] > 0) return rData[random];
			else maxRetries--;
		}
	}

	return -1;
}

//---------------------------------------------------------------------------
//
// this function drops random item using random pickup generator(s)
//
//---------------------------------------------------------------------------

static DBloodActor* randomDropPickupObject(DBloodActor* sourceactor, int prevItem)
{
	DBloodActor* spawned = nullptr;
	int selected = -1;
	int maxRetries = 9;
	if (sourceactor->hasX())
	{
		while ((selected = randomGetDataValue(sourceactor, kRandomizeItem)) == prevItem) if (maxRetries-- <= 0) break;
		if (selected > 0)
		{
			spawned = actDropObject(sourceactor, selected);
			if (spawned)
			{
				sourceactor->xspr.dropMsg = uint8_t(spawned->spr.type); // store dropped item type in dropMsg
				spawned->spr.pos = sourceactor->spr.pos;

				if ((sourceactor->spr.flags & kModernTypeFlag1) && (sourceactor->xspr.txID > 0 || (sourceactor->xspr.txID != 3 && sourceactor->xspr.lockMsg > 0)))
				{
					spawned->addX();

					// inherit spawn sprite trigger settings, so designer can send command when item picked up.
					spawned->xspr.txID = sourceactor->xspr.txID;
					spawned->xspr.command = sourceactor->xspr.sysData1;
					spawned->xspr.triggerOn = sourceactor->xspr.triggerOn;
					spawned->xspr.triggerOff = sourceactor->xspr.triggerOff;

					spawned->xspr.Pickup = true;

				}
			}
		}
	}
	return spawned;
}

//---------------------------------------------------------------------------
//
// this function spawns random dude using dudeSpawn
//
//---------------------------------------------------------------------------

DBloodActor* randomSpawnDude(DBloodActor* sourceactor, DBloodActor* origin, double dist, double zadd)
{
	DBloodActor* spawned = NULL; int selected = -1;

	if ((selected = randomGetDataValue(sourceactor, kRandomizeDude)) > 0)
		spawned = nnExtSpawnDude(sourceactor, origin, selected, dist, zadd);

	return spawned;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void windGenDoVerticalWind(int factor, sectortype* pSector)
{
	double maxZ = 0, zdiff;
	bool maxZfound = false;

	// find maxz marker first
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.type == kMarkerOn && actor->spr.statnum != kStatMarker)
		{
			maxZ = actor->spr.pos.Z;
			maxZfound = true;
			break;
		}
	}

	it.Reset(pSector);
	while (auto actor = it.Next())
	{
		switch (actor->spr.statnum)
		{
		case kStatFree:
			continue;
		case kStatFX:
			if (actor->vel.Z) break;
			continue;
		case kStatThing:
		case kStatDude:
			if (actor->spr.flags & kPhysGravity) break;
			continue;
		default:
			if (actor->hasX() && actor->xspr.physAttr & kPhysGravity) break;
			continue;
		}

		if (maxZfound && actor->spr.pos.Z <= maxZ)
		{
			zdiff = actor->spr.pos.Z - maxZ;
			if (actor->vel.Z < 0) actor->vel.Z += actor->vel.Z * zdiff / 4096;
			continue;
		}

		double val = -factor / 1024.;
		if (actor->vel.Z >= 0) actor->vel.Z += val;
		else actor->vel.Z = val;

		actor->spr.pos.Z += actor->vel.Z / 16.;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void nnExtProcessSuperSprites()
{
	// process tracking conditions
	if (gConditions.Size() > 0)
	{
		for (unsigned i = 0; i < gConditions.Size(); i++)
		{
			TRCONDITION const* pCond = &gConditions[i];
			auto aCond = pCond->actor;
			if (aCond->xspr.locked || aCond->xspr.isTriggered || ++aCond->xspr.busy < aCond->xspr.busyTime)
				continue;

			if (pCond->objects.Size() > 0)
			{
				aCond->xspr.busy = 0;
				for (unsigned k = 0; k < pCond->objects.Size(); k++)
				{
					EVENT evn;
					evn.target = pCond->objects[k].obj;
					evn.cmd = pCond->objects[k].cmd;
					evn.funcID = kCallbackMax;
					evn.initiator = nullptr;
					useCondition(pCond->actor, evn);
				}
			}
			else if (aCond->xspr.data1 >= kCondGameBase && aCond->xspr.data1 < kCondGameMax)
			{
				EVENT evn;
				evn.target = EventObject(pCond->actor);
				evn.cmd = (int8_t)aCond->xspr.command;
				evn.funcID = kCallbackMax;
				evn.initiator = nullptr;
				useCondition(pCond->actor, evn);
			}

		}
	}

	// process floor oriented kModernWindGenerator to create a vertical wind in the sectors
	BloodStatIterator it(kStatModernWindGen);
	while (auto windactor = it.Next())
	{

		if (!(windactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) || windactor->spr.statnum >= kMaxStatus || !windactor->hasX())
			continue;

		if (!windactor->xspr.state || windactor->xspr.locked)
			continue;

		int j, rx;
		bool fWindAlways = (windactor->spr.flags & kModernTypeFlag1);

		if (windactor->xspr.txID) {

			rx = windactor->xspr.txID;
			for (j = bucketHead[rx]; j < bucketHead[rx + 1]; j++)
			{
				if (!rxBucket[j].isSector()) continue;
				auto pSector = rxBucket[j].sector();
				XSECTOR* pXSector = &pSector->xs();

				if ((!pXSector->locked) && (fWindAlways || pXSector->windAlways || pXSector->busy))
					windGenDoVerticalWind(windactor->xspr.sysData2, pSector);
			}

			DBloodActor* pXRedir = nullptr; // check redirected TX buckets
			while ((pXRedir = evrListRedirectors(OBJ_SPRITE, nullptr, nullptr, windactor, pXRedir, &rx)) != nullptr)
			{
				for (j = bucketHead[rx]; j < bucketHead[rx + 1]; j++)
				{
					if (!rxBucket[j].isSector()) continue;
					auto pSector = rxBucket[j].sector();
					XSECTOR* pXSector = &pSector->xs();

					if ((!pXSector->locked) && (fWindAlways || pXSector->windAlways || pXSector->busy))
						windGenDoVerticalWind(windactor->xspr.sysData2, pSector);
				}
			}

		}
		else if (windactor->insector())
		{
			sectortype* pSect = windactor->sector();
			XSECTOR* pXSector = (pSect->hasX()) ? &pSect->xs() : nullptr;
			if ((fWindAlways) || (pXSector && !pXSector->locked && (pXSector->windAlways || pXSector->busy)))
				windGenDoVerticalWind(windactor->xspr.sysData2, windactor->sector());
		}
	}

	// process additional proximity sprites
	if (gProxySpritesCount > 0)
	{
		for (int i = 0; i < gProxySpritesCount; i++)
		{
			DBloodActor* pProx = gProxySpritesList[i];
			if (!pProx || !xsprIsFine(pProx)) continue;

			if (!pProx->xspr.Proximity || (!pProx->xspr.Interrutable && pProx->xspr.state != pProx->xspr.restState) || pProx->xspr.locked == 1
				|| pProx->xspr.isTriggered) continue;  // don't process locked or triggered sprites

			int okDist = (pProx->IsDudeActor()) ? 96 : max(int(pProx->clipdist * 12), 32);
			auto pos = pProx->spr.pos;
			auto pSect = pProx->sector();

			if (!pProx->xspr.DudeLockout)
			{
				BloodStatIterator itr(kStatDude);
				while (auto affected = itr.Next())
				{
					if (!xsprIsFine(affected) || affected->xspr.health <= 0) continue;
					else if (CheckProximity(affected, pos, pSect, okDist))
					{
						trTriggerSprite(pProx, kCmdSpriteProximity, affected);
						break;
					}
				}
			}
			else
			{
				for (int a = connecthead; a >= 0; a = connectpoint2[a])
				{
					DBloodPlayer* pPlayer = getPlayer(a);
					if (!pPlayer || !xsprIsFine(pPlayer->GetActor()) || pPlayer->GetActor()->xspr.health <= 0)
						continue;

					if (pPlayer->GetActor()->xspr.health > 0 && CheckProximity(getPlayer(0)->GetActor(), pos, pSect, okDist))
					{
						trTriggerSprite(pProx, kCmdSpriteProximity, pPlayer->GetActor());
					}
				}
			}
		}
	}

	// process sight sprites (for players only)
	if (gSightSpritesCount > 0)
	{
		for (int i = 0; i < gSightSpritesCount; i++)
		{
			DBloodActor* pSight = gSightSpritesList[i];
			if (!pSight || !xsprIsFine(pSight)) continue;

			if ((!pSight->xspr.Interrutable && pSight->xspr.state != pSight->xspr.restState) || pSight->xspr.locked == 1 ||
				pSight->xspr.isTriggered) continue; // don't process locked or triggered sprites

			// sprite is drawn for one of players
			if ((pSight->xspr.unused3 & kTriggerSpriteScreen) && (pSight->spr.cstat2 & CSTAT2_SPRITE_MAPPED))
			{
				trTriggerSprite(pSight, kCmdSpriteSight);
				pSight->spr.cstat2 &= ~CSTAT2_SPRITE_MAPPED;
				continue;
			}


			auto pSightSect = pSight->sector();
			double ztop2, zbot2;

			for (int a = connecthead; a >= 0; a = connectpoint2[a])
			{
				DBloodPlayer* pPlayer = getPlayer(a);
				if (!pPlayer || !xsprIsFine(pPlayer->GetActor()) || pPlayer->GetActor()->xspr.health <= 0)
					continue;

				auto plActor = pPlayer->GetActor();
				GetActorExtents(plActor, &ztop2, &zbot2);
				if (cansee(pSight->spr.pos, pSightSect, DVector3(plActor->spr.pos.XY(), ztop2), plActor->sector()))
				{
					if (pSight->xspr.Sight)
					{
						trTriggerSprite(pSight, kCmdSpriteSight, plActor);
					}

					if (pSight->xspr.unused3 & kTriggerSpriteAim)
					{
						bool vector = (pSight->spr.cstat & CSTAT_SPRITE_BLOCK_HITSCAN);
						if (!vector)
							pSight->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;

						HitScan(pPlayer->GetActor(), pPlayer->zWeapon, pPlayer->aim, CLIPMASK0 | CLIPMASK1, 0);

						if (!vector)
							pSight->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;

						if (gHitInfo.actor() == pSight)
						{
							trTriggerSprite(gHitInfo.actor(), kCmdSpriteSight, plActor);
						}
					}
				}
			}
		}
	}

	// process Debris sprites for movement
	if (gPhysSpritesCount > 0)
	{
		for (int i = 0; i < gPhysSpritesCount; i++)
		{
			DBloodActor* debrisactor = gPhysSpritesList[i];
			if (debrisactor == nullptr || !debrisactor->hasX()) continue;

			if (debrisactor->spr.statnum == kStatFree || (debrisactor->spr.flags & kHitagFree) != 0)
			{
				gPhysSpritesList[i] = nullptr;
				continue;
			}

			if (!(debrisactor->xspr.physAttr & kPhysMove) && !(debrisactor->xspr.physAttr & kPhysGravity))
			{
				gPhysSpritesList[i] = nullptr;
				continue;
			}

			XSECTOR* pXSector = (debrisactor->sector()->hasX()) ? &debrisactor->sector()->xs() : nullptr;
			viewBackupSpriteLoc(debrisactor);

			bool uwater = false;
			int mass = debrisactor->spriteMass.mass;
			int airVel = debrisactor->spriteMass.airVel;

			double top, bottom;
			GetActorExtents(debrisactor, &top, &bottom);

			if (pXSector != nullptr)
			{
				if ((uwater = pXSector->Underwater) != 0) airVel <<= 6;
				if (pXSector->panVel != 0 && getflorzofslopeptr(debrisactor->sector(), debrisactor->spr.pos) <= bottom)
				{
					DAngle angle = pXSector->panAngle;
					double speed = 0;
					if (pXSector->panAlways || pXSector->state || pXSector->busy)
					{
						speed = pXSector->panVel / 128.;
						if (!pXSector->panAlways && pXSector->busy)
							speed *= FixedToFloat(pXSector->busy);
					}
					if (debrisactor->sector()->floorstat & CSTAT_SECTOR_ALIGN)
						angle += debrisactor->sector()->walls[0].normalAngle();
					debrisactor->vel += angle.ToVector() * speed;
				}
			}

			actAirDrag(debrisactor, airVel);

			if (debrisactor->xspr.physAttr & kPhysDebrisTouch)
			{
				DBloodPlayer* pPlayer = NULL;
				for (int a = connecthead; a != -1; a = connectpoint2[a])
				{
					pPlayer = getPlayer(a);
					DBloodActor* pact = pPlayer->GetActor();

					if (pact && pact->hit.hit.type == kHitSprite && pact->hit.hit.actor() == debrisactor)
					{
						double nSpeed = pact->vel.XY().Length();
						nSpeed = max<double>(nSpeed - nSpeed * FixedToFloat<6>(mass), FixedToFloat(0x9000 - (mass << 3))); // very messy math (TM)...

						debrisactor->vel += pPlayer->GetActor()->spr.Angles.Yaw.ToVector() * nSpeed;
						debrisactor->hit.hit.setSprite(pPlayer->GetActor());
					}
				}
			}

			if (debrisactor->xspr.physAttr & kPhysGravity) debrisactor->xspr.physAttr |= kPhysFalling;
			if ((debrisactor->xspr.physAttr & kPhysFalling) || !debrisactor->vel.isZero() || debrisactor->sector()->velFloor || debrisactor->sector()->velCeil)
				debrisMove(i);

			if (!debrisactor->vel.XY().isZero())
				debrisactor->xspr.goalAng = debrisactor->vel.Angle();

			debrisactor->norm_ang();
			DAngle ang = debrisactor->spr.Angles.Yaw;
			if ((uwater = spriteIsUnderwater(debrisactor)) == false) evKillActor(debrisactor, kCallbackEnemeyBubble);
			else if (Chance(0x1000 - mass))
			{
				if (debrisactor->vel.Z > 0x100) debrisBubble(debrisactor);
				if (absangle(ang, debrisactor->xspr.goalAng) < minAngle) // need to be very careful with comparing angles for equality!
				{
					debrisactor->xspr.goalAng += RandomAngle(kAng60);
					debrisactor->norm_ang();
					debrisBubble(debrisactor);
				}
			}

			int vdist = max((int)(abs(debrisactor->vel.X) + abs(debrisactor->vel.Y) * 2048.), (uwater) ? 1 : 0);
			auto angStep = DAngle::fromBuild(vdist);

			if (ang < debrisactor->xspr.goalAng) debrisactor->spr.Angles.Yaw = min(ang + angStep, debrisactor->xspr.goalAng);
			else if (ang > debrisactor->xspr.goalAng) debrisactor->spr.Angles.Yaw = max(ang - angStep, debrisactor->xspr.goalAng);

			auto pSector = debrisactor->sector();
			double fz, cz;
			calcSlope(pSector, debrisactor->spr.pos, &cz, &fz);
			GetActorExtents(debrisactor, &top, &bottom);

			if (fz >= bottom && pSector->lowerLink == nullptr && !(pSector->ceilingstat & CSTAT_SECTOR_SKY)) debrisactor->spr.pos.Z += max(cz - top, 0.);
			if (cz <= top && pSector->upperLink == nullptr && !(pSector->floorstat & CSTAT_SECTOR_SKY)) debrisactor->spr.pos.Z += min(fz - bottom, 0.);
		}
	}
}

//---------------------------------------------------------------------------
//
// this function plays sound predefined in missile info
//
//---------------------------------------------------------------------------

void sfxPlayMissileSound(DBloodActor* actor, int missileId)
{
	const MISSILEINFO_EXTRA* pMissType = &gMissileInfoExtra[missileId - kMissileBase];
	sfxPlay3DSound(actor, Chance(0x5000) ? pMissType->fireSound[0] : pMissType->fireSound[1], -1, 0);
}

//---------------------------------------------------------------------------
//
// this function plays sound predefined in vector info
//
//---------------------------------------------------------------------------

void sfxPlayVectorSound(DBloodActor* actor, int vectorId)
{
	const VECTORINFO_EXTRA* pVectorData = &gVectorInfoExtra[vectorId];
	sfxPlay3DSound(actor, Chance(0x5000) ? pVectorData->fireSound[0] : pVectorData->fireSound[1], -1, 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int getSpriteMassBySize(DBloodActor* actor)
{
	int mass = 0;
	int seqId = -1;
	double clipDist = actor->clipdist;
	if (!actor->hasX())
	{
		I_Error("getSpriteMassBySize: actor->spr.hasX == false");
	}
	else if (actor->IsDudeActor())
	{
		switch (actor->spr.type)
		{
		case kDudePodMother: // fake dude, no seq
			break;
		case kDudeModernCustom:
		case kDudeModernCustomBurning:
			seqId = actor->xspr.data2;
			clipDist = actor->genDudeExtra.clipdist;
			break;
		default:
			seqId = getDudeInfo(actor->spr.type)->seqStartID;
			break;
		}
	}
	else
	{
		seqId = seqGetID(actor);
	}

	SPRITEMASS* cached = &actor->spriteMass;
	if (((seqId >= 0 && seqId == cached->seqId) || actor->spr.spritetexture() == cached->texid) && actor->spr.scale.X == cached->scale.X &&
		actor->spr.scale.Y == cached->scale.Y && clipDist == cached->clipDist)
	{
		return cached->mass;
	}

	auto texid = actor->spr.spritetexture();
	int massDiv = 30;
	int addMul = 2;
	int subMul = 2;

	if (seqId >= 0)
	{
		auto pSeq = getSequence(seqId);
		if (pSeq)
		{
			texid = seqGetTexture(&pSeq->frames[0]);
		}
	}
	auto tex = TexMan.GetGameTexture(texid);
	if (!tex) return 0;

	clipDist = max(actor->clipdist, 0.25);
	int x = (int)tex->GetDisplayWidth();
	int y = (int)tex->GetDisplayHeight();
	int xscale = int(actor->spr.scale.X * 64);
	int yscale = int(actor->spr.scale.Y * 64);

	// take surface type into account
	switch (GetExtInfo(actor->spr.spritetexture()).surftype)
	{
	case 1:  massDiv = 16; break; // stone
	case 2:  massDiv = 18; break; // metal
	case 3:  massDiv = 21; break; // wood
	case 4:  massDiv = 25; break; // flesh
	case 5:  massDiv = 28; break; // water
	case 6:  massDiv = 26; break; // dirt
	case 7:  massDiv = 27; break; // clay
	case 8:  massDiv = 35; break; // snow
	case 9:  massDiv = 22; break; // ice
	case 10: massDiv = 37; break; // leaves
	case 11: massDiv = 33; break; // cloth
	case 12: massDiv = 36; break; // plant
	case 13: massDiv = 24; break; // goo
	case 14: massDiv = 23; break; // lava
	}

	mass = ((x + y) * int(clipDist * 2)) / massDiv;

	if (xscale > 64) mass += ((xscale - 64) * addMul);
	else if (xscale < 64 && mass > 0)
	{
		for (int i = 64 - xscale; i > 0; i--)
		{
			if ((mass -= subMul) <= 100 && subMul-- <= 1)
			{
				mass -= i;
				break;
			}
		}
	}

	if (yscale > 64) mass += ((yscale - 64) * addMul);
	else if (yscale < 64 && mass > 0)
	{
		for (int i = 64 - yscale; i > 0; i--)
		{
			if ((mass -= subMul) <= 100 && subMul-- <= 1)
			{
				mass -= i;
				break;
			}
		}
	}

	if (mass <= 0) cached->mass = 1 + Random(10);
	else cached->mass = ClipRange(mass, 1, 65535);

	cached->airVel = ClipRange(400 - cached->mass, 32, 400);
	cached->fraction = ClipRange(60000 - (cached->mass << 7), 8192, 60000);

	cached->scale.X = actor->spr.scale.X;
	cached->scale.Y = actor->spr.scale.Y;
	cached->texid = actor->spr.spritetexture();
	cached->seqId = seqId;
	cached->clipDist = actor->clipdist;

	return cached->mass;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int debrisGetIndex(DBloodActor* actor)
{
	if (!actor->hasX() || actor->xspr.physAttr == 0)
		return -1;

	for (int i = 0; i < gPhysSpritesCount; i++)
	{
		if (gPhysSpritesList[i] != actor) continue;
		return i;
	}

	return -1;
}

int debrisGetFreeIndex(void)
{
	for (int i = 0; i < kMaxSuperXSprites; i++)
	{
		if (gPhysSpritesList[i] == nullptr) return i;
		auto actor = gPhysSpritesList[i];
		if (actor->spr.statnum == kStatFree) return i;

		else if ((actor->spr.flags & kHitagFree) || !gPhysSpritesList[i]->hasX()) return i;
		else if (gPhysSpritesList[i]->xspr.physAttr == 0) return i;
	}

	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void debrisConcuss(DBloodActor* owneractor, int listIndex, const DVector3& pos, int dmg)
{
	DBloodActor* actor = gPhysSpritesList[listIndex];
	if (actor != nullptr && actor->hasX())
	{
		auto dv = actor->spr.pos - pos;

		dmg = int(dmg * (0x4000 / (0x4000 + dv.LengthSquared())));
		bool thing = (actor->spr.type >= kThingBase && actor->spr.type < kThingMax);
		auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
		double size = (tex->GetDisplayWidth() * actor->spr.scale.X * tex->GetDisplayHeight() * actor->spr.scale.Y) * 2048;
		if (actor->xspr.physAttr & kPhysDebrisExplode)
		{
			if (actor->spriteMass.mass > 0)
			{
				double t = double(dmg) * size / actor->spriteMass.mass;

				actor->vel += dv * t / (1 << 24);
			}

			if (thing)
				actor->spr.statnum = kStatThing; // temporary change statnum property
		}

		actDamageSprite(owneractor, actor, kDamageExplode, dmg);

		if (thing)
			actor->spr.statnum = kStatDecoration; // return statnum property back
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void debrisBubble(DBloodActor* actor)
{
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);

	auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
	double nDist = (actor->spr.scale.X * tex->GetDisplayWidth()) * (1. / 2);
	for (unsigned int i = 0; i < 1 + Random(5); i++)
	{
		DAngle nAngle = RandomAngle();
		DVector3 pos(
			actor->spr.pos.XY() + nAngle.ToVector() * nDist,
			bottom - RandomD(bottom - top, 8)
		);
		auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23 + Random(3)), actor->sector(), pos, nullAngle);
		if (pFX) {
			pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
			pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
			pFX->vel.Z = actor->vel.Z + Random2F(0x1aaaa);
		}

	}

	if (Chance(0x2000))
		evPostActor(actor, 0, kCallbackEnemeyBubble);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void debrisMove(int listIndex)
{
	DBloodActor* actor = gPhysSpritesList[listIndex];
	auto pSector = actor->sector();

	if (!actor->hasX() || !pSector)
	{
		gPhysSpritesList[listIndex] = nullptr;
		return;
	}

	double top, bottom;
	GetActorExtents(actor, &top, &bottom);

	Collision moveHit;
	moveHit.setNone();
	double floorDist = (bottom - actor->spr.pos.Z) * 0.25;
	double ceilDist = (actor->spr.pos.Z - top) * 0.25;
	double clipDistf = actor->clipdist;
	int mass = actor->spriteMass.mass;

	bool uwater = false;
	int tmpFraction = actor->spriteMass.fraction;
	if (pSector->hasX() && pSector->xs().Underwater)
	{
		tmpFraction >>= 1;
		uwater = true;
	}

	if (actor->vel.X != 0 || actor->vel.Y != 0)
	{
		auto oldcstat = actor->spr.cstat;
		actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

		ClipMove(actor->spr.pos, &pSector, actor->vel.XY(), clipDistf, ceilDist, floorDist, CLIPMASK0, moveHit);
		actor->hit.hit = moveHit;

		actor->spr.cstat = oldcstat;
		if (actor->sector() != pSector)
		{
			if (!pSector) return;
			else ChangeActorSect(actor, pSector);
		}

		if (pSector->type >= kSectorPath && pSector->type <= kSectorRotate)
		{
			auto pSector2 = pSector;
			if (pushmove(actor->spr.pos, &pSector2, clipDistf, ceilDist, floorDist, CLIPMASK0) != -1)
				pSector = pSector2;
		}

		if (actor->hit.hit.type == kHitWall)
		{
			moveHit = actor->hit.hit;
			actWallBounceVector(actor, moveHit.hitWall, FixedToFloat(tmpFraction));
		}

	}
	else 
	{
		updatesectorz(actor->spr.pos, &pSector);
		if (!pSector) return;
	}

	if (actor->sector() != pSector)
	{
		assert(pSector);
		ChangeActorSect(actor, pSector);
		pSector = actor->sector();
	}

	if (pSector->hasX())
		uwater = pSector->xs().Underwater;

	actor->spr.pos.Z += actor->vel.Z;

	double ceilZ, floorZ;
	Collision ceilColl, floorColl;
	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, clipDistf, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
	GetActorExtents(actor, &top, &bottom);

	if ((actor->xspr.physAttr & kPhysDebrisSwim) && uwater)
	{
		double vc = 0;
		double cz = getceilzofslopeptr(pSector, actor->spr.pos);
		double fz = getflorzofslopeptr(pSector, actor->spr.pos);
		double div = max(bottom - top, 1 / 256.);

		if (pSector->lowerLink) cz += (cz < 0) ? 5. : -5.;
		if (top > cz && (!(actor->xspr.physAttr & kPhysDebrisFloat) || fz <= bottom * 4))
			actor->vel.Z -= (bottom - ceilZ) * mass / 64.;

		if (fz < bottom)
			vc = 0.888888 + ((bottom - fz) * -1.222222) / div;

		if (vc)
		{
			actor->spr.pos.Z += vc * 2;
			actor->vel.Z = vc;
		}

	}
	else if ((actor->xspr.physAttr & kPhysGravity) && bottom < floorZ)
	{
		actor->spr.pos.Z += 1.777;
		actor->vel.Z += 0.888888;
	}

	int i;
	if ((i = CheckLink(actor)) != 0)
	{
		GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, clipDistf, CLIPMASK0, PARALLAXCLIP_CEILING | PARALLAXCLIP_FLOOR);
		if (!(actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
		{
			switch (i)
			{
			case kMarkerUpWater:
			case kMarkerUpGoo:
				int pitch = (150000 - (actor->spriteMass.mass << 9)) + Random3(8192);
				sfxPlay3DSoundVolume(actor, 720, -1, 0, pitch, 75 - Random(40));
				if (!spriteIsUnderwater(actor))
				{
					evKillActor(actor, kCallbackEnemeyBubble);
				}
				else
				{
					evPostActor(actor, 0, kCallbackEnemeyBubble);
					for (int ii = 2; ii <= 5; ii++)
					{
						if (Chance(0x5000 * ii))
							evPostActor(actor, Random(5), kCallbackEnemeyBubble);
					}
				}
				break;
			}
		}
	}

	GetActorExtents(actor, &top, &bottom);

	if (floorZ <= bottom) {

		actor->hit.florhit = floorColl;
		double veldiff = actor->vel.Z - actor->sector()->velFloor;

		if (veldiff > 0)
		{
			actor->xspr.physAttr |= kPhysFalling;
			auto vec4 = actFloorBounceVector(actor, veldiff, actor->sector(), FixedToFloat(tmpFraction));
			actor->vel = vec4.XYZ();
			veldiff = actor->vel.Z;

			if (abs(actor->vel.Z) < 1)
			{
				actor->vel.Z = actor->sector()->velFloor;
				actor->xspr.physAttr &= ~kPhysFalling;
			}

			moveHit = floorColl;
			DBloodActor* pFX = NULL, * pFX2 = NULL;
			switch (tileGetSurfType(floorColl))
			{
			case kSurfLava:
				if ((pFX = gFX.fxSpawnActor(FX_10, actor->sector(), DVector3(actor->spr.pos.XY(), floorZ))) == NULL) break;
				for (i = 0; i < 7; i++)
				{
					if ((pFX2 = gFX.fxSpawnActor(FX_14, pFX->sector(), pFX->spr.pos)) == NULL) continue;
					pFX2->vel.X = Random2F(0x6aaaa);
					pFX2->vel.Y = Random2F(0x6aaaa);
					pFX2->vel.Z = -Random2F(0xd5555);
				}
				break;
			case kSurfWater:
				gFX.fxSpawnActor(FX_9, actor->sector(), DVector3(actor->spr.pos.XY(), floorZ));
				break;
			}

		}
		else if (actor->vel.Z == 0)
		{
			actor->xspr.physAttr &= ~kPhysFalling;
		}
	}
	else
	{
		actor->hit.florhit.setNone();
		if (actor->xspr.physAttr & kPhysGravity)
			actor->xspr.physAttr |= kPhysFalling;
	}

	if (top <= ceilZ)
	{
		actor->hit.ceilhit = moveHit = ceilColl;
		actor->spr.pos.Z += max(ceilZ - top, 0.);
		if (actor->vel.Z <= 0 && (actor->xspr.physAttr & kPhysFalling))
			actor->vel.Z *= 0.875;

	}
	else
	{
		actor->hit.ceilhit.setNone();
		GetActorExtents(actor, &top, &bottom);
	}

	if (moveHit.type != kHitNone && actor->xspr.Impact && !actor->xspr.locked && !actor->xspr.isTriggered && (actor->xspr.state == actor->xspr.restState || actor->xspr.Interrutable)) {
		if (actor->spr.type >= kThingBase && actor->spr.type < kThingMax)
			ChangeActorStat(actor, kStatThing);

		trTriggerSprite(actor, kCmdToggle, actor);
	}

	if (actor->vel.X == 0 && actor->vel.Y == 0) return;
	else if (floorColl.type == kHitSprite)
	{

		if ((floorColl.actor()->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
		{
			actor->vel.XY() += (actor->spr.pos.XY() - floorColl.actor()->spr.pos.XY()) / 4096.;
			return;
		}
	}

	actor->xspr.height = int(max(floorZ - bottom, 0.));
	if (uwater || actor->xspr.height >= 0x100)
		return;

	double nDrag = 0.1640625;
	if (actor->xspr.height > 0)
		nDrag *= 1 - actor->xspr.height / 256.;

	actor->vel.XY() *= 1 - nDrag;
	if (actor->vel.XY().LengthSquared() < 1 / 256.)
		actor->vel.XY().Zero();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilIsTooLow(DBloodActor* actor)
{
	if (actor != nullptr)
	{
		sectortype* pSector = actor->sector();
		double a = pSector->ceilingz - pSector->floorz;
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (a > top - bottom) return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiSetGenIdleState(DBloodActor* actor)
{
	switch (actor->spr.type)
	{
	case kDudeModernCustom:
	case kDudeModernCustomBurning:
		aiGenDudeNewState(actor, &genIdle);
		break;
	default:
		aiNewState(actor, &genIdle);
		break;
	}
}

//---------------------------------------------------------------------------
//
// this function stops wind on all TX sectors affected by WindGen after it goes off state.
//
//---------------------------------------------------------------------------

void windGenStopWindOnSectors(DBloodActor* sourceactor)
{
	if (sourceactor->xspr.txID <= 0 && sourceactor->sector()->hasX())
	{
		sourceactor->sector()->xs().windVel = 0;
		return;
	}

	for (int i = bucketHead[sourceactor->xspr.txID]; i < bucketHead[sourceactor->xspr.txID + 1]; i++)
	{
		if (!rxBucket[i].isSector()) continue;
		auto pSector = rxBucket[i].sector();
		XSECTOR* pXSector = &pSector->xs();

		if ((pXSector->state == 1 && !pXSector->windAlways)
			|| ((sourceactor->spr.flags & kModernTypeFlag1) && !(sourceactor->spr.flags & kModernTypeFlag2)))
		{
			pXSector->windVel = 0;
		}
	}

	// check redirected TX buckets
	int rx = -1;
	DBloodActor* pXRedir = nullptr;
	while ((pXRedir = evrListRedirectors(OBJ_SPRITE, nullptr, nullptr, sourceactor, pXRedir, &rx)) != nullptr)
	{
		for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
		{
			if (!rxBucket[i].isSector()) continue;
			auto pSector = rxBucket[i].sector();
			XSECTOR* pXSector = &pSector->xs();

			if ((pXSector->state == 1 && !pXSector->windAlways) || (sourceactor->spr.flags & kModernTypeFlag2))
				pXSector->windVel = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlStartScene(DBloodActor* sourceactor, DBloodPlayer* pPlayer, bool force)
{
	TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->pnum];

	if (pCtrl->qavScene.initiator != nullptr && !force) return;

	QAV* pQav = playerQavSceneLoad(sourceactor->xspr.data2);
	if (pQav != nullptr)
	{
		// save current weapon
		sourceactor->xspr.dropMsg = pPlayer->curWeapon;

		auto initiator = pCtrl->qavScene.initiator;
		if (initiator != nullptr && initiator != sourceactor && initiator->hasX())
			sourceactor->xspr.dropMsg = initiator->xspr.dropMsg;

		if (initiator == nullptr)
			WeaponLower(pPlayer);

		sourceactor->xspr.sysData1 = ClipLow((pQav->duration * sourceactor->xspr.waitTime) / 4, 0); // how many times animation should be played

		pCtrl->qavScene.initiator = sourceactor;
		pCtrl->qavScene.qavResrc = pQav;
		pCtrl->qavScene.dummy = -1;

		//pCtrl->qavScene.qavResrc->Preload();

		pPlayer->sceneQav = sourceactor->xspr.data2;
		pPlayer->weaponTimer = pCtrl->qavScene.qavResrc->duration;
		pPlayer->qavCallback = (sourceactor->xspr.data3 > 0) ? ClipRange(sourceactor->xspr.data3 - 1, 0, 32) : -1;
		pPlayer->qavLoop = false;
		pPlayer->qavLastTick = I_GetTime(pCtrl->qavScene.qavResrc->ticrate);
		pPlayer->qavTimer = pCtrl->qavScene.qavResrc->duration;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlStopScene(DBloodPlayer* pPlayer)
{
	TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->pnum];
	auto initiator = pCtrl->qavScene.initiator;
	if (initiator->hasX())
	{
		initiator->xspr.sysData1 = 0;
	}

	if (pCtrl->qavScene.initiator != nullptr)
	{
		pCtrl->qavScene.initiator = nullptr;
		pCtrl->qavScene.qavResrc = nullptr;
		pPlayer->sceneQav = -1;

		// restore weapon
		if (pPlayer->GetActor()->xspr.health > 0)
		{
			int oldWeapon = (initiator->hasX() && initiator->xspr.dropMsg != 0) ? initiator->xspr.dropMsg : 1;
			pPlayer->newWeapon = pPlayer->curWeapon = oldWeapon;
			WeaponRaise(pPlayer);
		}
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlLink(DBloodActor* sourceactor, DBloodPlayer* pPlayer, bool checkCondition)
{
	// save player's sprite index to let the tracking condition know it after savegame loading...
	auto actor = pPlayer->GetActor();
	sourceactor->prevmarker = actor;

	actor->xspr.txID = sourceactor->xspr.txID;
	actor->xspr.command = kCmdToggle;
	actor->xspr.triggerOn = sourceactor->xspr.triggerOn;
	actor->xspr.triggerOff = sourceactor->xspr.triggerOff;
	actor->xspr.busyTime = sourceactor->xspr.busyTime;
	actor->xspr.waitTime = sourceactor->xspr.waitTime;
	actor->xspr.restState = sourceactor->xspr.restState;

	actor->xspr.Push = sourceactor->xspr.Push;
	actor->xspr.Impact = sourceactor->xspr.Impact;
	actor->xspr.Vector = sourceactor->xspr.Vector;
	actor->xspr.Touch = sourceactor->xspr.Touch;
	actor->xspr.Sight = sourceactor->xspr.Sight;
	actor->xspr.Proximity = sourceactor->xspr.Proximity;

	actor->xspr.Decoupled = sourceactor->xspr.Decoupled;
	actor->xspr.Interrutable = sourceactor->xspr.Interrutable;
	actor->xspr.DudeLockout = sourceactor->xspr.DudeLockout;

	actor->xspr.data1 = sourceactor->xspr.data1;
	actor->xspr.data2 = sourceactor->xspr.data2;
	actor->xspr.data3 = sourceactor->xspr.data3;
	actor->xspr.data4 = sourceactor->xspr.data4;

	actor->xspr.key = sourceactor->xspr.key;
	actor->xspr.dropMsg = sourceactor->xspr.dropMsg;

	// let's check if there is tracking condition expecting objects with this TX id
	if (checkCondition && sourceactor->xspr.txID >= kChannelUser)
	{
		for (unsigned i = 0; i < gConditions.Size(); i++)
		{
			TRCONDITION* pCond = &gConditions[i];
			if (pCond->actor->xspr.rxID != sourceactor->xspr.txID)
				continue;

			// search for player control sprite and replace it with actual player sprite
			for (unsigned k = 0; k < pCond->objects.Size(); k++)
			{
				if (!pCond->objects[k].obj.isActor() || pCond->objects[k].obj.actor() != sourceactor) continue;
				pCond->objects[k].obj = EventObject(pPlayer->GetActor());
				pCond->objects[k].cmd = (uint8_t)pPlayer->GetActor()->xspr.command;
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlSetRace(int value, DBloodPlayer* pPlayer)
{
	playerSetRace(pPlayer, value);
	switch (pPlayer->lifeMode)
	{
	case kModeHuman:
	case kModeBeast:
		playerSizeReset(pPlayer);
		break;
	case kModeHumanShrink:
		playerSizeShrink(pPlayer, 2);
		break;
	case kModeHumanGrown:
		playerSizeGrow(pPlayer, 2);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlSetMoveSpeed(int value, DBloodPlayer* pPlayer)
{
	int speed = ClipRange(value, 0, 500);
	for (int i = 0; i < kModeMax; i++)
	{
		for (int a = 0; a < kPostureMax; a++)
		{
			POSTURE* curPosture = &pPlayer->pPosture[i][a]; POSTURE* defPosture = &gPostureDefaults[i][a];
			curPosture->frontAccel = (defPosture->frontAccel * speed) / kPercFull;
			curPosture->sideAccel = (defPosture->sideAccel * speed) / kPercFull;
			curPosture->backAccel = (defPosture->backAccel * speed) / kPercFull;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlSetJumpHeight(int value, DBloodPlayer* pPlayer)
{
	int jump = ClipRange(value, 0, 500);
	for (int i = 0; i < kModeMax; i++)
	{
		POSTURE* curPosture = &pPlayer->pPosture[i][kPostureStand]; POSTURE* defPosture = &gPostureDefaults[i][kPostureStand];
		curPosture->normalJumpZ = (defPosture->normalJumpZ * jump) / kPercFull;
		curPosture->pwupJumpZ = (defPosture->pwupJumpZ * jump) / kPercFull;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlSetScreenEffect(int value, int timeval, DBloodPlayer* pPlayer)
{
	int eff = ClipLow(value, 0);
	int time = (eff > 0) ? timeval : 0;
	switch (eff) {
	case 0: // clear all
	case 1: // tilting
		pPlayer->tiltEffect = ClipRange(time, 0, 220);
		if (eff) break;
		[[fallthrough]];
	case 2: // pain
		pPlayer->painEffect = ClipRange(time, 0, 2048);
		if (eff) break;
		[[fallthrough]];
	case 3: // blind
		pPlayer->blindEffect = ClipRange(time, 0, 2048);
		if (eff) break;
		[[fallthrough]];
	case 4: // pickup
		pPlayer->pickupEffect = ClipRange(time, 0, 2048);
		if (eff) break;
		[[fallthrough]];
	case 5: // quakeEffect
		pPlayer->quakeEffect = ClipRange(time, 0, 2048);
		if (eff) break;
		[[fallthrough]];
	case 6: // visibility
		pPlayer->visibility = ClipRange(time, 0, 2048);
		if (eff) break;
		[[fallthrough]];
	case 7: // delirium
		pPlayer->pwUpTime[kPwUpDeliriumShroom] = ClipHigh(time << 1, 432000);
		break;
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlSetLookAngle(int value, DBloodPlayer* pPlayer)
{
	static constexpr double upAngle = 289;
	static constexpr double downAngle = -347;
	static constexpr double lookStepUp = 4.0 * upAngle / 60.0;
	static constexpr double lookStepDown = -4.0 * downAngle / 60.0;

	if (const double adjustment = clamp(value * 0.125 * (value > 0 ? lookStepUp : lookStepDown), downAngle, upAngle))
	{
		gameInput.ForceInputSync(pPlayer->pnum);
		pPlayer->GetActor()->spr.Angles.Pitch = maphoriz(-100. * tan(adjustment * pi::pi() * (1. / 1024.)));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlEraseStuff(int value, DBloodPlayer* pPlayer)
{

	switch (value)
	{
	case 0: // erase all
		[[fallthrough]];
	case 1: // erase weapons
		WeaponLower(pPlayer);

		for (int i = 0; i < 14; i++) {
			pPlayer->hasWeapon[i] = false;
			// also erase ammo
			if (i < 12) pPlayer->ammoCount[i] = 0;
		}

		pPlayer->hasWeapon[kWeapPitchFork] = true;
		pPlayer->curWeapon = kWeapNone;
		pPlayer->nextWeapon = kWeapPitchFork;

		WeaponRaise(pPlayer);
		if (value) break;
		[[fallthrough]];
	case 2: // erase all armor
		for (int i = 0; i < 3; i++) pPlayer->armor[i] = 0;
		if (value) break;
		[[fallthrough]];
	case 3: // erase all pack items
		for (int i = 0; i < 5; i++) {
			pPlayer->packSlots[i].isActive = false;
			pPlayer->packSlots[i].curAmount = 0;
		}
		pPlayer->packItemId = -1;
		if (value) break;
		[[fallthrough]];
	case 4: // erase all keys
		for (int i = 0; i < 8; i++) pPlayer->hasKey[i] = false;
		if (value) break;
		[[fallthrough]];
	case 5: // erase powerups
		for (int i = 0; i < kMaxPowerUps; i++) pPlayer->pwUpTime[i] = 0;
		break;
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlGiveStuff(int data2, int weapon, int data4, DBloodPlayer* pPlayer, TRPLAYERCTRL* pCtrl)
{
	switch (data2)
	{
	case 1: // give N weapon and default ammo for it
	case 2: // give just N ammo for selected weapon
		if (weapon <= 0 || weapon > 13)
		{
			Printf(PRINT_HIGH, "Weapon #%d is out of a weapons range!", weapon);
			break;
		}
		else if (data2 == 2 && data4 == 0)
		{
			Printf(PRINT_HIGH, "Zero ammo for weapon #%d is specified!", weapon);
			break;
		}
		switch (weapon)
		{
		case kWeapProximity: // remote bomb 
		case kWeapRemote: // prox bomb
			pPlayer->hasWeapon[weapon] = true;
			weapon--;
			pPlayer->ammoCount[weapon] = ClipHigh(pPlayer->ammoCount[weapon] + ((data2 == 2) ? data4 : 1), gAmmoInfo[weapon].max);
			weapon++;
			break;
		default:
			for (int i = 0; i < 11; i++)
			{
				if (gWeaponItemData[i].type != weapon) continue;

				const WEAPONITEMDATA* pWeaponData = &gWeaponItemData[i];
				int nAmmoType = pWeaponData->ammoType;
				switch (data2) {
				case 1:
					pPlayer->hasWeapon[weapon] = true;
					if (pPlayer->ammoCount[nAmmoType] >= pWeaponData->count) break;
					pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponData->count, gAmmoInfo[nAmmoType].max);
					break;
				case 2:
					pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + data4, gAmmoInfo[nAmmoType].max);
					break;
				}
				break;
			}
			break;
		}
		if (pPlayer->hasWeapon[weapon] && data4 == 0) // switch on it
		{
			pPlayer->nextWeapon = kWeapNone;

			if (pPlayer->sceneQav >= 0 && pCtrl->qavScene.initiator && pCtrl->qavScene.initiator->hasX())
			{
				pCtrl->qavScene.initiator->xspr.dropMsg = weapon;
			}
			else if (pPlayer->curWeapon != weapon)
			{
				pPlayer->newWeapon = weapon;
				WeaponRaise(pPlayer);
			}
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlUsePackItem(int data2, int data3, int data4, DBloodPlayer* pPlayer, int evCmd)
{
	unsigned int invItem = data2 - 1;
	switch (evCmd)
	{
	case kCmdOn:
		if (!pPlayer->packSlots[invItem].isActive) packUseItem(pPlayer, invItem);
		break;
	case kCmdOff:
		if (pPlayer->packSlots[invItem].isActive) packUseItem(pPlayer, invItem);
		break;
	default:
		packUseItem(pPlayer, invItem);
		break;
	}

	switch (data4)
	{
	case 2: // both
	case 0: // switch on it
		if (pPlayer->packSlots[invItem].curAmount > 0) pPlayer->packItemId = invItem;
		if (!data4) break;
		[[fallthrough]];
	case 1: // force remove after use
		pPlayer->packSlots[invItem].isActive = false;
		pPlayer->packSlots[invItem].curAmount = 0;
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trPlayerCtrlUsePowerup(DBloodActor* sourceactor, DBloodPlayer* pPlayer, int evCmd)
{

	bool relative = (sourceactor->spr.flags & kModernTypeFlag1);

	int nPower = (kMinAllowedPowerup + sourceactor->xspr.data2) - 1;
	int nTime = ClipRange(abs(sourceactor->xspr.data3) * 100, -gPowerUpInfo[nPower].maxTime, gPowerUpInfo[nPower].maxTime);
	if (sourceactor->xspr.data3 < 0)
		nTime = -nTime;

	if (pPlayer->pwUpTime[nPower])
	{
		if (!relative && nTime <= 0)
			powerupDeactivate(pPlayer, nPower);
	}

	if (nTime != 0)
	{
		if (pPlayer->pwUpTime[nPower] <= 0)
			powerupActivate(pPlayer, nPower);  // MUST activate first for powerups like kPwUpDeathMask

		// ...so we able to change time amount
		if (relative) pPlayer->pwUpTime[nPower] += nTime;
		else pPlayer->pwUpTime[nPower] = nTime;
	}

	if (pPlayer->pwUpTime[nPower] <= 0)
		powerupDeactivate(pPlayer, nPower);

	return;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useObjResizer(DBloodActor* sourceactor, int targType, sectortype* targSect, walltype* targWall, DBloodActor* targetactor)
{
	switch (targType)
	{
		// for sectors
	case OBJ_SECTOR:
		if (!targSect) return;
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			targSect->floorxpan_ = (float)ClipRange(sourceactor->xspr.data1, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			targSect->floorypan_ = (float)ClipRange(sourceactor->xspr.data2, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			targSect->ceilingxpan_ = (float)ClipRange(sourceactor->xspr.data3, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
			targSect->ceilingypan_ = (float)ClipRange(sourceactor->xspr.data4, 0, 255);
		break;
		// for sprites
	case OBJ_SPRITE:
	{
		bool fit = false;
		// resize by seq scaling
		if (sourceactor->spr.flags & kModernTypeFlag1)
		{
			if (valueIsBetween(sourceactor->xspr.data1, -255, 32767))
			{
				int mulDiv = (valueIsBetween(sourceactor->xspr.data2, 0, 257)) ? sourceactor->xspr.data2 : 256;
				if (sourceactor->xspr.data1 > 0) targetactor->xspr.scale = mulDiv * ClipHigh(sourceactor->xspr.data1, 25);
				else if (sourceactor->xspr.data1 < 0) targetactor->xspr.scale = mulDiv / ClipHigh(abs(sourceactor->xspr.data1), 25);
				else targetactor->xspr.scale = 0;
				fit = true;
			}

			// resize by repeats
		}
		else
		{
			if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			{
				targetactor->spr.scale.X = (ClipRange(sourceactor->xspr.data1, 0, 255) * REPEAT_SCALE);
				fit = true;
			}

			if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			{
				targetactor->spr.scale.Y = (ClipRange(sourceactor->xspr.data2, 0, 255) * REPEAT_SCALE);
				fit = true;
			}
		}

		if (fit && (targetactor->spr.type == kDudeModernCustom || targetactor->spr.type == kDudeModernCustomBurning))
		{
			// request properties update for custom dude

			targetactor->genDudeExtra.updReq[kGenDudePropertySpriteSize] = true;
			targetactor->genDudeExtra.updReq[kGenDudePropertyAttack] = true;
			targetactor->genDudeExtra.updReq[kGenDudePropertyMass] = true;
			targetactor->genDudeExtra.updReq[kGenDudePropertyDmgScale] = true;
			evPostActor(targetactor, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);

		}

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			targetactor->spr.xoffset = ClipRange(sourceactor->xspr.data3, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
			targetactor->spr.yoffset = ClipRange(sourceactor->xspr.data4, 0, 255);
		break;
	}
	case OBJ_WALL:
		if (!targWall) return;
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			targWall->xrepeat = ClipRange(sourceactor->xspr.data1, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			targWall->yrepeat = ClipRange(sourceactor->xspr.data2, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			targWall->xpan_ = (float)ClipRange(sourceactor->xspr.data3, 0, 255);

		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
			targWall->ypan_ = (float)ClipRange(sourceactor->xspr.data4, 0, 255);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void usePropertiesChanger(DBloodActor* sourceactor, int objType, sectortype* pSector, walltype* pWall, DBloodActor* targetactor)
{
	bool flag1 = (sourceactor->spr.flags & kModernTypeFlag1);
	bool flag2 = (sourceactor->spr.flags & kModernTypeFlag2);
	bool flag4 = (sourceactor->spr.flags & kModernTypeFlag4);
	short data3 = sourceactor->xspr.data3;
	short data4 = sourceactor->xspr.data4;

	switch (objType)
	{
	case OBJ_WALL:
	{
		if (!pWall) return;

		// data1 = set this wall as first wall of sector or as sector.alignto wall
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
		{
			if (pWall->sectorp())
			{
				switch (sourceactor->xspr.data1) {
					case 1:
						// You got to be f***ing kidding...
						//setfirstwall(nSector, objIndex);
						Printf("Setting first wall not supported!"); // this will require a lot of mess...
						pWall->allocX();
						break;
					case 2:
						pWall->sectorp()->slopewallofs = max<int>(int(pWall - pWall->sectorp()->walls.Data()), 0);
						break;
				}
			}
		}

		// data3 = set wall hitag
		if (valueIsBetween(data3, -1, 32767))
		{
			if (flag1)
			{
				if (flag4) pWall->hitag &= ~data3;
				else pWall->hitag |= data3;

				pWall->hitag |= sourceactor->xspr.data3;
			}
			else pWall->hitag = sourceactor->xspr.data3;
		}

		// data4 = set wall cstat
		if (valueIsBetween(data4, -1, 65535))
		{
			auto old = pWall->cstat;
			// relative
			if (flag1)
			{
				if (flag4) pWall->cstat &= ~EWallFlags::FromInt(data4);
				else pWall->cstat |= EWallFlags::FromInt(data4);
			}
			// absolute
			else
			{
				pWall->cstat = EWallFlags::FromInt(data4);
				if (!flag2)
				{
					// check for exceptions
					pWall->cstat |= old & (CSTAT_WALL_BOTTOM_SWAP | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_1WAY);
					pWall->cstat = (pWall->cstat & ~CSTAT_WALL_MOVE_MASK) | (old & CSTAT_WALL_MOVE_MASK);
				}
			}
		}
	}
	break;
	case OBJ_SPRITE:
	{
		bool thing2debris = false;
		int old = -1;

		// data3 = set sprite hitag
		if (valueIsBetween(data3, -1, 32767))
		{
			old = targetactor->spr.flags;
			// relative
			if (flag1)
			{
				if (flag4) targetactor->spr.flags &= ~data3;
				else targetactor->spr.flags |= data3;
			}
			// absolute
			else
			{
				targetactor->spr.flags = data3;
			}
			
			// and handle exceptions
			if ((old & kHitagFree) && !(targetactor->spr.flags & kHitagFree)) targetactor->spr.flags |= kHitagFree;
			if ((old & kHitagRespawn) && !(targetactor->spr.flags & kHitagRespawn)) targetactor->spr.flags |= kHitagRespawn;

			// prepare things for different (debris) physics.
			thing2debris = (targetactor->spr.statnum == kStatThing && debrisGetFreeIndex() >= 0);
		}


		// data2 = sprite physics settings
		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767) || thing2debris)
		{
			switch (targetactor->spr.statnum)
			{
			case kStatDude: // dudes already treating in game
			case kStatFree:
			case kStatMarker:
			case kStatPathMarker:
				break;
			default:
				// store physics attributes in xsprite to avoid setting hitag for modern types!
				int flags = (targetactor->xspr.physAttr != 0) ? targetactor->xspr.physAttr : 0;
				int oldFlags = flags;

				if (thing2debris)
				{
					// converting thing to debris
					if ((targetactor->spr.flags & kPhysMove) != 0) flags |= kPhysMove;
					else flags &= ~kPhysMove;

					if ((targetactor->spr.flags & kPhysGravity) != 0) flags |= (kPhysGravity | kPhysFalling);
					else flags &= ~(kPhysGravity | kPhysFalling);

					targetactor->spr.flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
					targetactor->vel.Zero();
					targetactor->xspr.restState = targetactor->xspr.state;

				}
				else
				{
					// WTF is this?!?
					char digits[6] = {};
					snprintf(digits, 6, "%d", sourceactor->xspr.data2);
					for (unsigned int i = 0; i < sizeof(digits); i++)
						digits[i] = (digits[i] >= 48 && digits[i] <= 57) ? (digits[i] - 57) + 9 : 0;

					// first digit of data2: set main physics attributes
					switch (digits[0])
					{
					case 0:
						flags &= ~kPhysMove;
						flags &= ~(kPhysGravity | kPhysFalling);
						break;
					case 1:
						flags |= kPhysMove;
						flags &= ~(kPhysGravity | kPhysFalling);
						break;
					case 2:
						flags &= ~kPhysMove;
						flags |= (kPhysGravity | kPhysFalling);
						break;
					case 3:
						flags |= kPhysMove;
						flags |= (kPhysGravity | kPhysFalling);
						break;
					}

					// second digit of data2: touch physics flags
					switch (digits[1])
					{
					case 0:
						flags &= ~kPhysDebrisTouch;
						break;
					case 1:
						flags |= kPhysDebrisTouch;
						break;
					}

					// third digit of data2: weapon physics flags
					switch (digits[2])
					{
					case 0:
						flags &= ~kPhysDebrisVector;
						flags &= ~kPhysDebrisExplode;
						break;
					case 1:
						flags |= kPhysDebrisVector;
						flags &= ~kPhysDebrisExplode;
						break;
					case 2:
						flags &= ~kPhysDebrisVector;
						flags |= kPhysDebrisExplode;
						break;
					case 3:
						flags |= kPhysDebrisVector;
						flags |= kPhysDebrisExplode;
						break;
					}

					// fourth digit of data2: swimming / flying physics flags
					switch (digits[3])
					{
					case 0:
						flags &= ~kPhysDebrisSwim;
						flags &= ~kPhysDebrisFly;
						flags &= ~kPhysDebrisFloat;
						break;
					case 1:
						flags |= kPhysDebrisSwim;
						flags &= ~kPhysDebrisFly;
						flags &= ~kPhysDebrisFloat;
						break;
					case 2:
						flags |= kPhysDebrisSwim;
						flags |= kPhysDebrisFloat;
						flags &= ~kPhysDebrisFly;
						break;
					case 3:
						flags |= kPhysDebrisFly;
						flags &= ~kPhysDebrisSwim;
						flags &= ~kPhysDebrisFloat;
						break;
					case 4:
						flags |= kPhysDebrisFly;
						flags |= kPhysDebrisFloat;
						flags &= ~kPhysDebrisSwim;
						break;
					case 5:
						flags |= kPhysDebrisSwim;
						flags |= kPhysDebrisFly;
						flags &= ~kPhysDebrisFloat;
						break;
					case 6:
						flags |= kPhysDebrisSwim;
						flags |= kPhysDebrisFly;
						flags |= kPhysDebrisFloat;
						break;
					}
				}

				int nIndex = debrisGetIndex(targetactor); // check if there is no sprite in list

				// adding physics sprite in list
				if ((flags & kPhysGravity) != 0 || (flags & kPhysMove) != 0)
				{

					if (oldFlags == 0)
						targetactor->vel.Zero();

					if (nIndex != -1)
					{
						targetactor->xspr.physAttr = flags; // just update physics attributes
					}
					else if ((nIndex = debrisGetFreeIndex()) < 0)
					{
						viewSetSystemMessage("Max (%d) Physics affected sprites reached!", kMaxSuperXSprites);
					}
					else
					{
						targetactor->xspr.physAttr = flags; // update physics attributes

						// allow things to became debris, so they use different physics...
						if (targetactor->spr.statnum == kStatThing) ChangeActorStat(targetactor, 0);

						// set random goal ang for swimming so they start turning
						if ((flags & kPhysDebrisSwim) && targetactor->vel.isZero())
							targetactor->xspr.goalAng = (targetactor->spr.Angles.Yaw + DAngle::fromBuild(Random3(kAng45))).Normalized360();

						if (targetactor->xspr.physAttr & kPhysDebrisVector)
							targetactor->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;

						gPhysSpritesList[nIndex] = targetactor;
						if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
						getSpriteMassBySize(targetactor); // create physics cache

					}

					// removing physics from sprite in list (don't remove sprite from list)
				}
				else if (nIndex != -1)
				{

					targetactor->xspr.physAttr = flags;
					targetactor->vel.Zero();
					if (targetactor->spr.lotag >= kThingBase && targetactor->spr.lotag < kThingMax)
						ChangeActorStat(targetactor, kStatThing);  // if it was a thing - restore statnum
				}
				break;
			}
		}

		// data4 = sprite cstat
		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
		{
			// relative
			if (flag1)
			{
				if (flag4) targetactor->spr.cstat &= ~ESpriteFlags::FromInt(data4);
				else targetactor->spr.cstat |= ESpriteFlags::FromInt(data4);
			}
			// absolute
			else
			{
				auto oldstat = targetactor->spr.cstat;
				
				// set new cstat
				if ((sourceactor->spr.flags & kModernTypeFlag1)) targetactor->spr.cstat |= ESpriteFlags::FromInt(sourceactor->xspr.data4); // relative
				else targetactor->spr.cstat = ESpriteFlags::FromInt(sourceactor->xspr.data4 & 0xffff); // absolute
				
				// and handle exceptions
				if ((oldstat & CSTAT_SPRITE_BLOOD_BIT1)) targetactor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1; //kSpritePushable
				if ((oldstat & CSTAT_SPRITE_YCENTER)) targetactor->spr.cstat |= CSTAT_SPRITE_YCENTER;
				
				targetactor->spr.cstat = (targetactor->spr.cstat & ~CSTAT_SPRITE_MOVE_MASK) | (oldstat & CSTAT_SPRITE_MOVE_MASK);
#if 0
				// looks very broken.
				if (old & 0x6000)
				{
					if (!(targetactor->spr.cstat & 0x6000))
						targetactor->spr.cstat |= 0x6000; // kSpriteMoveMask
					
					if ((old & 0x0) && !(targetactor->spr.cstat & 0x0)) targetactor->spr.cstat |= 0x0; // kSpriteMoveNone
					else if ((old & 0x2000) && !(targetactor->spr.cstat & 0x2000)) targetactor->spr.cstat |= 0x2000; // kSpriteMoveForward, kSpriteMoveFloor
					else if ((old & 0x4000) && !(targetactor->spr.cstat & 0x4000)) targetactor->spr.cstat |= 0x4000; // kSpriteMoveReverse, kSpriteMoveCeiling
				}
#endif
			}
		}
	}
	break;
	case OBJ_SECTOR:
	{
		if (!pSector) return;
		XSECTOR* pXSector = &pSector->xs();

		// data1 = sector underwater status and depth level
		if (sourceactor->xspr.data1 >= 0 && sourceactor->xspr.data1 < 2) {

			pXSector->Underwater = (sourceactor->xspr.data1) ? true : false;


			auto aLower = barrier_cast<DBloodActor*>(pSector->lowerLink);
			DBloodActor* aUpper = nullptr;
			if (aLower)
			{
				// must be sure we found exact same upper link
				for (auto& sec : sector)
				{
					aUpper = barrier_cast<DBloodActor*>(sec.upperLink);
					if (aUpper == nullptr || aUpper->xspr.data1 != aLower->xspr.data1)
					{
						aUpper = nullptr;
						continue;
					}
					break;
				}
			}

			// treat sectors that have links, so warp can detect underwater status properly
			if (aLower)
			{
				if (pXSector->Underwater)
				{
					switch (aLower->spr.type)
					{
					case kMarkerLowStack:
					case kMarkerLowLink:
						aLower->xspr.sysData1 = aLower->spr.type;
						aLower->spr.type = kMarkerLowWater;
						break;
					default:
						if (pSector->portalflags != PORTAL_SECTOR_CEILING) aLower->xspr.sysData1 = kMarkerLowLink;
						else aLower->xspr.sysData1 = kMarkerLowStack;
						break;
					}
				}
				else if (aLower->xspr.sysData1 > 0) aLower->spr.type = aLower->xspr.sysData1;
				else if (pSector->portalflags != PORTAL_SECTOR_CEILING) aLower->spr.type = kMarkerLowLink;
				else aLower->spr.type = kMarkerLowStack;
			}

			if (aUpper)
			{
				if (pXSector->Underwater)
				{
					switch (aUpper->spr.type)
					{
					case kMarkerUpStack:
					case kMarkerUpLink:
						aUpper->xspr.sysData1 = aUpper->spr.type;
						aUpper->spr.type = kMarkerUpWater;
						break;
					default:
						if (pSector->portalflags != PORTAL_SECTOR_FLOOR) aUpper->xspr.sysData1 = kMarkerUpLink;
						else aUpper->xspr.sysData1 = kMarkerUpStack;
						break;
					}
				}
				else if (aUpper->xspr.sysData1 > 0) aUpper->spr.type = aUpper->xspr.sysData1;
				else if (pSector->portalflags != PORTAL_SECTOR_FLOOR) aUpper->spr.type = kMarkerUpLink;
				else aUpper->spr.type = kMarkerUpStack;
			}

			// search for dudes in this sector and change their underwater status
			BloodSectIterator it(pSector);
			while (auto iactor = it.Next())
			{
				if (iactor->spr.statnum != kStatDude || !iactor->IsDudeActor() || !iactor->hasX())
					continue;

				DBloodPlayer* pPlayer = getPlayerById(iactor->spr.type);
				if (pXSector->Underwater)
				{
					if (aLower)
						iactor->xspr.medium = (aLower->spr.type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

					if (pPlayer)
					{
						int waterPal = kMediumWater;
						if (aLower)
						{
							if (aLower->xspr.data2 > 0) waterPal = aLower->xspr.data2;
							else if (aLower->spr.type == kMarkerUpGoo) waterPal = kMediumGoo;
						}

						pPlayer->nWaterPal = waterPal;
						pPlayer->posture = kPostureSwim;
						pPlayer->GetActor()->xspr.burnTime = 0;
					}

				}
				else
				{
					iactor->xspr.medium = kMediumNormal;
					if (pPlayer)
					{
						pPlayer->posture = (!(pPlayer->cmd.ucmd.actions & SB_CROUCH)) ? kPostureStand : kPostureCrouch;
						pPlayer->nWaterPal = 0;
					}

				}
			}
		}
		else if (sourceactor->xspr.data1 > 9) pXSector->Depth = 7;
		else if (sourceactor->xspr.data1 > 1) pXSector->Depth = sourceactor->xspr.data1 - 2;


		// data2 = sector visibility
		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			pSector->visibility = ClipRange(sourceactor->xspr.data2, 0, 234);

	
		// data3 = sector ceil cstat
		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
		{
			// relative
			if (flag1)
			{
				if (flag4) pSector->ceilingstat &= ~ESectorFlags::FromInt(data3);
				else pSector->ceilingstat |= ESectorFlags::FromInt(data3);
			}
			// absolute
			else pSector->ceilingstat = ESectorFlags::FromInt(data3);
		}

		// data4 = sector floor cstat
		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
		{
			// relative
			if (flag1)
			{
				if (flag4) pSector->floorstat &= ~ESectorFlags::FromInt(data4);
				else pSector->floorstat |= ESectorFlags::FromInt(data4);
			}
			// absolute
			else pSector->floorstat = ESectorFlags::FromInt(data4);
		}
	}
	break;
	// no TX id
	case -1:
		// data2 = global visibility
		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			gVisibility = ClipRange(sourceactor->xspr.data2, 0, 4096);
		break;
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useVelocityChanger(DBloodActor* actor, sectortype* sect, DBloodActor* initiator, DBloodActor* pSprite)
//void useVelocityChanger(XSPRITE* pXSource, int causerID, short objType, int objIndex)
{
	const double kVelScale = 1 / 64.; // scaled 8 left, 14 right, i.e. 6 right altogether.
	const int kVelShift = 8;
	const int kScaleVal = 0x10000;

	int r = 0, t = 0;
	DAngle nAng = nullAngle;
	DVector3 vv(0, 0, 0);

	bool relative = (actor->spr.flags & kModernTypeFlag1);
	bool toDstAng = (actor->spr.flags & kModernTypeFlag2);
	bool toSrcAng = (actor->spr.flags & kModernTypeFlag4);
	bool toRndAng = (actor->spr.flags & kModernTypeFlag8);
	bool chgDstAng = !(actor->spr.flags & kModernTypeFlag16);
	bool toEvnAng = (toDstAng && toSrcAng && initiator != NULL);
	bool toAng = (toDstAng || toSrcAng || toEvnAng || toRndAng);
	bool toAng180 = (toRndAng && (toDstAng || toSrcAng || toEvnAng));

	if (actor)
	{
		if ((r = MulScale(actor->xspr.data4 << kVelShift, kScaleVal, 14)) != 0)
			r = nnExtRandom(-r, r);
		double rr = FixedToFloat(r);

		if (valueIsBetween(actor->xspr.data3, -32767, 32767))
		{
			vv.Z = actor->xspr.data3 * kVelScale;
			if (vv.Z != 0) vv.Z += rr;
		}

		if (!toAng)
		{
			if (valueIsBetween(actor->xspr.data1, -32767, 32767))
			{
				vv.X = actor->xspr.data1 * kVelScale;
				if (vv.X != 0) vv.X += rr;
			}

			if (valueIsBetween(actor->xspr.data2, -32767, 32767))
			{
				vv.Y = actor->xspr.data2 * kVelScale;
				if (vv.Y != 0) vv.Y += rr;
			}
		}
		else
		{
			if (toEvnAng)       nAng = initiator->spr.Angles.Yaw;
			else if (toSrcAng)  nAng = actor->spr.Angles.Yaw;
			else                nAng = pSprite->spr.Angles.Yaw;

			if (!toAng180 && toRndAng)
			{
				auto tempang = nAng;
				while (tempang == nAng)
					nAng = RandomAngle();
			}

			if (chgDstAng)
				changeSpriteAngle(pSprite, nAng);

			double v = actor->xspr.data1 * kVelScale;
			if (v != 0) v += rr;

			vv.XY() += nAng.ToVector() * v;
		}

		if (actor->xspr.physAttr)
		{
			t = 1;
			switch (pSprite->spr.statnum) {
			case kStatThing:
				break;
			case kStatFX:
				t = 0;
				[[fallthrough]];
			case kStatDude:
			case kStatProjectile:
				if (actor->xspr.physAttr & kPhysMove)     pSprite->spr.flags |= kPhysMove;     else pSprite->spr.flags &= ~kPhysMove;
				if (actor->xspr.physAttr & kPhysGravity)  pSprite->spr.flags |= kPhysGravity;  else pSprite->spr.flags &= ~kPhysGravity;
				if (actor->xspr.physAttr & kPhysFalling)  pSprite->spr.flags |= kPhysFalling;  else pSprite->spr.flags &= ~kPhysFalling;
				break;
			}

			// debris physics for sprites that is allowed
			if (t && ((t = debrisGetIndex(pSprite)) >= 0 || (t = debrisGetFreeIndex()) >= 0))
			{
				pSprite->addX();

				if (pSprite->spr.statnum == kStatThing)
				{
					pSprite->spr.flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
					ChangeActorStat(pSprite, 0);
				}

				pSprite->xspr.physAttr = actor->xspr.physAttr, gPhysSpritesList[t] = pSprite;

				getSpriteMassBySize(pSprite);
				if (t >= gPhysSpritesCount)
					gPhysSpritesCount++;
			}
		}

		if (relative)
		{
			pSprite->vel += vv;
		}
		else
		{
			pSprite->vel == vv;
		}

		auto vAng = pSprite->vel.Angle();

		if (toAng)
		{
			DAngle angl;
			if (toAng180) angl = DAngle180;
			else angl = nAng - vAng;
			
			auto velv = pSprite->vel.XY();
			auto pt = rotatepoint(pSprite->spr.pos.XY(), velv, angl);
			pSprite->vel.SetXY(pt);


			vAng = pSprite->vel.Angle();
		}

		if (chgDstAng)
			changeSpriteAngle(pSprite, vAng);

		if (pSprite->ownerActor)
		{
			// hack to make player projectiles damage it's owner
			if (pSprite->spr.statnum == kStatProjectile && pSprite->IsPlayerActor())
				pSprite->ownerActor = pSprite;
		}

		viewCorrectPrediction();

		//if (pXSource->rxID == 157)
		//viewSetSystemMessage("%d: %d  /  %d  /  %d, C: %d", pSprite->spr.sectnum, pSprite->vel.X, pSprite->vel.Y, pSprite->vel.Z, sprite[causerID].type);
	}
	else if (sect)
	{
		BloodSectIterator it(sect);
		while(auto act = it.Next())
		{
			useVelocityChanger(actor, nullptr, initiator, act);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useTeleportTarget(DBloodActor* sourceactor, DBloodActor* actor)
{
	DBloodPlayer* pPlayer = getPlayerById(actor->spr.type);
	XSECTOR* pXSector = (sourceactor->sector()->hasX()) ? &sourceactor->sector()->xs() : nullptr;
	bool isDude = (!pPlayer && actor->IsDudeActor());

	if (actor->sector() != sourceactor->sector())
		ChangeActorSect(actor, sourceactor->sector());

	actor->spr.pos.SetXY(sourceactor->spr.pos.XY());
	double zTop, zBot;
	GetActorExtents(sourceactor, &zTop, &zBot);
	actor->spr.pos.Z = zBot;

	clampSprite(actor, 0x01);

	if (sourceactor->spr.flags & kModernTypeFlag1) // force telefrag
		TeleFrag(actor, sourceactor->sector());

	if (actor->spr.flags & kPhysGravity)
		actor->spr.flags |= kPhysFalling;

	if (pXSector)
	{

		if (pXSector->Enter && (pPlayer || (isDude && !pXSector->dudeLockout)))
			trTriggerSector(sourceactor->sector(), kCmdSectorEnter, actor);

		if (pXSector->Underwater)
		{
			DBloodActor* aUpper = nullptr;
			auto aLink = barrier_cast<DBloodActor*>(sourceactor->sector()->lowerLink);
			if (aLink)
			{
				// must be sure we found exact same upper link
				for (auto& sec : sector)
				{
					aUpper = barrier_cast<DBloodActor*>(sec.upperLink);
					if (aUpper == nullptr || aUpper->xspr.data1 != aLink->xspr.data1)
					{
						aUpper = nullptr;
						continue;
					}
					break;
				}
			}

			if (aUpper)
				actor->xspr.medium = (aLink->spr.type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

			if (pPlayer)
			{
				int waterPal = kMediumWater;
				if (aUpper)
				{
					if (aLink->xspr.data2 > 0) waterPal = aLink->xspr.data2;
					else if (aLink->spr.type == kMarkerUpGoo) waterPal = kMediumGoo;
				}

				pPlayer->nWaterPal = waterPal;
				pPlayer->posture = kPostureSwim;
				pPlayer->GetActor()->xspr.burnTime = 0;
			}
		}
		else
		{
			actor->xspr.medium = kMediumNormal;
			if (pPlayer)
			{
				pPlayer->posture = (!(pPlayer->cmd.ucmd.actions & SB_CROUCH)) ? kPostureStand : kPostureCrouch;
				pPlayer->nWaterPal = 0;
			}

		}
	}

	if (actor->spr.statnum == kStatDude && actor->IsDudeActor() && !actor->IsPlayerActor())
	{
		auto pos = actor->xspr.TargetPos;
		auto target = actor->GetTarget();

		aiInitSprite(actor);

		if (target != nullptr && target->IsDudeActor())
		{
			actor->xspr.TargetPos = pos;
			actor->SetTarget(target);
			aiActivateDude(actor);
		}
	}

#if 0
	if (sourceactor->xspr.data2 == 1)
	{
		if (pPlayer)
		{
			pPlayer->angle.settarget(sourceactor->spr.angle);
			pPlayer->angle.lockinput();
		}
		else if (isDude) sourceactor->xspr.goalAng = actor->spr.angle = sourceactor->spr.angle;
		else actor->spr.__int_angle = sourceactor->spr.angle;
	}
#endif

	if (sourceactor->xspr.data3 == 1)
	{
		actor->vel.Zero();
	}
	else if (sourceactor->xspr.data3 > 0)
	{
		// change movement direction according source angle
		if (sourceactor->xspr.data3 & kModernTypeFlag2)
		{
			auto velv = actor->vel.XY();
			auto pt = rotatepoint(actor->spr.pos.XY(), velv, sourceactor->spr.Angles.Yaw - velv.Angle());
			actor->vel.SetXY(pt);
		}

		if (sourceactor->xspr.data3 & kModernTypeFlag4)
			actor->vel.Z = 0;
	}

	if (sourceactor->xspr.data2 == 1)
		changeSpriteAngle(actor, sourceactor->spr.Angles.Yaw);

	viewBackupSpriteLoc(actor);

	if (sourceactor->xspr.data4 > 0)
		sfxPlay3DSound(sourceactor, sourceactor->xspr.data4, -1, 0);

	if (pPlayer)
	{
		playerResetInertia(pPlayer);
		if (sourceactor->xspr.data2 == 1)
			pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useEffectGen(DBloodActor* sourceactor, DBloodActor* actor)
{
	if (!actor) actor = sourceactor;

	int fxId = (sourceactor->xspr.data3 <= 0) ? sourceactor->xspr.data2 : sourceactor->xspr.data2 + Random(sourceactor->xspr.data3 + 1);


	if (!actor->hasX()) return;
	else if (fxId >= kEffectGenCallbackBase)
	{
		int length = sizeof(gEffectGenCallbacks) / sizeof(gEffectGenCallbacks[0]);
		if (fxId < kEffectGenCallbackBase + length)
		{
			fxId = gEffectGenCallbacks[fxId - kEffectGenCallbackBase];
			evKillActor(actor, (CALLBACK_ID)fxId);
			evPostActor(actor, 0, (CALLBACK_ID)fxId);
		}

	}
	else if (valueIsBetween(fxId, 0, kFXMax))
	{
		double pos, top, bottom;
		GetActorExtents(actor, &top, &bottom);
		DBloodActor* pEffect = nullptr;

		// select where exactly effect should be spawned
		switch (sourceactor->xspr.data4)
		{
		case 1:
			pos = bottom;
			break;
		case 2: // middle
		{
			auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
			double center = tex->GetDisplayHeight() / 2 + tex->GetDisplayTopOffset();
			pos = actor->spr.pos.Z + center * actor->spr.scale.Y;
			break;
		}
		case 3:
		case 4:
			if (actor->insector())
			{
				if (sourceactor->xspr.data4 == 3)
					pos = getflorzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
				else
					pos = getceilzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
				break;
			}
			[[fallthrough]];
		default:
			pos = top;
			break;
		}

		if ((pEffect = gFX.fxSpawnActor((FX_ID)fxId, actor->sector(), DVector3(actor->spr.pos.XY(), pos))) != nullptr)
		{
			pEffect->SetOwner(sourceactor);

			if (sourceactor->spr.flags & kModernTypeFlag1)
			{
				pEffect->spr.pal = sourceactor->spr.pal;
				pEffect->spr.xoffset = sourceactor->spr.xoffset;
				pEffect->spr.yoffset = sourceactor->spr.yoffset;
				pEffect->spr.scale = sourceactor->spr.scale;
				pEffect->spr.shade = sourceactor->spr.shade;
			}

			if (sourceactor->spr.flags & kModernTypeFlag2)
			{
				pEffect->spr.cstat = sourceactor->spr.cstat;
				if (pEffect->spr.cstat & CSTAT_SPRITE_INVISIBLE)
					pEffect->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
			}

			if (sourceactor->spr.flags & kModernTypeFlag4)
			{
				pEffect->spr.Angles.Yaw = sourceactor->spr.Angles.Yaw;
			}

			if (pEffect->spr.cstat & CSTAT_SPRITE_ONE_SIDE)
				pEffect->spr.cstat &= ~CSTAT_SPRITE_ONE_SIDE;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useSectorWindGen(DBloodActor* sourceactor, sectortype* pSector)
{
	XSECTOR* pXSector = nullptr;

	if (pSector != nullptr)
	{
		pXSector = &pSector->xs();
	}
	else if (sourceactor->sector()->hasX())
	{
		pSector = sourceactor->sector();
		pXSector = &pSector->xs();
	}
	else
	{
		pSector = sourceactor->sector();
		pSector->allocX();
		pXSector = &pSector->xs();
		pXSector->windAlways = 1;
	}

	int windVel = ClipRange(sourceactor->xspr.data2, 0, 32767);
	if ((sourceactor->xspr.data1 & 0x0001))
		windVel = nnExtRandom(0, windVel);

	// process vertical wind in nnExtProcessSuperSprites();
	if ((sourceactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
	{
		sourceactor->xspr.sysData2 = windVel << 1;
		return;
	}

	pXSector->windVel = windVel;
	if ((sourceactor->spr.flags & kModernTypeFlag1))
		pXSector->panAlways = pXSector->windAlways = 1;

	DAngle angle = sourceactor->spr.Angles.Yaw;
	if (sourceactor->xspr.data4 <= 0)
	{
		if ((sourceactor->xspr.data1 & 0x0002))
		{
			while (sourceactor->spr.Angles.Yaw == angle)
				sourceactor->spr.Angles.Yaw = RandomAngle();
		}
	}
	else if (sourceactor->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD) sourceactor->spr.Angles.Yaw += mapangle(sourceactor->xspr.data4);
	else if (sourceactor->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE) sourceactor->spr.Angles.Yaw -= mapangle(sourceactor->xspr.data4);
	else if (sourceactor->xspr.sysData1 == 0)
	{
		angle += mapangle(sourceactor->xspr.data4);
		if (angle >= DAngle180) sourceactor->xspr.sysData1 = 1;
		sourceactor->spr.Angles.Yaw = min(angle, DAngle180);
	}
	else
	{
		angle -= mapangle(sourceactor->xspr.data4);
		if (angle <= -DAngle180) sourceactor->xspr.sysData1 = 0;
		sourceactor->spr.Angles.Yaw = max(angle, -DAngle180);
	}

	pXSector->windAng = sourceactor->spr.Angles.Yaw;

	if (sourceactor->xspr.data3 > 0 && sourceactor->xspr.data3 < 4)
	{
		switch (sourceactor->xspr.data3)
		{
		case 1:
			pXSector->panFloor = true;
			pXSector->panCeiling = false;
			break;
		case 2:
			pXSector->panFloor = false;
			pXSector->panCeiling = true;
			break;
		case 3:
			pXSector->panFloor = true;
			pXSector->panCeiling = true;
			break;
		}
		if (pXSector->panCeiling)
		{
			StartInterpolation(pSector, Interp_Sect_CeilingPanX);
			StartInterpolation(pSector, Interp_Sect_CeilingPanY);
		}
		if (pXSector->panFloor)
		{
			StartInterpolation(pSector, Interp_Sect_FloorPanX);
			StartInterpolation(pSector, Interp_Sect_FloorPanY);
		}

		int oldPan = pXSector->panVel;
		pXSector->panAngle = pXSector->windAng;
		pXSector->panVel = pXSector->windVel;

		// add to panList if panVel was set to 0 previously
		if (oldPan == 0 && pXSector->panVel != 0)
		{
			if (!panList.Contains(pSector))
				panList.Push(pSector);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useSpriteDamager(DBloodActor* sourceactor, int objType, sectortype* targSect, DBloodActor* targetactor)
{
	sectortype* pSector = sourceactor->sector();

	double top, bottom;
	bool floor, ceil, wall, enter;

	switch (objType)
	{
	case OBJ_SPRITE:
		damageSprites(sourceactor, targetactor);
		break;
	case OBJ_SECTOR:
	{
		GetActorExtents(sourceactor, &top, &bottom);
		floor = (bottom >= pSector->floorz);
		ceil = (top <= pSector->ceilingz);
		wall = (sourceactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL);
		enter = (!floor && !ceil && !wall);
		BloodSectIterator it(targSect);
		while (auto iactor = it.Next())
		{
			auto& hit = iactor->hit;

			if (!iactor->IsDudeActor() || !iactor->hasX())
				continue;
			else if (enter)
				damageSprites(sourceactor, iactor);
			else if (floor && hit.florhit.type == kHitSector && hit.florhit.hitSector == targSect)
				damageSprites(sourceactor, iactor);
			else if (ceil && hit.ceilhit.type == kHitSector && hit.ceilhit.hitSector == targSect)
				damageSprites(sourceactor, iactor);
			else if (wall && hit.hit.type == kHitWall && hit.hit.hitWall->sectorp() == targSect)
				damageSprites(sourceactor, iactor);
		}
		break;
	}
	case -1:
	{
		BloodStatIterator it(kStatDude);
		while (auto iactor = it.Next())
		{
			if (iactor->spr.statnum != kStatDude) continue;
			switch (sourceactor->xspr.data1)
			{
			case 667:
				if (iactor->IsPlayerActor()) continue;
				damageSprites(sourceactor, iactor);
				break;
			case 668:
				if (iactor->IsPlayerActor()) continue;
				damageSprites(sourceactor, iactor);
				break;
			default:
				damageSprites(sourceactor, iactor);
				break;
			}
		}
		break;
	}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


void damageSprites(DBloodActor* sourceactor, DBloodActor* actor)
{
	if (!actor->IsDudeActor() || !actor->hasX() || actor->xspr.health <= 0 || sourceactor->xspr.data3 < 0)
		return;


	int health = 0;

	DBloodPlayer* pPlayer = getPlayerById(actor->spr.type);
	int dmgType = (sourceactor->xspr.data2 >= kDmgFall) ? ClipHigh(sourceactor->xspr.data2, kDmgElectric) : -1;
	int dmg = actor->xspr.health << 4;
	int armor[3];

	bool godMode = (pPlayer && ((dmgType >= 0 && pPlayer->damageControl[dmgType]) || powerupCheck(pPlayer, kPwUpDeathMask) || pPlayer->godMode)); // kneeling

	if (godMode || actor->xspr.locked) return;
	else if (sourceactor->xspr.data3)
	{
		if (sourceactor->spr.flags & kModernTypeFlag1) dmg = ClipHigh(sourceactor->xspr.data3 << 1, 65535);
		else if (actor->xspr.sysData2 > 0) dmg = (ClipHigh(actor->xspr.sysData2 << 4, 65535) * sourceactor->xspr.data3) / kPercFull;
		else dmg = ((getDudeInfo(actor->spr.type)->startHealth << 4) * sourceactor->xspr.data3) / kPercFull;

		health = actor->xspr.health - dmg;
	}

	if (dmgType >= kDmgFall)
	{
		if (dmg < (int)actor->xspr.health << 4)
		{
			if (!nnExtIsImmune(actor, dmgType, 0))
			{
				if (pPlayer)
				{
					playerDamageArmor(pPlayer, (DAMAGE_TYPE)dmgType, dmg);
					for (int i = 0; i < 3; armor[i] = pPlayer->armor[i], pPlayer->armor[i] = 0, i++);
					actDamageSprite(sourceactor, actor, (DAMAGE_TYPE)dmgType, dmg);
					for (int i = 0; i < 3; pPlayer->armor[i] = armor[i], i++);

				}
				else
				{
					actDamageSprite(sourceactor, actor, (DAMAGE_TYPE)dmgType, dmg);
				}
			}
			else
			{
				//Printf(PRINT_HIGH, "Dude type %d is immune to damage type %d!", actor->spr.type, dmgType);
			}
		}
		else if (!pPlayer) actKillDude(sourceactor, actor, (DAMAGE_TYPE)dmgType, dmg);
		else playerDamageSprite(sourceactor, pPlayer, (DAMAGE_TYPE)dmgType, dmg);
	}
	else if ((actor->xspr.health = ClipLow(health, 1)) > 16);
	else if (!pPlayer) actKillDude(sourceactor, actor, kDamageBullet, dmg);
	else playerDamageSprite(sourceactor, pPlayer, kDamageBullet, dmg);

	if (actor->xspr.health > 0)
	{
		if (!(sourceactor->spr.flags & kModernTypeFlag8))
			actor->xspr.health = health;

		bool showEffects = !(sourceactor->spr.flags & kModernTypeFlag2); // show it by default
		bool forceRecoil = (sourceactor->spr.flags & kModernTypeFlag4);

		if (showEffects)
		{
			switch (dmgType)
			{
			case kDmgBurn:
				if (actor->xspr.burnTime > 0) break;
				actBurnSprite(sourceactor, actor, ClipLow(dmg >> 1, 128));
				evKillActor(actor, kCallbackFXFlameLick);
				evPostActor(actor, 0, kCallbackFXFlameLick); // show flames
				break;
			case kDmgElectric:
				forceRecoil = true; // show tesla recoil animation
				break;
			case kDmgBullet:
				evKillActor(actor, kCallbackFXBloodSpurt);
				for (int i = 1; i < 6; i++)
				{
					if (Chance(0x16000 >> i))
						fxSpawnBlood(actor, dmg << 4);
				}
				break;
			case kDmgChoke:
				if (!pPlayer || !Chance(0x2000)) break;
				else pPlayer->blindEffect += dmg << 2;
			}
		}

		if (forceRecoil && !pPlayer)
		{
			actor->xspr.data3 = 32767;
			actor->dudeExtra.teslaHit = (dmgType == kDmgElectric) ? 1 : 0;
			if (actor->xspr.aiState->stateType != kAiStateRecoil)
				RecoilDude(actor);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useSeqSpawnerGen(DBloodActor* sourceactor, int objType, sectortype* pSector, walltype* pWall, DBloodActor* iactor)
{
	if (sourceactor->xspr.data2 > 0 && !getSequence(sourceactor->xspr.data2))
	{
		Printf(PRINT_HIGH, "Missing sequence #%d", sourceactor->xspr.data2);
		return;
	}

	switch (objType)
	{
	case OBJ_SECTOR:
	{
		if (sourceactor->xspr.data2 <= 0)
		{
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 1)
				seqKill(SS_FLOOR, pSector);
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 2)
				seqKill(SS_CEILING, pSector);
		}
		else
		{
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 1)
				seqSpawn(sourceactor->xspr.data2, SS_FLOOR, pSector, -1);
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 2)
				seqSpawn(sourceactor->xspr.data2, SS_CEILING, pSector, -1);
		}
		return;
	}
	case OBJ_WALL:
	{
		if (sourceactor->xspr.data2 <= 0)
		{
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 1)
				seqKill(SS_WALL, pWall);
			if ((sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 2) && (pWall->cstat & CSTAT_WALL_MASKED))
				seqKill(SS_MASKED, pWall);
		}
		else
		{
			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 1)
				seqSpawn(sourceactor->xspr.data2, SS_WALL, pWall, -1);

			if (sourceactor->xspr.data3 == 3 || sourceactor->xspr.data3 == 2) {

				if (!pWall->twoSided()) {
					if (sourceactor->xspr.data3 == 3)
						seqSpawn(sourceactor->xspr.data2, SS_WALL, pWall, -1);

				}
				else {
					if (!(pWall->cstat & CSTAT_WALL_MASKED))
						pWall->cstat |= CSTAT_WALL_MASKED;

					seqSpawn(sourceactor->xspr.data2, SS_MASKED, pWall, -1);
				}
			}

			if (sourceactor->xspr.data4 > 0)
			{
				DVector3 cpos;
				
				cpos.SetXY(pWall->center());
				auto pMySector = pWall->sectorp();
				double ceilZ, floorZ;
				calcSlope(pSector, cpos, &ceilZ, &floorZ);
				double ceilZ2, floorZ2;
				calcSlope(pWall->nextSector(), cpos, &ceilZ2, &floorZ2);
				ceilZ = max(ceilZ, ceilZ2);
				floorZ = min(floorZ, floorZ2);
				cpos.Z = (ceilZ + floorZ) * 0.5;

				sfxPlay3DSound(cpos, sourceactor->xspr.data4, pSector);
			}
		}
		return;
	}
	case OBJ_SPRITE:
	{
		if (sourceactor->xspr.data2 <= 0) seqKill(iactor);
		else if (iactor->insector())
		{
			if (sourceactor->xspr.data3 > 0)
			{
				auto spawned = InsertSprite(iactor->sector(), kStatDecoration);
				if (spawned != nullptr)
				{
					double top, bottom;
					GetActorExtents(spawned, &top, &bottom);
					DVector3 pos = iactor->spr.pos;
					switch (sourceactor->xspr.data3)
					{
					default:
						break;
					case 2:
						pos.Z = bottom;
						break;
					case 3:
						pos.Z = top;
						break;
					case 4:
					{
						// this had no value shift and no repeat handling, which looks like a bug.
						auto tex = TexMan.GetGameTexture(iactor->spr.spritetexture());
						double center = tex->GetDisplayHeight() / 2 + tex->GetDisplayTopOffset();
						pos.Z += center * iactor->spr.scale.Y;
						break;
					}
					case 5:
					case 6:
						if (!iactor->insector()) pos.Z = top;
						else pos.Z = ((sourceactor->xspr.data3 == 5) ?
							getflorzofslopeptr(spawned->sector(), spawned->spr.pos) :
							getceilzofslopeptr(spawned->sector(), spawned->spr.pos));
						break;
					}

					spawned->spr.pos = pos;

					spawned->addX();
					seqSpawn(sourceactor->xspr.data2, spawned, -1);
					if (sourceactor->spr.flags & kModernTypeFlag1)
					{
						spawned->spr.pal = sourceactor->spr.pal;
						spawned->spr.shade = sourceactor->spr.shade;
						spawned->spr.scale = sourceactor->spr.scale;
						spawned->spr.xoffset = sourceactor->spr.xoffset;
						spawned->spr.yoffset = sourceactor->spr.yoffset;
					}

					if (sourceactor->spr.flags & kModernTypeFlag2)
					{
						spawned->spr.cstat |= sourceactor->spr.cstat;
					}

					if (sourceactor->spr.flags & kModernTypeFlag4)
					{
						spawned->spr.Angles.Yaw = sourceactor->spr.Angles.Yaw;
					}

					// should be: the more is seqs, the shorter is timer
					evPostActor(spawned, 1000, kCallbackRemove);
				}
			}
			else
			{
				seqSpawn(sourceactor->xspr.data2, iactor, -1);
			}

			if (sourceactor->xspr.data4 > 0)
				sfxPlay3DSound(iactor, sourceactor->xspr.data4, -1, 0);
		}
		return;
	}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void condPush(DBloodActor* actor, const EventObject& iactor) { actor->condition[0] = iactor; }
void condPush(DBloodActor* actor, DBloodActor* iactor) { actor->condition[0] = EventObject(iactor); }
void condPush(DBloodActor* actor, walltype* iactor) { actor->condition[0] = EventObject(iactor); }
void condPush(DBloodActor* actor, sectortype* iactor) { actor->condition[0] = EventObject(iactor); }
EventObject condGet(DBloodActor* actor) { return actor->condition[0]; }

void condBackup(DBloodActor* actor)
{
	actor->condition[1] = actor->condition[0];
}

void condRestore(DBloodActor* actor)
{
	actor->condition[0] = actor->condition[1];
}

// normal comparison
bool condCmp(int val, int arg1, int arg2, int comOp)
{
	if (comOp & 0x2000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val > arg1) : (val >= arg1); // blue sprite
	else if (comOp & 0x4000) return (comOp & CSTAT_SPRITE_BLOCK) ? (val < arg1) : (val <= arg1); // green sprite
	else if (comOp & CSTAT_SPRITE_BLOCK)
	{
		if (arg1 > arg2) I_Error("Value of argument #1 (%d) must be less than value of argument #2 (%d)", arg1, arg2);
		return (val >= arg1 && val <= arg2);
	}
	else return (val == arg1);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void condError(DBloodActor* aCond, const char* pzFormat, ...)
{
	char buffer[256];
	char buffer2[512];
	FString condType = "Unknown";
	for (int i = 0; i < 7; i++)
	{
		if (aCond->xspr.data1 < gCondTypeNames[i].rng1 || aCond->xspr.data1 >= gCondTypeNames[i].rng2) continue;
		condType = gCondTypeNames[i].name;
		condType.ToUpper();
		break;
	}

	snprintf(buffer, 256, "\n\n%s CONDITION RX: %d, TX: %d, SPRITE: #%d RETURNS:\n", condType.GetChars(), aCond->xspr.rxID, aCond->xspr.txID, aCond->GetIndex());
	va_list args;
	va_start(args, pzFormat);
	vsnprintf(buffer2, 512, pzFormat, args);
	I_Error("%s%s", buffer, buffer2);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckGame(DBloodActor* aCond, const EVENT& event, int cmpOp, bool PUSH)
{
	int cond = aCond->xspr.data1;
	int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3;
	int arg3 = aCond->xspr.data4;

	switch (cond) {
	case 1:  return condCmp(gFrameCount / (kTicsPerSec * 60), arg1, arg2, cmpOp);            // compare level minutes
	case 2:  return condCmp((gFrameCount / kTicsPerSec) % 60, arg1, arg2, cmpOp);            // compare level seconds
	case 3:  return condCmp(((gFrameCount % kTicsPerSec) * 33) / 10, arg1, arg2, cmpOp);     // compare level mseconds
	case 4:  return condCmp(gFrameCount, arg1, arg2, cmpOp);                                 // compare level time (unsafe)
	case 5:  return condCmp(Level.kills.got, arg1, arg2, cmpOp);                             // compare current global kills counter
	case 6:  return condCmp(Level.kills.max, arg1, arg2, cmpOp);                        // compare total global kills counter
	case 7:  return condCmp(Level.secrets.got, arg1, arg2, cmpOp);                          // compare how many secrets found
	case 8:  return condCmp(Level.secrets.max, arg1, arg2, cmpOp);                           // compare total secrets
	/*----------------------------------------------------------------------------------------------------------------------------------*/
	case 20: return condCmp(gVisibility, arg1, arg2, cmpOp);                                // compare global visibility value
	/*----------------------------------------------------------------------------------------------------------------------------------*/
	case 30: return Chance((0x10000 * arg3) / kPercFull);                                   // check chance
	case 31: return condCmp(nnExtRandom(arg1, arg2), arg1, arg2, cmpOp);
		/*----------------------------------------------------------------------------------------------------------------------------------*/
	case 47:
	{
		BloodStatIterator it(ClipRange(arg3, 0, kMaxStatus));
		int c = 0;
		while (it.Next()) c++;
		return condCmp(c, arg1, arg2, cmpOp); // compare counter of specific statnum sprites
	}
	case 48: return condCmp(Numsprites, arg1, arg2, cmpOp);                                 // compare counter of total sprites
	}

	condError(aCond, "Unexpected condition id (%d)!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckMixed(DBloodActor* aCond, const EVENT& event, int cmpOp, bool PUSH)
{
	//int var = -1;
	int cond = aCond->xspr.data1 - kCondMixedBase; int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3; int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);

	switch (cond)
	{
	case 0:  return (eob.isSector());
	case 5:  return (eob.isWall());
	case 10: return (eob.isActor() && eob.actor());
	case 15: // x-index is fine?
		if (eob.isWall()) return eob.wall()->hasX();
		if (eob.isSector()) return eob.sector()->hasX();
		if (eob.isActor()) return eob.actor() && eob.actor()->hasX();
		break;
	case 20: // type in a range?
		if (eob.isWall()) return condCmp(eob.wall()->type, arg1, arg2, cmpOp);
		if (eob.isSector())  return condCmp(eob.sector()->type, arg1, arg2, cmpOp);
		if (eob.isActor()) return eob.actor() && condCmp(eob.actor()->spr.type, arg1, arg2, cmpOp);
		break;
	case 24:
	case 25: case 26: case 27:
	case 28: case 29: case 30:
	case 31: case 32: case 33:
		if (eob.isWall())
		{
			walltype* pObj = eob.wall();
			switch (cond)
			{
			case 24: return condCmp(GetExtInfo(pObj->walltexture).surftype, arg1, arg2, cmpOp);
			case 25: return condCmp(legacyTileNum(pObj->walltexture), arg1, arg2, cmpOp);
			case 26: return condCmp(pObj->pal, arg1, arg2, cmpOp);
			case 27: return condCmp(pObj->shade, arg1, arg2, cmpOp);
			case 28: return (arg3) ? condCmp((pObj->cstat & EWallFlags::FromInt(arg3)), arg1, arg2, cmpOp) : (pObj->cstat & EWallFlags::FromInt(arg1));
			case 29: return (arg3) ? condCmp((pObj->hitag & arg3), arg1, arg2, cmpOp) : (pObj->hitag & arg1);
			case 30: return condCmp(pObj->xrepeat, arg1, arg2, cmpOp);
			case 31: return condCmp(pObj->xpan(), arg1, arg2, cmpOp);
			case 32: return condCmp(pObj->yrepeat, arg1, arg2, cmpOp);
			case 33: return condCmp(pObj->ypan(), arg1, arg2, cmpOp);
			}
		}
		else if (eob.isActor())
		{
			auto actor = eob.actor();
			if (!actor) break;
			switch (cond)
			{
			case 24: return condCmp(GetExtInfo(actor->spr.spritetexture()).surftype, arg1, arg2, cmpOp);
			case 25: return condCmp(legacyTileNum(actor->spr.spritetexture()), arg1, arg2, cmpOp);
			case 26: return condCmp(actor->spr.pal, arg1, arg2, cmpOp);
			case 27: return condCmp(actor->spr.shade, arg1, arg2, cmpOp);
			case 28: return (arg3) ? condCmp((actor->spr.cstat & ESpriteFlags::FromInt(arg3)), arg1, arg2, cmpOp) : (actor->spr.cstat & ESpriteFlags::FromInt(arg1));
			case 29: return (arg3) ? condCmp((actor->spr.hitag & arg3), arg1, arg2, cmpOp) : (actor->spr.hitag & arg1);
			case 30: return condCmp(int(actor->spr.scale.X * INV_REPEAT_SCALE), arg1, arg2, cmpOp);
			case 31: return condCmp(actor->spr.xoffset, arg1, arg2, cmpOp);
			case 32: return condCmp(int(actor->spr.scale.Y * INV_REPEAT_SCALE), arg1, arg2, cmpOp);
			case 33: return condCmp(actor->spr.yoffset, arg1, arg2, cmpOp);
			}
		}
		else if (eob.sector())
		{
			sectortype* pObj = eob.sector();
			switch (cond)
			{
			case 24:
				switch (arg3)
				{
				default: return (condCmp(GetExtInfo(pObj->floortexture).surftype, arg1, arg2, cmpOp) || condCmp(GetExtInfo(pObj->ceilingtexture).surftype, arg1, arg2, cmpOp));
				case 1: return condCmp(GetExtInfo(pObj->floortexture).surftype, arg1, arg2, cmpOp);
				case 2: return condCmp(GetExtInfo(pObj->ceilingtexture).surftype, arg1, arg2, cmpOp);
				}
				break;
			case 25:
				switch (arg3)
				{
				default: return (condCmp(legacyTileNum(pObj->floortexture), arg1, arg2, cmpOp) || condCmp(legacyTileNum(pObj->ceilingtexture), arg1, arg2, cmpOp));
				case 1:  return condCmp(legacyTileNum(pObj->floortexture), arg1, arg2, cmpOp);
				case 2:  return condCmp(legacyTileNum(pObj->ceilingtexture), arg1, arg2, cmpOp);
				}
				break;
			case 26:
				switch (arg3)
				{
				default: return (condCmp(pObj->floorpal, arg1, arg2, cmpOp) || condCmp(pObj->ceilingpal, arg1, arg2, cmpOp));
				case 1:  return condCmp(pObj->floorpal, arg1, arg2, cmpOp);
				case 2:  return condCmp(pObj->ceilingpal, arg1, arg2, cmpOp);
				}
				break;
			case 27:
				switch (arg3)
				{
				default: return (condCmp(pObj->floorshade, arg1, arg2, cmpOp) || condCmp(pObj->ceilingshade, arg1, arg2, cmpOp));
				case 1:  return condCmp(pObj->floorshade, arg1, arg2, cmpOp);
				case 2:  return condCmp(pObj->ceilingshade, arg1, arg2, cmpOp);
				}
				break;
			case 28:
			{
				auto a = ESectorFlags::FromInt(arg1);
				switch (arg3)
				{
				default: return ((pObj->floorstat & a) || (pObj->ceilingstat & a));
				case 1:  return (pObj->floorstat & a);
				case 2:  return (pObj->ceilingstat & a);
				}
				break;
			}
			case 29: return (arg3) ? condCmp((pObj->hitag & arg3), arg1, arg2, cmpOp) : (pObj->hitag & arg1);
			case 30: return condCmp(pObj->floorxpan(), arg1, arg2, cmpOp);
			case 31: return condCmp(pObj->ceilingxpan(), arg1, arg2, cmpOp);
			case 32: return condCmp(pObj->floorypan(), arg1, arg2, cmpOp);
			case 33: return condCmp(pObj->ceilingypan(), arg1, arg2, cmpOp);
			}
		}
		break;
	case 41:  case 42:  case 43:
	case 44:  case 50:  case 51:
	case 52:  case 53:  case 54:
	case 55:  case 56:  case 57:
	case 58:  case 59:  case 70:
	case 71:
		if (eob.isWall())
		{
			auto pObj = eob.wall();
			if (!pObj->hasX())
				return condCmp(0, arg1, arg2, cmpOp);

			XWALL* pXObj = &pObj->xw();
			switch (cond)
			{
			case 41: return condCmp(pXObj->data, arg1, arg2, cmpOp);
			case 50: return condCmp(pXObj->rxID, arg1, arg2, cmpOp);
			case 51: return condCmp(pXObj->txID, arg1, arg2, cmpOp);
			case 52: return pXObj->locked;
			case 53: return pXObj->triggerOn;
			case 54: return pXObj->triggerOff;
			case 55: return pXObj->triggerOnce;
			case 56: return pXObj->isTriggered;
			case 57: return pXObj->state;
			case 58: return condCmp((kPercFull * pXObj->busy) / 65536, arg1, arg2, cmpOp);
			case 59: return pXObj->dudeLockout;
			case 70:
				switch (arg3)
				{
				default: return (condCmp(seqGetID(SS_WALL, pObj), arg1, arg2, cmpOp) || condCmp(seqGetID(SS_MASKED, pObj), arg1, arg2, cmpOp));
				case 1:  return condCmp(seqGetID(SS_WALL, pObj), arg1, arg2, cmpOp);
				case 2:  return condCmp(seqGetID(SS_MASKED, pObj), arg1, arg2, cmpOp);
				}
				break;
			case 71:
				switch (arg3)
				{
				default: return (condCmp(seqGetStatus(SS_WALL, pObj), arg1, arg2, cmpOp) || condCmp(seqGetStatus(SS_MASKED, pObj), arg1, arg2, cmpOp));
				case 1:  return condCmp(seqGetStatus(SS_WALL, pObj), arg1, arg2, cmpOp);
				case 2:  return condCmp(seqGetStatus(SS_MASKED, pObj), arg1, arg2, cmpOp);
				}
				break;
			}
		}
		else if (eob.isActor())
		{
			auto objActor = eob.actor();
			if (!objActor) break;
			if (!objActor->hasX())
				return condCmp(0, arg1, arg2, cmpOp);

			switch (cond)
			{
			case 41: case 42:
			case 43: case 44:
				return condCmp(getDataFieldOfObject(eob, 1 + cond - 41), arg1, arg2, cmpOp);
			case 50: return condCmp(objActor->xspr.rxID, arg1, arg2, cmpOp);
			case 51: return condCmp(objActor->xspr.txID, arg1, arg2, cmpOp);
			case 52: return objActor->xspr.locked;
			case 53: return objActor->xspr.triggerOn;
			case 54: return objActor->xspr.triggerOff;
			case 55: return objActor->xspr.triggerOnce;
			case 56: return objActor->xspr.isTriggered;
			case 57: return objActor->xspr.state;
			case 58: return condCmp((kPercFull * objActor->xspr.busy) / 65536, arg1, arg2, cmpOp);
			case 59: return objActor->xspr.DudeLockout;
			case 70: return condCmp(seqGetID(objActor), arg1, arg2, cmpOp);
			case 71: return condCmp(seqGetStatus(objActor), arg1, arg2, cmpOp);
			}
		}
		else if (eob.isSector())
		{
			auto pObj = eob.sector();
			if (!pObj->hasX())
				return condCmp(0, arg1, arg2, cmpOp);

			XSECTOR* pXObj = &pObj->xs();
			switch (cond) {
			case 41: return condCmp(pXObj->data, arg1, arg2, cmpOp);
			case 50: return condCmp(pXObj->rxID, arg1, arg2, cmpOp);
			case 51: return condCmp(pXObj->txID, arg1, arg2, cmpOp);
			case 52: return pXObj->locked;
			case 53: return pXObj->triggerOn;
			case 54: return pXObj->triggerOff;
			case 55: return pXObj->triggerOnce;
			case 56: return pXObj->isTriggered;
			case 57: return pXObj->state;
			case 58: return condCmp((kPercFull * pXObj->busy) / 65536, arg1, arg2, cmpOp);
			case 59: return pXObj->dudeLockout;
			case 70:
				// wall???
				switch (arg3)
				{
				default: return (condCmp(seqGetID(SS_CEILING, pObj), arg1, arg2, cmpOp) || condCmp(seqGetID(SS_FLOOR, pObj), arg1, arg2, cmpOp));
				case 1:  return condCmp(seqGetID(SS_CEILING, pObj), arg1, arg2, cmpOp);
				case 2:  return condCmp(seqGetID(SS_FLOOR, pObj), arg1, arg2, cmpOp);
				}
				break;
			case 71:
				switch (arg3)
				{
				default: return (condCmp(seqGetStatus(SS_CEILING, pObj), arg1, arg2, cmpOp) || condCmp(seqGetStatus(SS_FLOOR, pObj), arg1, arg2, cmpOp));
				case 1:  return condCmp(seqGetStatus(SS_CEILING, pObj), arg1, arg2, cmpOp);
				case 2:  return condCmp(seqGetStatus(SS_FLOOR, pObj), arg1, arg2, cmpOp);
				}
				break;
			}
		}
		break;
	case 99: return condCmp(event.cmd, arg1, arg2, cmpOp);  // this codition received specified command?
	}

	condError(aCond, "Unexpected condition id (%d)!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckSector(DBloodActor* aCond, int cmpOp, bool PUSH)
{
	int cond = aCond->xspr.data1 - kCondSectorBase;
	int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3; //int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);

	if (!eob.isSector())
		condError(aCond, "Sector expected, got %s", eob.description().GetChars());

	sectortype* pSect = eob.sector();
	XSECTOR* pXSect = pSect->hasX() ? &pSect->xs() : nullptr;

	if (cond < (kCondRange >> 1))
	{
		switch (cond)
		{
		default: break;
		case 0: return condCmp(pSect->visibility, arg1, arg2, cmpOp);
		case 5: return condCmp(pSect->floorheinum, arg1, arg2, cmpOp);
		case 6: return condCmp(pSect->ceilingheinum, arg1, arg2, cmpOp);
		case 10: // required sprite type is in current sector?
			BloodSectIterator it(pSect);
			while (auto iactor = it.Next())
			{
				if (!condCmp(iactor->spr.type, arg1, arg2, cmpOp)) continue;
				else if (PUSH) condPush(aCond, iactor);
				return true;
			}
			return false;
		}
	}
	else if (pXSect)
	{
		switch (cond)
		{
		default: break;
		case 50: return pXSect->Underwater;
		case 51: return condCmp(pXSect->Depth, arg1, arg2, cmpOp);
		case 55: // compare floor height (in %)
		case 56: { // compare ceil height (in %)
			double h = 0, curH = 0;
			switch (pSect->type)
			{
			case kSectorZMotion:
			case kSectorRotate:
			case kSectorSlide:
				if (cond == 55)// 60)
				{
					h = max(abs(pXSect->onFloorZ - pXSect->offFloorZ), 1 / 256.);
					curH = abs(pSect->floorz - pXSect->offFloorZ);
				}
				else
				{
					h = max(abs(pXSect->onCeilZ - pXSect->offCeilZ), 1 / 256.);
					curH = abs(pSect->ceilingz - pXSect->offCeilZ);
				}
				return condCmp(int((kPercFull * curH) / h), arg1, arg2, cmpOp);
			default:
				condError(aCond, "Usupported sector type %d", pSect->type);
				return false;
			}
		}
		case 57: // this sector in movement?
			return !pXSect->unused1;
		}
	}
	else
	{
		switch (cond)
		{
		default: return false;
		case 55:
		case 56:
			return condCmp(0, arg1, arg2, cmpOp);
		}
	}

	condError(aCond, "Unexpected condition id (%d)!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckWall(DBloodActor* aCond, int cmpOp, bool PUSH)
{
	int var = -1;
	int cond = aCond->xspr.data1 - kCondWallBase; int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3; //int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);
	if (!eob.isWall())
		condError(aCond, "Wall expected, got %s", eob.description().GetChars());

	walltype* pWall = eob.wall();

	if (cond < (kCondRange >> 1))
	{
		switch (cond)
		{
		default: break;
		case 0:
			return condCmp(legacyTileNum(pWall->overtexture), arg1, arg2, cmpOp);
		case 5:
			if (PUSH) condPush(aCond, pWall->sectorp());
			return true;
		case 10: // this wall is a mirror?                          // must be as constants here
			return (pWall->portalflags == PORTAL_WALL_MIRROR);
		case 15:
			if (!pWall->twoSided()) return false;
			else if (PUSH) condPush(aCond, pWall->nextSector());
			return true;
		case 20:
			if (!pWall->twoSided()) return false;
			else if (PUSH) condPush(aCond, pWall->nextWall());
			return true;
		case 25: // next wall belongs to sector? (Note: This was 'sector of next wall' which is same as case 15 because we do not allow bad links!)
			if (!pWall->twoSided()) return false;
			else if (PUSH) condPush(aCond, pWall->nextSector());
			return true;
		}
	}

	condError(aCond, "Unexpected condition id (%d)!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckPlayer(DBloodActor* aCond, int cmpOp, bool PUSH)
{
	int var = -1;
	DBloodPlayer* pPlayer = NULL;
	int cond = aCond->xspr.data1 - kCondPlayerBase;
	int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3;
	int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);

	if (!eob.isActor() || !eob.actor())
		condError(aCond, "Sprite expected, got %s", eob.description().GetChars());

	auto objActor = eob.actor();
	for (int i = 0; i < kMaxPlayers; i++)
	{
		if (objActor != getPlayer(i)->GetActor()) continue;
		pPlayer = getPlayer(i);
		break;
	}

	if (!pPlayer)
	{
		condError(aCond, "Player expected, got %s", eob.description().GetChars());
		return false;
	}

	switch (cond) {
	case 0: // check if this player is connected
		if (!condCmp(pPlayer->pnum + 1, arg1, arg2, cmpOp) || pPlayer->GetActor() == nullptr) return false;
		else if (PUSH) condPush(aCond, pPlayer->GetActor());
		return true;
	case 1: return condCmp((gGameOptions.nGameType != 3) ? 0 : pPlayer->teamId + 1, arg1, arg2, cmpOp); // compare team
	case 2: return (arg1 > 0 && arg1 < 8 && pPlayer->hasKey[arg1 - 1]);
	case 3: return (arg1 > 0 && arg1 < 15 && pPlayer->hasWeapon[arg1 - 1]);
	case 4: return condCmp(pPlayer->curWeapon, arg1, arg2, cmpOp);
	case 5: return (arg1 > 0 && arg1 < 6 && condCmp(pPlayer->packSlots[arg1 - 1].curAmount, arg2, arg3, cmpOp));
	case 6: return (arg1 > 0 && arg1 < 6 && pPlayer->packSlots[arg1 - 1].isActive);
	case 7: return condCmp(pPlayer->packItemId + 1, arg1, arg2, cmpOp);
	case 8: // check for powerup amount in seconds
		if (arg3 > 0 && arg3 <= (kMaxAllowedPowerup - (kMinAllowedPowerup << 1) + 1)) {
			var = (kMinAllowedPowerup + arg3) - 1; // allowable powerups
			return condCmp(pPlayer->pwUpTime[var] / 100, arg1, arg2, cmpOp);
		}
		condError(aCond, "Unexpected powerup #%d", arg3);
		return false;
	case 9:
		if (!pPlayer->fragger) return false;
		else if (PUSH) condPush(aCond, pPlayer->fragger);
		return true;
	case 10: // check keys pressed
		switch (arg1) {
		case 1:  return (pPlayer->cmd.ucmd.vel.X > 0);            // forward
		case 2:  return (pPlayer->cmd.ucmd.vel.X < 0);            // backward
		case 3:  return (pPlayer->cmd.ucmd.vel.Y < 0);             // left
		case 4:  return (pPlayer->cmd.ucmd.vel.Y > 0);             // right
		case 5:  return !!(pPlayer->cmd.ucmd.actions & SB_JUMP);       // jump
		case 6:  return !!(pPlayer->cmd.ucmd.actions & SB_CROUCH);     // crouch
		case 7:  return !!(pPlayer->cmd.ucmd.actions & SB_FIRE);      // normal fire weapon
		case 8:  return !!(pPlayer->cmd.ucmd.actions & SB_ALTFIRE);     // alt fire weapon
		case 9:  return !!(pPlayer->cmd.ucmd.actions & SB_OPEN);        // use
		default:
			condError(aCond, "Specify a correct key!");
			break;
		}
		return false;
	case 11: return (pPlayer->isRunning);
	case 12: return (pPlayer->fallScream); // falling in abyss?
	case 13: return condCmp(pPlayer->lifeMode + 1, arg1, arg2, cmpOp);
	case 14: return condCmp(pPlayer->posture + 1, arg1, arg2, cmpOp);
	case 46: return condCmp(pPlayer->sceneQav, arg1, arg2, cmpOp);
	case 47: return (pPlayer->godMode || powerupCheck(pPlayer, kPwUpDeathMask));
	case 48: return isShrunk(pPlayer->GetActor());
	case 49: return isGrown(pPlayer->GetActor());
	}

	condError(aCond, "Unexpected condition #%d!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckDude(DBloodActor* aCond, int cmpOp, bool PUSH)
{
	int var = -1;
	int cond = aCond->xspr.data1 - kCondDudeBase; 
	int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3; 
	int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);

	if (!eob.isActor() || !eob.actor())
		condError(aCond, "Sprite expected, got %s", eob.description().GetChars());

	auto objActor = eob.actor();

	if (!objActor->hasX() || objActor->spr.type == kThingBloodChunks)
		condError(aCond, "Sprite #%d is dead!", objActor->GetIndex());

	if (!objActor->IsDudeActor() || objActor->IsPlayerActor())
		condError(aCond, "Object #%d is not an enemy!", objActor->GetIndex());

	auto targ = objActor->GetTarget();
	switch (cond)
	{
	default: break;
	case 0: // dude have any targets?
		if (!targ) return false;
		else if (!targ->IsDudeActor() && targ->spr.type != kMarkerPath) return false;
		else if (PUSH) condPush(aCond, targ);
		return true;

	case 1: return aiFightDudeIsAffected(objActor); // dude affected by ai fight?
	case 2: // distance to the target in a range?
	case 3: // is the target visible?
	case 4: // is the target visible with periphery?
	{

		if (!targ)
			condError(aCond, "Dude #%d has no target!", objActor->GetIndex());

		DUDEINFO* pInfo = getDudeInfo(objActor->spr.type);
		double height = (pInfo->eyeHeight * objActor->spr.scale.Y);

		auto delta = targ->spr.pos.XY() - objActor->spr.pos.XY();

		switch (cond)
		{
		case 2:
			var = condCmp(int(delta.Length() * 16), arg1 * 512, arg2 * 512, cmpOp);
			break;
		case 3:
		case 4:
			var = cansee(objActor->spr.pos, objActor->sector(), targ->spr.pos.plusZ(-height), targ->sector());
			if (cond == 4 && var > 0)
			{
				DAngle absang = absangle(delta.Angle(), objActor->spr.Angles.Yaw);
				var = absang < (arg1 <= 0 ? pInfo->Periphery() : min(mapangle(arg1), DAngle360));
			}
			break;
		}

		if (var <= 0) return false;
		else if (PUSH) condPush(aCond, targ);
		return true;

	}
	case 5: return objActor->xspr.dudeFlag4;
	case 6: return objActor->xspr.dudeDeaf;
	case 7: return objActor->xspr.dudeGuard;
	case 8: return objActor->xspr.dudeAmbush;
	case 9: return (objActor->xspr.unused1 & kDudeFlagStealth);
	case 10: // check if the marker is busy with another dude
	case 11: // check if the marker is reached
		if (!objActor->xspr.dudeFlag4 || !targ || targ->spr.type != kMarkerPath) return false;
		switch (cond) {
		case 10:
		{
			auto check = aiPatrolMarkerBusy(objActor, targ);
			if (!check) return false;
			else if (PUSH) condPush(aCond, check);
			break;
		}
		case 11:
			if (!aiPatrolMarkerReached(objActor)) return false;
			else if (PUSH) condPush(aCond, targ);
			break;
		}
		return true;
	case 12: // compare spot progress value in %
		if (!objActor->xspr.dudeFlag4 || !targ || targ->spr.type != kMarkerPath) var = 0;
		else if (!(objActor->xspr.unused1 & kDudeFlagStealth) || objActor->xspr.data3 < 0 || objActor->xspr.data3 > kMaxPatrolSpotValue) var = 0;
		else var = (kPercFull * objActor->xspr.data3) / kMaxPatrolSpotValue;
		return condCmp(var, arg1, arg2, cmpOp);
	case 15: return getDudeInfo(objActor->spr.type)->lockOut; // dude allowed to interact with objects?
	case 16: return condCmp(objActor->xspr.aiState->stateType, arg1, arg2, cmpOp);
	case 17: return condCmp(objActor->xspr.stateTimer, arg1, arg2, cmpOp);
	case 20: // kDudeModernCustom conditions
	case 21:
	case 22:
	case 23:
	case 24:
		switch (objActor->spr.type)
		{
		case kDudeModernCustom:
		case kDudeModernCustomBurning:
			switch (cond) {
			case 20: // life leech is thrown?
			{
				DBloodActor* act = objActor->genDudeExtra.pLifeLeech;
				if (!act) return false;
				else if (PUSH) condPush(aCond, act);
				return true;
			}

			case 21: // life leech is destroyed?
			{
				DBloodActor* act = objActor->genDudeExtra.pLifeLeech;
				if (!act) return false;
				if (objActor->GetSpecialOwner()) return true;
				else if (PUSH) condPush(aCond, act);
				return false;
			}

			case 22: // are required amount of dudes is summoned?
				return condCmp(objActor->genDudeExtra.slaveCount, arg1, arg2, cmpOp);

			case 23: // check if dude can...
				switch (arg3)
				{
				case 1: return objActor->genDudeExtra.canAttack;
				case 2: return objActor->genDudeExtra.canBurn;
				case 3: return objActor->genDudeExtra.canDuck;
				case 4: return objActor->genDudeExtra.canElectrocute;
				case 5: return objActor->genDudeExtra.canFly;
				case 6: return objActor->genDudeExtra.canRecoil;
				case 7: return objActor->genDudeExtra.canSwim;
				case 8: return objActor->genDudeExtra.canWalk;
				default: condError(aCond, "Invalid argument %d", arg3); break;
				}
				break;
			case 24: // compare weapon dispersion
				return condCmp(objActor->genDudeExtra.baseDispersion, arg1, arg2, cmpOp);
			}
			break;
		default:
			condError(aCond, "Dude #%d is not a Custom Dude!", objActor->GetIndex());
			return false;
		}
	}

	condError(aCond, "Unexpected condition #%d!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool condCheckSprite(DBloodActor* aCond, int cmpOp, bool PUSH)
{
	int var = -1, var2 = -1, var3 = -1; DBloodPlayer* pPlayer = NULL; bool retn = false;
	int cond = aCond->xspr.data1 - kCondSpriteBase; int arg1 = aCond->xspr.data2;
	int arg2 = aCond->xspr.data3; int arg3 = aCond->xspr.data4;

	auto eob = condGet(aCond);

	if (!eob.isActor() || !eob.actor())
		condError(aCond, "Sprite expected, got %s", eob.description().GetChars());

	auto objActor = eob.actor();

	if (cond < (kCondRange >> 1))
	{
		switch (cond)
		{
		default: break;
		case 0: return condCmp((arg3 == 0) ? (objActor->spr.Angles.Yaw.Normalized360().Buildang()) : objActor->spr.Angles.Yaw.Buildang(), arg1, arg2, cmpOp);
		case 5: return condCmp(objActor->spr.statnum, arg1, arg2, cmpOp);
		case 6: return ((objActor->spr.flags & kHitagRespawn) || objActor->spr.statnum == kStatRespawn);
		case 7: return condCmp(spriteGetSlope(objActor), arg1, arg2, cmpOp);
		case 10: return condCmp(int(objActor->clipdist * 4), arg1, arg2, cmpOp);
		case 15:
			if (!objActor->GetOwner()) return false;
			else if (PUSH) condPush(aCond, objActor->GetOwner());
			return true;
		case 20: // stays in a sector?
			if (!objActor->insector()) return false;
			else if (PUSH) condPush(aCond, objActor->sector());
			return true;
		case 25:
			if (arg3 == 1)
			{
				if (arg1 == 0)
				{
					if ((var = condCmp(FloatToFixed(objActor->vel.X), arg1, arg2, cmpOp)) == true) return var;
					if ((var = condCmp(FloatToFixed(objActor->vel.Y), arg1, arg2, cmpOp)) == true) return var;
					if ((var = condCmp(FloatToFixed(objActor->vel.Z), arg1, arg2, cmpOp)) == true) return var;
				}
				else if (arg1 == 1) return condCmp(FloatToFixed(objActor->vel.X), arg1, arg2, cmpOp);
				else if (arg1 == 2) return condCmp(FloatToFixed(objActor->vel.Y), arg1, arg2, cmpOp);
				else if (arg1 == 3) return condCmp(FloatToFixed(objActor->vel.Z), arg1, arg2, cmpOp);
			}
			else if (arg1 == 0) return (!objActor->vel.isZero());
			else if (arg1 == 1) return (FloatToFixed(objActor->vel.X));
			else if (arg1 == 2) return (FloatToFixed(objActor->vel.Y));
			else if (arg1 == 3) return (FloatToFixed(objActor->vel.Z));
			break;
		case 30:
			if (!spriteIsUnderwater(objActor) && !spriteIsUnderwater(objActor, true)) return false;
			else if (PUSH) condPush(aCond, objActor->sector());
			return true;
		case 31:
			if (arg1 == -1)
			{
				for (var = 0; var < kDmgMax; var++)
				{
					if (!nnExtIsImmune(objActor, arg1, 0))
						return false;
				}
				return true;
			}
			return nnExtIsImmune(objActor, arg1, 0);

		case 35: // hitscan: ceil?
		case 36: // hitscan: floor?
		case 37: // hitscan: wall?
		case 38: // hitscan: sprite?
		{
			switch (arg1)
			{
			case  0: arg1 = CLIPMASK0 | CLIPMASK1; break;
			case  1: arg1 = CLIPMASK0; break;
			case  2: arg1 = CLIPMASK1; break;
			}

			double range = arg3 * 2;
			if ((pPlayer = getPlayerById(objActor->spr.type)) != NULL)
				var = HitScan(objActor, pPlayer->zWeapon, pPlayer->aim, arg1, range);
			else if (objActor->IsDudeActor())
				var = HitScan(objActor, objActor->spr.pos.Z, DVector3(objActor->spr.Angles.Yaw.ToVector(), (!objActor->hasX()) ? 0 : objActor->dudeSlope), arg1, range);
			else if ((objActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR)
			{
				var3 = (objActor->spr.cstat & CSTAT_SPRITE_YFLIP) ? 8 : -8; // was 0x20000 - HitScan uses Q28.4 for dz!
				var = HitScan(objActor, objActor->spr.pos.Z, DVector3(objActor->spr.Angles.Yaw.ToVector(), var3), arg1, range);
			}
			else
			{
				var = HitScan(objActor, objActor->spr.pos.Z, DVector3(objActor->spr.Angles.Yaw.ToVector(), 0), arg1, range);
			}

			if (var < 0)
				return false;

			switch (cond) {
			case 35:
				if (var != 1) return false;
				else if (PUSH) condPush(aCond, gHitInfo.hitSector);
				return true;
			case 36:
				if (var != 2) return false;
				else if (PUSH) condPush(aCond, gHitInfo.hitSector);
				return true;
			case 37:
				if (var != 0 && var != 4) return false;
				else if (PUSH) condPush(aCond, gHitInfo.hitWall);
				return true;
			case 38:
				if (var != 3) return false;
				else if (PUSH) condPush(aCond, gHitInfo.actor());
				return true;
			}
			break;
		}
		case 45: // this sprite is a target of some dude?
			BloodStatIterator it(kStatDude);
			while (auto iactor = it.Next())
			{
				if (objActor == iactor) continue;

				if (iactor->IsDudeActor() && iactor->hasX())
				{
					if (iactor->xspr.health <= 0 || iactor->GetTarget() != objActor) continue;
					else if (PUSH) condPush(aCond, iactor);
					return true;
				}
			}
			return false;
		}
	}
	else if (objActor->hasX())
	{
		switch (cond)
		{
		default: break;
		case 50: // compare hp (in %)
			if (objActor->IsDudeActor()) var = (objActor->xspr.sysData2 > 0) ? ClipRange(objActor->xspr.sysData2 << 4, 1, 65535) : getDudeInfo(objActor->spr.type)->startHealth << 4;
			else if (objActor->spr.type == kThingBloodChunks) return condCmp(0, arg1, arg2, cmpOp);
			else if (objActor->spr.type >= kThingBase && objActor->spr.type < kThingMax) var = thingInfo[objActor->spr.type - kThingBase].startHealth << 4;
			return condCmp((kPercFull * objActor->xspr.health) / ClipLow(var, 1), arg1, arg2, cmpOp);
		case 55: // touching ceil of sector?
			if (objActor->hit.ceilhit.type != kHitSector) return false;
			else if (PUSH) condPush(aCond, objActor->hit.ceilhit.hitSector);
			return true;
		case 56: // touching floor of sector?
			if (objActor->hit.florhit.type != kHitSector) return false;
			else if (PUSH) condPush(aCond, objActor->hit.florhit.hitSector);
			return true;
		case 57: // touching walls of sector?
			if (objActor->hit.hit.type != kHitWall) return false;
			else if (PUSH) condPush(aCond, objActor->hit.hit.hitWall);
			return true;
		case 58: // touching another sprite?
		{
			DBloodActor* actorvar = nullptr;
			// Caution: The hit pointers here may be stale, so be careful with them.
			switch (arg3)
			{
			case 0:
			case 1:
				if (objActor->hit.florhit.type == kHitSprite) actorvar = objActor->hit.florhit.safeActor();
				if (arg3 || var >= 0) break;
				[[fallthrough]];
			case 2:
				if (objActor->hit.hit.type == kHitSprite) actorvar = objActor->hit.hit.safeActor();
				if (arg3 || var >= 0) break;
				[[fallthrough]];
			case 3:
				if (objActor->hit.ceilhit.type == kHitSprite) actorvar = objActor->hit.ceilhit.safeActor();
				break;
			}
			if (actorvar == nullptr && objActor->insector())
			{
				// check if something is touching this sprite
				BloodSectIterator it(objActor->sector());
				while (auto iactor = it.Next())
				{
					if (iactor->spr.flags & kHitagRespawn) continue;
					auto& hit = iactor->hit;
					switch (arg3)
					{
					case 0:
					case 1:
						if (hit.ceilhit.type == kHitSprite && hit.ceilhit.safeActor() == objActor) actorvar = iactor;
						if (arg3 || actorvar) break;
						[[fallthrough]];
					case 2:
						if (hit.hit.type == kHitSprite && hit.hit.safeActor() == objActor) actorvar = iactor;
						if (arg3 || actorvar) break;
						[[fallthrough]];
					case 3:
						if (hit.florhit.type == kHitSprite && hit.florhit.safeActor() == objActor) actorvar = iactor;
						break;
					}
				}
			}
			if (actorvar == nullptr) return false;
			else if (PUSH) condPush(aCond, actorvar);
			return true;
		}

		case 65: // compare burn time (in %)
			var = (objActor->IsDudeActor()) ? 2400 : 1200;
			if (!condCmp((kPercFull * objActor->xspr.burnTime) / var, arg1, arg2, cmpOp)) return false;
			else if (PUSH && objActor->GetBurnSource()) condPush(aCond, objActor->GetBurnSource());
			return true;

		case 66: // any flares stuck in this sprite?
		{
			BloodStatIterator it(kStatFlare);
			while (auto flareactor = it.Next())
			{
				if (!flareactor->hasX() || (flareactor->spr.flags & kHitagFree))
					continue;

				if (flareactor->GetTarget() != objActor) continue;
				else if (PUSH) condPush(aCond, flareactor);
				return true;
			}
			return false;
		}
		case 70:
			return condCmp(getSpriteMassBySize(objActor), arg1, arg2, cmpOp); // mass of the sprite in a range?
		}
	}
	else
	{
		switch (cond)
		{
		default: return false;
		case 50:
		case 65:
		case 70:
			return condCmp(0, arg1, arg2, cmpOp);
		}
	}

	condError(aCond, "Unexpected condition id (%d)!", cond);
	return false;
}

//---------------------------------------------------------------------------
//
// this updates index of object in all conditions
// only used when spawning players
//
//---------------------------------------------------------------------------

void conditionsUpdateIndex(DBloodActor* oldActor, DBloodActor* newActor)
{
	// update index in tracking conditions first
	for (unsigned i = 0; i < gConditions.Size(); i++)
	{
		TRCONDITION* pCond = &gConditions[i];
		for (unsigned k = 0; k < pCond->objects.Size(); k++)
		{
			if (!pCond->objects[k].obj.isActor() || pCond->objects[k].obj.actor() != oldActor) continue;
			pCond->objects[k].obj = EventObject(newActor);
			break;
		}
	}

	// puke...
	auto oldSerial = EventObject(oldActor);
	auto newSerial = EventObject(newActor);

	// then update serials
	BloodStatIterator it(kStatModernCondition);
	while (auto iActor = it.Next())
	{
		if (iActor->condition[0] == oldSerial) iActor->condition[0] = newSerial;
		if (iActor->condition[1] == oldSerial) iActor->condition[1] = newSerial;

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool modernTypeSetSpriteState(DBloodActor* actor, int nState, DBloodActor* initiator)
{
	if ((actor->xspr.busy & 0xffff) == 0 && actor->xspr.state == nState)
		return false;

	actor->xspr.busy = IntToFixed(nState);
	actor->xspr.state = nState;

	evKillActor(actor, initiator);
	if (actor->xspr.restState != nState && actor->xspr.waitTime > 0)
		evPostActor(actor, (actor->xspr.waitTime * 120) / 10, actor->xspr.restState ? kCmdOn : kCmdOff, initiator);

	if (actor->xspr.txID != 0 && ((actor->xspr.triggerOn && actor->xspr.state) || (actor->xspr.triggerOff && !actor->xspr.state)))
		modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);

	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void modernTypeSendCommand(DBloodActor* actor, int destChannel, COMMAND_ID command, DBloodActor* initiator)
{
	switch (command)
	{
	case kCmdLink:
		evSendActor(actor, destChannel, kCmdModernUse, initiator); // just send command to change properties
		return;
	case kCmdUnlock:
		evSendActor(actor, destChannel, command, initiator); // send normal command first
		evSendActor(actor, destChannel, kCmdModernUse, initiator);  // then send command to change properties
		return;
	default:
		evSendActor(actor, destChannel, kCmdModernUse, initiator); // send first command to change properties
		evSendActor(actor, destChannel, command, initiator); // then send normal command
		return;
	}
}

//---------------------------------------------------------------------------
//
// this function used by various new modern types.
//
//---------------------------------------------------------------------------

void modernTypeTrigger(int destObjType, sectortype* destSect, walltype* destWall, DBloodActor* destactor, EVENT& event)
{
	if (!event.isActor()) return;
	auto sourceactor = event.getActor();
	if (!sourceactor || !sourceactor->hasX()) return;

	switch (destObjType) {
	case OBJ_SECTOR:
		if (!destSect || !destSect->hasX()) return;
		break;
	case OBJ_WALL:
		if (!destWall || !destWall->hasX()) return;
		break;
	case OBJ_SPRITE:
	{
		if (!destactor) return;
		if (destactor->spr.flags & kHitagFree) return;

		// allow redirect events received from some modern types.
		// example: it allows to spawn FX effect if event was received from kModernEffectGen
		// on many TX channels instead of just one.
		DBloodActor* initiator = event.initiator;
		switch (destactor->spr.type)
		{
		case kModernRandomTX:
		case kModernSequentialTX:
			if (destactor->xspr.command != kCmdLink || destactor->xspr.locked) break; // no redirect mode detected
			switch (destactor->spr.type)
			{
			case kModernRandomTX:
				useRandomTx(destactor, (COMMAND_ID)sourceactor->xspr.command, false, initiator); // set random TX id
				break;
			case kModernSequentialTX:
				if (destactor->spr.flags & kModernTypeFlag1)
				{
					seqTxSendCmdAll(destactor, sourceactor, (COMMAND_ID)sourceactor->xspr.command, true, initiator);
					return;
				}
				useSequentialTx(destactor, (COMMAND_ID)sourceactor->xspr.command, false, initiator); // set next TX id
				break;
			}
			if (destactor->xspr.txID <= 0 || destactor->xspr.txID >= kChannelUserMax) return;
			modernTypeSendCommand(sourceactor, destactor->xspr.txID, (COMMAND_ID)sourceactor->xspr.command, initiator);
			return;
		}
		break;
	}
	default:
		return;
	}

	switch (sourceactor->spr.type)
	{
		// allows teleport any sprite from any location to the source destination
	case kMarkerWarpDest:
		if (destObjType != OBJ_SPRITE) break;
		useTeleportTarget(sourceactor, destactor);
		break;
		// changes slope of sprite or sector
	case kModernSlopeChanger:
		switch (destObjType)
		{
		case OBJ_SPRITE:
		case OBJ_SECTOR:
			useSlopeChanger(sourceactor, destObjType, destSect, destactor);
			break;
		}
		break;
	case kModernSpriteDamager:
		// damages xsprite via TX ID or xsprites in a sector
		switch (destObjType)
		{
		case OBJ_SPRITE:
		case OBJ_SECTOR:
			useSpriteDamager(sourceactor, destObjType, destSect, destactor);
			break;
		}
		break;
		// can spawn any effect passed in data2 on it's or txID sprite
	case kModernEffectSpawner:
		if (destObjType != OBJ_SPRITE) break;
		useEffectGen(sourceactor, destactor);
		break;
		// takes data2 as SEQ ID and spawns it on it's or TX ID object
	case kModernSeqSpawner:
		useSeqSpawnerGen(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// creates wind on TX ID sector
	case kModernWindGenerator:
		if (destObjType != OBJ_SECTOR || sourceactor->xspr.data2 < 0) break;
		useSectorWindGen(sourceactor, destSect);
		break;
		// size and pan changer of sprite/wall/sector via TX ID
	case kModernObjSizeChanger:
		useObjResizer(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// iterate data filed value of destination object
	case kModernObjDataAccumulator:
		useIncDecGen(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// change data field value of destination object
	case kModernObjDataChanger:
		useDataChanger(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// change sector lighting dynamically
	case kModernSectorFXChanger:
		if (destObjType != OBJ_SECTOR) break;
		useSectorLightChanger(sourceactor, destSect);
		break;
		// change target of dudes and make it fight
	case kModernDudeTargetChanger:
		if (destObjType != OBJ_SPRITE) break;
		useTargetChanger(sourceactor, destactor);
		break;
		// change picture and palette of TX ID object
	case kModernObjPicnumChanger:
		usePictureChanger(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// change various properties
	case kModernObjPropertiesChanger:
		usePropertiesChanger(sourceactor, destObjType, destSect, destWall, destactor);
		break;
		// updated vanilla sound gen that now allows to play sounds on TX ID sprites
		// change velocity of the sprite
	case kModernVelocityChanger:
		switch (destObjType) {
		case OBJ_SPRITE:
		case OBJ_SECTOR:
			useVelocityChanger(sourceactor, destSect, event.initiator, destactor);
			break;
		}
		break;
	case kGenModernSound:
		if (destObjType != OBJ_SPRITE) break;
		useSoundGen(sourceactor, destactor);
		break;
		// updated ecto skull gen that allows to fire missile from TX ID sprites
	case kGenModernMissileUniversal:
		if (destObjType != OBJ_SPRITE) break;
		useUniMissileGen(sourceactor, destactor);
		break;
		// spawn enemies on TX ID sprites
	case kMarkerDudeSpawn:
		if (destObjType != OBJ_SPRITE) break;
		useDudeSpawn(sourceactor, destactor);
		break;
		// spawn custom dude on TX ID sprites
	case kModernCustomDudeSpawn:
		if (destObjType != OBJ_SPRITE) break;
		useCustomDudeSpawn(sourceactor, destactor);
		break;
	}
}

//---------------------------------------------------------------------------
//
// the following functions required for kModernDudeTargetChanger
//
//---------------------------------------------------------------------------

DBloodActor* aiFightGetTargetInRange(DBloodActor* actor, int minDist, int maxDist, int data, int teamMode)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);

	BloodStatIterator it(kStatDude);
	while (auto targactor = it.Next())
	{
		if (!aiFightDudeCanSeeTarget(actor, pDudeInfo, targactor)) continue;

		int dist = aiFightGetTargetDist(actor, pDudeInfo, targactor);
		if (dist < minDist || dist > maxDist) continue;
		else if (actor->GetTarget() == targactor) return targactor;
		else if (!targactor->IsDudeActor() || targactor == actor || targactor->IsPlayerActor()) continue;
		else if (IsBurningDude(targactor) || !IsKillableDude(targactor) || targactor->GetOwner() == actor) continue;
		else if ((teamMode == 1 && actor->xspr.rxID == targactor->xspr.rxID) || aiFightMatesHaveSameTarget(actor, targactor, 1)) continue;
		else if (data == 666 || targactor->xspr.data1 == data)
		{
			if (actor->GetTarget())
			{
				double fineDist1 = aiFightGetFineTargetDist(actor, actor->GetTarget());
				double fineDist2 = aiFightGetFineTargetDist(actor, targactor);
				if (fineDist1 < fineDist2)
					continue;
			}
			return targactor;
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* aiFightTargetIsPlayer(DBloodActor* actor)
{
	auto targ = actor->GetTarget();
	if (targ && targ->IsPlayerActor()) return targ;
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* aiFightGetMateTargets(DBloodActor* actor)
{
	int rx = actor->xspr.rxID;

	for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
	{
		if (rxBucket[i].isActor())
		{
			auto mate = rxBucket[i].actor();
			if (!mate || !mate->hasX() || mate == actor || !mate->IsDudeActor())
				continue;

			if (mate->GetTarget())
			{
				if (!mate->GetTarget()->IsPlayerActor())
					return mate->GetTarget();
			}
		}
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiFightMatesHaveSameTarget(DBloodActor* leaderactor, DBloodActor* targetactor, int allow)
{
	int rx = leaderactor->xspr.rxID;

	for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto mate = rxBucket[i].actor();

		if (!mate || !mate->hasX() || mate == leaderactor || !mate->IsDudeActor())
			continue;

		if (mate->GetTarget() == targetactor && allow-- <= 0)
			return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiFightDudeCanSeeTarget(DBloodActor* dudeactor, DUDEINFO* pDudeInfo, DBloodActor* targetactor)
{
	auto dv = targetactor->spr.pos.XY() - dudeactor->spr.pos.XY();

	// check target
	if (dv.Length() < pDudeInfo->SeeDist())
	{
		double height = (pDudeInfo->eyeHeight * dudeactor->spr.scale.Y);

		// is there a line of sight to the target?
		if (cansee(dudeactor->spr.pos, dudeactor->sector(), targetactor->spr.pos.plusZ(-height), targetactor->sector()))
		{
			return true;
		}

	}

	return false;

}

//---------------------------------------------------------------------------
//
// this function required if monsters in genIdle ai state. It wakes up monsters
// when kModernDudeTargetChanger goes to off state, so they won't ignore the world.
//
//---------------------------------------------------------------------------

void aiFightActivateDudes(int rx)
{
	for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto dudeactor = rxBucket[i].actor();

		if (!dudeactor || !dudeactor->hasX() || !dudeactor->IsDudeActor() || dudeactor->xspr.aiState->stateType != kAiStateGenIdle) continue;
		aiInitSprite(dudeactor);
	}
}

//---------------------------------------------------------------------------
//
// this function sets target to -1 for all dudes that hunting for nSprite
//
//---------------------------------------------------------------------------

void aiFightFreeTargets(DBloodActor* actor)
{
	BloodStatIterator it(kStatDude);
	while (auto targetactor = it.Next())
	{
		if (!targetactor->IsDudeActor() || !targetactor->hasX()) continue;
		else if (targetactor->GetTarget() == actor)
			aiSetTarget(targetactor, targetactor->spr.pos);
	}
}

//---------------------------------------------------------------------------
//
// this function sets target to -1 for all targets that hunting for dudes affected by selected kModernDudeTargetChanger
//
//---------------------------------------------------------------------------

void aiFightFreeAllTargets(DBloodActor* sourceactor)
{
	auto txID = sourceactor->xspr.txID;
	if (txID <= 0) return;
	for (int i = bucketHead[txID]; i < bucketHead[txID + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto actor = rxBucket[i].actor();

		if (actor && actor->hasX())
			aiFightFreeTargets(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiFightDudeIsAffected(DBloodActor* dudeactor)
{
	if (dudeactor->xspr.rxID <= 0 || dudeactor->xspr.locked == 1) return false;
	BloodStatIterator it(kStatModernDudeTargetChanger);
	while (auto actor = it.Next())
	{
		if (!actor->hasX()) continue;
		if (actor->xspr.txID <= 0 || actor->xspr.state != 1) continue;
		for (int i = bucketHead[actor->xspr.txID]; i < bucketHead[actor->xspr.txID + 1]; i++)
		{
			if (!rxBucket[i].isActor()) continue;
			auto rxactor = rxBucket[i].actor();

			if (!rxactor || !rxactor->hasX() || !rxactor->IsDudeActor()) continue;
			else if (rxactor == dudeactor) return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
// this function tells if there any dude found for kModernDudeTargetChanger
//
//---------------------------------------------------------------------------

bool aiFightGetDudesForBattle(DBloodActor* actor)
{
	auto txID = actor->xspr.txID;
	for (int i = bucketHead[txID]; i < bucketHead[txID + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto rxactor = rxBucket[i].actor();
		if (!rxactor || !rxactor->hasX() || !rxactor->IsDudeActor()) continue;
		if (rxactor->xspr.health > 0) return true;
	}

	// check redirected TX buckets
	int rx = -1;
	DBloodActor* pXRedir = nullptr;
	while ((pXRedir = evrListRedirectors(OBJ_SPRITE, nullptr, nullptr, actor, pXRedir, &rx)) != nullptr)
	{
		for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
		{
			if (!rxBucket[i].isActor()) continue;
			auto rxactor = rxBucket[i].actor();
			if (!rxactor || !rxactor->hasX() || !rxactor->IsDudeActor()) continue;
			if (rxactor->xspr.health > 0) return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiFightAlarmDudesInSight(DBloodActor* actor, int max)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);

	BloodStatIterator it(kStatDude);
	while (auto dudeactor = it.Next())
	{
		if (dudeactor == actor || !dudeactor->IsDudeActor() || !dudeactor->hasX())
			continue;

		if (aiFightDudeCanSeeTarget(actor, pDudeInfo, dudeactor))
		{
			if (dudeactor->GetTarget() != nullptr || dudeactor->xspr.rxID > 0)
				continue;

			aiSetTarget(dudeactor, dudeactor->spr.pos);
			aiActivateDude(dudeactor);
			if (max-- < 1)
				break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiFightUnitCanFly(DBloodActor* dude)
{
	return (dude->IsDudeActor() && gDudeInfoExtra[dude->spr.type - kDudeBase].flying);
}

bool aiFightIsMeleeUnit(DBloodActor* dude)
{
	if (dude->spr.type == kDudeModernCustom) return (dude->hasX() && dudeIsMelee(dude));
	else return (dude->IsDudeActor() && gDudeInfoExtra[dude->spr.type - kDudeBase].melee);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int aiFightGetTargetDist(DBloodActor* actor, DUDEINFO* pDudeInfo, DBloodActor* target)
{
	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	double dist = dvec.Length();

	if (dist <= pDudeInfo->MeleeDist()) return 0;
	double seeDist = pDudeInfo->SeeDist();
	if (dist >= seeDist) return 13;
	if (dist <= seeDist / 12) return 1;
	if (dist <= seeDist / 11) return 2;
	if (dist <= seeDist / 10) return 3;
	if (dist <= seeDist / 9) return 4;
	if (dist <= seeDist / 8) return 5;
	if (dist <= seeDist / 7) return 6;
	if (dist <= seeDist / 6) return 7;
	if (dist <= seeDist / 5) return 8;
	if (dist <= seeDist / 4) return 9;
	if (dist <= seeDist / 3) return 10;
	if (dist <= seeDist / 2) return 11;
	return 12;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

double aiFightGetFineTargetDist(DBloodActor* actor, DBloodActor* target)
{
	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	return (dvec).LengthSquared();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void sectorKillSounds(sectortype* pSector)
{
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.type != kSoundSector) continue;
		sfxKill3DSound(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void sectorPauseMotion(sectortype* pSector, DBloodActor* initiator)
{
	if (!pSector->hasX()) return;
	XSECTOR* pXSector = &pSector->xs();
	pXSector->unused1 = 1;

	evKillSector(pSector, initiator);

	sectorKillSounds(pSector);
	if ((pXSector->busy == 0 && !pXSector->state) || (pXSector->busy == 65536 && pXSector->state))
		SectorEndSound(pSector, pXSector->state);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void sectorContinueMotion(sectortype* pSector, EVENT event)
{
	if (!pSector->hasX()) return;

	XSECTOR* pXSector = &pSector->xs();
	pXSector->unused1 = 0;

	int busyTimeA = pXSector->busyTimeA;
	int waitTimeA = pXSector->waitTimeA;
	int busyTimeB = pXSector->busyTimeB;
	int waitTimeB = pXSector->waitTimeB;
	if (pSector->type == kSectorPath)
	{
		if (!pXSector->marker0) return;
		busyTimeA = busyTimeB = pXSector->marker0->xspr.busyTime;
		waitTimeA = waitTimeB = pXSector->marker0->xspr.waitTime;
	}

	if (!pXSector->interruptable && event.cmd != kCmdSectorMotionContinue
		&& ((!pXSector->state && pXSector->busy) || (pXSector->state && pXSector->busy != 65536)))
	{
		event.cmd = kCmdSectorMotionContinue;
	}
	else if (event.cmd == kCmdToggle)
	{
		event.cmd = (pXSector->state) ? kCmdOn : kCmdOff;
	}

	int nDelta = 1;
	switch (event.cmd)
	{
	case kCmdOff:
		if (pXSector->busy == 0)
		{
			if (pXSector->reTriggerB && waitTimeB) evPostSector(pSector, (waitTimeB * 120) / 10, kCmdOff, event.initiator);
			return;
		}
		pXSector->state = 1;
		nDelta = 65536 / ClipLow((busyTimeB * 120) / 10, 1);
		break;

	case kCmdOn:
		if (pXSector->busy == 65536)
		{
			if (pXSector->reTriggerA && waitTimeA) evPostSector(pSector, (waitTimeA * 120) / 10, kCmdOn, event.initiator);
			return;
		}
		pXSector->state = 0;
		nDelta = 65536 / ClipLow((busyTimeA * 120) / 10, 1);
		break;

	case kCmdSectorMotionContinue:
		nDelta = 65536 / ClipLow((((pXSector->state) ? busyTimeB : busyTimeA) * 120) / 10, 1);
		break;
	}

	//bool crush = pXSector->Crush;
	int busyFunc = BUSYID_0;
	switch (pSector->type)
	{
	case kSectorZMotion:
		busyFunc = BUSYID_2;
		break;
	case kSectorZMotionSprite:
		busyFunc = BUSYID_1;
		break;
	case kSectorSlideMarked:
	case kSectorSlide:
		busyFunc = BUSYID_3;
		break;
	case kSectorRotateMarked:
	case kSectorRotate:
		busyFunc = BUSYID_4;
		break;
	case kSectorRotateStep:
		busyFunc = BUSYID_5;
		break;
	case kSectorPath:
		busyFunc = BUSYID_7;
		break;
	default:
		I_Error("Unsupported sector type %d", pSector->type);
		break;
	}

	SectorStartSound(pSector, pXSector->state);
	nDelta = (pXSector->state) ? -nDelta : nDelta;
	BUSY b = { pSector, nDelta, (int)pXSector->busy, (BUSYID)busyFunc };
	gBusy.Push(b);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool modernTypeOperateSector(sectortype* pSector, const EVENT& event)
{
	auto pXSector = &pSector->xs();
	if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock)
	{
		switch (event.cmd)
		{
		case kCmdLock:
			pXSector->locked = 1;
			break;
		case kCmdUnlock:
			pXSector->locked = 0;
			break;
		case kCmdToggleLock:
			pXSector->locked = pXSector->locked ^ 1;
			break;
		}

		switch (pSector->type)
		{
		case kSectorCounter:
			if (pXSector->locked != 1) break;
			SetSectorState(pSector, 0, event.initiator.Get());
			evPostSector(pSector, 0, kCallbackCounterCheck);
			break;
		}
		return true;

		// continue motion of the paused sector
	}
	else if (pXSector->unused1)
	{
		switch (event.cmd)
		{
		case kCmdOff:
		case kCmdOn:
		case kCmdToggle:
		case kCmdSectorMotionContinue:
			sectorContinueMotion(pSector, event);
			return true;
		}

		// pause motion of the sector
	}
	else if (event.cmd == kCmdSectorMotionPause)
	{
		sectorPauseMotion(pSector, event.initiator.Get());
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void useCustomDudeSpawn(DBloodActor* pSource, DBloodActor* pActor)
{
	genDudeSpawn(pSource, pActor, pActor->clipdist * 0.5);
}

void useDudeSpawn(DBloodActor* pSource, DBloodActor* pActor)
{
	if (randomSpawnDude(pSource, pActor, pActor->clipdist * 0.5, 0) == nullptr)
		nnExtSpawnDude(pSource, pActor, pActor->xspr.data1, pActor->clipdist * 0.5, 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool modernTypeOperateSprite(DBloodActor* actor, EVENT& event)
{
	DBloodActor* initiator = event.initiator;
	if (event.cmd >= kCmdLock && event.cmd <= kCmdToggleLock)
	{
		switch (event.cmd)
		{
		case kCmdLock:
			actor->xspr.locked = 1;
			break;
		case kCmdUnlock:
			actor->xspr.locked = 0;
			break;
		case kCmdToggleLock:
			actor->xspr.locked = actor->xspr.locked ^ 1;
			break;
		}

		switch (actor->spr.type)
		{
		case kModernCondition:
		case kModernConditionFalse:
			actor->xspr.restState = 0;
			if (actor->xspr.busyTime <= 0) break;
			else if (!actor->xspr.locked) actor->xspr.busy = 0;
			break;
		}
		return true;
	}
	else if (event.cmd == kCmdDudeFlagsSet)
	{
		if (!event.isActor())
		{
			viewSetSystemMessage("Only sprites can use command #%d", event.cmd);
			return true;

		}
		else
		{
			auto pEvActor = event.getActor();
			if (pEvActor && pEvActor->hasX())
			{

				// copy dude flags from the source to destination sprite
				aiPatrolFlagsMgr(pEvActor, actor, true, false);
			}

		}
	}
	if (actor->spr.statnum == kStatDude && actor->IsDudeActor())
	{
		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.state) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			if (!actor->xspr.state) SetSpriteState(actor, 1, initiator);
			if (!actor->IsDudeActor() || actor->IsPlayerActor() || actor->xspr.health <= 0) break;
			else if (actor->xspr.aiState->stateType >= kAiStatePatrolBase && actor->xspr.aiState->stateType < kAiStatePatrolMax)
				break;

			switch (actor->xspr.aiState->stateType)
			{
			case kAiStateIdle:
			case kAiStateGenIdle:
				aiActivateDude(actor);
				break;
			}
			break;

		case kCmdDudeFlagsSet:
			if (event.isActor())
			{
				auto pEvActor = event.getActor();
				if (!pEvActor || !pEvActor->hasX()) break;
				else aiPatrolFlagsMgr(pEvActor, actor, false, true); // initialize patrol dude with possible new flags
			}
			break;

		default:
			if (!actor->xspr.state) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;
	}

	switch (actor->spr.type)
	{
	default:
		return false; // no modern type found to work with, go normal OperateSprite();
	case kThingBloodBits:
	case kThingBloodChunks:
		// dude to thing morphing causing a lot of problems since it continues receiving commands after dude is dead.
		// this leads to weird stuff like exploding with gargoyle gib or corpse disappearing immediately.
		// let's allow only specific commands here to avoid this.
		if (actor->spr.inittype < kDudeBase || actor->spr.inittype >= kDudeMax) return false;
		else if (event.cmd != kCmdToggle && event.cmd != kCmdOff && event.cmd != kCmdSpriteImpact) return true;
		DudeToGibCallback1(0, actor); // set proper gib type just in case DATAs was changed from the outside.
		return false;

	case kModernCondition:
	case kModernConditionFalse:
		if (!actor->xspr.isTriggered) useCondition(actor, event);
		return true;

		// add spawn random dude feature - works only if at least 2 data fields are not empty.
	case kMarkerDudeSpawn:
		if (!gGameOptions.nMonsterSettings) return true;
		else if (!(actor->spr.flags & kModernTypeFlag4)) useDudeSpawn(actor, actor);
		else if (actor->xspr.txID) evSendActor(actor, actor->xspr.txID, kCmdModernUse, initiator);
		return true;

	case kModernCustomDudeSpawn:
		if (!gGameOptions.nMonsterSettings) return true;
		else if (!(actor->spr.flags & kModernTypeFlag4)) useCustomDudeSpawn(actor, actor);
		else if (actor->xspr.txID) evSendActor(actor, actor->xspr.txID, kCmdModernUse, initiator);
		return true;

	case kModernRandomTX: // random Event Switch takes random data field and uses it as TX ID
	case kModernSequentialTX: // sequential Switch takes values from data fields starting from data1 and uses it as TX ID
		if (actor->xspr.command == kCmdLink) return true; // work as event redirector
		switch (actor->spr.type)
		{
		case kModernRandomTX:
			useRandomTx(actor, (COMMAND_ID)actor->xspr.command, true, initiator);
			break;

		case kModernSequentialTX:
			if (!(actor->spr.flags & kModernTypeFlag1)) useSequentialTx(actor, (COMMAND_ID)actor->xspr.command, true, initiator);
			else seqTxSendCmdAll(actor, actor, (COMMAND_ID)actor->xspr.command, false, initiator);
			break;
		}
		return true;

	case kModernSpriteDamager:
		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID > 0) modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else if (actor->xspr.data1 == 0 && actor->insector()) useSpriteDamager(actor, OBJ_SECTOR, actor->sector(), nullptr);
			else if (actor->xspr.data1 >= 666 && actor->xspr.data1 < 669) useSpriteDamager(actor, -1, nullptr, nullptr);
			else
			{
				DBloodPlayer* pPlayer = getPlayerById(actor->xspr.data1);
				if (pPlayer != NULL)
					useSpriteDamager(actor, OBJ_SPRITE, 0, pPlayer->GetActor());
			}

			if (actor->xspr.busyTime > 0)
				evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;

		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kMarkerWarpDest:
		if (actor->xspr.txID <= 0) 
		{
			DBloodPlayer* pPlayer = getPlayerById(actor->xspr.data1);
			if (pPlayer != NULL && SetSpriteState(actor, actor->xspr.state ^ 1, initiator) == 1)
				useTeleportTarget(actor, pPlayer->GetActor());
			return true;
		}
		[[fallthrough]];
	case kModernObjPropertiesChanger:
		if (actor->xspr.txID <= 0)
		{
			if (SetSpriteState(actor, actor->xspr.state ^ 1, initiator) == 1)
				usePropertiesChanger(actor, -1, nullptr, nullptr, nullptr);
			return true;
		}
		[[fallthrough]];
	case kModernSlopeChanger:
	case kModernObjSizeChanger:
	case kModernObjPicnumChanger:
	case kModernSectorFXChanger:
	case kModernObjDataChanger:
		modernTypeSetSpriteState(actor, actor->xspr.state ^ 1, initiator);
		return true;

	case kModernSeqSpawner:
	case kModernEffectSpawner:
		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.state == 1) 
			{
				SetSpriteState(actor, 0, initiator);
				if (actor->spr.type == kModernEffectSpawner)
					killEffectGenCallbacks(actor);
			}
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			if (actor->spr.type == kModernSeqSpawner) seqSpawnerOffSameTx(actor);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID > 0) modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else if (actor->spr.type == kModernSeqSpawner) useSeqSpawnerGen(actor, OBJ_SPRITE, nullptr, nullptr, actor);
			else useEffectGen(actor, nullptr);

			if (actor->xspr.busyTime > 0)
				evPostActor(actor, ClipLow((int(actor->xspr.busyTime) + Random2(actor->xspr.data1)) * 120 / 10, 0), kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kModernWindGenerator:
		switch (event.cmd)
		{
		case kCmdOff:
			windGenStopWindOnSectors(actor);
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID > 0) modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else useSectorWindGen(actor, nullptr);
			if (actor->xspr.busyTime > 0) evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kModernVelocityChanger:
		switch (event.cmd) 
		{
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID > 0) modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else useVelocityChanger(actor, actor->sector(), initiator, nullptr);
			if (actor->xspr.busyTime > 0) evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kModernDudeTargetChanger:

		// this one is required if data4 of generator was dynamically changed
		// it turns monsters in normal idle state instead of genIdle, so they not ignore the world.
		if (actor->xspr.dropMsg == 3 && 3 != actor->xspr.data4)
			aiFightActivateDudes(actor->xspr.txID);

		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.data4 == 3) aiFightActivateDudes(actor->xspr.txID);
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID <= 0 || !aiFightGetDudesForBattle(actor))
			{
				aiFightFreeAllTargets(actor);
				evPostActor(actor, 0, kCmdOff, initiator);
				break;
			}
			else
			{
				modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			}

			if (actor->xspr.busyTime > 0) evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		actor->xspr.dropMsg = uint8_t(actor->xspr.data4);
		return true;

	case kGenTrigger:
		if (!(actor->spr.flags & kModernTypeFlag1)) return false; // work as vanilla generator
		switch (event.cmd) { // work as fast generator
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID)
				evSendActor(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			if (actor->xspr.busyTime > 0) evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kModernObjDataAccumulator:
		switch (event.cmd) {
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			// force OFF after *all* TX objects reach the goal value
			if (actor->spr.flags == kModernTypeFlag0 && incDecGoalValueIsReached(actor))
			{
				evPostActor(actor, 0, kCmdOff, initiator);
				break;
			}

			modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			if (actor->xspr.busyTime > 0) evPostActor(actor, actor->xspr.busyTime, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;
	case kModernRandom:
	case kModernRandom2:
		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			useRandomItemGen(actor);
			if (actor->xspr.busyTime > 0)
				evPostActor(actor, (120 * actor->xspr.busyTime) / 10, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;

	case kModernThingTNTProx:
		if (actor->spr.statnum != kStatRespawn)
		{
			switch (event.cmd)
			{
			case kCmdSpriteProximity:
				if (actor->xspr.state) break;
				sfxPlay3DSound(actor, 452, 0, 0);
				evPostActor(actor, 30, kCmdOff, nullptr);
				actor->xspr.state = 1;
				[[fallthrough]];
			case kCmdOn:
				sfxPlay3DSound(actor, 451, 0, 0);
				actor->xspr.Proximity = 1;
				break;
			default:
				actExplodeSprite(actor);
				break;
			}
		}
		return true;
	case kModernThingEnemyLifeLeech:
		dudeLeechOperate(actor, event);
		return true;
	case kModernPlayerControl: 
	{ // WIP
		DBloodPlayer* pPlayer = NULL; 
		int cmd = (event.cmd >= kCmdNumberic) ? event.cmd : actor->xspr.command;


		int playerID;
		if ((actor->xspr.txID == kChannelEventCauser || actor->xspr.data1 == 0) && initiator && initiator->IsPlayerActor())
			playerID = initiator->spr.type;
		else
			playerID = actor->xspr.data1;

		if ((pPlayer = getPlayerById(playerID)) == NULL
			|| ((cmd < 67 || cmd > 68) && !modernTypeSetSpriteState(actor, actor->xspr.state ^ 1, initiator)))
			return true;

		TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->pnum];

		/// !!! COMMANDS OF THE CURRENT SPRITE, NOT OF THE EVENT !!! ///
		if ((cmd -= kCmdNumberic) < 0) return true;
		else if (pPlayer->GetActor()->xspr.health <= 0)
		{

			switch (cmd) {
			case 36:
				actHealDude(pPlayer->GetActor(), ((actor->xspr.data2 > 0) ? ClipHigh(actor->xspr.data2, 200) : getDudeInfo(pPlayer->GetActor()->spr.type)->startHealth), 200);
				pPlayer->curWeapon = kWeapPitchFork;
				break;
			}
			return true;
		}

		switch (cmd)
		{
		case 0: // 64 (player life form)
			if (actor->xspr.data2 < kModeHuman || actor->xspr.data2 > kModeHumanGrown) break;
			else trPlayerCtrlSetRace(actor->xspr.data2, pPlayer);
			break;
		case 1: // 65 (move speed and jump height)
			// player movement speed (for all races and postures)
			if (valueIsBetween(actor->xspr.data2, -1, 32767))
				trPlayerCtrlSetMoveSpeed(actor->xspr.data2, pPlayer);

			// player jump height (for all races and stand posture only)
			if (valueIsBetween(actor->xspr.data3, -1, 32767))
				trPlayerCtrlSetJumpHeight(actor->xspr.data3, pPlayer);
			break;
		case 2: // 66 (player screen effects)
			if (actor->xspr.data3 < 0) break;
			else trPlayerCtrlSetScreenEffect(actor->xspr.data2, actor->xspr.data3, pPlayer);
			break;
		case 3: // 67 (start playing qav scene)
			trPlayerCtrlStartScene(actor, pPlayer, (actor->xspr.data4 == 1) ? true : false);
			break;
		case 4: // 68 (stop playing qav scene)
			if (actor->xspr.data2 > 0 && actor->xspr.data2 != pPlayer->sceneQav) break;
			else trPlayerCtrlStopScene(pPlayer);
			break;
		case 5: // 69 (set player look angle, TO-DO: if tx > 0, take a look on TX ID sprite)
			//data4 is reserved
			if (actor->xspr.data4 != 0) break;
			else if (valueIsBetween(actor->xspr.data2, -128, 128))
				trPlayerCtrlSetLookAngle(actor->xspr.data2, pPlayer);
			break;
		case 6: // 70 (erase player stuff...)
			if (actor->xspr.data2 < 0) break;
			else trPlayerCtrlEraseStuff(actor->xspr.data2, pPlayer);
			break;
		case 7: // 71 (give something to player...)
			if (actor->xspr.data2 <= 0) break;
			else trPlayerCtrlGiveStuff(actor->xspr.data2, actor->xspr.data3, actor->xspr.data4, pPlayer, pCtrl);
			break;
		case 8: // 72 (use inventory item)
			if (actor->xspr.data2 < 1 || actor->xspr.data2 > 5) break;
			else trPlayerCtrlUsePackItem(actor->xspr.data2, actor->xspr.data3, actor->xspr.data4, pPlayer, event.cmd);
			break;
		case 9: // 73 (set player's sprite angle, TO-DO: if tx > 0, take a look on TX ID sprite)
			//data4 is reserved
			if (actor->xspr.data4 != 0) break;
			else if (actor->spr.flags & kModernTypeFlag1)
			{
				gameInput.ForceInputSync(pPlayer->pnum);
				pPlayer->GetActor()->spr.Angles.Yaw = actor->spr.Angles.Yaw;
			}
			else if (valueIsBetween(actor->xspr.data2, -kAng360, kAng360))
			{
				gameInput.ForceInputSync(pPlayer->pnum);
				pPlayer->GetActor()->spr.Angles.Yaw = mapangle(actor->xspr.data2);
			}
			break;
		case 10: // 74 (de)activate powerup
			if (actor->xspr.data2 <= 0 || actor->xspr.data2 > (kMaxAllowedPowerup - (kMinAllowedPowerup << 1) + 1)) break;
			trPlayerCtrlUsePowerup(actor, pPlayer, event.cmd);
			break;
			// case 11: // 75 (print the book)
				 // data2: RFF TXT id
				 // data3: background tile
				 // data4: font base tile
				 // pal: font / background palette
				 // hitag:
				 // d1: 0: print whole text at a time, 1: print line by line, 2: word by word, 3: letter by letter
				 // d2: 1: force pause the game (sp only)
				 // d3: 1: inherit palette for font, 2: inherit palette for background, 3: both
				 // busyTime: speed of word/letter/line printing
				 // waitTime: if TX ID > 0 and TX ID object is book reader, trigger it?
				 //break;

		}
	}
							 return true;

	case kGenModernSound:
		switch (event.cmd) {
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID)  modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else useSoundGen(actor, actor);

			if (actor->xspr.busyTime > 0)
				evPostActor(actor, (120 * actor->xspr.busyTime) / 10, kCmdRepeat, initiator);
			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;
	case kGenModernMissileUniversal:
		switch (event.cmd)
		{
		case kCmdOff:
			if (actor->xspr.state == 1) SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			evKillActor(actor, initiator); // queue overflow protect
			if (actor->xspr.state == 0) SetSpriteState(actor, 1, initiator);
			[[fallthrough]];
		case kCmdRepeat:
			if (actor->xspr.txID)  modernTypeSendCommand(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			else useUniMissileGen(actor, actor);

			if (actor->xspr.busyTime > 0)
				evPostActor(actor, (120 * actor->xspr.busyTime) / 10, kCmdRepeat, initiator);

			break;
		default:
			if (actor->xspr.state == 0) evPostActor(actor, 0, kCmdOn, initiator);
			else evPostActor(actor, 0, kCmdOff, initiator);
			break;
		}
		return true;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool modernTypeOperateWall(walltype* pWall, const EVENT& event)
{
	DBloodActor* initiator = event.initiator.Get();
	switch (pWall->type)
	{
	case kSwitchOneWay:
		switch (event.cmd) {
		case kCmdOff:
			SetWallState(pWall, 0, initiator);
			break;
		case kCmdOn:
			SetWallState(pWall, 1, initiator);
			break;
		default:
			SetWallState(pWall, pWall->xw().restState ^ 1, initiator);
			break;
		}
		return true;
	default:
		return false; // no modern type found to work with, go normal OperateWall();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool txIsRanged(DBloodActor* sourceactor)
{
	if (!sourceactor->hasX()) return false;
	if (sourceactor->xspr.data1 > 0 && sourceactor->xspr.data2 <= 0 && sourceactor->xspr.data3 <= 0 && sourceactor->xspr.data4 > 0)
	{
		if (sourceactor->xspr.data1 > sourceactor->xspr.data4)
		{
			// data1 must be less than data4
			int tmp = sourceactor->xspr.data1; sourceactor->xspr.data1 = sourceactor->xspr.data4;
			sourceactor->xspr.data4 = tmp;
		}
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqTxSendCmdAll(DBloodActor* sourceactor, DBloodActor* actor, COMMAND_ID cmd, bool modernSend, DBloodActor* initiator)
{
	bool ranged = txIsRanged(sourceactor);
	if (ranged)
	{
		for (sourceactor->xspr.txID = sourceactor->xspr.data1; sourceactor->xspr.txID <= sourceactor->xspr.data4; sourceactor->xspr.txID++)
		{
			if (sourceactor->xspr.txID <= 0 || sourceactor->xspr.txID >= kChannelUserMax) continue;
			else if (!modernSend) evSendActor(actor, sourceactor->xspr.txID, cmd, initiator);
			else modernTypeSendCommand(actor, sourceactor->xspr.txID, cmd, initiator);
		}
	}
	else
	{
		for (int i = 0; i <= 3; i++)
		{
			sourceactor->xspr.txID = GetDataVal(sourceactor, i);
			if (sourceactor->xspr.txID <= 0 || sourceactor->xspr.txID >= kChannelUserMax) continue;
			else if (!modernSend) evSendActor(actor, sourceactor->xspr.txID, cmd, initiator);
			else modernTypeSendCommand(actor, sourceactor->xspr.txID, cmd, initiator);
		}
	}
	sourceactor->xspr.txID = sourceactor->xspr.sysData1 = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useRandomTx(DBloodActor* sourceactor, COMMAND_ID cmd, bool setState, DBloodActor* initiator)
{
	int tx = 0; int maxRetries = kMaxRandomizeRetries;

	if (txIsRanged(sourceactor))
	{
		while (maxRetries-- >= 0)
		{
			if ((tx = nnExtRandom(sourceactor->xspr.data1, sourceactor->xspr.data4)) != sourceactor->xspr.txID)
				break;
		}
	}
	else
	{
		while (maxRetries-- >= 0)
		{
			if ((tx = randomGetDataValue(sourceactor, kRandomizeTX)) > 0 && tx != sourceactor->xspr.txID)
				break;
		}
	}

	sourceactor->xspr.txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
	if (setState)
		SetSpriteState(sourceactor, sourceactor->xspr.state ^ 1, initiator);
	//evSendActor(sourceactor->spr.index, sourceactor->xspr.txID, (COMMAND_ID)sourceactor->xspr.command);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useSequentialTx(DBloodActor* sourceactor, COMMAND_ID cmd, bool setState, DBloodActor* initiator)
{
	bool range = txIsRanged(sourceactor); int cnt = 3; int tx = 0;

	if (range)
	{
		// make sure sysData is correct as we store current index of TX ID here.
		if (sourceactor->xspr.sysData1 < sourceactor->xspr.data1) sourceactor->xspr.sysData1 = sourceactor->xspr.data1;
		else if (sourceactor->xspr.sysData1 > sourceactor->xspr.data4) sourceactor->xspr.sysData1 = sourceactor->xspr.data4;

	}
	else
	{
		// make sure sysData is correct as we store current index of data field here.
		if (sourceactor->xspr.sysData1 > 3) sourceactor->xspr.sysData1 = 0;
		else if (sourceactor->xspr.sysData1 < 0) sourceactor->xspr.sysData1 = 3;
	}

	switch (cmd)
	{
	case kCmdOff:
		if (!range)
		{
			while (cnt-- >= 0) // skip empty data fields
			{
				if (sourceactor->xspr.sysData1-- < 0) sourceactor->xspr.sysData1 = 3;
				if ((tx = GetDataVal(sourceactor, sourceactor->xspr.sysData1)) <= 0) continue;
				else break;
			}
		}
		else
		{
			if (--sourceactor->xspr.sysData1 < sourceactor->xspr.data1) sourceactor->xspr.sysData1 = sourceactor->xspr.data4;
			tx = sourceactor->xspr.sysData1;
		}
		break;

	default:
		if (!range)
		{
			while (cnt-- >= 0) // skip empty data fields
			{
				if (sourceactor->xspr.sysData1 > 3) sourceactor->xspr.sysData1 = 0;
				if ((tx = GetDataVal(sourceactor, sourceactor->xspr.sysData1++)) <= 0) continue;
				else break;
			}
		}
		else
		{
			tx = sourceactor->xspr.sysData1;
			if (sourceactor->xspr.sysData1 >= sourceactor->xspr.data4)
			{
				sourceactor->xspr.sysData1 = sourceactor->xspr.data1;
				break;
			}
			sourceactor->xspr.sysData1++;
		}
		break;
	}

	sourceactor->xspr.txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
	if (setState)
		SetSpriteState(sourceactor, sourceactor->xspr.state ^ 1, initiator);
	//evSendActor(sourceactor->spr.index, sourceactor->xspr.txID, (COMMAND_ID)sourceactor->xspr.command);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int useCondition(DBloodActor* sourceactor, EVENT& event)
{
	bool srcIsCondition = false;

	auto const pActor = event.isActor() ? event.getActor() : nullptr;

	if (event.isActor() && pActor == nullptr) return -1;
	if (event.isActor() && pActor != sourceactor)
		srcIsCondition = (pActor->spr.type == kModernCondition || pActor->spr.type == kModernConditionFalse);

	// if it's a tracking condition, it must ignore all the commands sent from objects
	if (sourceactor->xspr.busyTime > 0 && event.funcID != kCallbackMax) return -1;
	else if (!srcIsCondition) // save object serials in the stack and make copy of initial object
	{
		condPush(sourceactor, event.target);
		condBackup(sourceactor);
	}
	else  // or grab serials of objects from previous conditions
	{
		sourceactor->condition[0] = pActor->condition[0];
		sourceactor->condition[1] = pActor->condition[1];

	}

	int cond = sourceactor->xspr.data1;
	bool ok = false;
	bool RVRS = (sourceactor->spr.type == kModernConditionFalse);
	bool RSET = (sourceactor->xspr.command == kCmdNumberic + 36);
	bool PUSH = (sourceactor->xspr.command == kCmdNumberic);
	int comOp = sourceactor->spr.cstat; // comparison operator

	if (sourceactor->xspr.restState == 0)
	{
		if (cond == 0) ok = true; // dummy
		else if (cond >= kCondGameBase && cond < kCondGameMax) ok = condCheckGame(sourceactor, event, comOp, PUSH);
		else if (cond >= kCondMixedBase && cond < kCondMixedMax) ok = condCheckMixed(sourceactor, event, comOp, PUSH);
		else if (cond >= kCondWallBase && cond < kCondWallMax) ok = condCheckWall(sourceactor, comOp, PUSH);
		else if (cond >= kCondSectorBase && cond < kCondSectorMax) ok = condCheckSector(sourceactor, comOp, PUSH);
		else if (cond >= kCondPlayerBase && cond < kCondPlayerMax) ok = condCheckPlayer(sourceactor, comOp, PUSH);
		else if (cond >= kCondDudeBase && cond < kCondDudeMax) ok = condCheckDude(sourceactor, comOp, PUSH);
		else if (cond >= kCondSpriteBase && cond < kCondSpriteMax) ok = condCheckSprite(sourceactor, comOp, PUSH);
		else condError(sourceactor, "Unexpected condition id %d!", cond);

		sourceactor->xspr.state = (ok ^ RVRS);

		if (sourceactor->xspr.waitTime > 0 && sourceactor->xspr.state > 0)
		{
			sourceactor->xspr.restState = 1;
			evKillActor(sourceactor);
			evPostActor(sourceactor, (sourceactor->xspr.waitTime * 120) / 10, kCmdRepeat, event.initiator);
			return -1;
		}
	}
	else if (event.cmd == kCmdRepeat)
	{
		sourceactor->xspr.restState = 0;
	}
	else
	{
		return -1;
	}

	int retn = sourceactor->xspr.state;
	if (retn)
	{
		sourceactor->xspr.isTriggered = sourceactor->xspr.triggerOnce;

		if (RSET)
			condRestore(sourceactor); // reset focus to the initial object

		// send command to rx bucket
		if (sourceactor->xspr.txID)
		{
			evSendActor(sourceactor, sourceactor->xspr.txID, (COMMAND_ID)sourceactor->xspr.command, sourceactor->condition[0].isActor()? sourceactor->condition[0].actor() : nullptr);
		}

		if (sourceactor->spr.flags) {

			// send it for object currently in the focus
			if (sourceactor->spr.flags & kModernTypeFlag1)
			{
				nnExtTriggerObject(event.target, sourceactor->xspr.command, sourceactor);
			}

			// send it for initial object
			if ((sourceactor->spr.flags & kModernTypeFlag2) && (sourceactor->condition[0] != sourceactor->condition[1] || !(sourceactor->spr.hitag & kModernTypeFlag1)))
			{
				auto co = condGet(sourceactor);
				nnExtTriggerObject(co, sourceactor->xspr.command, sourceactor);
			}
		}
	}
	return retn;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useRandomItemGen(DBloodActor* actor)
{
	// let's first search for previously dropped items and remove it
	if (actor->xspr.dropMsg > 0)
	{
		BloodStatIterator it(kStatItem);
		while (auto iactor = it.Next())
		{
			if ((unsigned int)iactor->spr.type == actor->xspr.dropMsg && iactor->spr.pos == actor->spr.pos)
			{
				gFX.fxSpawnActor((FX_ID)29, actor->sector(), actor->spr.pos);
				iactor->spr.type = kSpriteDecoration;
				actPostSprite(iactor, kStatFree);
				break;
			}
		}
	}

	// then drop item
	auto dropactor = randomDropPickupObject(actor, actor->xspr.dropMsg);

	if (dropactor != nullptr)
	{
		clampSprite(dropactor);

		// check if generator affected by physics
		if (debrisGetIndex(actor) != -1)
		{
			dropactor->addX();
			int nIndex = debrisGetFreeIndex();
			if (nIndex >= 0)
			{
				dropactor->xspr.physAttr |= kPhysMove | kPhysGravity | kPhysFalling; // must fall always
				actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;

				gPhysSpritesList[nIndex] = dropactor;
				if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
				getSpriteMassBySize(dropactor); // create mass cache
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useUniMissileGen(DBloodActor* sourceactor, DBloodActor* actor)
{
	if (actor == nullptr) actor = sourceactor;
	DVector3 dv(0, 0, 0);

	if (sourceactor->xspr.data1 < kMissileBase || sourceactor->xspr.data1 >= kMissileMax)
		return;

	if (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
	{
		if (actor->spr.cstat & CSTAT_SPRITE_YFLIP) dv.Z = 1;
		else dv.Z = -1;
	}
	else
	{
		dv = DVector3(
			actor->spr.Angles.Yaw.ToVector(),
			clamp(sourceactor->xspr.data3 / 256., -4., 4.) // add slope controlling
		);
	}

	auto missileactor = actFireMissile(actor, 0, 0, dv, actor->xspr.data1);
	if (missileactor)
	{
		int from; // inherit some properties of the generator
		if ((from = (sourceactor->spr.flags & kModernTypeFlag3)) > 0)
		{
			int canInherit = 0xF;
			if (missileactor->hasX() && seqGetStatus(missileactor) >= 0)
			{
				canInherit &= ~0x8;

				SEQINST* pInst = GetInstance(missileactor);
				Seq* pSeq = pInst->pSequence;
				for (int i = 0; i < pSeq->nFrames; i++)
				{
					if ((canInherit & 0x4) && pSeq->frames[i].palette != 0) canInherit &= ~0x4;
					if ((canInherit & 0x2) && pSeq->frames[i].scalex != 0) canInherit &= ~0x2;
					if ((canInherit & 0x1) && pSeq->frames[i].scaley != 0) canInherit &= ~0x1;
				}
			}

			if (canInherit != 0)
			{
				if (canInherit & 0x2)
					missileactor->spr.scale.X = ((from == kModernTypeFlag1) ? sourceactor->spr.scale.X : actor->spr.scale.X);

				if (canInherit & 0x1)
					missileactor->spr.scale.Y = ((from == kModernTypeFlag1) ? sourceactor->spr.scale.Y : actor->spr.scale.Y);

				if (canInherit & 0x4)
					missileactor->spr.pal = (from == kModernTypeFlag1) ? sourceactor->spr.pal : actor->spr.pal;

				if (canInherit & 0x8)
					missileactor->spr.shade = (from == kModernTypeFlag1) ? sourceactor->spr.shade : actor->spr.shade;
			}
		}

		// add velocity controlling
		if (sourceactor->xspr.data2 > 0)
		{
			missileactor->vel = dv * sourceactor->xspr.data2 / 16.;
		}

		// add bursting for missiles
		if (missileactor->spr.type != kMissileFlareAlt && sourceactor->xspr.data4 > 0)
			evPostActor(missileactor, ClipHigh(sourceactor->xspr.data4, 500), kCallbackMissileBurst);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useSoundGen(DBloodActor* sourceactor, DBloodActor* actor)
{
	int pitch = sourceactor->xspr.data4 << 1;
	if (pitch < 2000) pitch = 0;
	sfxPlay3DSoundVolume(actor, sourceactor->xspr.data2, -1, 0, pitch, sourceactor->xspr.data3);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useIncDecGen(DBloodActor* sourceactor, int objType, sectortype* destSect, walltype* destWall, DBloodActor* objactor)
{
	char buffer[7];
	int data = -65535;
	short tmp = 0;
	int dataIndex = 0;
	snprintf(buffer, 7, "%d", abs(sourceactor->xspr.data1));
	int len = int(strlen(buffer));

	for (int i = 0; i < len; i++)
	{
		dataIndex = (buffer[i] - 52) + 4;
		if ((data = getDataFieldOfObject(objType, destSect, destWall, objactor, dataIndex)) == -65535)
		{
			Printf(PRINT_HIGH, "\nWrong index of data (%c) for IncDec Gen! Only 1, 2, 3 and 4 indexes allowed!\n", buffer[i]);
			continue;
		}

		if (sourceactor->xspr.data2 < sourceactor->xspr.data3)
		{
			data = ClipRange(data, sourceactor->xspr.data2, sourceactor->xspr.data3);
			if ((data += sourceactor->xspr.data4) >= sourceactor->xspr.data3)
			{
				switch (sourceactor->spr.flags)
				{
				case kModernTypeFlag0:
				case kModernTypeFlag1:
					if (data > sourceactor->xspr.data3) data = sourceactor->xspr.data3;
					break;
				case kModernTypeFlag2:
					if (data > sourceactor->xspr.data3) data = sourceactor->xspr.data3;
					if (!incDecGoalValueIsReached(sourceactor)) break;
					tmp = sourceactor->xspr.data3;
					sourceactor->xspr.data3 = sourceactor->xspr.data2;
					sourceactor->xspr.data2 = tmp;
					break;
				case kModernTypeFlag3:
					if (data > sourceactor->xspr.data3) data = sourceactor->xspr.data2;
					break;
				}
			}

		}
		else if (sourceactor->xspr.data2 > sourceactor->xspr.data3)
		{
			data = ClipRange(data, sourceactor->xspr.data3, sourceactor->xspr.data2);
			if ((data -= sourceactor->xspr.data4) <= sourceactor->xspr.data3)
			{
				switch (sourceactor->spr.flags)
				{
				case kModernTypeFlag0:
				case kModernTypeFlag1:
					if (data < sourceactor->xspr.data3) data = sourceactor->xspr.data3;
					break;
				case kModernTypeFlag2:
					if (data < sourceactor->xspr.data3) data = sourceactor->xspr.data3;
					if (!incDecGoalValueIsReached(sourceactor)) break;
					tmp = sourceactor->xspr.data3;
					sourceactor->xspr.data3 = sourceactor->xspr.data2;
					sourceactor->xspr.data2 = tmp;
					break;
				case kModernTypeFlag3:
					if (data < sourceactor->xspr.data3) data = sourceactor->xspr.data2;
					break;
				}
			}
		}
		sourceactor->xspr.sysData1 = data;
		setDataValueOfObject(objType, destSect, destWall, objactor, dataIndex, data);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void sprite2sectorSlope(DBloodActor* actor, sectortype* pSector, int rel, bool forcez)
{
	int slope = 0;
	double z = 0;
	switch (rel) {
	default:
		z = getflorzofslopeptr(actor->sector(), actor->spr.pos);
		if ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) && actor->hasX() && actor->xspr.Touch) z--;
		slope = pSector->floorheinum;
		break;
	case 1:
		z = getceilzofslopeptr(actor->sector(), actor->spr.pos);
		if ((actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) && actor->hasX() && actor->xspr.Touch) z++;
		slope = pSector->ceilingheinum;
		break;
	}

	spriteSetSlope(actor, slope);
	if (forcez) actor->spr.pos.Z = z;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useSlopeChanger(DBloodActor* sourceactor, int objType, sectortype* pSect, DBloodActor* objActor)
{
	int slope, oslope;
	bool flag2 = (sourceactor->spr.flags & kModernTypeFlag2);

	if (sourceactor->spr.flags & kModernTypeFlag1) slope = ClipRange(sourceactor->xspr.data2, -32767, 32767);
	else slope = (32767 / kPercFull) * ClipRange(sourceactor->xspr.data2, -kPercFull, kPercFull);

	if (objType == OBJ_SECTOR)
	{
		switch (sourceactor->xspr.data1)
		{
		case 2:
		case 0:

			// just set floor slope
			if (flag2)
			{
				pSect->setfloorslope(slope);
			}
			else
			{
				// force closest floor aligned sprites to inherit slope of the sector's floor
				oslope = pSect->floorheinum;
				BloodSectIterator it(pSect);
				while (auto iactor = it.Next())
				{
					if (!(iactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) continue;
					else if (getflorzofslopeptr(pSect, iactor->spr.pos) - kSlopeDist <= iactor->spr.pos.Z)
					{
						sprite2sectorSlope(iactor, pSect, 0, true);

						// set temporary slope of floor
						pSect->floorheinum = slope;

						// force sloped sprites to be on floor slope z
						sprite2sectorSlope(iactor, pSect, 0, true);

						// restore old slope for next sprite
						pSect->floorheinum = oslope;
					}
				}

				// finally set new slope of floor
				pSect->setfloorslope(slope);

			}

			if (sourceactor->xspr.data1 == 0) break;
			[[fallthrough]];
		case 1:

			// just set ceiling slope
			if (flag2)
			{
				pSect->setceilingslope(slope);
			}
			else
			{
				oslope = pSect->ceilingheinum;
				BloodSectIterator it(pSect);
				while (auto iactor = it.Next())
				{
					if (!(iactor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) continue;
					else if (getceilzofslopeptr(pSect, iactor->spr.pos) + kSlopeDist >= iactor->spr.pos.Z)
					{
						sprite2sectorSlope(iactor, pSect, 1, true);

						// set new slope of ceiling
						pSect->ceilingheinum = slope;

						// force sloped sprites to be on ceiling slope z
						sprite2sectorSlope(iactor, pSect, 1, true);

						// restore old slope for next sprite
						pSect->ceilingheinum = oslope;
					}
				}

				// finally set new slope of ceiling
				pSect->setceilingslope(slope);

			}
			break;
		}

		// let's give a little impulse to the physics sprites...
		BloodSectIterator it(pSect);
		while (auto iactor = it.Next())
		{
			if (iactor->hasX() && iactor->xspr.physAttr > 0)
			{
				iactor->xspr.physAttr |= kPhysFalling;
				iactor->vel.Z += FixedToFloat(1);
			}
			else if ((iactor->spr.statnum == kStatThing || iactor->spr.statnum == kStatDude) && (iactor->spr.flags & kPhysGravity))
			{
				iactor->spr.flags |= kPhysFalling;
				iactor->vel.Z += FixedToFloat(1);
			}
		}
	}
	else if (objType == OBJ_SPRITE)
	{
		if (!(objActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) objActor->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		if ((objActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_SLOPE) != CSTAT_SPRITE_ALIGNMENT_SLOPE)
			objActor->spr.cstat |= CSTAT_SPRITE_ALIGNMENT_SLOPE;

		switch (sourceactor->xspr.data4)
		{
		case 1:
		case 2:
		case 3:
			if (!objActor->insector()) break;
			switch (sourceactor->xspr.data4)
			{
			case 1: sprite2sectorSlope(objActor, objActor->sector(), 0, flag2); break;
			case 2: sprite2sectorSlope(objActor, objActor->sector(), 1, flag2); break;
			case 3:
				if (getflorzofslopeptr(objActor->sector(), objActor->spr.pos) - kSlopeDist <= objActor->spr.pos.Z) sprite2sectorSlope(objActor, objActor->sector(), 0, flag2);
				if (getceilzofslopeptr(objActor->sector(), objActor->spr.pos) + kSlopeDist >= objActor->spr.pos.Z) sprite2sectorSlope(objActor, objActor->sector(), 1, flag2);
				break;
			}
			break;
		default:
			spriteSetSlope(objActor, slope);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useDataChanger(DBloodActor* sourceactor, int objType, sectortype* pSector, walltype* pWall, DBloodActor* objActor)
{
	bool flag1 = (sourceactor->spr.flags & kModernTypeFlag1);
	switch (objType) {
	case OBJ_SECTOR:
		if (flag1 || valueIsBetween(sourceactor->xspr.data1, -1, 32767)) setDataValueOfObject(objType, pSector, pWall, nullptr, 1, sourceactor->xspr.data1);
		break;
	case OBJ_SPRITE:
		if (flag1 || valueIsBetween(sourceactor->xspr.data1, -1, 32767)) setDataValueOfObject(objType, pSector, pWall, objActor, 1, sourceactor->xspr.data1);
		if (flag1 || valueIsBetween(sourceactor->xspr.data2, -1, 32767)) setDataValueOfObject(objType, pSector, pWall, objActor, 2, sourceactor->xspr.data1);
		if (flag1 || valueIsBetween(sourceactor->xspr.data3, -1, 32767)) setDataValueOfObject(objType, pSector, pWall, objActor, 3, sourceactor->xspr.data1);
		if (flag1 || valueIsBetween(sourceactor->xspr.data4, -1, 65535)) setDataValueOfObject(objType, pSector, pWall, objActor, 4, sourceactor->xspr.data1);
		break;
	case OBJ_WALL:
		if (flag1 || valueIsBetween(sourceactor->xspr.data1, -1, 32767)) setDataValueOfObject(objType, pSector, pWall, nullptr, 1, sourceactor->xspr.data1);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


void useSectorLightChanger(DBloodActor* sourceactor, sectortype* pSector)
{
	bool relative = (sourceactor->spr.flags & kModernTypeFlag16);
	auto pXSector = &pSector->xs();

	if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
	{
		if (relative)
			pXSector->wave = ClipHigh(pXSector->wave + sourceactor->xspr.data1, 11);
		else
			pXSector->wave = ClipHigh(sourceactor->xspr.data1, 11);
	}

	if (valueIsBetween(sourceactor->xspr.data2, -128, 128))
	{
		if (relative)
			pXSector->amplitude = ClipRange(pXSector->amplitude + sourceactor->xspr.data2, -127, 127);
		else
			pXSector->amplitude = (int8_t)sourceactor->xspr.data2;
	}

	if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
	{
		if (relative)
			pXSector->freq = ClipHigh(pXSector->freq + sourceactor->xspr.data3, 255);
		else
			pXSector->freq = ClipHigh(sourceactor->xspr.data3, 255);
	}

	if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
	{
		if (relative)
			pXSector->phase = ClipHigh(pXSector->phase + sourceactor->xspr.data4, 255);
		else
			pXSector->phase = ClipHigh(sourceactor->xspr.data4, 255);
	}

	if (sourceactor->spr.flags)
	{
		if (sourceactor->spr.flags != kModernTypeFlag1)
		{
			pXSector->shadeAlways = (sourceactor->spr.flags & kModernTypeFlag1) ? true : false;
			pXSector->shadeFloor = (sourceactor->spr.flags & kModernTypeFlag2) ? true : false;
			pXSector->shadeCeiling = (sourceactor->spr.flags & kModernTypeFlag4) ? true : false;
			pXSector->shadeWalls = (sourceactor->spr.flags & kModernTypeFlag8) ? true : false;
			pXSector->color = (sourceactor->spr.pal) ? true : false;

			auto cstat = sourceactor->spr.cstat;
			if ((cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR)
			{
				// !!! xsector pal bits must be extended
				if (cstat & CSTAT_SPRITE_ONE_SIDE)
				{
					if (cstat & CSTAT_SPRITE_YFLIP)
						pXSector->ceilpal = sourceactor->spr.pal;
					else
						pXSector->floorpal = sourceactor->spr.pal;
				}
				else
				{
					pXSector->floorpal = sourceactor->spr.pal;
					pXSector->ceilpal = sourceactor->spr.pal;
				}
			}
		}
		else
		{
			pXSector->shadeAlways = true;
		}
	}

	// add to shadeList
	if (!shadeList.Contains(pSector))
		shadeList.Push(pSector);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void useTargetChanger(DBloodActor* sourceactor, DBloodActor* actor)
{
	if (!actor->IsDudeActor() || actor->spr.statnum != kStatDude)
	{
		switch (actor->spr.type) // can be dead dude turned in gib
		{
			// make current target and all other dudes not attack this dude anymore
		case kThingBloodBits:
		case kThingBloodChunks:
			aiFightFreeTargets(actor);
			return;
		default:
			return;
		}
	}


	int receiveHp = 33 + Random(33);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int matesPerEnemy = 1;

	// dude is burning?
	if (actor->xspr.burnTime > 0 && actor->GetBurnSource())
	{
		if (IsBurningDude(actor)) return;
		else
		{
			auto burnactor = actor->GetBurnSource();
			if (burnactor->hasX())
			{
				if (sourceactor->xspr.data2 == 1 && actor->xspr.rxID == burnactor->xspr.rxID)
				{
					actor->xspr.burnTime = 0;

					// heal dude a bit in case of friendly fire
					int startHp = (actor->xspr.sysData2 > 0) ? ClipRange(actor->xspr.sysData2 << 4, 1, 65535) : pDudeInfo->startHealth << 4;
					if (actor->xspr.health < (unsigned)startHp) actHealDude(actor, receiveHp, startHp);
				}
				else if (burnactor->xspr.health <= 0)
				{
					actor->xspr.burnTime = 0;
				}
			}
		}
	}

	auto playeractor = aiFightTargetIsPlayer(actor);
	// special handling for player(s) if target changer data4 > 2.
	if (playeractor != nullptr)
	{
		auto actLeech = leechIsDropped(actor);
		if (sourceactor->xspr.data4 == 3)
		{
			aiSetTarget(actor, actor->spr.pos);
			aiSetGenIdleState(actor);
			if (actor->spr.type == kDudeModernCustom && actLeech)
				removeLeech(actLeech);
		}
		else if (sourceactor->xspr.data4 == 4)
		{
			aiSetTarget(actor, playeractor->spr.pos);
			if (actor->spr.type == kDudeModernCustom && actLeech)
				removeLeech(actLeech);
		}
	}

	int maxAlarmDudes = 8 + Random(8);
	auto targetactor = actor->GetTarget();
	if (targetactor && targetactor->hasX() && playeractor == nullptr)
	{
		if (aiFightUnitCanFly(actor) && aiFightIsMeleeUnit(targetactor) && !aiFightUnitCanFly(targetactor))
			actor->spr.flags |= 0x0002;
		else if (aiFightUnitCanFly(actor))
			actor->spr.flags &= ~0x0002;

		if (!targetactor->IsDudeActor() || targetactor->xspr.health < 1 || !aiFightDudeCanSeeTarget(actor, pDudeInfo, targetactor))
		{
			aiSetTarget(actor, actor->spr.pos);
		}
		// dude attack or attacked by target that does not fit by data id?
		else if (sourceactor->xspr.data1 != 666 && targetactor->xspr.data1 != sourceactor->xspr.data1)
		{
			if (aiFightDudeIsAffected(targetactor))
			{
				// force stop attack target
				aiSetTarget(actor, actor->spr.pos);
				if (actor->GetBurnSource() == targetactor)
				{
					actor->xspr.burnTime = 0;
					actor->SetBurnSource(nullptr);
				}

				// force stop attack dude
				aiSetTarget(targetactor, targetactor->spr.pos);
				if (targetactor->GetBurnSource() == actor)
				{
					targetactor->xspr.burnTime = 0;
					targetactor->SetBurnSource(nullptr);
				}
			}
		}
		else if (sourceactor->xspr.data2 == 1 && actor->xspr.rxID == targetactor->xspr.rxID)
		{
			auto mateactor = targetactor;

			// heal dude
			int startHp = (actor->xspr.sysData2 > 0) ? ClipRange(actor->xspr.sysData2 << 4, 1, 65535) : pDudeInfo->startHealth << 4;
			if (actor->xspr.health < (unsigned)startHp) actHealDude(actor, receiveHp, startHp);

			// heal mate
			startHp = (mateactor->xspr.sysData2 > 0) ? ClipRange(mateactor->xspr.sysData2 << 4, 1, 65535) : getDudeInfo(mateactor->spr.type)->startHealth << 4;
			if (mateactor->xspr.health < (unsigned)startHp) actHealDude(mateactor, receiveHp, startHp);

			auto matetarget = mateactor->GetTarget();
			if (matetarget != nullptr && matetarget->hasX())
			{
				// force mate stop attack dude, if he does
				if (matetarget == actor)
				{
					aiSetTarget(mateactor, mateactor->spr.pos);
				}
				else if (actor->xspr.rxID != matetarget->xspr.rxID)
				{
					// force dude to attack same target that mate have
					aiSetTarget(actor, matetarget);
					return;
				}
				else
				{
					// force mate to stop attack another mate
					aiSetTarget(mateactor, mateactor->spr.pos);
				}
			}

			// force dude stop attack mate, if target was not changed previously
			if (actor == mateactor)
				aiSetTarget(actor, actor->spr.pos);


		}
		// check if targets aims player then force this target to fight with dude
		else if (aiFightTargetIsPlayer(actor) != nullptr)
		{
			aiSetTarget(targetactor, actor);
		}

		int mDist = 3;
		if (aiFightIsMeleeUnit(actor)) mDist = 2;

		if (targetactor != nullptr && aiFightGetTargetDist(actor, pDudeInfo, targetactor) < mDist)
		{
			if (!isActive(actor)) aiActivateDude(actor);
			return;
		}
		// lets try to look for target that fits better by distance
		else if ((PlayClock & 256) != 0 && (targetactor == nullptr || aiFightGetTargetDist(actor, pDudeInfo, targetactor) >= mDist))
		{
			auto newtargactor = aiFightGetTargetInRange(actor, 0, mDist, sourceactor->xspr.data1, sourceactor->xspr.data2);
			if (newtargactor != nullptr)
			{
				// Make prev target not aim in dude
				if (targetactor)
				{
					aiSetTarget(targetactor, targetactor->spr.pos);
					if (!isActive(newtargactor))
						aiActivateDude(newtargactor);
				}

				// Change target for dude
				aiSetTarget(actor, newtargactor);
				if (!isActive(actor))
					aiActivateDude(actor);

				// ...and change target of target to dude to force it fight
				if (sourceactor->xspr.data3 > 0 && newtargactor->GetTarget() != actor)
				{
					aiSetTarget(newtargactor, actor);
					if (!isActive(newtargactor))
						aiActivateDude(newtargactor);
				}

				return;
			}
		}
	}

	if ((targetactor == nullptr || playeractor != nullptr) && (PlayClock & 32) != 0)
	{
		// try find first target that dude can see
		BloodStatIterator it(kStatDude);
		while (auto newtargactor = it.Next())
		{
			if (newtargactor->GetTarget() == actor)
			{
				aiSetTarget(actor, newtargactor);
				return;
			}

			// skip non-dudes and players
			if (!newtargactor->IsDudeActor() || (newtargactor->IsPlayerActor() && sourceactor->xspr.data4 > 0) || newtargactor->GetOwner() == actor) continue;
			// avoid self aiming, those who dude can't see, and those who dude own
			else if (!aiFightDudeCanSeeTarget(actor, pDudeInfo, newtargactor) || actor == newtargactor) continue;
			// if Target Changer have data1 = 666, everyone can be target, except AI team mates.
			else if (sourceactor->xspr.data1 != 666 && sourceactor->xspr.data1 != newtargactor->xspr.data1) continue;
			// don't attack immortal, burning dudes and mates
			if (IsBurningDude(newtargactor) || !IsKillableDude(newtargactor) || (sourceactor->xspr.data2 == 1 && actor->xspr.rxID == newtargactor->xspr.rxID))
				continue;

			if (sourceactor->xspr.data2 == 0 || (sourceactor->xspr.data2 == 1 && !aiFightMatesHaveSameTarget(actor, newtargactor, matesPerEnemy)))
			{
				// Change target for dude
				aiSetTarget(actor, newtargactor);
				if (!isActive(actor))
					aiActivateDude(actor);

				// ...and change target of target to dude to force it fight
				if (sourceactor->xspr.data3 > 0 && newtargactor->GetTarget() != actor)
				{
					aiSetTarget(newtargactor, actor);
					if (playeractor == nullptr && !isActive(newtargactor))
						aiActivateDude(newtargactor);

					if (sourceactor->xspr.data3 == 2)
						aiFightAlarmDudesInSight(newtargactor, maxAlarmDudes);
				}
				return;
			}
			break;
		}
	}

	// got no target - let's ask mates if they have targets
	if ((actor->GetTarget() == nullptr || playeractor != nullptr) && sourceactor->xspr.data2 == 1 && (PlayClock & 64) != 0)
	{
		DBloodActor* pMateTargetActor = aiFightGetMateTargets(actor);
		if (pMateTargetActor != nullptr && pMateTargetActor->hasX())
		{

			if (aiFightDudeCanSeeTarget(actor, pDudeInfo, pMateTargetActor))
			{
				if (pMateTargetActor->GetTarget() == nullptr)
				{
					aiSetTarget(pMateTargetActor, actor);
					if (pMateTargetActor->IsDudeActor() && !isActive(pMateTargetActor))
						aiActivateDude(pMateTargetActor);
				}

				aiSetTarget(actor, pMateTargetActor);
				if (!isActive(actor))
					aiActivateDude(actor);
				return;

				// try walk in mate direction in case if not see the target
			}
			else if (pMateTargetActor->GetTarget() && aiFightDudeCanSeeTarget(actor, pDudeInfo, pMateTargetActor->GetTarget()))
			{
				actor->SetTarget(pMateTargetActor);
				auto pMate = pMateTargetActor->GetTarget();
				actor->xspr.TargetPos = pMate->spr.pos;
				if (!isActive(actor))
					aiActivateDude(actor);
				return;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void usePictureChanger(DBloodActor* sourceactor, int objType, sectortype* targSect, walltype* targWall, DBloodActor* objActor)
{
	switch (objType) {
	case OBJ_SECTOR:
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			targSect->setfloortexture(tileGetTextureID(sourceactor->xspr.data1));

		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			targSect->setceilingtexture(tileGetTextureID(sourceactor->xspr.data2));

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			targSect->floorpal = uint8_t(sourceactor->xspr.data3);

		if (valueIsBetween(sourceactor->xspr.data4, -1, 65535))
			targSect->ceilingpal = uint8_t(sourceactor->xspr.data4);
		break;
	case OBJ_SPRITE:
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			objActor->spr.setspritetexture(tileGetTextureID(sourceactor->xspr.data1));

		if (sourceactor->xspr.data2 >= 0) objActor->spr.shade = (sourceactor->xspr.data2 > 127) ? 127 : sourceactor->xspr.data2;
		else if (sourceactor->xspr.data2 < -1) objActor->spr.shade = (sourceactor->xspr.data2 < -127) ? -127 : sourceactor->xspr.data2;

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			objActor->spr.pal = uint8_t(sourceactor->xspr.data3);
		break;
	case OBJ_WALL:
		if (valueIsBetween(sourceactor->xspr.data1, -1, 32767))
			targWall->setwalltexture(tileGetTextureID(sourceactor->xspr.data1));

		if (valueIsBetween(sourceactor->xspr.data2, -1, 32767))
			targWall->setovertexture(tileGetTextureID(sourceactor->xspr.data2));

		if (valueIsBetween(sourceactor->xspr.data3, -1, 32767))
			targWall->pal = uint8_t(sourceactor->xspr.data3);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

QAV* playerQavSceneLoad(int qavId)
{
	QAV* pQav = getQAV(qavId);
	if (!pQav) viewSetSystemMessage("Failed to load QAV animation #%d", qavId);
	return pQav;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerQavSceneProcess(DBloodPlayer* pPlayer, QAVSCENE* pQavScene)
{
	auto initiator = pQavScene->initiator;
	if (initiator->hasX())
	{
		if (initiator->xspr.waitTime > 0 && --initiator->xspr.sysData1 <= 0)
		{
			if (initiator->xspr.txID >= kChannelUser)
			{
				for (int i = bucketHead[initiator->xspr.txID]; i < bucketHead[initiator->xspr.txID + 1]; i++)
				{
					if (rxBucket[i].isActor())
					{
						auto rxactor = rxBucket[i].actor();
						if (!rxactor || !rxactor->hasX() || rxactor == initiator) continue;

						if (rxactor->spr.type == kModernPlayerControl && rxactor->xspr.command == 67)
						{
							if (rxactor->xspr.data2 == initiator->xspr.data2 || rxactor->xspr.locked) continue;
							else trPlayerCtrlStartScene(rxactor, pPlayer, true);
							return;
						}

					}
					nnExtTriggerObject(rxBucket[i], initiator->xspr.command, pPlayer->GetActor());

				}
			}
			trPlayerCtrlStopScene(pPlayer);
		}
		else
		{
			playerQavScenePlay(pPlayer);
			pPlayer->weaponTimer = ClipLow(pPlayer->weaponTimer -= 4, 0);
		}
	}
	else
	{

		pQavScene->initiator = nullptr;
		pPlayer->sceneQav = -1;
		pQavScene->qavResrc = NULL;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerQavSceneDraw(DBloodPlayer* pPlayer, int shade, double xpos, double ypos, int palnum, DAngle angle)
{
	if (pPlayer == NULL || pPlayer->sceneQav == -1) return;

	QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->pnum].qavScene;
	auto actor = pQavScene->initiator;

	if (pQavScene->qavResrc != NULL)
	{
		QAV* pQAV = pQavScene->qavResrc;
		int duration;
		double interpfrac;

		qavProcessTimer(pPlayer, pQAV, &duration, &interpfrac);

		int flags = 2; int nInv = powerupCheck(pPlayer, kPwUpShadowCloak);
		if (nInv >= 120 * 8 || (nInv != 0 && (PlayClock & 32)))
		{
			shade = -128; flags |= 1;
		}

		// draw as weapon
		if (!(actor->spr.flags & kModernTypeFlag1))
		{
			pQAV->x = xpos; pQAV->y = ypos;
			pQAV->Draw(duration, flags, shade, palnum, true, interpfrac, angle);

			// draw fullscreen (currently 4:3 only)
		}
		else
		{
			// What an awful hack. This throws proper ordering out of the window, but there is no way to reproduce this better with strict layering of elements.
			// From the above commit it seems to be incomplete anyway...
			pQAV->Draw(duration, flags, shade, palnum, false, interpfrac, angle);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerQavScenePlay(DBloodPlayer* pPlayer)
{
	if (pPlayer == NULL) return;

	QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->pnum].qavScene;
	if (pPlayer->sceneQav == -1 && pQavScene->initiator != nullptr)
		pPlayer->sceneQav = pQavScene->initiator->xspr.data2;

	if (pQavScene->qavResrc != NULL)
	{
		QAV* pQAV = pQavScene->qavResrc;
		int nTicks = pQAV->duration - pPlayer->weaponTimer;
		pQAV->Play(nTicks - 4, nTicks, pPlayer->qavCallback, pPlayer);
	}
}

void playerQavSceneReset(DBloodPlayer* pPlayer)
{
	QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->pnum].qavScene;
	pQavScene->initiator = nullptr;
	pQavScene->dummy = pPlayer->sceneQav = -1;
	pQavScene->qavResrc = NULL;
}

bool playerSizeShrink(DBloodPlayer* pPlayer, int divider)
{
	pPlayer->GetActor()->xspr.scale = 256 / divider;
	playerSetRace(pPlayer, kModeHumanShrink);
	return true;
}

bool playerSizeGrow(DBloodPlayer* pPlayer, int multiplier)
{
	pPlayer->GetActor()->xspr.scale = 256 * multiplier;
	playerSetRace(pPlayer, kModeHumanGrown);
	return true;
}

bool playerSizeReset(DBloodPlayer* pPlayer)
{
	playerSetRace(pPlayer, kModeHuman);
	pPlayer->GetActor()->xspr.scale = 0;
	return true;
}

void playerDeactivateShrooms(DBloodPlayer* pPlayer)
{
	powerupDeactivate(pPlayer, kPwUpGrowShroom);
	pPlayer->pwUpTime[kPwUpGrowShroom] = 0;

	powerupDeactivate(pPlayer, kPwUpShrinkShroom);
	pPlayer->pwUpTime[kPwUpShrinkShroom] = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodPlayer* getPlayerById(int id)
{
	// relative to connected players
	if (id >= 1 && id <= kMaxPlayers)
	{
		id = id - 1;
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			if (id == getPlayer(i)->pnum)
				return getPlayer(i);
		}

		// absolute sprite type
	}
	else if (id >= kDudePlayer1 && id <= kDudePlayer8)
	{
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
		{
			if (id == getPlayer(i)->GetActor()->spr.type)
				return getPlayer(i);
		}
	}

	//viewSetSystemMessage("There is no player id #%d", id);
	return NULL;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool IsBurningDude(DBloodActor* actor)
{
	if (actor == NULL) return false;
	switch (actor->spr.type)
	{
	case kDudeBurningInnocent:
	case kDudeBurningCultist:
	case kDudeBurningZombieAxe:
	case kDudeBurningZombieButcher:
	case kDudeBurningTinyCaleb:
	case kDudeBurningBeast:
	case kDudeModernCustomBurning:
		return true;
	}

	return false;
}

bool IsKillableDude(DBloodActor* actor)
{
	switch (actor->spr.type)
	{
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:
		return false;
	default:
		if (!actor->IsDudeActor() || actor->xspr.locked == 1) return false;
		return true;
	}
}

bool isGrown(DBloodActor* actor)
{
	if (powerupCheck(getPlayer(actor->spr.type - kDudePlayer1), kPwUpGrowShroom) > 0) return true;
	else if (actor->hasX() && actor->xspr.scale >= 512) return true;
	else return false;
}

bool isShrunk(DBloodActor* actor)
{
	if (powerupCheck(getPlayer(actor->spr.type - kDudePlayer1), kPwUpShrinkShroom) > 0) return true;
	else if (actor->hasX() && actor->xspr.scale > 0 && actor->xspr.scale <= 128) return true;
	else return false;
}

bool isActive(DBloodActor* actor)
{
	if (!actor->hasX())
		return false;

	switch (actor->xspr.aiState->stateType)
	{
	case kAiStateIdle:
	case kAiStateGenIdle:
	case kAiStateSearch:
	case kAiStateMove:
	case kAiStateOther:
		return false;
	default:
		return true;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getDataFieldOfObject(EventObject& eob, int dataIndex)
{
	int data = -65535;

	if (eob.isActor())
	{
		auto actor = eob.actor();
		if (actor)
		{
			switch (dataIndex)
			{
			case 1: return actor->xspr.data1;
			case 2: return actor->xspr.data2;
			case 3:
				switch (actor->spr.type)
				{
				case kDudeModernCustom: return actor->xspr.sysData1;
				default: return actor->xspr.data3;
				}
			case 4: return actor->xspr.data4;
			default: return data;
			}
		}
	}
	else if (eob.isSector())
	{
		return eob.sector()->xs().data;
	}
	else if (eob.isWall())
	{
		return eob.wall()->xw().data;
	}
	return data;
}

int getDataFieldOfObject(int objType, sectortype* sect, walltype* wal, DBloodActor* actor, int dataIndex)
{
	int data = -65535;
	switch (objType)
	{
	case OBJ_SPRITE:
		switch (dataIndex)
		{
		case 1: return actor->xspr.data1;
		case 2: return actor->xspr.data2;
		case 3:
			switch (actor->spr.type)
			{
			case kDudeModernCustom: return actor->xspr.sysData1;
			default: return actor->xspr.data3;
			}
		case 4: return actor->xspr.data4;
		default: return data;
		}
	case OBJ_SECTOR: return sect->xs().data;
	case OBJ_WALL: return wal->xw().data;
	default: return data;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool setDataValueOfObject(int objType, sectortype* sect, walltype* wal, DBloodActor* objActor, int dataIndex, int value)
{
	switch (objType)
	{
	case OBJ_SPRITE:
	{
		int type = objActor->spr.type;

		// exceptions
		if (objActor->IsDudeActor() && objActor->xspr.health <= 0) return true;
		switch (type)
		{
		case kThingBloodBits:
		case kThingBloodChunks:
		case kThingZombieHead:
			return true;
			break;
		}

		switch (dataIndex)
		{
		case 1:
			objActor->xspr.data1 = value;
			switch (type)
			{
			case kSwitchCombo:
				if (value == objActor->xspr.data2) SetSpriteState(objActor, 1, nullptr);
				else SetSpriteState(objActor, 0, nullptr);
				break;
			case kDudeModernCustom:
			case kDudeModernCustomBurning:
				objActor->genDudeExtra.updReq[kGenDudePropertyWeapon] = true;
				objActor->genDudeExtra.updReq[kGenDudePropertyDmgScale] = true;
				evPostActor(objActor, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
				break;
			}
			return true;
		case 2:
			objActor->xspr.data2 = value;
			switch (type)
			{
			case kDudeModernCustom:
			case kDudeModernCustomBurning:
				objActor->genDudeExtra.updReq[kGenDudePropertySpriteSize] = true;
				objActor->genDudeExtra.updReq[kGenDudePropertyMass] = true;
				objActor->genDudeExtra.updReq[kGenDudePropertyDmgScale] = true;
				objActor->genDudeExtra.updReq[kGenDudePropertyStates] = true;
				objActor->genDudeExtra.updReq[kGenDudePropertyAttack] = true;
				evPostActor(objActor, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
				break;
			}
			return true;
		case 3:
			objActor->xspr.data3 = value;
			switch (type)
			{
			case kDudeModernCustom:
			case kDudeModernCustomBurning:
				objActor->xspr.sysData1 = value;
				break;
			}
			return true;
		case 4:
			objActor->xspr.data4 = value;
			return true;
		default:
			return false;
		}
	}
	case OBJ_SECTOR:
		sect->xs().data = value;
		return true;
	case OBJ_WALL:
		wal->xw().data = value;
		return true;
	default:
		return false;
	}
}

//---------------------------------------------------------------------------
//
// a replacement of vanilla CanMove for patrol dudes
//
//---------------------------------------------------------------------------

bool nnExtCanMove(DBloodActor* actor, DBloodActor* target, DAngle nAngle, double nRange)
{
	DVector3 pos = actor->spr.pos;
	DVector3 nAngVect(nAngle.ToVector(), 0);

	auto pSector = actor->sector();
	HitScan(actor, pos.Z, nAngVect, CLIPMASK0, nRange);
	double nDist = (actor->spr.pos.XY() - gHitInfo.hitpos.XY()).Length();

	if (target != nullptr && nDist - actor->clipdist < nRange)
		return (target == gHitInfo.actor());

	pos += nAngVect * nRange;
	updatesectorz(pos, &pSector);
	if (!pSector) return false;

	if (pSector->hasX()) {

		XSECTOR* pXSector = &pSector->xs();
		return !((pSector->type == kSectorDamage || pXSector->damageType > 0) && pXSector->state && !nnExtIsImmune(actor, pXSector->damageType, 16));

	}

	return true;

}

//---------------------------------------------------------------------------
//
// a replacement of vanilla aiChooseDirection for patrol dudes
//
//---------------------------------------------------------------------------

void nnExtAiSetDirection(DBloodActor* actor, DAngle direction)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);

	DAngle vc = deltaangle(actor->spr.Angles.Yaw, direction);
	DAngle v8 = vc > nullAngle ? DAngle180 / 3 : -DAngle180 / 3;
	double range = actor->vel.XY().dot(actor->spr.Angles.Yaw.ToVector()) * 120;

	if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + vc, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + vc;
	else if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + vc / 2, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + vc / 2;
	else if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw - vc / 2, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw - vc / 2;
	else if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + v8, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + v8;
	else if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw;
	else if (nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw - v8, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw - v8;
	else
		actor->xspr.goalAng = actor->spr.Angles.Yaw + DAngle180 / 3;

	if (actor->xspr.dodgeDir)
	{
		if (!nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + DAngle90 * actor->xspr.dodgeDir, 512))
		{
			actor->xspr.dodgeDir = -actor->xspr.dodgeDir;
			if (!nnExtCanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + DAngle90 * actor->xspr.dodgeDir, 512))
				actor->xspr.dodgeDir = 0;
		}
	}
}


//---------------------------------------------------------------------------
//
/// patrol functions
//
//---------------------------------------------------------------------------

void aiPatrolState(DBloodActor* actor, int state)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax&& actor->hasX());
	assert(actor->GetTarget());

	auto markeractor = actor->GetTarget();
	assert(markeractor->spr.type == kMarkerPath);

	bool nSeqOverride = false, crouch = false;
	int i, seq = -1, start = 0, end = kPatrolStateSize;

	const DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[actor->spr.type - kDudeBase];

	switch (state) {
	case kAiStatePatrolWaitL:
		seq = pExtra->idlgseqofs;
		start = 0; end = 2;
		break;
	case kAiStatePatrolMoveL:
		seq = pExtra->mvegseqofs;
		start = 2, end = 7;
		break;
	case kAiStatePatrolTurnL:
		seq = pExtra->mvegseqofs;
		start = 7, end = 12;
		break;
	case kAiStatePatrolWaitW:
		seq = pExtra->idlwseqofs;
		start = 12; end = 18;
		break;
	case kAiStatePatrolMoveW:
		seq = pExtra->mvewseqofs;
		start = 18; end = 25;
		break;
	case kAiStatePatrolTurnW:
		seq = pExtra->mvewseqofs;
		start = 25; end = 32;
		break;
	case kAiStatePatrolWaitC:
		seq = pExtra->idlcseqofs;
		start = 32; end = 36;
		crouch = true;
		break;
	case kAiStatePatrolMoveC:
		seq = pExtra->mvecseqofs;
		start = 36; end = 39;
		crouch = true;
		break;
	case kAiStatePatrolTurnC:
		seq = pExtra->mvecseqofs;
		start = 39; end = kPatrolStateSize;
		crouch = true;
		break;
	}


	if (markeractor->xspr.data4 > 0) seq = markeractor->xspr.data4, nSeqOverride = true;
	else if (!nSeqOverride && state == kAiStatePatrolWaitC && (actor->spr.type == kDudeCultistTesla || actor->spr.type == kDudeCultistTNT))
		seq = 11537, nSeqOverride = true;  // these don't have idle crouch seq for some reason...

	if (seq < 0)
		return aiPatrolStop(actor, nullptr);

	for (i = start; i < end; i++)
	{
		AISTATE* newState = &genPatrolStates[i];
		if (newState->stateType != state || (!nSeqOverride && seq != newState->seqId))
			continue;

		if (actor->spr.type == kDudeModernCustom) aiGenDudeNewState(actor, newState);
		else aiNewState(actor, newState);

		if (crouch) actor->xspr.unused1 |= kDudeFlagCrouch;
		else actor->xspr.unused1 &= ~kDudeFlagCrouch;

		if (nSeqOverride)
			seqSpawn(seq, actor);

		return;
	}

	if (i == end)
	{
		viewSetSystemMessage("No patrol state #%d found for dude #%d (type = %d)", state, actor->GetIndex(), actor->spr.type);
		aiPatrolStop(actor, nullptr);
	}
}

//---------------------------------------------------------------------------
//
// check if some dude already follows the given marker
//
//---------------------------------------------------------------------------

DBloodActor* aiPatrolMarkerBusy(DBloodActor* except, DBloodActor* marker)
{
	BloodStatIterator it(kStatDude);
	while (auto actor = it.Next())
	{
		if (!actor->IsDudeActor() || actor == except || !actor->hasX())
			continue;

		auto targ = actor->GetTarget();
		if (actor->xspr.health > 0 && targ != nullptr && targ->spr.type == kMarkerPath && targ == marker)
			return actor;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// check if some dude already follows the given marker
//
//---------------------------------------------------------------------------

bool aiPatrolMarkerReached(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);

	const DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[actor->spr.type - kDudeBase];
	auto markeractor = actor->GetTarget();
	if (markeractor && markeractor->spr.type == kMarkerPath)
	{
		double okDist = max(markeractor->clipdist * 8, 4.);
		auto ov = markeractor->spr.pos.XY() - actor->spr.pos.XY(); // this was already shifted right by 4 in the old code.

		if (ov.Length() <= okDist)
		{
			if (spriteIsUnderwater(actor) || pExtra->flying)
			{
				okDist = markeractor->clipdist * 16;
				double ztop, zbot, ztop2, zbot2;
				GetActorExtents(actor, &ztop, &zbot);
				GetActorExtents(markeractor, &ztop2, &zbot2);

				double oZ1 = abs(zbot - ztop2);
				double oZ2 = abs(ztop - zbot2);
				if (oZ1 > okDist && oZ2 > okDist)
					return false;
			}
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* findNextMarker(DBloodActor* mark, bool back)
{
	BloodStatIterator it(kStatPathMarker);
	while (auto next = it.Next())
	{
		if (!next->hasX() || next == mark) continue;

		if ((next->xspr.locked || next->xspr.isTriggered || next->xspr.DudeLockout) || (back && next->xspr.data2 != mark->xspr.data1) || (!back && next->xspr.data1 != mark->xspr.data2))
			continue;

		return next;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool markerIsNode(DBloodActor* mark, bool back)
{
	int cnt = 0;

	BloodStatIterator it(kStatPathMarker);
	while (auto next = it.Next())
	{
		if (!next->hasX() || next == mark) continue;

		if ((next->xspr.locked || next->xspr.isTriggered || next->xspr.DudeLockout) || (back && next->xspr.data2 != mark->xspr.data1) || (!back && next->xspr.data1 != mark->xspr.data2))
			continue;

		if (++cnt > 1)
			return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolSetMarker(DBloodActor* actor)
{
	auto targetactor = actor->GetTarget();

	DBloodActor* selected = nullptr;
	double closest = DBL_MAX;

	// select closest marker that dude can see
	if (targetactor == nullptr)
	{
		double zt1, zb1, zt2, zb2;
		double dist;
		GetActorExtents(actor, &zt2, &zb2);

		BloodStatIterator it(kStatPathMarker);
		while (auto nextactor = it.Next())
		{
			if (!nextactor->hasX()) continue;

			if (nextactor->xspr.locked || nextactor->xspr.isTriggered || nextactor->xspr.DudeLockout || (dist = (nextactor->spr.pos.XY() - actor->spr.pos.XY()).LengthSquared()) > closest)
				continue;

			GetActorExtents(nextactor, &zt1, &zb1);
			if (cansee(DVector3(nextactor->spr.pos.XY(), zt1), nextactor->sector(), DVector3(actor->spr.pos.XY(), zt2), actor->sector()))
			{
				closest = dist;
				selected = nextactor;
			}
		}
	}
	// set next marker
	else if (targetactor->spr.type == kMarkerPath && targetactor->hasX())
	{
		// idea: which one of next (allowed) markers are closer to the potential target?
		// idea: -3 select random next marker that dude can see in radius of reached marker
		// if reached marker is in radius of another marker with -3, but greater radius, use that marker
		// idea: for nodes only flag32 = specify if enemy must return back to node or allowed to select
		// another marker which belongs that node?
		DBloodActor* prevactor = nullptr;

		DBloodActor* firstFinePath = nullptr;
		int next;

		int breakChance = 0;
		if (actor->prevmarker)
		{
			prevactor = actor->prevmarker;
		}

		bool node = markerIsNode(targetactor, false);
		actor->xspr.unused2 = aiPatrolGetPathDir(actor, targetactor); // decide if it should go back or forward
		if (actor->xspr.unused2 == kPatrolMoveBackward && Chance(0x8000) && node)
			actor->xspr.unused2 = kPatrolMoveForward;

		bool back = (actor->xspr.unused2 == kPatrolMoveBackward); next = (back) ? targetactor->xspr.data1 : targetactor->xspr.data2;
		BloodStatIterator it(kStatPathMarker);
		while (auto nextactor = it.Next())
		{
			if (nextactor == targetactor || !nextactor->hasX()) continue;
			else if (actor->xspr.TargetPos.X >= 0 && nextactor == prevactor && node)
			{
				if (targetactor->xspr.data2 == prevactor->xspr.data1)
					continue;
			}

			if ((nextactor->xspr.locked || nextactor->xspr.isTriggered || nextactor->xspr.DudeLockout) || (back && nextactor->xspr.data2 != next) || (!back && nextactor->xspr.data1 != next))
				continue;

			if (firstFinePath == nullptr) firstFinePath = nextactor;
			if (aiPatrolMarkerBusy(actor, nextactor) && !Chance(0x0010)) continue;
			else selected = nextactor;

			breakChance += nnExtRandom(1, 5);
			if (breakChance >= 5)
				break;

		}

		if (firstFinePath == nullptr)
		{
			viewSetSystemMessage("No markers with id #%d found for dude #%d! (back = %d)", next, actor->GetIndex(), back);
			return;
		}

		if (selected == nullptr)
			selected = firstFinePath;
	}

	if (!selected)
		return;

	actor->SetTarget(selected);
	selected->SetOwner(actor);
	actor->prevmarker = targetactor; // keep previous marker index here, use actual sprite coords when selecting direction
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolStop(DBloodActor* actor, DBloodActor* targetactor, bool alarm)
{
	if (actor->hasX())
	{
		actor->xspr.data3 = 0; // reset spot progress
		actor->xspr.unused1 &= ~kDudeFlagCrouch; // reset the crouch status
		actor->xspr.unused2 = kPatrolMoveForward; // reset path direction
		actor->prevmarker = nullptr;
		actor->xspr.TargetPos.X = -1; // reset the previous marker index
		if (actor->xspr.health <= 0)
			return;

		auto mytarget = actor->GetTarget();

		if (mytarget && mytarget->spr.type == kMarkerPath)
		{
			if (targetactor == nullptr) actor->spr.Angles.Yaw = mytarget->spr.Angles.Yaw;
			actor->SetTarget(nullptr);
		}

		bool patrol = actor->xspr.dudeFlag4;
		actor->xspr.dudeFlag4 = 0;
		if (targetactor && targetactor->hasX() && targetactor->IsDudeActor())
		{
			aiSetTarget(actor, targetactor);
			aiActivateDude(actor);

			// alarm only when in non-recoil state?
			//if (((actor->xspr.unused1 & kDudeFlagStealth) && stype != kAiStateRecoil) || !(actor->xspr.unused1 & kDudeFlagStealth)) {
			//if (alarm) aiPatrolAlarmFull(actor, targetactor, Chance(0x0100));
			if (alarm) aiPatrolAlarmLite(actor, targetactor);
			//}

		}
		else
		{
			aiInitSprite(actor);
			aiSetTarget(actor, actor->xspr.TargetPos);
		}

		actor->xspr.dudeFlag4 = patrol; // this must be kept so enemy can patrol after respawn again
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolRandGoalAng(DBloodActor* actor)
{
	DAngle goal = DAngle90;
	if (Chance(0x4000))
		goal = DAngle360 / 3;

	if (Chance(0x4000))
		goal = DAngle180;

	if (Chance(0x8000))
		goal = -goal;

	actor->xspr.goalAng = (actor->spr.Angles.Yaw + goal).Normalized360();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolTurn(DBloodActor* actor)
{
	DAngle nTurnRange = mapangle((getDudeInfo(actor->spr.type)->angSpeed << 1) >> 4);
	DAngle nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolMove(DBloodActor* actor)
{
	auto targetactor = actor->GetTarget();

	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax) || !targetactor)
		return;


	int dudeIdx = actor->spr.type - kDudeBase;
	switch (actor->spr.type)
	{
	case kDudeCultistShotgunProne:  
		dudeIdx = kDudeCultistShotgun - kDudeBase;  
		break;
	case kDudeCultistTommyProne:    
		dudeIdx = kDudeCultistTommy - kDudeBase;   
		break;
	}

	DUDEINFO* pDudeInfo = &dudeInfo[dudeIdx];
	const DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[dudeIdx];

	DVector3 dv = targetactor->spr.pos - actor->spr.pos.plusZ(-pDudeInfo->eyeHeight); // eyeHeight is in map units!

	DAngle goalAng = DAngle180 / 3;

	if (pExtra->flying || spriteIsUnderwater(actor))
	{
		goalAng *= 0.5;
		actor->vel.Z = dv.Z * 6;
		if (actor->spr.flags & kPhysGravity)
			actor->spr.flags &= ~kPhysGravity;
	}
	else if (!pExtra->flying)
	{
		actor->spr.flags |= kPhysGravity | kPhysFalling;
	}

	DAngle nTurnRange = pDudeInfo->TurnRange() / 64;
	DAngle nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);


	if (abs(nAng) > goalAng || ((targetactor->xspr.waitTime > 0 || targetactor->xspr.data1 == targetactor->xspr.data2) && aiPatrolMarkerReached(actor)))
	{
		actor->vel.XY().Zero();
		return;
	}

	if (actor->hit.hit.type == kHitSprite)
	{
		actor->xspr.dodgeDir = Chance(0x5000) ? 1 : -1;
		auto hitactor = actor->hit.hit.actor();
		if (hitactor)
		{
			if (xsprIsFine(hitactor) && hitactor->xspr.health)
			{
				hitactor->xspr.dodgeDir = (actor->xspr.dodgeDir > 0) ? -1 : 1;
				if (!hitactor->vel.XY().isZero())
					aiMoveDodge(hitactor);
			}
		}
		aiMoveDodge(actor);
	}
	else
	{
		int frontSpeed = pDudeInfo->frontSpeed;
		switch (actor->spr.type)
		{
			case kDudeModernCustom:
			case kDudeModernCustomBurning:
				frontSpeed = actor->genDudeExtra.moveSpeed;
				break;
		}

		frontSpeed = aiPatrolGetVelocity(pDudeInfo->frontSpeed, targetactor->xspr.busyTime);
		actor->vel += actor->spr.Angles.Yaw.ToVector() * FixedToFloat(frontSpeed);
	}

	double vel = (actor->xspr.unused1 & kDudeFlagCrouch) ? kMaxPatrolCrouchVelocity : kMaxPatrolVelocity;

	vel *= dv.XY().Length() / 1024; // was: MulScale16 with length << 6, effectively resulting in >> 10.
	actor->vel.X = clamp(actor->vel.X, -vel, vel);
	actor->vel.Y = clamp(actor->vel.Y, -vel, vel);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolAlarmLite(DBloodActor* actor, DBloodActor* targetactor)
{
	if (!xsprIsFine(actor) || !actor->IsDudeActor())
		return;

	if (actor->xspr.health <= 0)
		return;

	double zt1, zb1, zt2, zb2;
	GetActorExtents(actor, &zt1, &zb1);
	GetActorExtents(targetactor, &zt2, &zb2);

	BloodStatIterator it(kStatDude);
	while (auto dudeactor = it.Next())
	{
		if (dudeactor == actor || !dudeactor->IsDudeActor() || dudeactor->IsPlayerActor() || !dudeactor->hasX())
			continue;

		if (dudeactor->xspr.health <= 0)
			continue;

		double eaz2 = (getDudeInfo(targetactor->spr.type)->eyeHeight * targetactor->spr.scale.Y);
		double nDist = (dudeactor->spr.pos.XY() - actor->spr.pos.XY()).LengthSquared();
		if (nDist >= kPatrolAlarmSeeDistSq || !cansee(DVector3(actor->spr.pos.XY(), zt1), actor->sector(), dudeactor->spr.pos.plusZ(-eaz2), dudeactor->sector()))
		{
			nDist = (dudeactor->spr.pos.XY() - targetactor->spr.pos.XY()).LengthSquared();
			if (nDist >= kPatrolAlarmSeeDistSq || !cansee(DVector3(targetactor->spr.pos.XY(), zt2), targetactor->sector(), dudeactor->spr.pos.plusZ(-eaz2), dudeactor->sector()))
				continue;
		}

		if (aiInPatrolState(dudeactor->xspr.aiState)) aiPatrolStop(dudeactor, dudeactor->GetTarget());
		if (dudeactor->GetTarget() && dudeactor->GetTarget() == actor->GetTarget())
			continue;

		aiSetTarget(dudeactor, targetactor);
		aiActivateDude(dudeactor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolAlarmFull(DBloodActor* actor, DBloodActor* targetactor, bool chain)
{
	if (!xsprIsFine(actor) || !actor->IsDudeActor())
		return;

	if (actor->xspr.health <= 0)
		return;

	double eaz2 = (getDudeInfo(actor->spr.type)->eyeHeight * actor->spr.scale.Y);
	auto pos2 = actor->spr.pos.plusZ(-eaz2);

	auto pSect2 = actor->sector();

	double tzt, tzb;
	GetActorExtents(targetactor, &tzt, &tzb);
	DVector3 pos3(targetactor->spr.pos.XY(), tzt);

	auto pSect3 = targetactor->sector();

	BloodStatIterator it(kStatDude);
	while (auto dudeactor = it.Next())
	{
		if (dudeactor == actor || !dudeactor->IsDudeActor() || dudeactor->IsPlayerActor() || !dudeactor->hasX())
			continue;

		if (dudeactor->xspr.health <= 0)
			continue;

		double eaz1 = (getDudeInfo(dudeactor->spr.type)->eyeHeight * dudeactor->spr.scale.Y);
		auto pos1 = dudeactor->spr.pos.plusZ(-eaz1);

		auto pSect1 = dudeactor->sector();

		double nDist1 = (pos1 - pos2).Length();
		double nDist2 = (pos1 - pos3).Length();

		//double hdist = (dudeactor->xspr.dudeDeaf)  ? 0 : getDudeInfo(dudeactor->spr.type)->HearDist() / 4;
		double sdist = (dudeactor->xspr.dudeGuard) ? 0 : getDudeInfo(dudeactor->spr.type)->SeeDist() / 2;

		if (//(nDist1 < hdist || nDist2 < hdist) ||
			((nDist1 < sdist && cansee(pos1, pSect1, pos2, pSect2)) || (nDist2 < sdist && cansee(pos1, pSect1, pos3, pSect3)))) 
		{

			if (aiInPatrolState(dudeactor->xspr.aiState)) aiPatrolStop(dudeactor, dudeactor->GetTarget());
			if (dudeactor->GetTarget() && dudeactor->GetTarget() == actor->GetTarget())
				continue;

			if (actor->GetTarget()) aiSetTarget(dudeactor, actor->GetTarget());
			else aiSetTarget(dudeactor, actor->spr.pos);
			aiActivateDude(dudeactor);

			if (chain)
				aiPatrolAlarmFull(dudeactor, targetactor, Chance(0x0010));
			//Printf("Dude #%d alarms dude #%d", actor->GetIndex(), dudeactor->spr.index);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool spritesTouching(DBloodActor* actor1, DBloodActor* actor2)
{
	if (!actor1->hasX() || !actor2->hasX())
		return false;

	auto hit = &actor1->hit;
	DBloodActor* hitactor = nullptr;
	if (hit->hit.type == kHitSprite) hitactor = hit->hit.actor();
	else if (hit->florhit.type == kHitSprite) hitactor = hit->florhit.actor();
	else if (hit->ceilhit.type == kHitSprite) hitactor = hit->ceilhit.actor();
	else return false;
	return hitactor->hasX() && hitactor == actor2;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiCanCrouch(DBloodActor* actor)
{
	if (actor->spr.type >= kDudeBase && actor->spr.type < kDudeVanillaMax)
		return (gDudeInfoExtra[actor->spr.type - kDudeBase].idlcseqofs >= 0 && gDudeInfoExtra[actor->spr.type - kDudeBase].mvecseqofs >= 0);
	else if (actor->spr.type == kDudeModernCustom || actor->spr.type == kDudeModernCustomBurning)
		return actor->genDudeExtra.canDuck;

	return false;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool readyForCrit(DBloodActor* hunter, DBloodActor* victim)
{
	if (!(hunter->spr.type >= kDudeBase && hunter->spr.type < kDudeMax) || !(victim->spr.type >= kDudeBase && victim->spr.type < kDudeMax))
		return false;

	auto dvect = victim->spr.pos.XY() - hunter->spr.pos.XY();
	if (dvect.Length() >= (437.5 / max(gGameOptions.nDifficulty >> 1, 1)))
		return false;

	return absangle(victim->spr.Angles.Yaw, dvect.Angle()) <= DAngle45;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* aiPatrolSearchTargets(DBloodActor* actor)
{
	enum { kMaxPatrolFoundSounds = 256 }; // should be the maximum amount of sound channels the engine can play at the same time.
	PATROL_FOUND_SOUNDS patrolBonkles[kMaxPatrolFoundSounds];

	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type - kDudeBase); DBloodPlayer* pPlayer = NULL;

	for (int i = 0; i < kMaxPatrolFoundSounds; i++)
	{
		patrolBonkles[i].snd = NO_SOUND;
		patrolBonkles[i].cur = 0;
		patrolBonkles[i].max = ClipLow((gGameOptions.nDifficulty + 1) >> 1, 1);
	}

	int i, mod, sndCnt = 0, seeChance, hearChance;
	bool stealth = (actor->xspr.unused1 & kDudeFlagStealth);
	bool blind = (actor->xspr.dudeGuard);
	bool deaf = (actor->xspr.dudeDeaf);
	int nRandomSkill = Random(gGameOptions.nDifficulty);

	DBloodActor* newtarget = nullptr;
	// search for player targets
	for (i = connecthead; i != -1; i = connectpoint2[i])
	{
		pPlayer = getPlayer(i);
		if (!xsprIsFine(pPlayer->GetActor())) continue;

		auto plActor = pPlayer->GetActor();
		if (plActor->xspr.health <= 0)
			continue;

		newtarget = nullptr;
		seeChance = hearChance = 0x0000;
		auto pos = plActor->spr.pos;
		auto dv = pos.XY() - actor->spr.pos.XY();
		double nDistf = dv.Length();
		double seeDistf = (stealth) ? pDudeInfo->SeeDist() / 3 : pDudeInfo->SeeDist() / 4;
		double hearDistf = pDudeInfo->HearDist(); 
		double feelDistf = hearDistf / 2;

		// TO-DO: is there any dudes that sees this patrol dude and sees target?


		if (nDistf <= seeDistf)
		{
			double scratch;
			double eyeAboveZ = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (nDistf < seeDistf / 8) GetActorExtents(pPlayer->GetActor(), &pos.Z, &scratch); //use ztop of the target sprite
			if (!cansee(pos, plActor->sector(), actor->spr.pos - eyeAboveZ, actor->sector()))
				continue;
		}
		else
			continue;

		bool invisible = (powerupCheck(pPlayer, kPwUpShadowCloak) > 0);
		if (spritesTouching(actor, pPlayer->GetActor()) || spritesTouching(pPlayer->GetActor(), actor))
		{
			DPrintf(DMSG_SPAMMY, "Patrol dude #%d spot the Player #%d via touch.", actor->GetIndex(), pPlayer->pnum + 1);
			if (invisible) pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
			newtarget = pPlayer->GetActor();
			break;
		}

		if (!deaf)
		{

			soundEngine->EnumerateChannels([&](FSoundChan* chan)
				{
					DVector2 sndv;
					sectortype* searchsect = nullptr;
					if (chan->SourceType == SOURCE_Actor)
					{
						auto emitterActor = (DBloodActor*)chan->Source;
						if (emitterActor == nullptr) return false; // not a valid source.
						sndv = emitterActor->spr.pos.XY();

						// sound attached to the sprite
						if (pPlayer->GetActor() != emitterActor && emitterActor->GetOwner() != actor)
						{

							if (!emitterActor->insector()) return false;
							searchsect = emitterActor->sector();
						}
					}
					else if (chan->SourceType == SOURCE_Unattached)
					{
						if (chan->UserData < 0 || !validSectorIndex(chan->UserData)) return false; // not a vaild sector sound.
						sndv.X = chan->Point[0];
						sndv.Y = -chan->Point[1];
						searchsect = &sector[chan->UserData];
					}
					if (searchsect == nullptr) return false;
					double nsDist = (sndv - actor->spr.pos.XY()).Length();
					if (nsDist > hearDistf) return false;


					auto sndnum = chan->OrgID;

					// N same sounds per single enemy
					for (int f = 0; f < sndCnt; f++)
					{
						if (patrolBonkles[f].snd != sndnum) continue;
						else if (++patrolBonkles[f].cur >= patrolBonkles[f].max)
							return false;
					}
					if (sndCnt < kMaxPatrolFoundSounds - 1)
						patrolBonkles[sndCnt++].snd = sndnum;

					bool found = false;
					BloodSectIterator it(searchsect);
					while (auto act = it.Next())
					{
						if (act->GetOwner() == pPlayer->GetActor())
						{
							found = true;
							break;
						}
					}
					if (!found) return false;

					int f = max(int((hearDistf - nsDist) * 16), 0);
					int sndvol = int(chan->Volume * (80.f / 0.8f));
					hearChance += sndvol * f + nRandomSkill;
					return (hearChance >= kMaxPatrolSpotValue);
				});
			/*
			if (invisible && hearChance >= kMaxPatrolSpotValue >> 2)
			{
				newtarget = pPlayer->actor;
				pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
				invisible = false;
				break;
			}
			 */
		}

		if (!invisible && (!deaf || !blind))
		{
			if (stealth)
			{
				switch (pPlayer->lifeMode)
				{
				case kModeHuman:
				case kModeHumanShrink:
					if (pPlayer->lifeMode == kModeHumanShrink)
					{
						seeDistf *= 92. / 256;
						feelDistf *= 92. / 256;
					}
					if (pPlayer->posture == kPostureCrouch)
					{
						seeDistf *= 0.75;
						feelDistf *= 0.5;
					}
					break;
				case kModeHumanGrown:
					if (pPlayer->posture != kPostureCrouch)
					{
						seeDistf *= 328. / 256;
						feelDistf *= 320. / 256;
					}
					else
					{
						seeDistf *= 304. / 256;
					}
					break;
				}
			}

			bool itCanHear = false; 
			bool itCanSee = false;
			feelDistf = max(feelDistf, 0.);
			seeDistf = max(seeDistf, 0.);

			if (hearDistf)
			{
				DBloodActor* act = pPlayer->GetActor();
				itCanHear = (!deaf && (nDistf < hearDistf || hearChance > 0));
				if (act && itCanHear && nDistf < feelDistf && (!act->vel.isZero()))
					hearChance += (int)max(((feelDistf - nDistf) + act->vel.Sum() * 64, 0.) / 256, 0.);

			}

			if (seeDistf)
			{
				DAngle periphery = max(pDudeInfo->Periphery(), DAngle60);
				DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dv.Angle());
				if ((itCanSee = (!blind && nDistf < seeDistf && nDeltaAngle < periphery)) == true)
				{
					int base = 100 + ((20 * gGameOptions.nDifficulty) - (nDeltaAngle.Buildang() / 5));
					//seeChance = base - MulScale(ClipRange(5 - gGameOptions.nDifficulty, 1, 4), nDist >> 1, 16);
					//scale(0x40000, a6, dist2);
					int d = int(nDistf * 4);
					int m = DivScale(d, 0x2000, 8);
					int t = MulScale(d, m, 8);
					seeChance = ClipRange(DivScale(base, t, 8), 0, kMaxPatrolSpotValue >> 1);
				}
			}

			if (!itCanSee && !itCanHear)
				continue;

			if (stealth)
			{
				// search in stealth regions to modify spot chances
				BloodStatIterator it(kStatModernStealthRegion);
				while (auto steal = it.Next())
				{
					if (!steal->hasX())
						continue;

					if (steal->xspr.locked) // ignore locked regions
						continue;

					bool fixd = (steal->spr.flags & kModernTypeFlag1); // fixed percent value
					bool both = (steal->spr.flags & kModernTypeFlag4); // target AND dude must be in this region
					bool dude = (both || (steal->spr.flags & kModernTypeFlag2)); // dude must be in this region
					bool trgt = (both || !dude); // target must be in this region
					bool crouch = (steal->spr.flags & kModernTypeFlag8); // target must crouch
					//bool floor = (iactor->spr.cstat & CSTAT_SPRITE_BLOCK); // target (or dude?) must touch floor of the sector
					if (trgt)
					{

						if (steal->xspr.data1 > 0)
						{
							if ((steal->spr.pos.XY() - plActor->spr.pos.XY()).Length() >= steal->xspr.data1)
								continue;

						}
						else if (plActor->sector() != steal->sector())
							continue;

						if (crouch && pPlayer->posture == kPostureStand)
							continue;
					}

					if (dude)
					{
						if (steal->xspr.data1 > 0)
						{
							if ((steal->spr.pos.XY() - plActor->spr.pos.XY()).Length() >= steal->xspr.data1)
								continue;

						}
						else if (plActor->sector() != steal->sector())
							continue;
					}

					if (itCanHear)
					{
						if (fixd)
							hearChance = ClipLow(hearChance, steal->xspr.data2);

						mod = (hearChance * steal->xspr.data2) / kPercFull;
						if (fixd)  hearChance = mod; else hearChance += mod;

						hearChance = ClipRange(hearChance, -kMaxPatrolSpotValue, kMaxPatrolSpotValue);
					}

					if (itCanSee)
					{
						if (fixd)
							seeChance = ClipLow(seeChance, steal->xspr.data3);

						mod = (seeChance * steal->xspr.data3) / kPercFull;
						if (fixd) seeChance = mod; else seeChance += mod;

						seeChance = ClipRange(seeChance, -kMaxPatrolSpotValue, kMaxPatrolSpotValue);
					}

					// trigger this region if target gonna be spot
					if (steal->xspr.txID && actor->xspr.data3 + hearChance + seeChance >= kMaxPatrolSpotValue)
						trTriggerSprite(steal, kCmdToggle, pPlayer->GetActor());

					// continue search another stealth regions to affect chances
				}
			}

			if (itCanHear && hearChance > 0)
			{
				DPrintf(DMSG_SPAMMY, "Patrol dude #%d hearing the Player #%d.", actor->GetIndex(), pPlayer->pnum + 1);
				actor->xspr.data3 = ClipRange(actor->xspr.data3 + hearChance, -kMaxPatrolSpotValue, kMaxPatrolSpotValue);
				if (!stealth)
				{
					newtarget = pPlayer->GetActor();
					break;
				}
			}

			if (itCanSee && seeChance > 0)
			{
				//DPrintf(DMSG_SPAMMY, "Patrol dude #%d seeing the Player #%d.", actor->GetIndex(), pPlayer->nPlayer + 1);
				//actor->xspr.data3 += seeChance;
				actor->xspr.data3 = ClipRange(actor->xspr.data3 + seeChance, -kMaxPatrolSpotValue, kMaxPatrolSpotValue);
				if (!stealth)
				{
					newtarget = pPlayer->GetActor();
					break;
				}
			}
		}

		// add check for corpses?

		if ((actor->xspr.data3 = ClipRange(actor->xspr.data3, 0, kMaxPatrolSpotValue)) == kMaxPatrolSpotValue)
		{
			newtarget = pPlayer->GetActor();
			break;
		}

		//int perc = (100 * ClipHigh(actor->xspr.data3, kMaxPatrolSpotValue)) / kMaxPatrolSpotValue;
		//viewSetSystemMessage("%d / %d / %d / %d", hearChance, seeDist, seeChance, perc);

	}

	if (newtarget) return newtarget;
	actor->xspr.data3 -= ClipLow(((kPercFull * actor->xspr.data3) / kMaxPatrolSpotValue) >> 2, 3);
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolFlagsMgr(DBloodActor* sourceactor, DBloodActor* destactor, bool copy, bool init)
{
	// copy flags
	if (copy)
	{
		destactor->xspr.dudeFlag4 = sourceactor->xspr.dudeFlag4;
		destactor->xspr.dudeAmbush = sourceactor->xspr.dudeAmbush;
		destactor->xspr.dudeGuard = sourceactor->xspr.dudeGuard;
		destactor->xspr.dudeDeaf = sourceactor->xspr.dudeDeaf;
		destactor->xspr.unused1 = sourceactor->xspr.unused1;

		if (sourceactor->xspr.unused1 & kDudeFlagStealth) destactor->xspr.unused1 |= kDudeFlagStealth;
		else destactor->xspr.unused1 &= ~kDudeFlagStealth;
	}

	// do init
	if (init && destactor->IsDudeActor() && !destactor->IsPlayerActor())
	{
		if (!destactor->xspr.dudeFlag4)
		{
			if (aiInPatrolState(destactor->xspr.aiState))
				aiPatrolStop(destactor, nullptr);
		}
		else
		{
			if (aiInPatrolState(destactor->xspr.aiState))
				return;

			destactor->SetTarget(nullptr);
			destactor->xspr.stateTimer = 0;

			aiPatrolSetMarker(destactor);
			if (spriteIsUnderwater(destactor)) aiPatrolState(destactor, kAiStatePatrolWaitW);
			else aiPatrolState(destactor, kAiStatePatrolWaitL);
			destactor->xspr.data3 = 0; // reset the spot progress
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool aiPatrolGetPathDir(DBloodActor* actor, DBloodActor* marker)
{
	if (actor->xspr.unused2 == kPatrolMoveForward) return (marker->xspr.data2 == -2) ? (bool)kPatrolMoveBackward : (bool)kPatrolMoveForward;
	else return (findNextMarker(marker, kPatrolMoveBackward) != nullptr) ? (bool)kPatrolMoveBackward : (bool)kPatrolMoveForward;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiPatrolThink(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);

	DBloodActor* targetactor;
	unsigned int stateTimer;
	auto markeractor = actor->GetTarget();
	if ((targetactor = aiPatrolSearchTargets(actor)) != nullptr)
	{
		aiPatrolStop(actor, targetactor, actor->xspr.dudeAmbush);
		return;
	}


	bool crouch = (actor->xspr.unused1 & kDudeFlagCrouch), uwater = spriteIsUnderwater(actor);
	if (markeractor == nullptr || (actor->spr.type == kDudeModernCustom && ((uwater && !canSwim(actor)) || !canWalk(actor))))
	{
		aiPatrolStop(actor, nullptr);
		return;
	}

	const DUDEINFO_EXTRA* pExtra = &gDudeInfoExtra[actor->spr.type - kDudeBase];
	bool isFinal = ((!actor->xspr.unused2 && markeractor->xspr.data2 == -1) || (actor->xspr.unused2 && markeractor->xspr.data1 == -1));
	bool reached = false;

	if (aiPatrolWaiting(actor->xspr.aiState))
	{
		//viewSetSystemMessage("WAIT %d / %d", actor->xspr.targetY, actor->xspr.stateTimer);

		if (actor->xspr.stateTimer > 0 || markeractor->xspr.data1 == markeractor->xspr.data2)
		{
			if (pExtra->flying)
				actor->vel.Z = Random2F(0x8000);

			// turn while waiting
			if (markeractor->spr.flags & kModernTypeFlag16)
			{
				stateTimer = actor->xspr.stateTimer;

				if (--actor->xspr.unused4 <= 0)
				{
					if (uwater) aiPatrolState(actor, kAiStatePatrolTurnW);
					else if (crouch) aiPatrolState(actor, kAiStatePatrolTurnC);
					else aiPatrolState(actor, kAiStatePatrolTurnL);
					actor->xspr.unused4 = kMinPatrolTurnDelay + Random(kPatrolTurnDelayRange);
				}

				// must restore stateTimer for waiting
				actor->xspr.stateTimer = stateTimer;
			}
			return;
		}

		// trigger at departure
		if (markeractor->xspr.triggerOff)
		{
			// send command
			if (markeractor->xspr.txID)
			{
				evSendActor(markeractor, markeractor->xspr.txID, (COMMAND_ID)markeractor->xspr.command, actor);

				// copy dude flags for current dude
			}
			else if (markeractor->xspr.command == kCmdDudeFlagsSet)
			{
				aiPatrolFlagsMgr(markeractor, actor, true, true);
				if (!actor->xspr.dudeFlag4) // this dude is not in patrol anymore
					return;
			}
		}

		// release the enemy
		if (isFinal)
		{
			aiPatrolStop(actor, nullptr);
			return;
		}

		// move next marker
		aiPatrolSetMarker(actor);

	}
	else if (aiPatrolTurning(actor->xspr.aiState))
	{
		//viewSetSystemMessage("TURN");
		if (absangle(actor->spr.Angles.Yaw, actor->xspr.goalAng) < minAngle)
		{
			// save imer for waiting
			stateTimer = actor->xspr.stateTimer;

			if (uwater) aiPatrolState(actor, kAiStatePatrolWaitW);
			else if (crouch) aiPatrolState(actor, kAiStatePatrolWaitC);
			else aiPatrolState(actor, kAiStatePatrolWaitL);

			// must restore it
			actor->xspr.stateTimer = stateTimer;
		}
		return;

	}
	else if ((reached = aiPatrolMarkerReached(actor)) == true)
	{
		markeractor->xspr.isTriggered = markeractor->xspr.triggerOnce; // can't select this marker for path anymore if true

		if (markeractor->spr.flags > 0)
		{
			if ((markeractor->spr.flags & kModernTypeFlag2) && (markeractor->spr.flags & kModernTypeFlag1)) crouch = !crouch;
			else if (markeractor->spr.flags & kModernTypeFlag2) crouch = false;
			else if ((markeractor->spr.flags & kModernTypeFlag1) && aiCanCrouch(actor)) crouch = true;
		}

		if (markeractor->xspr.waitTime > 0 || markeractor->xspr.data1 == markeractor->xspr.data2)
		{
			// take marker's angle
			if (!(markeractor->spr.flags & kModernTypeFlag4))
			{
				actor->xspr.goalAng = ((!(markeractor->spr.flags & kModernTypeFlag8) && actor->xspr.unused2) ? markeractor->spr.Angles.Yaw+ DAngle180 : markeractor->spr.Angles.Yaw).Normalized360();
				if (absangle(actor->spr.Angles.Yaw, actor->xspr.goalAng) > minAngle) // let the enemy play move animation while turning
					return;
			}

			if (markeractor->GetOwner() == actor)
				markeractor->SetOwner(aiPatrolMarkerBusy(actor, markeractor));

			// trigger at arrival
			if (markeractor->xspr.triggerOn)
			{
				// send command
				if (markeractor->xspr.txID)
				{
					evSendActor(markeractor, markeractor->xspr.txID, (COMMAND_ID)markeractor->xspr.command, actor);
				}
				else if (markeractor->xspr.command == kCmdDudeFlagsSet)
				{
					// copy dude flags for current dude
					aiPatrolFlagsMgr(markeractor, actor, true, true);
					if (!actor->xspr.dudeFlag4) // this dude is not in patrol anymore
						return;
				}
			}

			if (uwater) aiPatrolState(actor, kAiStatePatrolWaitW);
			else if (crouch) aiPatrolState(actor, kAiStatePatrolWaitC);
			else aiPatrolState(actor, kAiStatePatrolWaitL);

			if (markeractor->xspr.waitTime)
				actor->xspr.stateTimer = (markeractor->xspr.waitTime * 120) / 10;

			if (markeractor->spr.flags & kModernTypeFlag16)
				actor->xspr.unused4 = kMinPatrolTurnDelay + Random(kPatrolTurnDelayRange);

			return;
		}
		else
		{
			if (markeractor->GetOwner() == actor)
				markeractor->SetOwner(aiPatrolMarkerBusy(actor, markeractor));

			if (markeractor->xspr.triggerOn || markeractor->xspr.triggerOff)
			{
				if (markeractor->xspr.txID)
				{
					// send command at arrival
					if (markeractor->xspr.triggerOn)
						evSendActor(markeractor, markeractor->xspr.txID, (COMMAND_ID)markeractor->xspr.command, actor);

					// send command at departure
					if (markeractor->xspr.triggerOff)
						evSendActor(markeractor, markeractor->xspr.txID, (COMMAND_ID)markeractor->xspr.command, actor);

					// copy dude flags for current dude
				}
				else if (markeractor->xspr.command == kCmdDudeFlagsSet)
				{
					aiPatrolFlagsMgr(markeractor, actor, true, true);
					if (!actor->xspr.dudeFlag4) // this dude is not in patrol anymore
						return;
				}
			}

			// release the enemy
			if (isFinal)
			{
				aiPatrolStop(actor, nullptr);
				return;
			}

			// move the next marker
			aiPatrolSetMarker(actor);
		}
	}

	nnExtAiSetDirection(actor, (markeractor->spr.pos - actor->spr.pos).Angle());

	if (aiPatrolMoving(actor->xspr.aiState) && !reached) return;
	else if (uwater) aiPatrolState(actor, kAiStatePatrolMoveW);
	else if (crouch) aiPatrolState(actor, kAiStatePatrolMoveC);
	else aiPatrolState(actor, kAiStatePatrolMoveL);
	return;

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int listTx(DBloodActor* actor, int tx)
{
	if (txIsRanged(actor))
	{
		if (tx == -1) tx = actor->xspr.data1;
		else if (tx < actor->xspr.data4) tx++;
		else tx = -1;
	}
	else
	{
		if (tx == -1)
		{
			for (int i = 0; i <= 3; i++)
			{
				if ((tx = GetDataVal(actor, i)) <= 0) continue;
				else return tx;
			}
		}
		else
		{
			int saved = tx; bool savedFound = false;
			for (int i = 0; i <= 3; i++)
			{
				tx = GetDataVal(actor, i);
				if (savedFound && tx > 0) return tx;
				else if (tx != saved) continue;
				else savedFound = true;
			}
		}
		tx = -1;
	}
	return tx;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* evrIsRedirector(DBloodActor* actor)
{
	if (actor)
	{
		switch (actor->spr.type)
		{
		case kModernRandomTX:
		case kModernSequentialTX:
			if (actor->hasX() && actor->xspr.command == kCmdLink && !actor->xspr.locked) return actor;
			break;
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* evrListRedirectors(int objType, sectortype* pSector, walltype* pWall, DBloodActor* objActor, DBloodActor* pXRedir, int* tx)
{
	if (!gEventRedirectsUsed) return nullptr;
	else if (pXRedir && (*tx = listTx(pXRedir, *tx)) != -1)
		return pXRedir;

	int id = 0;
	switch (objType)
	{
	case OBJ_SECTOR:
	{
		if (!pSector->hasX()) return nullptr;
		id = pSector->xs().txID;
		break;
	}
	case OBJ_SPRITE:
		if (!objActor) return nullptr;
		id = objActor->xspr.txID;
		break;
	case OBJ_WALL:
	{
		if (!pWall->hasX()) return nullptr;
		id = pWall->xw().txID;
		break;
	}
	default:
		return nullptr;
	}

	bool prevFound = false;
	for (int i = bucketHead[id]; i < bucketHead[id + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto rxactor = rxBucket[i].actor();

		auto pXSpr = evrIsRedirector(rxactor);
		if (!pXSpr) continue;
		else if (prevFound || pXRedir == nullptr)
		{
			*tx = listTx(pXSpr, *tx);
			return pXSpr;
		}
		else if (pXRedir != pXSpr) continue;
		else prevFound = true;
	}

	*tx = -1;
	return NULL;
}

//---------------------------------------------------------------------------
//
// this function checks if all TX objects have the same value
//
//---------------------------------------------------------------------------

bool incDecGoalValueIsReached(DBloodActor* actor)
{
	if (actor->xspr.data3 != actor->xspr.sysData1) return false;
	char buffer[7];
	snprintf(buffer, 7, "%d", abs(actor->xspr.data1));
	int len = int(strlen(buffer));
	int rx = -1;

	for (int i = bucketHead[actor->xspr.txID]; i < bucketHead[actor->xspr.txID + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto rxactor = rxBucket[i].actor();

		if (evrIsRedirector(rxactor)) continue;
		for (int a = 0; a < len; a++)
		{
			if (getDataFieldOfObject(rxBucket[i], (buffer[a] - 52) + 4) != actor->xspr.data3)
				return false;
		}
	}

	DBloodActor* pXRedir = nullptr; // check redirected TX buckets
	while ((pXRedir = evrListRedirectors(OBJ_SPRITE, nullptr, nullptr, actor, pXRedir, &rx)) != nullptr)
	{
		for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++)
		{
			for (int a = 0; a < len; a++)
			{
				if (getDataFieldOfObject(rxBucket[i], (buffer[a] - 52) + 4) != actor->xspr.data3)
					return false;
			}
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqSpawnerOffSameTx(DBloodActor* actor)
{
	for (auto&sect : sector)
	{
		BloodSectIterator it(&sect);
		while (auto iactor = it.Next())
		{
			if (iactor->spr.type != kModernSeqSpawner || !iactor->hasX() || iactor == actor) continue;
			if (/*iactor->xspr.txID == actor->xspr.txID &&*/ iactor->xspr.state == 1)
			{
				evKillActor(iactor);
				iactor->xspr.state = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// this function can be called via sending numbered command to TX kChannelModernEndLevelCustom
// it allows to set custom next level instead of taking it from INI file.
//
//---------------------------------------------------------------------------

void levelEndLevelCustom(int nLevel)
{
	gGameOptions.uGameFlags |= GF_AdvanceLevel;
	gNextLevel = FindMapByIndex(currentLevel->cluster, nLevel + 1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void callbackUniMissileBurst(DBloodActor* actor, sectortype*) // 22
{
	if (!actor) return;
	if (actor->spr.statnum != kStatProjectile) return;
	auto nAngVec = actor->vel.XY().Angle().ToVector();
	double nRadius = FixedToFloat(0x55555);

	for (int i = 0; i < 8; i++)
	{
		auto burstactor = actSpawnSprite(actor, 5);
		if (!burstactor) break;

		burstactor->spr.type = actor->spr.type;
		burstactor->spr.shade = actor->spr.shade;
		burstactor->spr.setspritetexture(actor->spr.spritetexture());


		burstactor->spr.cstat = actor->spr.cstat;
		if ((burstactor->spr.cstat & CSTAT_SPRITE_BLOCK))
		{
			burstactor->spr.cstat &= ~CSTAT_SPRITE_BLOCK; // we don't want missiles impact each other
			evPostActor(burstactor, 100, kCallbackMissileSpriteBlock); // so set blocking flag a bit later
		}

		burstactor->spr.pal = actor->spr.pal;
		burstactor->clipdist = actor->clipdist * 0.25;
		burstactor->spr.flags = actor->spr.flags;
		burstactor->spr.scale = actor->spr.scale;
		burstactor->spr.scale *= 0.5;

		burstactor->spr.Angles.Yaw = actor->spr.Angles.Yaw + mapangle(missileInfo[actor->spr.type - kMissileBase].angleOfs);
		burstactor->SetOwner(actor);

		actBuildMissile(burstactor, actor);

		auto spAngVec = DAngle::fromBam(i << 29).ToVector().Rotated90CW() * nRadius;
		if (i & 1) spAngVec *= 0.5;
		burstactor->vel += DVector3(DVector2(0, spAngVec.X).Rotated(nAngVec.X, nAngVec.Y), spAngVec.Y);
		evPostActor(burstactor, 960, kCallbackRemove);
	}
	evPostActor(actor, 0, kCallbackRemove);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void callbackMakeMissileBlocking(DBloodActor* actor, sectortype*) // 23
{
	if (!actor || actor->spr.statnum != kStatProjectile) return;
	actor->spr.cstat |= CSTAT_SPRITE_BLOCK;
}

void callbackGenDudeUpdate(DBloodActor* actor, sectortype*) // 24
{
	if (actor)
		genDudeUpdate(actor);
}

void clampSprite(DBloodActor* actor, int which)
{
	double zTop, zBot;
	if (actor->insector())
	{
		GetActorExtents(actor, &zTop, &zBot);
		if (which & 0x01)
			actor->spr.pos.Z += min(getflorzofslopeptr(actor->sector(), actor->spr.pos) - zBot, 0.);
		if (which & 0x02)
			actor->spr.pos.Z += max(getceilzofslopeptr(actor->sector(), actor->spr.pos) - zTop, 0.);

	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void killEvents(int nRx, int nCmd)
{
	for (int i = bucketHead[nRx]; i < bucketHead[nRx + 1]; i++)
	{
		if (nCmd == kCmdEventKillFull)
			evKill_(rxBucket[i]);
	}
}

void triggerTouchSprite(DBloodActor* actor, DBloodActor* hActor)
{
	if (hActor && hActor->hasX())
	{
		if (hActor->xspr.Touch && !hActor->xspr.isTriggered && (!hActor->xspr.DudeLockout || actor->IsPlayerActor()))
			trTriggerSprite(hActor, kCmdSpriteTouch, actor);

		// enough to reset gSpriteHit values
		actor->vel.X += FixedToFloat(5);
	}
}

void triggerTouchWall(DBloodActor* actor, walltype* pHWall)
{
	if (pHWall && pHWall->hasX())
	{
		if (pHWall->xw().triggerTouch && !pHWall->xw().isTriggered && (!pHWall->xw().dudeLockout || actor->IsPlayerActor()))
			trTriggerWall(pHWall, kCmdWallTouch, actor);

		// enough to reset gSpriteHit values
		actor->vel.X += FixedToFloat(5);
	}
}

void changeSpriteAngle(DBloodActor* pSpr, DAngle nAng)
{
	if (!pSpr->IsDudeActor())
		pSpr->spr.Angles.Yaw = nAng;
	else
	{
		DBloodPlayer* pPlayer = getPlayerById(pSpr->spr.type);
		if (pPlayer)
			pPlayer->GetActor()->spr.Angles.Yaw = nAng;
		else
		{
			pSpr->spr.Angles.Yaw = nAng;
			if (xsprIsFine(pSpr))
				pSpr->xspr.goalAng = pSpr->spr.Angles.Yaw;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void dokillEffectGenCallbacks(DBloodActor* actor)
{
	int l = sizeof(gEffectGenCallbacks) / sizeof(gEffectGenCallbacks[0]);
	while (--l >= 0)
		evKillActor(actor, (CALLBACK_ID)gEffectGenCallbacks[l]);
}

void killEffectGenCallbacks(DBloodActor* actor)
{
	int i;
	if (actor->xspr.data2 < kEffectGenCallbackBase)
		return;

	switch (actor->xspr.txID)
	{
	case kChannelZero:  // self 
		dokillEffectGenCallbacks(actor);
		break;
	case kChannelAllPlayers: // player sprites
		for (i = connecthead; i >= 0; i = connectpoint2[i])
		{
			if (getPlayer(i)->GetActor() != nullptr)
				dokillEffectGenCallbacks(getPlayer(i)->GetActor());
		}
		break;
	case kChannelEventCauser: // worst case...
		for(auto& sec : sector)
		{
			BloodSectIterator it(&sec);
			while (auto act = it.Next())
				dokillEffectGenCallbacks(act);
		}
		break;
	default:
		if (actor->xspr.txID >= kChannelUser) // TX ID scope
		{
			for (i = bucketHead[actor->xspr.txID]; i < bucketHead[actor->xspr.txID + 1]; i++)
			{
				if (rxBucket[i].isActor())
					dokillEffectGenCallbacks(rxBucket[i].actor());
			}
		}
		else if (actor->xspr.txID >= kChannelPlayer0 && actor->xspr.txID < kChannelPlayer0 + kMaxPlayers)
		{
			// player sprites
			DBloodPlayer* pPlayer = getPlayerById(actor->xspr.txID - kChannelPlayer0);
			if (pPlayer && pPlayer->GetActor() != nullptr)
				dokillEffectGenCallbacks(pPlayer->GetActor());
		}
		break;

	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool xsprIsFine(DBloodActor* pSpr)
{
	if (pSpr && pSpr->hasX() && !(pSpr->spr.flags & kHitagFree))
	{
		if (!(pSpr->spr.flags & kHitagRespawn) || (pSpr->spr.statnum != kStatThing && pSpr->spr.statnum != kStatDude))
			return true;
	}

	return false;
}



//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, GENDUDEEXTRA& w, GENDUDEEXTRA* def)
{
	if (arc.BeginObject(keyname))
	{
		arc ("clipdist", w.clipdist)
			.Array("availdeaths", w.availDeaths, kDamageMax)
			("movespeed", w.moveSpeed)
			("firedist", w.fireDist)
			("throwdist", w.throwDist)
			("curweapon", w.curWeapon)
			("weapontype", w.weaponType)
			("basedispersion", w.baseDispersion)
			("slavecount", w.slaveCount)
			("lifeleech", w.pLifeLeech)
			.Array("slaves", w.slave, w.slaveCount)
			.Array("dmgcontrol", w.dmgControl, kDamageMax)
			.Array("updreq", w.updReq, kGenDudePropertyMax)
			("flags", w.flags)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, SPRITEMASS& w, SPRITEMASS* def)
{
	static SPRITEMASS nul;
	if (arc.isReading()) w = {};
	if (arc.BeginObject(keyname))
	{
		arc("seq", w.seqId, &nul.seqId)
			("texid", w.texid, &nul.texid)
			("scale", w.scale, &nul.scale)
			("clipdist", w.clipDist)
			("mass", w.mass)
			("airvel", w.airVel)
			("fraction", w.fraction)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, OBJECTS_TO_TRACK& w, OBJECTS_TO_TRACK* def)
{
	static OBJECTS_TO_TRACK nul;
	if (arc.isReading()) w = {};
	if (arc.BeginObject(keyname))
	{
		arc("obj", w.obj, &nul.obj)
			("cmd", w.cmd, &nul.cmd)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, TRCONDITION& w, TRCONDITION* def)
{
	static TRCONDITION nul;
	if (arc.isReading()) w = {};
	if (arc.BeginObject(keyname))
	{
		arc("xindex", w.actor, &nul.actor)
			("obj", w.objects)
			.EndObject();
	}
	return arc;
}

void SerializeNNExts(FSerializer& arc)
{
	if (arc.BeginObject("nnexts"))
	{
		arc("proxyspritescount", gProxySpritesCount)
			.Array("proxyspriteslist", gProxySpritesList, gProxySpritesCount)
			("sightspritescount", gSightSpritesCount)
			.Array("sightspriteslist", gSightSpritesList, gSightSpritesCount)
			("physspritescount", gPhysSpritesCount)
			.Array("physspriteslist", gPhysSpritesList, gPhysSpritesCount)
			("impactspritescount", gImpactSpritesCount)
			.Array("impactspriteslist", gImpactSpritesList, gImpactSpritesCount)
			("eventredirects", gEventRedirectsUsed)
			("trcondition", gConditions);
		gSprNSect.Serialize(arc);
		arc.EndObject();
	}
}

///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////
END_BLD_NS

#endif
