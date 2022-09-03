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

static void eelThinkTarget(DBloodActor*);
static void eelThinkSearch(DBloodActor*);
static void eelThinkGoto(DBloodActor*);
static void eelThinkPonder(DBloodActor*);
static void eelMoveDodgeUp(DBloodActor*);
static void eelMoveDodgeDown(DBloodActor*);
static void eelThinkChase(DBloodActor*);
static void eelMoveForward(DBloodActor*);
static void eelMoveSwoop(DBloodActor*);
static void eelMoveAscend(DBloodActor* actor);
static void eelMoveToCeil(DBloodActor*);


AISTATE eelIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelFlyIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelChase = { kAiStateChase, 0, -1, 0, NULL, eelMoveForward, eelThinkChase, &eelIdle };
AISTATE eelPonder = { kAiStateOther, 0, -1, 0, NULL, NULL, eelThinkPonder, NULL };
AISTATE eelGoto = { kAiStateMove, 0, -1, 600, NULL, NULL, eelThinkGoto, &eelIdle };
AISTATE eelBite = { kAiStateChase, 7, nEelBiteClient, 60, NULL, NULL, NULL, &eelChase };
AISTATE eelRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &eelChase };
AISTATE eelSearch = { kAiStateSearch, 0, -1, 120, NULL, eelMoveForward, eelThinkSearch, &eelIdle };
AISTATE eelSwoop = { kAiStateOther, 0, -1, 60, NULL, eelMoveSwoop, eelThinkChase, &eelChase };
AISTATE eelFly = { kAiStateMove, 0, -1, 0, NULL, eelMoveAscend, eelThinkChase, &eelChase };
AISTATE eelTurn = { kAiStateMove, 0, -1, 60, NULL, aiMoveTurn, NULL, &eelChase };
AISTATE eelHide = { kAiStateOther, 0, -1, 0, NULL, eelMoveToCeil, eelMoveForward, NULL };
AISTATE eelDodgeUp = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeDown = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };

void eelBiteSeqCallback(int, DBloodActor* actor)
{
	/*
	 * workaround for
	 * actor->xspr.target >= 0 in file NBlood/source/blood/src/aiboneel.cpp at line 86
	 * The value of actor->xspr.target is -1.
	 * copied from lines 177:181
	 * resolves this case, but may cause other issues?
	 */
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	auto target = actor->GetTarget();
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	actFireVector(actor, 0, 0, dx, dy, height2 - height, kVectorBoneelBite);
}

static void eelThinkTarget(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	if (pDudeExtraE->active && pDudeExtraE->thinkTime < 10)
		pDudeExtraE->thinkTime++;
	else if (pDudeExtraE->thinkTime >= 10 && pDudeExtraE->active)
	{
		pDudeExtraE->thinkTime = 0;
		actor->xspr.goalAng += DAngle45;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &eelTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
			if (pPlayer->actor->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
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
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void eelThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	eelThinkTarget(actor);
}

static void eelThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	auto nAngle = VecToAngle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && absangle(actor->spr.angle, nAngle) < DAngle::fromBuild(pDudeInfo->periphery))
		aiNewState(actor, &eelSearch);
	eelThinkTarget(actor);
}

static void eelThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		int height2 = (getDudeInfo(target->spr.type)->eyeHeight * target->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height * zinttoworld), actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < -0x2000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < 0xccc && nDist < 0x399 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < -0x2000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height < -0x2000 && abs(nDeltaAngle) < 85 && nDist > 0x1400)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc)
				aiNewState(actor, &eelDodgeDown);
			else
				aiNewState(actor, &eelDodgeUp);
			return;
		}
	}
	aiNewState(actor, &eelGoto);
	actor->SetTarget(nullptr);
}

static void eelMoveDodgeUp(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += FixedToFloat(pDudeInfo->sideSpeed);
		else
			t2 -= FixedToFloat(pDudeInfo->sideSpeed);
	});

	actor->vel.Z = -0.5;
}

static void eelMoveDodgeDown(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	if (actor->xspr.dodgeDir == 0)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += FixedToFloat(pDudeInfo->sideSpeed);
		else
			t2 -= FixedToFloat(pDudeInfo->sideSpeed);
	});

	actor->vel.Z = FixedToFloat(0x44444);
}

static void eelThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		int top2, bottom2;
		GetActorExtents(target, &top2, &bottom2);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x399 && top2 > top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelSwoop);
				else if (nDist <= 0x399 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelBite);
				else if (bottom2 > top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelSwoop);
				else if (top2 < top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelFly);
			}
		}
		return;
	}

	actor->SetTarget(nullptr);
	aiNewState(actor, &eelSearch);
}

static void eelMoveForward(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.angle += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	if (nDist <= 0x399)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += FixedToFloat(nAccel);
		else
			t1 += FixedToFloat(nAccel * 0.5);
	});

}

static void eelMoveSwoop(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > DAngle60)
		return;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	if (Chance(0x8000) && nDist <= 0x399)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += FixedToFloat(nAccel * 0.5);
	});

	actor->vel.Z = FixedToFloat(0x22222);
}

static void eelMoveAscend(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > DAngle60)
		return;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	if (Chance(0x4000) && nDist <= 0x399)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += FixedToFloat(nAccel * 0.5);
	});

	actor->vel.Z = FixedToFloat(-0x8000);
}

void eelMoveToCeil(DBloodActor* actor)
{
	if (actor->spr.pos.Z - actor->xspr.TargetPos.Z < 0x10)
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &eelIdle);
	}
	else
		aiSetTarget(actor, DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz));
}

END_BLD_NS
