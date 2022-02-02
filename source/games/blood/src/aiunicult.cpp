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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "raze_sound.h"

#include "blood.h"

#ifdef NOONE_EXTENSIONS


BEGIN_BLD_NS
static void ThrowThing(DBloodActor*, bool);
static void unicultThinkSearch(DBloodActor*);
static void unicultThinkGoto(DBloodActor*);
static void unicultThinkChase(DBloodActor*);
static void forcePunch(DBloodActor*);

AISTATE genDudeIdleL = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE genDudeIdleW = { kAiStateIdle, 13, -1, 0, NULL, NULL, aiThinkTarget, NULL };
// ---------------------
AISTATE genDudeSearchL = { kAiStateSearch, 9, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchW = { kAiStateSearch, 13, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeSearchShortL = { kAiStateSearch, 9, -1, 200, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchShortW = { kAiStateSearch, 13, -1, 200, NULL, aiGenDudeMoveForward, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeSearchNoWalkL = { kAiStateSearch, 0, -1, 600, NULL, aiMoveTurn, unicultThinkSearch, &genDudeIdleL };
AISTATE genDudeSearchNoWalkW = { kAiStateSearch, 13, -1, 600, NULL, aiMoveTurn, unicultThinkSearch, &genDudeIdleW };
// ---------------------
AISTATE genDudeGotoL = { kAiStateMove, 9, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkGoto, &genDudeIdleL };
AISTATE genDudeGotoW = { kAiStateMove, 13, -1, 600, NULL, aiGenDudeMoveForward, unicultThinkGoto, &genDudeIdleW };
// ---------------------
AISTATE genDudeDodgeL = { kAiStateMove, 9, -1, 90, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeD = { kAiStateMove, 14, -1, 90, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeW = { kAiStateMove, 13, -1, 90, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeDodgeShortL = { kAiStateMove, 9, -1, 60, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeShortD = { kAiStateMove, 14, -1, 60, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeShortW = { kAiStateMove, 13, -1, 60, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeDodgeShorterL = { kAiStateMove, 9, -1, 20, NULL,	aiMoveDodge,	NULL, &genDudeChaseL };
AISTATE genDudeDodgeShorterD = { kAiStateMove, 14, -1, 20, NULL, aiMoveDodge,	NULL, &genDudeChaseD };
AISTATE genDudeDodgeShorterW = { kAiStateMove, 13, -1, 20, NULL, aiMoveDodge,	NULL, &genDudeChaseW };
// ---------------------
AISTATE genDudeChaseL = { kAiStateChase, 9, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
AISTATE genDudeChaseD = { kAiStateChase, 14, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
AISTATE genDudeChaseW = { kAiStateChase, 13, -1, 0, NULL,	aiGenDudeMoveForward, unicultThinkChase, NULL };
// ---------------------
AISTATE genDudeChaseNoWalkL = { kAiStateChase, 0, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
AISTATE genDudeChaseNoWalkD = { kAiStateChase, 14, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
AISTATE genDudeChaseNoWalkW = { kAiStateChase, 13, -1, 0, NULL,	aiMoveTurn, unicultThinkChase, NULL };
// ---------------------
AISTATE genDudeFireL = { kAiStateChase, 6, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireL };
AISTATE genDudeFireD = { kAiStateChase, 8, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireD };
AISTATE genDudeFireW = { kAiStateChase, 8, nGenDudeAttack1, 0, NULL, aiMoveTurn, unicultThinkChase, &genDudeFireW };
// ---------------------z
AISTATE genDudeRecoilL = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseL };
AISTATE genDudeRecoilD = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseD };
AISTATE genDudeRecoilW = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &genDudeChaseW };
AISTATE genDudeRecoilTesla = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &genDudeDodgeShortL };
// ---------------------
AISTATE genDudeThrow = { kAiStateChase, 7, nGenDudeThrow1, 0, NULL, NULL, NULL, &genDudeChaseL };
AISTATE genDudeThrow2 = { kAiStateChase, 7, nGenDudeThrow2, 0, NULL, NULL, NULL, &genDudeChaseL };
// ---------------------
AISTATE genDudePunch = { kAiStateChase,10, nGenDudePunch, 0, NULL, NULL, forcePunch, &genDudeChaseL };
// ---------------------

const GENDUDESND gCustomDudeSnd[] = {
	{ 1003, 2, 0, true, false   },      // spot sound
	{ 1013, 2, 2, true, true    },      // pain sound
	{ 1018, 2, 4, false, true   },      // death sound
	{ 1031, 2, 6, true, true    },      // burning state sound
	{ 1018, 2, 8, false, true   },      // explosive death or end of burning state sound
	{ 4021, 2, 10, true, false  },	    // target of dude is dead
	{ 1005, 2, 12, true, false  },	    // chase sound
	{ -1, 0, 14, false, true    },	    // weapon attack
	{ -1, 0, 15, false, true    },	    // throw attack
	{ -1, 0, 16, false, true    },	    // melee attack
	{ 9008, 0, 17, false, false },      // transforming in other dude
};

// for kModernThingThrowableRock
const int16_t gCustomDudeDebrisPics[6] = {

	2406, 2280, 2185, 2155, 2620, 3135

};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void forcePunch(DBloodActor* actor)
{
	if (actor->genDudeExtra.forcePunch && seqGetStatus(actor) == -1)
		punchCallback(0, actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool genDudeAdjustSlope(DBloodActor* actor, int dist, int weaponType, int by = 64)
{
	if (actor->GetTarget() != nullptr)
	{
		int fStart = 0;
		int fEnd = 0;
		GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
		unsigned int clipMask = (weaponType == kGenDudeWeaponMissile) ? CLIPMASK0 : CLIPMASK1;

		for (int i = -8191; i < 8192; i += by)
		{
			HitScan(actor, actor->int_pos().Z, bcos(actor->spr.ang), bsin(actor->spr.ang), i, clipMask, dist);
			if (!fStart && actor->GetTarget() == gHitInfo.actor()) fStart = i;
			else if (fStart && actor->GetTarget() != gHitInfo.actor())
			{
				fEnd = i;
				break;
			}
		}

		if (fStart != fEnd)
		{
			if (weaponType == kGenDudeWeaponHitscan)
			{
				actor->dudeSlope = fStart - ((fStart - fEnd) >> 2);
			}
			else if (weaponType == kGenDudeWeaponMissile)
			{
				const MissileType* pMissile = &missileInfo[pExtra->curWeapon - kMissileBase];
				actor->dudeSlope = (fStart - ((fStart - fEnd) >> 2)) - (pMissile->clipDist << 1);
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

void genDudeUpdate(DBloodActor* actor)
{
	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	for (int i = 0; i < kGenDudePropertyMax; i++) {
		if (pExtra->updReq[i]) genDudePrepare(actor, i);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void punchCallback(int, DBloodActor* actor)
{
	auto const target = actor->GetTarget();
	if (target != nullptr)
	{
		int nZOffset1 = getDudeInfo(actor->spr.type)->eyeHeight * actor->spr.yrepeat << 2;
		int nZOffset2 = 0;


		if (target->IsDudeActor())
			nZOffset2 = getDudeInfo(target->spr.type)->eyeHeight * target->spr.yrepeat << 2;

		int dx = bcos(actor->spr.ang);
		int dy = bsin(actor->spr.ang);
		int dz = nZOffset1 - nZOffset2;

		if (!playGenDudeSound(actor, kGenDudeSndAttackMelee))
			sfxPlay3DSound(actor, 530, 1, 0);

		actFireVector(actor, 0, 0, dx, dy, dz, kVectorGenDudePunch);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void genDudeAttack1(int, DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr) return;

	int dx, dy, dz;
	actor->vel.X = actor->vel.Y = 0;

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	int dispersion = pExtra->baseDispersion;
	if (inDuck(actor->xspr.aiState))
		dispersion = ClipLow(dispersion >> 1, kGenDudeMinDispesion);

	if (pExtra->weaponType == kGenDudeWeaponHitscan)
	{
		dx = bcos(actor->spr.ang); dy = bsin(actor->spr.ang); dz = actor->dudeSlope;
		// dispersal modifiers here in case if non-melee enemy
		if (!dudeIsMelee(actor))
		{
			dx += Random3(dispersion); dy += Random3(dispersion); dz += Random3(dispersion);
		}

		actFireVector(actor, 0, 0, dx, dy, dz, (VECTOR_TYPE)pExtra->curWeapon);
		if (!playGenDudeSound(actor, kGenDudeSndAttackNormal))
			sfxPlayVectorSound(actor, pExtra->curWeapon);
	}
	else if (pExtra->weaponType == kGenDudeWeaponSummon)
	{
		DBloodActor* spawned = nullptr;
		int dist = actor->spr.clipdist << 4;
		if (pExtra->slaveCount <= gGameOptions.nDifficulty)
		{
			if ((spawned = actSpawnDude(actor, pExtra->curWeapon, dist + Random(dist), 0)) != NULL)
			{
				spawned->SetOwner(actor);

				if (spawned->hasX())
				{
					spawned->SetTarget(actor->GetTarget());
					if (spawned->GetTarget() != nullptr)
						aiActivateDude(spawned);
				}

				gKillMgr.AddKill(spawned);
				pExtra->slave[pExtra->slaveCount++] = spawned;
				if (!playGenDudeSound(actor, kGenDudeSndAttackNormal))
					sfxPlay3DSoundCP(actor, 379, 1, 0, 0x10000 - Random3(0x3000));
			}
		}
	}
	else if (pExtra->weaponType == kGenDudeWeaponMissile)
	{
		dx = bcos(actor->spr.ang); dy = bsin(actor->spr.ang); dz = actor->dudeSlope;

		// dispersal modifiers here
		dx += Random3(dispersion); dy += Random3(dispersion); dz += Random3(dispersion >> 1);

		actFireMissile(actor, 0, 0, dx, dy, dz, pExtra->curWeapon);
		if (!playGenDudeSound(actor, kGenDudeSndAttackNormal))
			sfxPlayMissileSound(actor, pExtra->curWeapon);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowCallback1(int, DBloodActor* actor)
{
	ThrowThing(actor, true);
}

void ThrowCallback2(int, DBloodActor* actor)
{
	ThrowThing(actor, false);
}

static void ThrowThing(DBloodActor* actor, bool impact)
{
	auto target = actor->GetTarget();

	if (target == nullptr)
		return;

	if (!(target->spr.type >= kDudeBase && target->spr.type < kDudeMax))
		return;

	int curWeapon = actor->genDudeExtra.curWeapon;
	int weaponType = actor->genDudeExtra.weaponType;
	if (weaponType != kGenDudeWeaponThrow) return;

	const THINGINFO* pThinkInfo = &thingInfo[curWeapon - kThingBase];
	if (!gThingInfoExtra[curWeapon - kThingBase].allowThrow) return;
	else if (!playGenDudeSound(actor, kGenDudeSndAttackThrow))
		sfxPlay3DSound(actor, 455, -1, 0);

	int zThrow = 14500;
	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	int dz = target->int_pos().Z - actor->int_pos().Z;
	int dist = approxDist(dx, dy);

	auto actLeech = leechIsDropped(actor);

	switch (curWeapon) {
	case kModernThingEnemyLifeLeech:
	case kThingDroppedLifeLeech:
		zThrow = 5000;
		// pickup life leech before throw it again
		if (actLeech != NULL) removeLeech(actLeech);
		break;
	}

	DBloodActor* spawned = nullptr;
	if ((spawned = actFireThing(actor, 0, 0, (dz / 128) - zThrow, curWeapon, DivScale(dist / 540, 120, 23))) == nullptr) return;

	if (pThinkInfo->picnum < 0 && spawned->spr.type != kModernThingThrowableRock) spawned->spr.picnum = 0;

	spawned->SetOwner(actor);

	switch (curWeapon) {
	case kThingNapalmBall:
		spawned->spr.xrepeat = spawned->spr.yrepeat = 24;
		spawned->xspr.data4 = 3 + gGameOptions.nDifficulty;
		impact = true;
		break;
	case kModernThingThrowableRock:
		spawned->spr.picnum = gCustomDudeDebrisPics[Random(5)];
		spawned->spr.xrepeat = spawned->spr.yrepeat = 24 + Random(42);
		spawned->spr.cstat |= CSTAT_SPRITE_BLOCK;
		spawned->spr.pal = 5;

		if (Chance(0x5000)) spawned->spr.cstat |= CSTAT_SPRITE_XFLIP;
		if (Chance(0x5000)) spawned->spr.cstat |= CSTAT_SPRITE_YFLIP;

		if (spawned->spr.xrepeat > 60) spawned->xspr.data1 = 43;
		else if (spawned->spr.xrepeat > 40) spawned->xspr.data1 = 33;
		else if (spawned->spr.xrepeat > 30) spawned->xspr.data1 = 23;
		else spawned->xspr.data1 = 12;
		return;
	case kThingTNTBarrel:
	case kThingArmedProxBomb:
	case kThingArmedSpray:
		impact = false;
		break;
	case kModernThingTNTProx:
		spawned->xspr.state = 0;
		spawned->xspr.Proximity = true;
		return;
	case kModernThingEnemyLifeLeech:
		if (actLeech != nullptr) spawned->xspr.health = actLeech->xspr.health;
		else spawned->xspr.health = ((pThinkInfo->startHealth << 4) * gGameOptions.nDifficulty) >> 1;

		sfxPlay3DSound(actor, 490, -1, 0);

		spawned->xspr.data3 = 512 / (gGameOptions.nDifficulty + 1);
		spawned->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
		spawned->spr.pal = 6;
		spawned->spr.clipdist = 0;
		spawned->SetTarget(actor->GetTarget());
		spawned->xspr.Proximity = true;
		spawned->xspr.stateTimer = 1;

		actor->genDudeExtra.pLifeLeech = spawned;
		evPostActor(spawned, 80, kCallbackLeechStateTimer);
		return;
	}

	if (impact == true && dist <= 7680) spawned->xspr.Impact = true;
	else {
		spawned->xspr.Impact = false;
		evPostActor(spawned, 120 * Random(2) + 120, kCmdOn, actor);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void unicultThinkSearch(DBloodActor* actor)
{
	// TO DO: if can't see the target, but in fireDist range - stop moving and look around

	aiChooseDirection(actor, actor->xspr.goalAng);
	aiLookForTarget(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void unicultThinkGoto(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax))
	{
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}

	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);

	aiChooseDirection(actor, nAngle);

	// if reached target, change to search mode
	if (approxDist(dx, dy) < 5120 && abs(actor->spr.ang - nAngle) < getDudeInfo(actor->spr.type)->periphery)
	{
		if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeSearchW);
		else aiGenDudeNewState(actor, &genDudeSearchL);
	}
	aiThinkTarget(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void unicultThinkChase(DBloodActor* actor)
{
	if (actor->spr.type < kDudeBase || actor->spr.type >= kDudeMax) return;

	auto const target = actor->GetTarget();
	if (target == nullptr)
	{
		if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeGotoW);
		else aiGenDudeNewState(actor, &genDudeGotoL);
		return;
	}
	else
	{
		genDudeUpdate(actor);
	}

	if (!target || !target->IsDudeActor() || !target->hasX())  // target lost
	{
		if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeSearchShortW);
		else aiGenDudeNewState(actor, &genDudeSearchShortL);
		actor->SetTarget(nullptr);
		return;
	}

	if (target->xspr.health <= 0) // target is dead
	{
		PLAYER* pPlayer = NULL;
		if ((!target->IsPlayerActor()) || ((pPlayer = getPlayerById(target->spr.type)) != NULL && pPlayer->fragger == actor))
		{
			playGenDudeSound(actor, kGenDudeSndTargetDead);
			if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeSearchShortW);
			else aiGenDudeNewState(actor, &genDudeSearchShortL);
		}
		else if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeGotoW);
		else aiGenDudeNewState(actor, &genDudeGotoL);
		actor->SetTarget(nullptr);
		return;
	}

	// check target
	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	int dist = ClipLow((int)approxDist(dx, dy), 1);

	// quick hack to prevent spinning around or changing attacker's sprite angle on high movement speeds
	// when attacking the target. It happens because vanilla function takes in account x and y velocity, 
	// so i use fake velocity with fixed value and pass it as argument.
	int xvelocity = actor->vel.X;
	int yvelocity = actor->vel.Y;
	if (inAttack(actor->xspr.aiState))
		xvelocity = yvelocity = ClipLow(actor->spr.clipdist >> 1, 1);

	//aiChooseDirection(actor,getangle(dx, dy));
	aiGenDudeChooseDirection(actor, getangle(dx, dy), xvelocity, yvelocity);

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	if (!pExtra->canAttack)
	{
		if (pExtra->canWalk) aiSetTarget(actor, actor); // targeting self???
		if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeGotoW);
		else aiGenDudeNewState(actor, &genDudeGotoL);
		return;
	}
	else if (target->IsPlayerActor())
	{
		PLAYER* pPlayer = &gPlayer[target->spr.type - kDudePlayer1];
		if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
		{
			if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeSearchShortW);
			else aiGenDudeNewState(actor, &genDudeSearchShortL);
			actor->SetTarget(nullptr);
			return;
		}
	}

	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int losAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
	int eyeAboveZ = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;

	if (dist > pDudeInfo->seeDist || !cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(),
		actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - eyeAboveZ, actor->sector()))
	{
		if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeSearchW);
		else aiGenDudeNewState(actor, &genDudeSearchL);
		actor->SetTarget(nullptr);
		return;
	}

	// is the target visible?
	if (dist < pDudeInfo->seeDist && abs(losAngle) <= pDudeInfo->periphery) {

		if ((PlayClock & 64) == 0 && Chance(0x3000) && !spriteIsUnderwater(actor, false))
			playGenDudeSound(actor, kGenDudeSndChasing);

		actor->dudeSlope = dist == 0 ? 0 : DivScale(target->int_pos().Z - actor->int_pos().Z, dist, 10);

		int curWeapon = actor->genDudeExtra.curWeapon;
		int weaponType = actor->genDudeExtra.weaponType;

		auto actLeech = leechIsDropped(actor);

		const VECTORDATA* meleeVector = &gVectorData[22];
		if (weaponType == kGenDudeWeaponThrow)
		{
			if (abs(losAngle) < kAng15)
			{
				if (!gThingInfoExtra[curWeapon - kThingBase].allowThrow)
				{
					if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
					else aiGenDudeNewState(actor, &genDudeChaseL);
					return;

				}
				else if (dist < 12264 && dist > 7680 && !spriteIsUnderwater(actor, false) && curWeapon != kModernThingEnemyLifeLeech)
				{
					int pHit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, 16777280, 0);
					switch (pHit) {
					case 0:
					case 4:
						return;
					default:
						aiGenDudeNewState(actor, &genDudeThrow);
						return;
					}

				}
				else if (dist > 4072 && dist <= 11072 && !spriteIsUnderwater(actor, false) && !actor->GetSpecialOwner())
				{
					switch (curWeapon)
					{
					case kModernThingEnemyLifeLeech:
					{
						if (actLeech == nullptr)
						{
							aiGenDudeNewState(actor, &genDudeThrow2);
							genDudeThrow2.nextState = &genDudeDodgeShortL;
							return;
						}

						int ldist = aiFightGetTargetDist(target, pDudeInfo, actLeech);
						if (ldist > 3 || !cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(),
							actLeech->int_pos().X, actLeech->int_pos().Y, actLeech->int_pos().Z, actLeech->sector()) || actLeech->GetTarget() == nullptr)
						{
							aiGenDudeNewState(actor, &genDudeThrow2);
							genDudeThrow2.nextState = &genDudeDodgeShortL;
						}
						else
						{
							genDudeThrow2.nextState = &genDudeChaseL;
							if (dist > 5072 && Chance(0x5000))
							{
								if (!canDuck(actor) || Chance(0x4000)) aiGenDudeNewState(actor, &genDudeDodgeShortL);
								else aiGenDudeNewState(actor, &genDudeDodgeShortD);
							}
							else
							{
								aiGenDudeNewState(actor, &genDudeChaseL);
							}

						}
					}
					return;
					case kModernThingThrowableRock:
						if (Chance(0x4000)) aiGenDudeNewState(actor, &genDudeThrow2);
						else playGenDudeSound(actor, kGenDudeSndTargetSpot);
						return;
					default:
						aiGenDudeNewState(actor, &genDudeThrow2);
						return;
					}

				}
				else if (dist <= meleeVector->maxDist)
				{

					if (spriteIsUnderwater(actor, false))
					{
						if (Chance(0x9000)) aiGenDudeNewState(actor, &genDudePunch);
						else aiGenDudeNewState(actor, &genDudeDodgeW);
					}
					else if (Chance(0x9000)) aiGenDudeNewState(actor, &genDudePunch);
					else aiGenDudeNewState(actor, &genDudeDodgeL);
					return;

				}
				else
				{
					int state = checkAttackState(actor);
					if (state == 1) aiGenDudeNewState(actor, &genDudeChaseW);
					else if (state == 2)
					{
						if (Chance(0x5000)) aiGenDudeNewState(actor, &genDudeChaseD);
						else aiGenDudeNewState(actor, &genDudeChaseL);
					}
					else  aiGenDudeNewState(actor, &genDudeChaseL);
					return;
				}
			}
		}
		else
		{
			int vdist; int mdist; int defDist;
			defDist = vdist = mdist = actor->genDudeExtra.fireDist;

			if (weaponType == kGenDudeWeaponHitscan)
			{
				if ((vdist = gVectorData[curWeapon].maxDist) <= 0)
					vdist = defDist;

			}
			else if (weaponType == kGenDudeWeaponSummon)
			{
				// don't attack slaves
				if (actor->GetTarget() != nullptr && actor->GetTarget()->GetOwner() == actor)
				{
					aiSetTarget(actor, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z);
					return;
				}
				else if (actor->genDudeExtra.slaveCount > gGameOptions.nDifficulty || dist < meleeVector->maxDist)
				{
					if (dist <= meleeVector->maxDist)
					{
						aiGenDudeNewState(actor, &genDudePunch);
						return;
					}
					else
					{
						int state = checkAttackState(actor);
						if (state == 1) aiGenDudeNewState(actor, &genDudeChaseW);
						else if (state == 2) aiGenDudeNewState(actor, &genDudeChaseD);
						else aiGenDudeNewState(actor, &genDudeChaseL);
						return;
					}
				}

			}
			else if (weaponType == kGenDudeWeaponMissile)
			{
				// special handling for flame, explosive and life leech missiles
				int state = checkAttackState(actor);
				switch (curWeapon)
				{
				case kMissileLifeLeechRegular:
					// pickup life leech if it was thrown previously
					if (actLeech != NULL) removeLeech(actLeech);
					mdist = 1500;
					break;
				case kMissileFlareAlt:
					mdist = 2500;
					[[fallthrough]];
				case kMissileFireball:
				case kMissileFireballNapalm:
				case kMissileFireballCerberus:
				case kMissileFireballTchernobog:
					if (mdist == defDist) mdist = 3000;
					if (dist > mdist || actor->xspr.locked == 1) break;
					else if (dist <= meleeVector->maxDist && Chance(0x9000))
						aiGenDudeNewState(actor, &genDudePunch);
					else if (state == 1) aiGenDudeNewState(actor, &genDudeChaseW);
					else if (state == 2) aiGenDudeNewState(actor, &genDudeChaseD);
					else aiGenDudeNewState(actor, &genDudeChaseL);
					return;
				case kMissileFlameSpray:
				case kMissileFlameHound:
					//viewSetSystemMessage("%d", target->xspr.burnTime);
					if (spriteIsUnderwater(actor, false))
					{
						if (dist > meleeVector->maxDist) aiGenDudeNewState(actor, &genDudeChaseW);
						else if (Chance(0x8000)) aiGenDudeNewState(actor, &genDudePunch);
						else aiGenDudeNewState(actor, &genDudeDodgeShortW);
						return;
					}
					else if (dist <= 4000 && target->xspr.burnTime >= 2000 && target->GetBurnSource() == actor)
					{
						if (dist > meleeVector->maxDist) aiGenDudeNewState(actor, &genDudeChaseL);
						else aiGenDudeNewState(actor, &genDudePunch);
						return;
					}
					vdist = 3500 + (gGameOptions.nDifficulty * 400);
					break;
				}
			}
			else if (weaponType == kGenDudeWeaponKamikaze)
			{
				int nType = curWeapon - kTrapExploder;
				const EXPLOSION* pExpl = &explodeInfo[nType];
				if (CheckProximity(actor, target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), pExpl->radius >> 1))
				{
					actor->vel.X = actor->vel.Y = actor->vel.Z = 0;
					if (doExplosion(actor, nType) && actor->xspr.health > 0)
						actDamageSprite(actor, actor, kDamageExplode, 65535);
				}
				return;
			}

			int state = checkAttackState(actor);
			int kAngle = (dudeIsMelee(actor) || dist <= kGenDudeMaxMeleeDist) ? pDudeInfo->periphery : kGenDudeKlabsAng;

			if (dist < vdist && abs(losAngle) < kAngle)
			{
				if (pExtra->canWalk)
				{
					int objDist = -1; int targetDist = -1; int hit = -1;
					if (weaponType == kGenDudeWeaponHitscan)
						hit = HitScan(actor, actor->int_pos().Z, bcos(actor->spr.ang), bsin(actor->spr.ang), actor->dudeSlope, CLIPMASK1, dist);
					else if (weaponType == kGenDudeWeaponMissile)
						hit = HitScan(actor, actor->int_pos().Z, bcos(actor->spr.ang), bsin(actor->spr.ang), actor->dudeSlope, CLIPMASK0, dist);

					if (hit >= 0)
					{
						targetDist = dist - (target->spr.clipdist << 2);
						objDist = approxDist(gHitInfo.hitpos.X - actor->int_pos().X, gHitInfo.hitpos.Y - actor->int_pos().Y);
					}

					if (actor != gHitInfo.actor() && targetDist > objDist)
					{
						DBloodActor* hitactor = nullptr;
						walltype* pHWall = NULL;
						XWALL* pXHWall = NULL;
						bool hscn = false;
						bool blck = false;
						bool failed = false;

						switch (hit)
						{
						case 3:
							hitactor = gHitInfo.actor();
							if (hitactor)
							{
								hscn = (hitactor->spr.cstat & CSTAT_SPRITE_BLOCK_HITSCAN); blck = (hitactor->spr.cstat & CSTAT_SPRITE_BLOCK);
							}
							break;
						case 0:
						case 4:
							pHWall = gHitInfo.hitWall;
							if (pHWall->hasX()) pXHWall = &pHWall->xw();
							hscn = (pHWall->cstat & CSTAT_WALL_BLOCK_HITSCAN); blck = (pHWall->cstat & CSTAT_WALL_BLOCK);
							break;
						}

						switch (hit) {
						case 0:
						case 1:
						case 2:
							if (weaponType != kGenDudeWeaponMissile && genDudeAdjustSlope(actor, dist, weaponType)
								&& dist < (int)(6000 + Random(2000)) && pExtra->baseDispersion < kGenDudeMaxDispersion >> 1) break;

							else if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
							else aiGenDudeNewState(actor, &genDudeChaseL);
							return;
						case 3:
							if (hitactor->spr.statnum == kStatFX || hitactor->spr.statnum == kStatProjectile || hitactor->spr.statnum == kStatDebris)
								break;
							if (hitactor->IsDudeActor() && (weaponType != kGenDudeWeaponHitscan || hscn))
							{
								// dodge a bit in sides
								if (hitactor->xspr.health > 0 && hitactor->GetTarget() != actor)
								{
									if (pExtra->baseDispersion < 1024 && weaponType != kGenDudeWeaponMissile)
									{
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeDodgeShorterW);
										else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(actor, &genDudeDodgeShorterD);
										else aiGenDudeNewState(actor, &genDudeDodgeShorterL);
									}
									else if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeDodgeShortW);
									else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(actor, &genDudeDodgeShortD);
									else aiGenDudeNewState(actor, &genDudeDodgeShortL);

									switch (hitactor->spr.type)
									{
									case kDudeModernCustom: // and make dude which could be hit to dodge too
										if (!dudeIsMelee(hitactor) && Chance(dist << 4))
										{
											if (!inAttack(hitactor->xspr.aiState))
											{
												if (spriteIsUnderwater(hitactor)) aiGenDudeNewState(hitactor, &genDudeDodgeShorterW);
												else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(hitactor, &genDudeDodgeShorterD);
												else aiGenDudeNewState(hitactor, &genDudeDodgeShorterL);

												// preferable in opposite sides
												if (Chance(0x8000))
												{
													if (actor->xspr.dodgeDir == 1) hitactor->xspr.dodgeDir = -1;
													else if (actor->xspr.dodgeDir == -1) hitactor->xspr.dodgeDir = 1;
												}
												break;
											}
											if (actor->int_pos().X < hitactor->int_pos().X)
											{
												if (Chance(0x9000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = -1;
												else actor->xspr.dodgeDir = 1;
											}
											else
											{
												if (Chance(0x9000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = 1;
												else actor->xspr.dodgeDir = -1;
											}
										}
										break;
									default:
										if (actor->int_pos().X < hitactor->int_pos().X)
										{
											if (Chance(0x9000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = -1;
											else actor->xspr.dodgeDir = 1;
										}
										else
										{
											if (Chance(0x9000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = 1;
											else actor->xspr.dodgeDir = -1;
										}
										break;
									}
									return;
								}
								break;
							}
							else if (weaponType == kGenDudeWeaponHitscan && hscn)
							{
								if (genDudeAdjustSlope(actor, dist, weaponType)) break;
								VectorScan(actor, 0, 0, bcos(actor->spr.ang), bsin(actor->spr.ang), actor->dudeSlope, dist, 1);
								if (actor == gHitInfo.actor()) break;

								bool immune = nnExtIsImmune(hitactor, gVectorData[curWeapon].dmgType);
								if (!(hitactor->hasX() && (!immune || (immune && hitactor->spr.statnum == kStatThing && hitactor->xspr.Vector)) && !hitactor->xspr.locked))
								{
									if ((approxDist(gHitInfo.hitpos.X - actor->int_pos().X, gHitInfo.hitpos.Y - actor->int_pos().Y) <= 1500 && !blck)
										|| (dist <= (int)(pExtra->fireDist / ClipLow(Random(4), 1))))
									{
										//viewSetSystemMessage("GO CHASE");
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
										else aiGenDudeNewState(actor, &genDudeChaseL);
										return;

									}

									int wd1 = picWidth(hitactor->spr.picnum, hitactor->spr.xrepeat);
									int wd2 = picWidth(actor->spr.picnum, actor->spr.xrepeat);
									if (wd1 < (wd2 << 3))
									{
										//viewSetSystemMessage("OBJ SIZE: %d   DUDE SIZE: %d", wd1, wd2);
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeDodgeShorterW);
										else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(actor, &genDudeDodgeShorterD);
										else aiGenDudeNewState(actor, &genDudeDodgeShorterL);

										if (actor->int_pos().X < hitactor->int_pos().X)
										{
											if (Chance(0x3000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = -1;
											else actor->xspr.dodgeDir = 1;
										}
										else
										{
											if (Chance(0x3000) && target->int_pos().X > hitactor->int_pos().X) actor->xspr.dodgeDir = 1;
											else actor->xspr.dodgeDir = -1;
										}

										if (actor->hit.hit.type == kHitWall || actor->hit.hit.type == kHitSprite)
										{
											if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
											else aiGenDudeNewState(actor, &genDudeChaseL);
											actor->xspr.goalAng = Random(kAng360);
											//viewSetSystemMessage("WALL OR SPRITE TOUCH");
										}

									}
									else
									{
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
										else aiGenDudeNewState(actor, &genDudeChaseL);
										//viewSetSystemMessage("TOO BIG OBJECT TO DODGE!!!!!!!!");
									}
									return;
								}
								break;
							}
							[[fallthrough]];
						case 4:
							if (hit == 4 && weaponType == kGenDudeWeaponHitscan && hscn)
							{
								bool masked = (pHWall->cstat & CSTAT_WALL_MASKED);
								if (masked) VectorScan(actor, 0, 0, bcos(actor->spr.ang), bsin(actor->spr.ang), actor->dudeSlope, dist, 1);

								if ((actor != gHitInfo.actor()) && (pHWall->type != kWallGib || !masked || pXHWall == NULL || !pXHWall->triggerVector || pXHWall->locked))
								{
									if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
									else aiGenDudeNewState(actor, &genDudeChaseL);
									return;
								}
							}
							else if (hit >= 3 && weaponType == kGenDudeWeaponMissile && blck)
							{
								switch (curWeapon) {
								case kMissileLifeLeechRegular:
								case kMissileTeslaAlt:
								case kMissileFlareAlt:
								case kMissileFireball:
								case kMissileFireballNapalm:
								case kMissileFireballCerberus:
								case kMissileFireballTchernobog:
								{
									// allow attack if dude is far from object, but target is close to it
									int dudeDist = approxDist(gHitInfo.hitpos.X - actor->int_pos().X, gHitInfo.hitpos.Y - actor->int_pos().Y);
									int targetDist1 = approxDist(gHitInfo.hitpos.X - target->int_pos().X, gHitInfo.hitpos.Y - target->int_pos().Y);
									if (dudeDist < mdist)
									{
										//viewSetSystemMessage("DUDE CLOSE TO OBJ: %d, MDIST: %d", dudeDist, mdist);
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeChaseW);
										else aiGenDudeNewState(actor, &genDudeChaseL);
										return;
									}
									else if (targetDist1 <= mdist >> 1)
									{
										//viewSetSystemMessage("TARGET CLOSE TO OBJ: %d, MDIST: %d", targetDist, mdist >> 1);
										break;
									}
									[[fallthrough]];
								}
								default:
									//viewSetSystemMessage("DEF HIT: %d, MDIST: %d", hit, mdist);
									if (hit == 4) failed = (pHWall->type != kWallGib || pXHWall == NULL || !pXHWall->triggerVector || pXHWall->locked);
									else if (hit == 3 && (failed = (hitactor->spr.statnum != kStatThing || !hitactor->hasX() || hitactor->xspr.locked)) == false)
									{
										// check also for damage resistance (all possible damages missile can use)
										for (int i = 0; i < kDmgMax; i++)
										{
											if (gMissileInfoExtra[curWeapon - kMissileBase].dmgType[i] && (failed = nnExtIsImmune(hitactor, i)) == false)
												break;
										}
									}

									if (failed)
									{
										if (spriteIsUnderwater(actor)) aiGenDudeNewState(actor, &genDudeSearchW);
										else aiGenDudeNewState(actor, &genDudeSearchL);
										return;
									}
									break;
								}
							}
							break;
						}
					}
				}

				aiSetTarget(actor, actor->GetTarget());
				switch (state)
				{
				case 1:
					aiGenDudeNewState(actor, &genDudeFireW);
					actor->xspr.aiState->nextState = &genDudeFireW;
					break;
				case 2:
					aiGenDudeNewState(actor, &genDudeFireD);
					actor->xspr.aiState->nextState = &genDudeFireD;
					break;
				default:
					aiGenDudeNewState(actor, &genDudeFireL);
					actor->xspr.aiState->nextState = &genDudeFireL;
					break;
				}
			}
			else
			{
				if (seqGetID(actor) == actor->xspr.data2 + ((state < 3) ? 8 : 6))
				{
					if (state == 1) actor->xspr.aiState->nextState = &genDudeChaseW;
					else if (state == 2) actor->xspr.aiState->nextState = &genDudeChaseD;
					else actor->xspr.aiState->nextState = &genDudeChaseL;

				}
				else if (state == 1 && actor->xspr.aiState != &genDudeChaseW && actor->xspr.aiState != &genDudeFireW)
				{
					aiGenDudeNewState(actor, &genDudeChaseW);
					actor->xspr.aiState->nextState = &genDudeFireW;

				}
				else if (state == 2 && actor->xspr.aiState != &genDudeChaseD && actor->xspr.aiState != &genDudeFireD)
				{
					aiGenDudeNewState(actor, &genDudeChaseD);
					actor->xspr.aiState->nextState = &genDudeFireD;

				}
				else if (actor->xspr.aiState != &genDudeChaseL && actor->xspr.aiState != &genDudeFireL)
				{
					aiGenDudeNewState(actor, &genDudeChaseL);
					actor->xspr.aiState->nextState = &genDudeFireL;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int checkAttackState(DBloodActor* actor)
{
	if (dudeIsPlayingSeq(actor, 14) || spriteIsUnderwater(actor, false))
	{
		if (!dudeIsPlayingSeq(actor, 14) || spriteIsUnderwater(actor, false))
		{
			if (spriteIsUnderwater(actor, false))
			{
				return 1; //water
			}
		}
		else
		{
			return 2; //duck
		}
	}
	else
	{
		return 3; //land
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int getGenDudeMoveSpeed(DBloodActor* actor, int which, bool mul, bool shift)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int speed = -1; int step = 2500; int maxSpeed = 146603;
	switch (which)
	{
	case 0:
		speed = pDudeInfo->frontSpeed;
		break;
	case 1:
		speed = pDudeInfo->sideSpeed;
		break;
	case 2:
		speed = pDudeInfo->backSpeed;
		break;
	case 3:
		speed = pDudeInfo->angSpeed;
		break;
	default:
		return -1;
	}
	if (actor->xspr.busyTime > 0) speed /= 3;
	if (speed > 0 && mul)
	{
		if (actor->xspr.busyTime > 0)
			speed += (step * actor->xspr.busyTime);
	}

	if (shift) speed *= 4 >> 4;
	if (speed > maxSpeed) speed = maxSpeed;

	return speed;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiGenDudeMoveForward(DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	int maxTurn = pDudeInfo->angSpeed * 4 >> 4;

	if (pExtra->canFly)
	{
		int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
		int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
		actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
		int nAccel = pDudeInfo->frontSpeed << 2;
		if (abs(nAng) > 341)
			return;
		if (actor->GetTarget() == nullptr)
			actor->spr.ang = (actor->spr.ang + 256) & 2047;
		int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
		int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
		int nDist = approxDist(dx, dy);
		if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
			return;
		int nCos = Cos(actor->spr.ang);
		int nSin = Sin(actor->spr.ang);
		int vx = actor->vel.X;
		int vy = actor->vel.Y;
		int t1 = DMulScale(vx, nCos, vy, nSin, 30);
		int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel >> 1;
		actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
		actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	}
	else
	{
		int dang = ((kAng180 + actor->xspr.goalAng - actor->spr.ang) & 2047) - kAng180;
		actor->spr.ang = ((actor->spr.ang + ClipRange(dang, -maxTurn, maxTurn)) & 2047);

		// don't move forward if trying to turn around
		if (abs(dang) > kAng60)
			return;

		int sin = Sin(actor->spr.ang);
		int cos = Cos(actor->spr.ang);

		int frontSpeed = actor->genDudeExtra.moveSpeed;
		actor->vel.X += MulScale(cos, frontSpeed, 30);
		actor->vel.Y += MulScale(sin, frontSpeed, 30);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiGenDudeChooseDirection(DBloodActor* actor, int a3, int xvel, int yvel)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax))
	{
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}

	// TO-DO: Take in account if sprite is flip-x, so enemy select correct angle

	int vc = ((a3 + 1024 - actor->spr.ang) & 2047) - 1024;
	int t1 = DMulScale(xvel, Cos(actor->spr.ang), yvel, Sin(actor->spr.ang), 30);
	int vsi = ((t1 * 15) >> 12) / 2; int v8 = (vc >= 0) ? 341 : -341;

	if (CanMove(actor, actor->GetTarget(), actor->spr.ang + vc, vsi))
		actor->xspr.goalAng = actor->spr.ang + vc;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.ang + vc / 2, vsi))
		actor->xspr.goalAng = actor->spr.ang + vc / 2;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.ang - vc / 2, vsi))
		actor->xspr.goalAng = actor->spr.ang - vc / 2;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.ang + v8, vsi))
		actor->xspr.goalAng = actor->spr.ang + v8;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.ang, vsi))
		actor->xspr.goalAng = actor->spr.ang;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.ang - v8, vsi))
		actor->xspr.goalAng = actor->spr.ang - v8;
	else
		actor->xspr.goalAng = actor->spr.ang + 341;

	actor->xspr.dodgeDir = (Chance(0x8000)) ? 1 : -1;

	if (!CanMove(actor, actor->GetTarget(), actor->spr.ang + actor->xspr.dodgeDir * 512, 512))
	{
		actor->xspr.dodgeDir = -actor->xspr.dodgeDir;
		if (!CanMove(actor, actor->GetTarget(), actor->spr.ang + actor->xspr.dodgeDir * 512, 512))
			actor->xspr.dodgeDir = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiGenDudeNewState(DBloodActor* actor, AISTATE* pAIState)
{
	if (!actor->hasX())
	{
		return;
	}
	
	if (actor->xspr.health <= 0 || actor->xspr.sysData1 == kGenDudeTransformStatus)
		return;


	// redirect dudes which cannot walk to non-walk states
	if (!actor->genDudeExtra.canWalk)
	{

		if (pAIState == &genDudeDodgeL || pAIState == &genDudeDodgeShortL || pAIState == &genDudeDodgeShorterL)
			pAIState = &genDudeRecoilL;

		else if (pAIState == &genDudeDodgeD || pAIState == &genDudeDodgeShortD || pAIState == &genDudeDodgeShorterD)
			pAIState = &genDudeRecoilD;

		else if (pAIState == &genDudeDodgeW || pAIState == &genDudeDodgeShortW || pAIState == &genDudeDodgeShorterW)
			pAIState = &genDudeRecoilW;

		else if (pAIState == &genDudeSearchL || pAIState == &genDudeSearchShortL)
			pAIState = &genDudeSearchNoWalkL;

		else if (pAIState == &genDudeSearchW || pAIState == &genDudeSearchShortW)
			pAIState = &genDudeSearchNoWalkW;

		else if (pAIState == &genDudeGotoL) pAIState = &genDudeIdleL;
		else if (pAIState == &genDudeGotoW) pAIState = &genDudeIdleW;
		else if (pAIState == &genDudeChaseL) pAIState = &genDudeChaseNoWalkL;
		else if (pAIState == &genDudeChaseD) pAIState = &genDudeChaseNoWalkD;
		else if (pAIState == &genDudeChaseW) pAIState = &genDudeChaseNoWalkW;
		else if (pAIState == &genDudeRecoilTesla) {

			if (spriteIsUnderwater(actor, false)) pAIState = &genDudeRecoilW;
			else pAIState = &genDudeRecoilL;

		}

	}

	if (!actor->genDudeExtra.canRecoil)
	{
		if (pAIState == &genDudeRecoilL || pAIState == &genDudeRecoilD) pAIState = &genDudeIdleL;
		else if (pAIState == &genDudeRecoilW) pAIState = &genDudeIdleW;
	}

	actor->xspr.stateTimer = pAIState->stateTicks; actor->xspr.aiState = pAIState;

	int stateSeq = actor->xspr.data2 + pAIState->seqId;
	if (pAIState->seqId >= 0 && getSequence(stateSeq))
	{
		seqSpawn(stateSeq, actor, pAIState->funcId);
	}

	if (pAIState->enterFunc)
		pAIState->enterFunc(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool playGenDudeSound(DBloodActor* actor, int mode)
{
	if (mode < kGenDudeSndTargetSpot || mode >= kGenDudeSndMax) return false;
	const GENDUDESND* sndInfo = &gCustomDudeSnd[mode]; bool gotSnd = false;
	int sndStartId = actor->xspr.sysData1;
	int rand = sndInfo->randomRange;
	int sndId = (sndStartId <= 0) ? sndInfo->defaultSndId : sndStartId + sndInfo->sndIdOffset;
	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;

	// let's check if there same sounds already played by other dudes
	// so we won't get a lot of annoying screams in the same time and ensure sound played in it's full length (if not interruptable)
	if (pExtra->sndPlaying && !sndInfo->interruptable) {
#if 0
		for (int i = 0; i < 256; i++) {
			if (Bonkle[i].atc <= 0) continue;
			for (int a = 0; a < rand; a++) {
				if (sndId + a == Bonkle[i].atc) {
					if (Bonkle[i].at0 <= 0) {
						pExtra->sndPlaying = false;
						break;
					}
					return true;
				}
			}
		}
#endif

		pExtra->sndPlaying = false;

	}

	if (sndId < 0) return false;
	else if (sndStartId <= 0) { sndId += Random(rand); gotSnd = true; }
	else
	{
		// Let's try to get random snd
		int maxRetries = 5;
		while (maxRetries-- > 0) {
			int random = Random(rand);
			if (!soundEngine->FindSoundByResID(sndId + random)) continue;
			sndId = sndId + random;
			gotSnd = true;
			break;
		}

		// If no success in getting random snd, get first existing one
		if (gotSnd == false)
		{
			int maxSndId = sndId + rand;
			while (sndId++ < maxSndId)
			{
				if (!soundEngine->FindSoundByResID(sndId)) continue;
				gotSnd = true;
				break;
			}
		}

	}

	if (gotSnd == false) return false;
	else if (sndInfo->aiPlaySound) aiPlay3DSound(actor, sndId, AI_SFX_PRIORITY_2, -1);
	else sfxPlay3DSound(actor, sndId, -1, 0);

	pExtra->sndPlaying = true;
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool spriteIsUnderwater(DBloodActor* actor, bool oldWay)
{
	return (IsUnderwaterSector(actor->sector())
		|| (oldWay && (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo)));
}

DBloodActor* leechIsDropped(DBloodActor* actor)
{
	return actor->genDudeExtra.pLifeLeech;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void removeDudeStuff(DBloodActor* actor)
{
	BloodStatIterator it(kStatThing);
	while (auto actor2 = it.Next())
	{
		if (actor2->GetOwner() != actor) continue;
		switch (actor2->spr.type) {
		case kThingArmedProxBomb:
		case kThingArmedRemoteBomb:
		case kModernThingTNTProx:
			actor2->spr.type = kSpriteDecoration;
			actPostSprite(actor2, kStatFree);
			break;
		case kModernThingEnemyLifeLeech:
			killDudeLeech(actor2);
			break;
		}
	}

	it.Reset(kStatDude);
	while (auto actor2 = it.Next())
	{
		if (actor2->GetOwner() != actor) continue;
		actDamageSprite(actor2->GetOwner(), actor2, kDamageFall, 65535);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void removeLeech(DBloodActor* actLeech, bool delSprite)
{
	if (actLeech != nullptr)
	{
		auto effectactor = gFX.fxSpawnActor((FX_ID)52, actLeech->sector(), actLeech->int_pos().X, actLeech->int_pos().Y, actLeech->int_pos().Z, actLeech->spr.ang);
		if (effectactor != nullptr)
		{
			effectactor->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
			effectactor->spr.pal = 6;
			int repeat = 64 + Random(50);
			effectactor->spr.xrepeat = repeat;
			effectactor->spr.yrepeat = repeat;
		}

		sfxPlay3DSoundCP(actLeech, 490, -1, 0, 60000);

		if (actLeech->GetOwner())
			actLeech->GetOwner()->genDudeExtra.pLifeLeech = nullptr;

		if (delSprite)
		{
			actLeech->spr.type = kSpriteDecoration;
			actPostSprite(actLeech, kStatFree);
		}


	}
}


void killDudeLeech(DBloodActor* actLeech)
{
	if (actLeech != NULL)
	{
		actDamageSprite(actLeech->GetOwner(), actLeech, kDamageExplode, 65535);
		sfxPlay3DSoundCP(actLeech, 522, -1, 0, 60000);

		if (actLeech->GetOwner() != nullptr)
			actLeech->GetOwner()->genDudeExtra.pLifeLeech = nullptr;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* getNextIncarnation(DBloodActor* actor)
{
	for (int i = bucketHead[actor->xspr.txID]; i < bucketHead[actor->xspr.txID + 1]; i++)
	{
		if (!rxBucket[i].isActor()) continue;
		auto rxactor = rxBucket[i].actor();
		if (actor != rxactor && rxactor->spr.statnum == kStatInactive) return rxactor;
	}
	return nullptr;
}

bool dudeIsMelee(DBloodActor* actor)
{
	return actor->genDudeExtra.isMelee;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void scaleDamage(DBloodActor* actor)
{
	int curWeapon = actor->genDudeExtra.curWeapon;
	int weaponType = actor->genDudeExtra.weaponType;
	signed short* curScale = actor->genDudeExtra.dmgControl;
	for (int i = 0; i < kDmgMax; i++)
		curScale[i] = getDudeInfo(kDudeModernCustom)->startDamage[i];

	switch (weaponType) {
		// just copy damage resistance of dude that should be summoned
	case kGenDudeWeaponSummon:
		for (int i = 0; i < kDmgMax; i++)
			curScale[i] = getDudeInfo(curWeapon)->startDamage[i];
		break;
		// these does not like the explosions and burning
	case kGenDudeWeaponKamikaze:
		curScale[kDmgBurn] = curScale[kDmgExplode] = curScale[kDmgElectric] = 1024;
		break;
	case kGenDudeWeaponMissile:
	case kGenDudeWeaponThrow:
		switch (curWeapon)
		{
		case kMissileButcherKnife:
			curScale[kDmgBullet] = 100;
			[[fallthrough]];
		case kMissileEctoSkull:
			curScale[kDmgSpirit] = 32;
			break;
		case kMissileLifeLeechAltNormal:
		case kMissileLifeLeechAltSmall:
		case kMissileArcGargoyle:
			curScale[kDmgSpirit] -= 32;
			curScale[kDmgElectric] = 52;
			break;
		case kMissileFlareRegular:
		case kMissileFlareAlt:
		case kMissileFlameSpray:
		case kMissileFlameHound:
		case kThingArmedSpray:
		case kThingPodFireBall:
		case kThingNapalmBall:
			curScale[kDmgBurn] = 32;
			break;
		case kMissileLifeLeechRegular:
			curScale[kDmgBurn] = 60 + Random(4);
			[[fallthrough]];
		case kThingDroppedLifeLeech:
		case kModernThingEnemyLifeLeech:
			curScale[kDmgSpirit] = 32 + Random(18);
			break;
		case kMissileFireball:
		case kMissileFireballNapalm:
		case kMissileFireballCerberus:
		case kMissileFireballTchernobog:
			curScale[kDmgBurn] = 50;
			curScale[kDmgExplode] -= 32;
			curScale[kDmgFall] = 65 + Random(15);
			break;
		case kThingTNTBarrel:
		case kThingArmedProxBomb:
		case kThingArmedRemoteBomb:
		case kThingArmedTNTBundle:
		case kThingArmedTNTStick:
		case kModernThingTNTProx:
			curScale[kDmgBurn] -= 32;
			curScale[kDmgExplode] -= 32;
			curScale[kDmgFall] = 65 + Random(15);
			break;
		case kMissileTeslaAlt:
		case kMissileTeslaRegular:
			curScale[kDmgElectric] = 32 + Random(8);
			break;
		}
		break;

	}

	// add resistance if have an armor item to drop
	if (actor->xspr.dropMsg >= kItemArmorAsbest && actor->xspr.dropMsg <= kItemArmorSuper)
	{
		switch (actor->xspr.dropMsg)
		{
		case kItemArmorAsbest:
			curScale[kDmgBurn] = 0;
			curScale[kDmgExplode] -= 30;
			break;
		case kItemArmorBasic:
			curScale[kDmgBurn] -= 15;
			curScale[kDmgExplode] -= 15;
			curScale[kDmgBullet] -= 15;
			curScale[kDmgSpirit] -= 15;
			break;
		case kItemArmorBody:
			curScale[kDmgBullet] -= 30;
			break;
		case kItemArmorFire:
			curScale[kDmgBurn] -= 30;
			curScale[kDmgExplode] -= 30;
			break;
		case kItemArmorSpirit:
			curScale[kDmgSpirit] -= 30;
			break;
		case kItemArmorSuper:
			curScale[kDmgBurn] -= 60;
			curScale[kDmgExplode] -= 60;
			curScale[kDmgBullet] -= 60;
			curScale[kDmgSpirit] -= 60;
			break;
		}
	}

	// take in account yrepeat of sprite
	int yrepeat = actor->spr.yrepeat;
	if (yrepeat < 64)
	{
		for (int i = 0; i < kDmgMax; i++) curScale[i] += (64 - yrepeat);
	}
	else if (yrepeat > 64)
	{
		for (int i = 0; i < kDmgMax; i++) curScale[i] -= ((yrepeat - 64) >> 2);
	}

	// take surface type into account
	int surfType = tileGetSurfType(actor->spr.picnum);
	switch (surfType)
	{
	case 1:  // stone
		curScale[kDmgFall] = 0;
		curScale[kDmgBullet] -= 200;
		curScale[kDmgBurn] -= 100;
		curScale[kDmgExplode] -= 80;
		curScale[kDmgChoke] += 30;
		curScale[kDmgElectric] += 20;
		break;
	case 2:  // metal
		curScale[kDmgFall] = 16;
		curScale[kDmgBullet] -= 128;
		curScale[kDmgBurn] -= 90;
		curScale[kDmgExplode] -= 55;
		curScale[kDmgChoke] += 20;
		curScale[kDmgElectric] += 30;
		break;
	case 3:  // wood 
		curScale[kDmgBullet] -= 10;
		curScale[kDmgBurn] += 50;
		curScale[kDmgExplode] += 40;
		curScale[kDmgChoke] += 10;
		curScale[kDmgElectric] -= 60;
		break;
	case 5:  // water
	case 6:  // dirt
	case 7:  // clay
	case 13: // goo
		curScale[kDmgFall] = 8;
		curScale[kDmgBullet] -= 20;
		curScale[kDmgBurn] -= 200;
		curScale[kDmgExplode] -= 60;
		curScale[kDmgChoke] = 0;
		curScale[kDmgElectric] += 40;
		break;
	case 8:  // snow
	case 9:  // ice
		curScale[kDmgFall] = 8;
		curScale[kDmgBullet] -= 20;
		curScale[kDmgBurn] -= 100;
		curScale[kDmgExplode] -= 50;
		curScale[kDmgChoke] = 0;
		curScale[kDmgElectric] += 40;
		break;
	case 10: // leaves
	case 12: // plant
		curScale[kDmgFall] = 0;
		curScale[kDmgBullet] -= 10;
		curScale[kDmgBurn] += 70;
		curScale[kDmgExplode] += 50;
		break;
	case 11: // cloth
		curScale[kDmgFall] = 8;
		curScale[kDmgBullet] -= 10;
		curScale[kDmgBurn] += 30;
		curScale[kDmgExplode] += 20;
		break;
	case 14: // lava
		curScale[kDmgBurn] = 0;
		curScale[kDmgExplode] = 0;
		curScale[kDmgChoke] += 30;
		break;
	}

	// finally, scale dmg for difficulty
	for (int i = 0; i < kDmgMax; i++)
		curScale[i] = MulScale(DudeDifficulty[gGameOptions.nDifficulty], ClipLow(curScale[i], 1), 8);

	//short* dc = curScale;
	//if (actor->xspr.rxID == 788)
		//viewSetSystemMessage("0: %d, 1: %d, 2: %d, 3: %d, 4: %d, 5: %d, 6: %d", dc[0], dc[1], dc[2], dc[3], dc[4], dc[5], dc[6]);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int getDispersionModifier(DBloodActor* actor, int minDisp, int maxDisp)
{
	// the faster fire rate, the less frames = more dispersion
	Seq* pSeq = getSequence(actor->xspr.data2 + 6);
	int disp = 1;
	if (pSeq != nullptr)
	{
		int nFrames = pSeq->nFrames; int ticks = pSeq->ticksPerFrame; int shots = 0;
		for (int i = 0; i <= pSeq->nFrames; i++) {
			if (pSeq->frames[i].trigger) shots++;
		}

		disp = (((shots * 1000) / nFrames) / ticks) * 20;
		if (gGameOptions.nDifficulty > 0)
			disp /= gGameOptions.nDifficulty;

		//viewSetSystemMessage("DISP: %d FRAMES: %d SHOTS: %d TICKS %d", disp, nFrames, shots, ticks);

	}

	return ClipRange(disp, minDisp, maxDisp);
}

//---------------------------------------------------------------------------
//
// the distance counts from sprite size
//
//---------------------------------------------------------------------------

static int getRangeAttackDist(DBloodActor* actor, int minDist, int maxDist)
{
	int yrepeat = actor->spr.yrepeat;
	int dist = 0;
	int seqId = actor->xspr.data2;
	int mul = 550;
	int picnum = actor->spr.picnum;

	if (yrepeat > 0)
	{
		if (seqId >= 0)
		{
			Seq* pSeq = getSequence(seqId);
			if (pSeq)
			{
				picnum = seqGetTile(&pSeq->frames[0]);
			}
		}

		dist = tileHeight(picnum) << 8;
		if (yrepeat < 64) dist -= (64 - yrepeat) * mul;
		else if (yrepeat > 64) dist += (yrepeat - 64) * (mul / 3);
	}

	dist = ClipRange(dist, minDist, maxDist);
	//viewSetSystemMessage("DIST: %d, SPRHEIGHT: %d: YREPEAT: %d PIC: %d", dist, tileHeight(actor->spr.picnum), yrepeat, picnum);
	return dist;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getBaseChanceModifier(int baseChance)
{
	return ((gGameOptions.nDifficulty > 0) ? baseChance - (0x0500 * gGameOptions.nDifficulty) : baseChance);
}

int getRecoilChance(DBloodActor* actor)
{
	int mass = getSpriteMassBySize(actor);
	int baseChance = (!dudeIsMelee(actor) ? 0x8000 : 0x4000);
	baseChance = getBaseChanceModifier(baseChance) + actor->xspr.data3;

	int chance = ((baseChance / mass) << 7);
	return chance;
}

int getDodgeChance(DBloodActor* actor)
{
	int mass = getSpriteMassBySize(actor);
	int baseChance = (!dudeIsMelee(actor) ? 0x6000 : 0x1000);
	baseChance = getBaseChanceModifier(baseChance) + actor->xspr.data3;

	int chance = ((baseChance / mass) << 7);
	return chance;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void dudeLeechOperate(DBloodActor* actor, const EVENT& event)
{
	if (event.cmd == kCmdOff)
	{
		actPostSprite(actor, kStatFree);
		return;
	}

	auto actTarget = actor->GetTarget();
	if (actTarget != nullptr && actTarget != actor->GetOwner())
	{
		if (actTarget->spr.statnum == kStatDude && !(actTarget->spr.flags & 32) && actTarget->hasX() && !actor->xspr.stateTimer)
		{
			if (actTarget->IsPlayerActor())
			{
				PLAYER* pPlayer = &gPlayer[actTarget->spr.type - kDudePlayer1];
				if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0) return;
			}
			int top, bottom;
			GetActorExtents(actor, &top, &bottom);
			int nType = actTarget->spr.type - kDudeBase;
			DUDEINFO* pDudeInfo = &dudeInfo[nType];
			int z1 = (top - actor->int_pos().Z) - 256;
			int x = actTarget->int_pos().X; int y = actTarget->int_pos().Y; int z = actTarget->int_pos().Z;
			int nDist = approxDist(x - actor->int_pos().X, y - actor->int_pos().Y);

			if (nDist != 0 && cansee(actor->int_pos().X, actor->int_pos().Y, top, actor->sector(), x, y, z, actTarget->sector()))
			{
				int t = DivScale(nDist, 0x1aaaaa, 12);
				x += (actTarget->vel.X * t) >> 12;
				y += (actTarget->vel.Y * t) >> 12;
				int angBak = actor->spr.ang;
				actor->spr.ang = getangle(x - actor->int_pos().X, y - actor->int_pos().Y);
				int dx = bcos(actor->spr.ang);
				int dy = bsin(actor->spr.ang);
				int tz = actTarget->int_pos().Z - (actTarget->spr.yrepeat * pDudeInfo->aimHeight) * 4;
				int dz = DivScale(tz - top - 256, nDist, 10);
				int nMissileType = kMissileLifeLeechAltNormal + (actor->xspr.data3 ? 1 : 0);
				int t2;

				if (!actor->xspr.data3) t2 = 120 / 10;
				else t2 = (3 * 120) / 10;

				auto missile = actFireMissile(actor, 0, z1, dx, dy, dz, nMissileType);
				if (missile)
				{
					missile->SetOwner(actor);
					actor->xspr.stateTimer = 1;
					evPostActor(actor, t2, kCallbackLeechStateTimer);
					actor->xspr.data3 = ClipLow(actor->xspr.data3 - 1, 0);
				}
				actor->spr.ang = angBak;
			}
		}

	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool doExplosion(DBloodActor* actor, int nType)
{
	auto actExplosion = actSpawnSprite(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, kStatExplosion, true);
	if (!actExplosion->hasX())
		return false;

	int nSeq = 4; int nSnd = 304; const EXPLOSION* pExpl = &explodeInfo[nType];

	actExplosion->spr.type = nType;
	actExplosion->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	actExplosion->SetOwner(actor);
	actExplosion->spr.shade = -127;

	actExplosion->spr.yrepeat = actExplosion->spr.xrepeat = pExpl->repeat;

	actExplosion->xspr.data1 = pExpl->ticks;
	actExplosion->xspr.data2 = pExpl->quakeEffect;
	actExplosion->xspr.data3 = pExpl->flashEffect;

	if (nType == 0) { nSeq = 3; nSnd = 303; }
	else if (nType == 2) { nSeq = 4; nSnd = 305; }
	else if (nType == 3) { nSeq = 9; nSnd = 307; }
	else if (nType == 4) { nSeq = 5; nSnd = 307; }
	else if (nType <= 6) { nSeq = 4; nSnd = 303; }
	else if (nType == 7) { nSeq = 4; nSnd = 303; }

	seqSpawn(nSeq, actExplosion, -1);
	sfxPlay3DSound(actExplosion, nSnd, -1, 0);

	return true;
}

//---------------------------------------------------------------------------
//
// this function allows to spawn new custom dude and inherit spawner settings,
// so custom dude can have different weapons, hp and so on...
//
//---------------------------------------------------------------------------

DBloodActor* genDudeSpawn(DBloodActor* source, DBloodActor* actor, int nDist)
{
	auto spawned = actSpawnSprite(actor, kStatDude);
	int x, y, z = actor->int_pos().Z, nAngle = actor->spr.ang, nType = kDudeModernCustom;

	if (nDist > 0)
	{

		x = actor->int_pos().X + mulscale30r(Cos(nAngle), nDist);
		y = actor->int_pos().Y + mulscale30r(Sin(nAngle), nDist);
	}
	else
	{

		x = actor->int_pos().X;
		y = actor->int_pos().Y;

	}

	spawned->spr.type = nType; spawned->spr.ang = nAngle;
	vec3_t pos = { x, y, z };
	SetActor(spawned, &pos);
	spawned->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_BLOOD_BIT1;
	spawned->spr.clipdist = dudeInfo[nType - kDudeBase].clipdist;

	// inherit weapon, seq and sound settings.
	spawned->xspr.data1 = source->xspr.data1;
	spawned->xspr.data2 = source->xspr.data2;
	spawned->xspr.sysData1 = source->xspr.data3; // move sndStartId from data3 to sysData1
	spawned->xspr.data3 = 0;

	// spawn seq
	seqSpawn(genDudeSeqStartId(spawned), spawned, -1);

	// inherit movement speed.
	spawned->xspr.busyTime = source->xspr.busyTime;

	// inherit clipdist?
	if (source->spr.clipdist > 0)
		spawned->spr.clipdist = source->spr.clipdist;

	// inherit custom hp settings
	if (source->xspr.data4 <= 0) spawned->xspr.health = dudeInfo[nType - kDudeBase].startHealth << 4;
	else spawned->xspr.health = ClipRange(source->xspr.data4 << 4, 1, 65535);


	if (source->spr.flags & kModernTypeFlag1)
	{
		switch (source->spr.type) {
		case kModernCustomDudeSpawn:
			//inherit pal?
			if (spawned->spr.pal <= 0) spawned->spr.pal = source->spr.pal;

			// inherit spawn sprite trigger settings, so designer can count monsters.
			spawned->xspr.txID = source->xspr.txID;
			spawned->xspr.command = source->xspr.command;
			spawned->xspr.triggerOn = source->xspr.triggerOn;
			spawned->xspr.triggerOff = source->xspr.triggerOff;

			// inherit drop items
			spawned->xspr.dropMsg = source->xspr.dropMsg;

			// inherit required key so it can be dropped
			spawned->xspr.key = source->xspr.key;

			// inherit dude flags
			spawned->xspr.dudeDeaf = source->xspr.dudeDeaf;
			spawned->xspr.dudeGuard = source->xspr.dudeGuard;
			spawned->xspr.dudeAmbush = source->xspr.dudeAmbush;
			spawned->xspr.dudeFlag4 = source->xspr.dudeFlag4;
			spawned->xspr.unused1 = source->xspr.unused1;
			break;
		}
	}

	// inherit sprite size (useful for seqs with zero repeats)
	if (source->spr.flags & kModernTypeFlag2)
	{
		spawned->spr.xrepeat = source->spr.xrepeat;
		spawned->spr.yrepeat = source->spr.yrepeat;
	}

	gKillMgr.AddKill(spawned);
	aiInitSprite(spawned);
	return spawned;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void genDudeTransform(DBloodActor* actor)
{
	if (!actor->hasX()) return;

	auto actIncarnation = getNextIncarnation(actor);
	if (actIncarnation == NULL)
	{
		if (actor->xspr.sysData1 == kGenDudeTransformStatus) actor->xspr.sysData1 = 0;
		trTriggerSprite(actor, kCmdOff, actor);
		return;
	}

	actor->xspr.key = actor->xspr.dropMsg = actor->xspr.locked = 0;

	// save incarnation's going on and off options
	bool triggerOn = actIncarnation->xspr.triggerOn;
	bool triggerOff = actIncarnation->xspr.triggerOff;

	// then remove it from incarnation so it will not send the commands
	actIncarnation->xspr.triggerOn = false;
	actIncarnation->xspr.triggerOff = false;

	// trigger dude death before transform
	trTriggerSprite(actor, kCmdOff, actor);

	actor->spr.type = actor->spr.inittype = actIncarnation->spr.type;
	actor->spr.flags = actIncarnation->spr.flags;
	actor->spr.pal = actIncarnation->spr.pal;
	actor->spr.shade = actIncarnation->spr.shade;
	actor->spr.clipdist = actIncarnation->spr.clipdist;
	actor->spr.xrepeat = actIncarnation->spr.xrepeat;
	actor->spr.yrepeat = actIncarnation->spr.yrepeat;

	actor->xspr.txID = actIncarnation->xspr.txID;
	actor->xspr.command = actIncarnation->xspr.command;
	actor->xspr.triggerOn = triggerOn;
	actor->xspr.triggerOff = triggerOff;
	actor->xspr.busyTime = actIncarnation->xspr.busyTime;
	actor->xspr.waitTime = actIncarnation->xspr.waitTime;

	// inherit respawn properties
	actor->xspr.respawn = actIncarnation->xspr.respawn;
	actor->xspr.respawnPending = actIncarnation->xspr.respawnPending;

	actor->xspr.burnTime = 0;
	actor->SetBurnSource(nullptr);

	actor->xspr.data1 = actIncarnation->xspr.data1;
	actor->xspr.data2 = actIncarnation->xspr.data2;

	actor->xspr.sysData1 = actIncarnation->xspr.data3;  // soundBase id
	actor->xspr.sysData2 = actIncarnation->xspr.data4;  // start hp

	actor->xspr.dudeGuard = actIncarnation->xspr.dudeGuard;
	actor->xspr.dudeDeaf = actIncarnation->xspr.dudeDeaf;
	actor->xspr.dudeAmbush = actIncarnation->xspr.dudeAmbush;
	actor->xspr.dudeFlag4 = actIncarnation->xspr.dudeFlag4;
	actor->xspr.unused1 = actIncarnation->xspr.unused1;

	actor->xspr.dropMsg = actIncarnation->xspr.dropMsg;
	actor->xspr.key = actIncarnation->xspr.key;

	actor->xspr.locked = actIncarnation->xspr.locked;
	actor->xspr.Decoupled = actIncarnation->xspr.Decoupled;

	// clear drop items of the incarnation
	actIncarnation->xspr.key = actIncarnation->xspr.dropMsg = 0;

	// set hp
	if (actor->xspr.sysData2 <= 0) actor->xspr.health = dudeInfo[actor->spr.type - kDudeBase].startHealth << 4;
	else actor->xspr.health = ClipRange(actor->xspr.sysData2 << 4, 1, 65535);

	int seqId = dudeInfo[actor->spr.type - kDudeBase].seqStartID;
	switch (actor->spr.type) {
	case kDudePodMother: // fake dude
	case kDudeTentacleMother: // fake dude
		break;
	case kDudeModernCustom:
	case kDudeModernCustomBurning:
		seqId = genDudeSeqStartId(actor);
		genDudePrepare(actor, kGenDudePropertyMass);
		[[fallthrough]]; // go below
	default:
		seqSpawn(seqId, actor, -1);

		// save target
		auto target = actor->GetTarget();

		// re-init sprite
		aiInitSprite(actor);

		// try to restore target
		if (target == nullptr) aiSetTarget(actor, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z);
		else aiSetTarget(actor, target);

		// finally activate it
		aiActivateDude(actor);

		break;
	}
	actIncarnation->xspr.triggerOn = triggerOn;
	actIncarnation->xspr.triggerOff = triggerOff;

	/*// remove the incarnation in case if non-locked
	if (actIncarnation->xspr.locked == 0) {
		actIncarnation->xspr.txID = actIncarnation->spr.type = 0;
		actPostSprite(pIncarnation, kStatFree);
		// or restore triggerOn and off options
	} else {
		actIncarnation->xspr.triggerOn = triggerOn;
		actIncarnation->xspr.triggerOff = triggerOff;
	}*/
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void updateTargetOfLeech(DBloodActor* actor)
{
	if (!actor->hasX()) return;

	auto actLeech = leechIsDropped(actor);
	if (actLeech == NULL || !actLeech->hasX()) actor->genDudeExtra.pLifeLeech = nullptr;
	else
	{
		if (actor->GetTarget() != actLeech->GetTarget())
		{
			if (actor->GetTarget() == nullptr && actLeech->GetTarget() != nullptr)
			{
				aiSetTarget(actor, actLeech->GetTarget());
				if (inIdle(actor->xspr.aiState))
					aiActivateDude(actor);
			}
			else
			{
				actLeech->SetTarget(actor->GetTarget());
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void updateTargetOfSlaves(DBloodActor* actor)
{
	if (!actor->hasX()) return;

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	auto slave = pExtra->slave;
	auto actTarget = actor->GetTarget();
	if (!actTarget || !actTarget->IsDudeActor() || !actTarget->hasX() || actTarget->xspr.health <= 0) actTarget = nullptr;

	for (int i = 0; i <= gGameOptions.nDifficulty; i++)
	{
		if (slave[i] != nullptr)
		{
			if (!slave[i]->IsDudeActor() || !slave[i]->hasX() || slave[i]->xspr.health <= 0)
			{
				slave[i]->SetOwner(nullptr);
				slave[i] = nullptr;
				continue;
			}

			if (actTarget != nullptr)
			{
				if (actTarget != slave[i]->GetTarget()) aiSetTarget(slave[i], actTarget);
				// check if slave have proper target
				if (slave[i]->GetTarget() == nullptr || slave[i]->GetTarget()->GetOwner() == actor)
					aiSetTarget(slave[i], actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z);
			}
			else
			{
				aiSetTarget(slave[i], actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z); // try return to master
			}
		}
	}
	// compact the array after processing.
	int writeindex = 0;
	for (int i = 0; i <= gGameOptions.nDifficulty; i++)
	{
		if (slave[i] != nullptr)
		{
			slave[writeindex++] = slave[i];
		}
	}
	pExtra->slaveCount = writeindex;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int inDodge(AISTATE* aiState)
{
	if (aiState == &genDudeDodgeL) return 1;
	else if (aiState == &genDudeDodgeD) return 2;
	else if (aiState == &genDudeDodgeW) return 3;
	else if (aiState == &genDudeDodgeShortL) return 4;
	else if (aiState == &genDudeDodgeShortD) return 5;
	else if (aiState == &genDudeDodgeShortW) return 6;
	else if (aiState == &genDudeDodgeShorterL) return 7;
	else if (aiState == &genDudeDodgeShorterD) return 8;
	else if (aiState == &genDudeDodgeShorterW) return 9;
	return 0;

}

bool inIdle(AISTATE* aiState) {
	return (aiState == &genDudeIdleW || aiState == &genDudeIdleL);
}

bool inAttack(AISTATE* aiState) {
	return (aiState == &genDudeFireL || aiState == &genDudeFireW
		|| aiState == &genDudeFireD || aiState == &genDudeThrow || aiState == &genDudeThrow2 || aiState == &genDudePunch);
}

bool inSearch(AISTATE* aiState) {
	return (aiState->stateType == kAiStateSearch);
}

int inChase(AISTATE* aiState) {
	if (aiState == &genDudeChaseL) return 1;
	else if (aiState == &genDudeChaseD) return 2;
	else if (aiState == &genDudeChaseW) return 3;
	else if (aiState == &genDudeChaseNoWalkL) return 4;
	else if (aiState == &genDudeChaseNoWalkD) return 5;
	else if (aiState == &genDudeChaseNoWalkW) return 6;
	else return 0;
}

int inRecoil(AISTATE* aiState) {
	if (aiState == &genDudeRecoilL || aiState == &genDudeRecoilTesla) return 1;
	else if (aiState == &genDudeRecoilD) return 2;
	else if (aiState == &genDudeRecoilW) return 3;
	else return 0;
}

int inDuck(AISTATE* aiState) {
	if (aiState == &genDudeFireD) return 1;
	else if (aiState == &genDudeChaseD) return 2;
	else if (aiState == &genDudeChaseNoWalkD) return 3;
	else if (aiState == &genDudeRecoilD) return 4;
	else if (aiState == &genDudeDodgeShortD) return 5;
	return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool canSwim(DBloodActor* actor)
{
	return actor->genDudeExtra.canSwim;
}

bool canDuck(DBloodActor* actor)
{
	return actor->genDudeExtra.canDuck;
}

bool canWalk(DBloodActor* actor)
{
	return actor->genDudeExtra.canWalk;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int genDudeSeqStartId(DBloodActor* actor)
{
	if (genDudePrepare(actor, kGenDudePropertyStates)) return actor->xspr.data2;
	else return kGenDudeDefaultSeq;
}

bool genDudePrepare(DBloodActor* actor, int propId)
{
	if (!actor || !actor->hasX()) return false;

	if (actor->spr.type != kDudeModernCustom) {
		Printf(PRINT_HIGH, "actor->spr.type != kDudeModernCustom");
		return false;
	}
	else if (propId < kGenDudePropertyAll || propId >= kGenDudePropertyMax) {
		viewSetSystemMessage("Unknown custom dude #%d property (%d)", actor->GetIndex(), propId);
		return false;
	}

	GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
	pExtra->updReq[propId] = false;

	switch (propId) {
	case kGenDudePropertyAll:
	case kGenDudePropertyInitVals:
		pExtra->moveSpeed = getGenDudeMoveSpeed(actor, 0, true, false);
		pExtra->initVals[0] = actor->spr.xrepeat;
		pExtra->initVals[1] = actor->spr.yrepeat;
		pExtra->initVals[2] = actor->spr.clipdist;
		if (propId) break;
		[[fallthrough]];

	case kGenDudePropertyWeapon: {
		pExtra->curWeapon = actor->xspr.data1;
		switch (actor->xspr.data1) {
		case VECTOR_TYPE_19: pExtra->curWeapon = kVectorBullet; break;
		case kMissileUnused: pExtra->curWeapon = kMissileArcGargoyle; break;
		case kThingDroppedLifeLeech: pExtra->curWeapon = kModernThingEnemyLifeLeech; break;
		}

		pExtra->canAttack = false;
		if (pExtra->curWeapon > 0 && getSequence(actor->xspr.data2 + kGenDudeSeqAttackNormalL))
			pExtra->canAttack = true;

		pExtra->weaponType = kGenDudeWeaponNone;
		if (pExtra->curWeapon > 0 && pExtra->curWeapon < kVectorMax) pExtra->weaponType = kGenDudeWeaponHitscan;
		else if (pExtra->curWeapon >= kDudeBase && pExtra->curWeapon < kDudeMax) pExtra->weaponType = kGenDudeWeaponSummon;
		else if (pExtra->curWeapon >= kMissileBase && pExtra->curWeapon < kMissileMax) pExtra->weaponType = kGenDudeWeaponMissile;
		else if (pExtra->curWeapon >= kThingBase && pExtra->curWeapon < kThingMax) pExtra->weaponType = kGenDudeWeaponThrow;
		else if (pExtra->curWeapon >= kTrapExploder && pExtra->curWeapon < (kTrapExploder + kExplodeMax) - 1)
			pExtra->weaponType = kGenDudeWeaponKamikaze;

		pExtra->isMelee = false;
		if (pExtra->weaponType == kGenDudeWeaponKamikaze) pExtra->isMelee = true;
		else if (pExtra->weaponType == kGenDudeWeaponHitscan) {
			if (gVectorData[pExtra->curWeapon].maxDist > 0 && gVectorData[pExtra->curWeapon].maxDist <= kGenDudeMaxMeleeDist)
				pExtra->isMelee = true;
		}

		if (propId) break;
		[[fallthrough]];

	}
	case kGenDudePropertyDmgScale:
		scaleDamage(actor);
		if (propId) break;
		[[fallthrough]];

	case kGenDudePropertyMass: {
		// to ensure mass gets updated, let's clear all cache
		SPRITEMASS* pMass = &actor->spriteMass;
		pMass->seqId = pMass->picnum = pMass->xrepeat = pMass->yrepeat = pMass->clipdist = 0;
		pMass->mass = pMass->airVel = pMass->fraction = 0;
		getSpriteMassBySize(actor);
		if (propId) break;
		[[fallthrough]];
	}
	case kGenDudePropertyAttack:
		pExtra->fireDist = getRangeAttackDist(actor, 3000, 45000);
		pExtra->throwDist = pExtra->fireDist; // temp
		pExtra->baseDispersion = getDispersionModifier(actor, 200, 3500);
		if (propId) break;
		[[fallthrough]];

	case kGenDudePropertyStates: {

		pExtra->canFly = false;

		// check the animation
		int seqStartId = -1;
		if (actor->xspr.data2 <= 0) seqStartId = actor->xspr.data2 = getDudeInfo(actor->spr.type)->seqStartID;
		else seqStartId = actor->xspr.data2;

		for (int i = seqStartId; i < seqStartId + kGenDudeSeqMax; i++) {
			switch (i - seqStartId) {
			case kGenDudeSeqIdleL:
			case kGenDudeSeqDeathDefault:
			case kGenDudeSeqAttackNormalL:
			case kGenDudeSeqAttackThrow:
			case kGenDudeSeqAttackPunch:
			{
				Seq* pSeq = getSequence(i);
				if (!pSeq)
				{
					actor->xspr.data2 = getDudeInfo(actor->spr.type)->seqStartID;
					viewSetSystemMessage("No SEQ animation id %d found for custom dude #%d!", i, actor->GetIndex());
					viewSetSystemMessage("SEQ base id: %d", seqStartId);
				}
				else if ((i - seqStartId) == kGenDudeSeqAttackPunch)
				{
					pExtra->forcePunch = true; // required for those who don't have fire trigger in punch seq and for default animation
					for (int ii = 0; ii < pSeq->nFrames; ii++) {
						if (!pSeq->frames[ii].trigger) continue;
						pExtra->forcePunch = false;
						break;
					}
				}
				break;
			}
			case kGenDudeSeqDeathExplode:
				pExtra->availDeaths[kDmgExplode] = !!getSequence(i);
				break;
			case kGenDudeSeqBurning:
				pExtra->canBurn = !!getSequence(i);
				break;
			case kGenDudeSeqElectocuted:
				pExtra->canElectrocute = !!getSequence(i);
				break;
			case kGenDudeSeqRecoil:
				pExtra->canRecoil = !!getSequence(i);
				break;
			case kGenDudeSeqMoveL: {
				bool oldStatus = pExtra->canWalk;
				pExtra->canWalk = !!getSequence(i);
				if (oldStatus != pExtra->canWalk) {
					if (actor->GetTarget() == nullptr)
					{
						if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeIdleW);
						else aiGenDudeNewState(actor, &genDudeIdleL);
					}
					else if (pExtra->canWalk)
					{
						if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeChaseW);
						else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(actor, &genDudeChaseD);
						else aiGenDudeNewState(actor, &genDudeChaseL);
					}
					else
					{
						if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeChaseNoWalkW);
						else if (inDuck(actor->xspr.aiState)) aiGenDudeNewState(actor, &genDudeChaseNoWalkD);
						else aiGenDudeNewState(actor, &genDudeChaseNoWalkL);
					}
				}
				break;
			}
			case kGenDudeSeqAttackNormalDW:
				pExtra->canDuck = (getSequence(i) && getSequence(seqStartId + 14));
				pExtra->canSwim = (getSequence(i) && getSequence(seqStartId + 13)
					&& getSequence(seqStartId + 17));
				break;
			case kGenDudeSeqDeathBurn1: {
				bool seq15 = getSequence(i);
				bool seq16 = getSequence(seqStartId + 16);
				if (seq15 && seq16) pExtra->availDeaths[kDmgBurn] = 3;
				else if (seq16) pExtra->availDeaths[kDmgBurn] = 2;
				else if (seq15) pExtra->availDeaths[kDmgBurn] = 1;
				else pExtra->availDeaths[kDmgBurn] = 0;
				break;
			}
			case kGenDudeSeqMoveW:
			case kGenDudeSeqMoveD:
			case kGenDudeSeqDeathBurn2:
			case kGenDudeSeqIdleW:
				break;
			case kGenDudeSeqReserved3:
			case kGenDudeSeqReserved4:
			case kGenDudeSeqReserved5:
			case kGenDudeSeqReserved6:
			case kGenDudeSeqReserved7:
			case kGenDudeSeqReserved8:
				/*if (getSequence(i)) {
					viewSetSystemMessage("Found reserved SEQ animation (%d) for custom dude #%d!", i, actor->GetIndex());
					viewSetSystemMessage("Using reserved animation is not recommended.");
					viewSetSystemMessage("SEQ base id: %d", seqStartId);
				}*/
				break;
			}
		}
		if (propId) break;
		[[fallthrough]];
	}
	case kGenDudePropertyLeech:
		pExtra->pLifeLeech = nullptr;
		if (!actor->GetSpecialOwner())
		{
			BloodStatIterator it(kStatThing);
			while (auto actor2 = it.Next())
			{
				if (actor2->GetOwner() == actor && actor2->spr.type == kModernThingEnemyLifeLeech) {
					pExtra->pLifeLeech = actor2;
					break;
				}
			}
		}
		if (propId) break;
		[[fallthrough]];

	case kGenDudePropertySlaves:
	{
		pExtra->slaveCount = 0;
		for (auto i = 0; i < kGenDudeMaxSlaves; i++)
		{
			pExtra->slave[i] = nullptr;
		}

		BloodStatIterator it(kStatDude);
		while (auto actor2 = it.Next())
		{
			if (actor2->GetOwner() != actor) continue;
			else if (!actor2->IsDudeActor() || !actor2->hasX() || actor2->xspr.health <= 0) {
				actor2->SetOwner(nullptr);
				continue;
			}

			pExtra->slave[pExtra->slaveCount++] = actor2;
			if (pExtra->slaveCount > gGameOptions.nDifficulty)
				break;
		}
		if (propId) break;
		[[fallthrough]];
	}
	case kGenDudePropertySpriteSize: {
		if (seqGetStatus(actor) == -1)
			seqSpawn(actor->xspr.data2 + actor->xspr.aiState->seqId, actor, -1);

		// make sure dudes aren't in the floor or ceiling
		int zTop, zBot; GetActorExtents(actor, &zTop, &zBot);
		if (!(actor->sector()->ceilingstat & CSTAT_SECTOR_SKY))
			actor->add_int_z(ClipLow(actor->sector()->int_ceilingz() - zTop, 0));
		if (!(actor->sector()->floorstat & CSTAT_SECTOR_SKY))
			actor->add_int_z(ClipHigh(actor->sector()->int_floorz() - zBot, 0));

		actor->spr.clipdist = ClipRange((actor->spr.xrepeat + actor->spr.yrepeat) >> 1, 4, 120);
		if (propId) break;
	}
	}

	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void genDudePostDeath(DBloodActor* actor, DAMAGE_TYPE damageType, int damage)
{
	if (damageType == kDamageExplode)
	{
		DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
		for (int i = 0; i < 3; i++)
			if (pDudeInfo->nGibType[i] > -1)
				GibSprite(actor, (GIBTYPE)pDudeInfo->nGibType[i], NULL, NULL);

		for (int i = 0; i < 4; i++)
			fxSpawnBlood(actor, damage);
	}

	gKillMgr.AddKill(actor);

	actor->spr.type = kThingBloodChunks;
	actPostSprite(actor, kStatThing);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiGenDudeInitSprite(DBloodActor* actor)
{
	switch (actor->spr.type)
	{
	case kDudeModernCustom:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = pDudeExtraE->thinkTime = 0;
		aiGenDudeNewState(actor, &genDudeIdleL);
		break;
	}
	case kDudeModernCustomBurning:
		aiGenDudeNewState(actor, &genDudeBurnGoto);
		actor->xspr.burnTime = 1200;
		break;
	}

	actor->spr.flags = 15;
	return;
}

END_BLD_NS
#endif
