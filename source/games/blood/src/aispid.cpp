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

#include "ns.h"	// Must come before everything else!

#include "build.h"

#include "blood.h"

BEGIN_BLD_NS

static void spidThinkSearch(DBloodActor*);
static void spidThinkGoto(DBloodActor*);
static void spidThinkChase(DBloodActor*);


AISTATE spidIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE spidChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, spidThinkChase, NULL };
AISTATE spidDodge = { kAiStateMove, 7, -1, 90, NULL, aiMoveDodge, NULL, &spidChase };
AISTATE spidGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, spidThinkGoto, &spidIdle };
AISTATE spidSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, spidThinkSearch, &spidIdle };
AISTATE spidBite = { kAiStateChase, 6, nSpidBiteClient, 60, NULL, NULL, NULL, &spidChase };
AISTATE spidJump = { kAiStateChase, 8, nSpidJumpClient, 60, NULL, aiMoveForward, NULL, &spidChase };
AISTATE spidBirth = { kAiStateOther, 0, nSpidBirthClient, 60, NULL, NULL, NULL, &spidIdle };

static void spidBlindEffect(DBloodActor* actor, int nBlind, int max)
{
	if (actor->IsPlayerActor())
	{
		nBlind <<= 4;
		max <<= 4;
		PLAYER* pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];
		if (pPlayer->blindEffect < max)
		{
			pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect + nBlind, max);
		}
	}
}

void SpidBiteSeqCallback(int, DBloodActor* actor)
{
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	dx += Random2(2000);
	dy += Random2(2000);
	int dz = Random2(2000);
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto const target = actor->GetTarget();
	if (target->IsPlayerActor())
	{
		int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
		if (hit == 3 && gHitInfo.actor()->IsPlayerActor())
		{
			dz += target->int_pos().Z - actor->int_pos().Z;
			PLAYER* pPlayer = &gPlayer[target->spr.type - kDudePlayer1];
			switch (actor->spr.type)
			{
			case kDudeSpiderBrown:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				if (target->IsPlayerActor() && !pPlayer->godMode && powerupCheck(pPlayer, kPwUpDeathMask) <= 0 && Chance(0x4000))
					powerupActivate(pPlayer, kPwUpDeliriumShroom);
				break;
			case kDudeSpiderRed:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				if (Chance(0x5000)) spidBlindEffect(target, 4, 16);
				break;
			case kDudeSpiderBlack:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			case kDudeSpiderMother:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);

				dx += Random2(2000);
				dy += Random2(2000);
				dz += Random2(2000);
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			}
		}

	}
}

void SpidJumpSeqCallback(int, DBloodActor* actor)
{
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	dx += Random2(200);
	dy += Random2(200);
	int dz = Random2(200);
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor()) {
		dz += target->int_pos().Z - actor->int_pos().Z;
		switch (actor->spr.type) {
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
			actor->vel.X = IntToFixed(dx);
			actor->vel.Y = IntToFixed(dy);
			actor->vel.Z = IntToFixed(dz);
			break;
		}
	}
}

void SpidBirthSeqCallback(int, DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);

	DBloodActor* spawned = nullptr;
	if (target->IsPlayerActor() && pDudeExtraE->birthCounter < 10)
	{
		if (nDist < 0x1a00 && nDist > 0x1400 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderRed, actor->spr.clipdist, 0);
		else if (nDist < 0x1400 && nDist > 0xc00 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderBrown, actor->spr.clipdist, 0);
		else if (nDist < 0xc00 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderBrown, actor->spr.clipdist, 0);

		if (spawned)
		{
			pDudeExtraE->birthCounter++;
			spawned->SetOwner(spawned);
			gKillMgr.AddKill(spawned);
		}
	}

}

static void spidThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void spidThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &spidSearch);
	aiThinkTarget(actor);
}

static void spidThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &spidGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &spidSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &spidSearch);
		return;
	}

	if (nDist <= pDudeInfo->seeDist) {
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery) {
				aiSetTarget(actor, actor->GetTarget());

				switch (actor->spr.type) {
				case kDudeSpiderRed:
					if (nDist < 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidBite);
					break;
				case kDudeSpiderBrown:
				case kDudeSpiderBlack:
					if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidJump);
					else if (nDist < 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidBite);
					break;
				case kDudeSpiderMother:
					if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidJump);
					else if (Chance(0x8000))
						aiNewState(actor, &spidBirth);
					break;
				}

				return;
			}
		}
	}

	aiNewState(actor, &spidGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
