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

static void batThinkTarget(DBloodActor*);
static void batThinkSearch(DBloodActor*);
static void batThinkGoto(DBloodActor*);
static void batThinkPonder(DBloodActor*);
static void batMoveDodgeUp(DBloodActor*);
static void batMoveDodgeDown(DBloodActor*);
static void batThinkChase(DBloodActor*);
static void batMoveForward(DBloodActor*);
static void batMoveSwoop(DBloodActor*);
static void batMoveFly(DBloodActor*);
static void batMoveToCeil(DBloodActor*);


AISTATE batIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batFlyIdle = { kAiStateIdle, 6, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batChase = { kAiStateChase, 6, -1, 0, NULL, batMoveForward, batThinkChase, &batFlyIdle };
AISTATE batPonder = { kAiStateOther, 6, -1, 0, NULL, NULL, batThinkPonder, NULL };
AISTATE batGoto = { kAiStateMove, 6, -1, 600, NULL, batMoveForward, batThinkGoto, &batFlyIdle };
AISTATE batBite = { kAiStateChase, 7, nBatBiteClient, 60, NULL, NULL, NULL, &batPonder };
AISTATE batRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &batChase };
AISTATE batSearch = { kAiStateSearch, 6, -1, 120, NULL, batMoveForward, batThinkSearch, &batFlyIdle };
AISTATE batSwoop = { kAiStateOther, 6, -1, 60, NULL, batMoveSwoop, batThinkChase, &batChase };
AISTATE batFly = { kAiStateMove, 6, -1, 0, NULL, batMoveFly, batThinkChase, &batChase };
AISTATE batTurn = { kAiStateMove, 6, -1, 60, NULL, aiMoveTurn, NULL, &batChase };
AISTATE batHide = { kAiStateOther, 6, -1, 0, NULL, batMoveToCeil, batMoveForward, NULL };
AISTATE batDodgeUp = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeDown = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };

void batBiteSeqCallback(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();

	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	DUDEINFO* pDudeInfoT = getDudeInfo(pTarget);

	double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
	double height2 = (pDudeInfoT->eyeHeight * pTarget->spr.scale.Y);
	actFireVector(actor, 0., 0., DVector3(actor->spr.Angles.Yaw.ToVector() * 64, height2 - height), kVectorBatBite);
}

static void batThinkTarget(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	
	if (actor->dudeExtra.active && actor->dudeExtra.thinkTime < 10)
		actor->dudeExtra.thinkTime++;
	else if (actor->dudeExtra.thinkTime >= 10 && actor->dudeExtra.active)
	{
		actor->dudeExtra.thinkTime = 0;
		actor->xspr.goalAng += DAngle45;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &batTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			if (pPlayer->GetActor()->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->GetActor()->spr.pos;
			auto dvec = ppos.XY() - actor->spr.pos.XY();
			auto pSector = pPlayer->GetActor()->sector();

			double nDist = dvec.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvec.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void batThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	batThinkTarget(actor);
}

static void batThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	auto nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, &batSearch);
	batThinkTarget(actor);
}

static void batThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	auto dvec = pTarget->spr.pos.XY() - actor->spr.pos.XY();
	aiChooseDirection(actor, dvec.Angle());
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	double nDist = dvec.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvec.Angle());
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		double height2 = (getDudeInfo(pTarget)->eyeHeight * pTarget->spr.scale.Y);
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->spr.pos, pTarget->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < 48 && nDist < 0x180 && nDist > 0xc0 && nDeltaAngle < DAngle15)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x50 && nDist < 0x180 && nDist > 0xc0 && nDeltaAngle < DAngle15)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x20 && nDist < 0x20 && nDeltaAngle < DAngle15)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x60 && nDist < 0x140 && nDist > 0x80 && nDeltaAngle < DAngle15)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x20 && nDist < 0x140 && nDist > 0x80 && nDeltaAngle < DAngle15)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height < 0x20 && nDeltaAngle < DAngle15 && nDist > 0x140)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x40)
				aiNewState(actor, &batDodgeDown);
			else
				aiNewState(actor, &batDodgeUp);
			return;
		}
	}
	aiNewState(actor, &batGoto);
	actor->SetTarget(nullptr);
}

static void batMoveDodgeUp(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += pDudeInfo->sideSpeed;
		else
			t2 -= pDudeInfo->sideSpeed;
	});

	actor->vel.Z = FixedToFloat(-0x52aaa);
}

static void batMoveDodgeDown(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	if (actor->xspr.dodgeDir == 0)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += FixedToFloat(pDudeInfo->sideSpeed);
		else
			t2 -= FixedToFloat(pDudeInfo->sideSpeed);
	});

	actor->vel.Z = 4.26666;
}

static void batThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batGoto);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	auto dvec = pTarget->spr.pos.XY() - actor->spr.pos.XY();

	aiChooseDirection(actor, dvec.Angle());
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	if (pTarget->IsPlayerActor() && powerupCheck(getPlayer(pTarget), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	double nDist = dvec.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvec.Angle());
		double height = pDudeInfo->eyeHeight * actor->spr.scale.Y;
		// Should be dudeInfo[pTarget]
		double height2 = pDudeInfo->eyeHeight * pTarget->spr.scale.Y;
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->spr.pos, pTarget->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				double floorZ = getflorzofslopeptr(actor->sector(), actor->spr.pos);
				double floorDelta = floorZ - bottom;
				double heightDelta = height2 - height;
				bool angWithinRange = nDeltaAngle < DAngle15;
				if (heightDelta < 32 && nDist < 0x20 && angWithinRange)
					aiNewState(actor, &batBite);
				else if ((heightDelta > 80 || floorDelta > 80) && nDist < 0x140 && nDist > 0x80 && angWithinRange)
					aiNewState(actor, &batSwoop);
				else if ((heightDelta < 48 || floorDelta < 48) && angWithinRange)
					aiNewState(actor, &batFly);
				return;
			}
		}
		else
		{
			aiNewState(actor, &batFly);
			return;
		}
	}

	actor->SetTarget(nullptr);
	aiNewState(actor, &batHide);
}

static void batMoveForward(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.Angles.Yaw += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	double nDist = dvec.Length();
	if ((unsigned int)Random(64) < 32 && nDist <= 0x20)
		return;
	
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel * 0.5;
	});
}

static void batMoveSwoop(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	double nDist = dvec.Length();
	if (Chance(0x600) && nDist <= 0x20)
		return;
	
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});
	actor->vel.Z = 4.26666;
}

static void batMoveFly(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
	{
		actor->spr.Angles.Yaw += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	double nDist = dvec.Length();
	if (Chance(0x4000) && nDist <= 0x20)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});
	actor->vel.Z = FixedToFloat(-0x2d555);
}

void batMoveToCeil(DBloodActor* actor)
{
	if (actor->spr.pos.Z - actor->xspr.TargetPos.Z < 0x10)
	{
		
		actor->dudeExtra.thinkTime = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &batIdle);
	}
	else
		aiSetTarget(actor, DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz));
}

END_BLD_NS
