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

static void zombaThinkSearch(DBloodActor*);
static void zombaThinkGoto(DBloodActor*);
static void zombaThinkChase(DBloodActor*);
static void zombaThinkPonder(DBloodActor*);
static void myThinkTarget(DBloodActor*);
static void myThinkSearch(DBloodActor*);
static void entryEZombie(DBloodActor*);
static void entryAIdle(DBloodActor*);
static void entryEStand(DBloodActor*);


AISTATE zombieAIdle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, aiThinkTarget, NULL };
AISTATE zombieAChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, zombaThinkChase, NULL };
AISTATE zombieAPonder = { kAiStateOther, 0, -1, 0, NULL, aiMoveTurn, zombaThinkPonder, NULL };
AISTATE zombieAGoto = { kAiStateMove, 8, -1, 1800, NULL, aiMoveForward, zombaThinkGoto, &zombieAIdle };
AISTATE zombieAHack = { kAiStateChase, 6, nHackClient, 80, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieASearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, zombaThinkSearch, &zombieAIdle };
AISTATE zombieARecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieATeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieARecoil2 = { kAiStateRecoil, 1, -1, 360, NULL, NULL, NULL, &zombieAStand };
AISTATE zombieAStand = { kAiStateMove, 11, nStandClient, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieEIdle = { kAiStateIdle, 12, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombieEUp2 = { kAiStateMove, 0, -1, 1, entryEZombie, NULL, NULL, &zombieASearch };
AISTATE zombieEUp = { kAiStateMove, 9, -1, 180, entryEStand, NULL, NULL, &zombieEUp2 };
AISTATE zombie2Idle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, myThinkTarget, NULL };
AISTATE zombie2Search = { kAiStateSearch, 8, -1, 1800, NULL, NULL, myThinkSearch, &zombie2Idle };
AISTATE zombieSIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombie13AC2C = { kAiStateOther, 11, nStandClient, 0, entryEZombie, NULL, NULL, &zombieAPonder };

void HackSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int tx = actor->xspr.int_TargetPos().X - actor->int_pos().X;
	int ty = actor->xspr.int_TargetPos().Y - actor->int_pos().Y;
	int nAngle = getangle(tx, ty);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	int dz = height - height2;
	int dx = bcos(nAngle);
	int dy = bsin(nAngle);
	sfxPlay3DSound(actor, 1101, 1, 0);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorAxe);
}

void StandSeqCallback(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 1102, -1, 0);
}

static void zombaThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiLookForTarget(actor);
}

static void zombaThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (nDist < 921 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &zombieASearch);
	aiThinkTarget(actor);
}

static void zombaThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);

	if (target->xspr.health == 0)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	if (target->IsPlayerActor() && (powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpDeathMaskUseless) > 0))
	{
		aiNewState(actor, &zombieAGoto);
		return;
	}
	// If the zombie gets whacked while rising from the grave it never executes this change and if it isn't done here at the very latest, will just aimlessly run around.
	if (!VanillaMode() && actor->spr.type == kDudeZombieAxeBuried)
		actor->spr.type = kDudeZombieAxeNormal;


	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &zombieAHack);
				return;
			}
		}
	}

	aiNewState(actor, &zombieAGoto);
	actor->SetTarget(nullptr);
}

static void zombaThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	if (target->IsPlayerActor() && (powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpDeathMaskUseless) > 0))
	{
		aiNewState(actor, &zombieAGoto);
		return;
	}

	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400)
				{
					if (abs(nDeltaAngle) < 85)
					{
						sfxPlay3DSound(actor, 1101, 1, 0);
						aiNewState(actor, &zombieAHack);
					}
					return;
				}
			}
		}
	}

	aiNewState(actor, &zombieAChase);
}

static void myThinkTarget(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		PLAYER* pPlayer = &gPlayer[p];
		auto owneractor = actor->GetOwner();
		if (owneractor == nullptr || owneractor == pPlayer->actor || pPlayer->actor->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
			continue;
		auto ppos = pPlayer->actor->spr.pos;
		auto dvect = ppos.XY() - actor->spr.pos;
		auto pSector = pPlayer->actor->sector();
		int nDist = approxDist(dvect);
		if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
			continue;
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
			continue;
		int nDeltaAngle = getincangle(actor->int_ang(), getangle(dvect));
		if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
		{
			aiSetTarget(actor, pPlayer->actor);
			aiActivateDude(actor);
		}
		else if (nDist < pDudeInfo->hearDist)
		{
			aiSetTarget(actor, ppos);
			aiActivateDude(actor);
		}
		else
			continue;
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void myThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	myThinkTarget(actor);
}

static void entryEZombie(DBloodActor* actor)
{
	actor->spr.type = kDudeZombieAxeNormal;
	actor->spr.flags |= 1;
}

static void entryAIdle(DBloodActor* actor)
{
	actor->SetTarget(nullptr);
}

static void entryEStand(DBloodActor* actor)
{
	sfxPlay3DSound(actor, 1100, -1, 0);
	actor->spr.angle = VecToAngle(actor->xspr.TargetPos - actor->spr.pos);
}

END_BLD_NS
