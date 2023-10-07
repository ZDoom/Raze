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


AISTATE eelIdle = { kAiStateIdle, 0, nullptr, 0, NULL, NULL, &AF(eelThinkTarget), NULL };
AISTATE eelFlyIdle = { kAiStateIdle, 0, nullptr, 0, NULL, NULL, &AF(eelThinkTarget), NULL };
AISTATE eelChase = { kAiStateChase, 0, nullptr, 0, NULL, &AF(eelMoveForward), &AF(eelThinkChase), &eelIdle };
AISTATE eelPonder = { kAiStateOther, 0, nullptr, 0, NULL, NULL, &AF(eelThinkPonder), NULL };
AISTATE eelGoto = { kAiStateMove, 0, nullptr, 600, NULL, NULL, &AF(eelThinkGoto), &eelIdle };
AISTATE eelBite = { kAiStateChase, 7, &AF(eelBiteSeqCallback), 60, NULL, NULL, NULL, &eelChase };
AISTATE eelRecoil = { kAiStateRecoil, 5, nullptr, 0, NULL, NULL, NULL, &eelChase };
AISTATE eelSearch = { kAiStateSearch, 0, nullptr, 120, NULL, &AF(eelMoveForward), &AF(eelThinkSearch), &eelIdle };
AISTATE eelSwoop = { kAiStateOther, 0, nullptr, 60, NULL, &AF(eelMoveSwoop), &AF(eelThinkChase), &eelChase };
AISTATE eelFly = { kAiStateMove, 0, nullptr, 0, NULL, &AF(eelMoveAscend), &AF(eelThinkChase), &eelChase };
AISTATE eelTurn = { kAiStateMove, 0, nullptr, 60, NULL, &AF(aiMoveTurn), NULL, &eelChase };
AISTATE eelHide = { kAiStateOther, 0, nullptr, 0, NULL, &AF(eelMoveToCeil), &AF(eelMoveForward), NULL };
AISTATE eelDodgeUp = { kAiStateMove, 0, nullptr, 120, NULL, &AF(eelMoveDodgeUp), NULL, &eelChase };
AISTATE eelDodgeUpRight = { kAiStateMove, 0, nullptr, 90, NULL, &AF(eelMoveDodgeUp), NULL, &eelChase };
AISTATE eelDodgeUpLeft = { kAiStateMove, 0, nullptr, 90, NULL, &AF(eelMoveDodgeUp), NULL, &eelChase };
AISTATE eelDodgeDown = { kAiStateMove, 0, nullptr, 120, NULL, &AF(eelMoveDodgeDown), NULL, &eelChase };
AISTATE eelDodgeDownRight = { kAiStateMove, 0, nullptr, 90, NULL, &AF(eelMoveDodgeDown), NULL, &eelChase };
AISTATE eelDodgeDownLeft = { kAiStateMove, 0, nullptr, 90, NULL, &AF(eelMoveDodgeDown), NULL, &eelChase };

void eelBiteSeqCallback(DBloodActor* actor)
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	DUDEINFO* pDudeInfoT = getDudeInfo(target);
	double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
	double height2 = (pDudeInfoT->eyeHeight * target->spr.scale.Y);
	DVector3 vect(actor->spr.Angles.Yaw.ToVector() * 1024, height2 - height);

	actFireVector(actor, 0., 0., vect, kVectorBoneelBite);
}

static void eelThinkTarget(DBloodActor* actor)
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
		aiNewState(actor, &eelTurn);
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
			auto dvect = ppos.XY() - actor->spr.pos;
			auto pSector = pPlayer->GetActor()->sector();
			double nDist = dvect.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvect.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				actor->dudeExtra.thinkTime = 0;
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				actor->dudeExtra.thinkTime = 0;
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	auto nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		double height2 = (getDudeInfo(target)->eyeHeight * target->spr.scale.Y);
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < -0x20 && nDist < 0x180 && nDist > 0xc0 && nDeltaAngle < DAngle15)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 12.8 && nDist < 0x180 && nDist > 0xc0 && nDeltaAngle < DAngle15)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < 12.8 && nDist < 57.5625 && nDeltaAngle < DAngle15)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 12.8 && nDist < 0x140 && nDist > 0x80 && nDeltaAngle < DAngle15)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < -0x20 && nDist < 0x140 && nDist > 0x80 && nDeltaAngle < DAngle15)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height < -0x20 && nDeltaAngle < DAngle15 && nDist > 0x140)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 12.8)
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
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

static void eelThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelGoto);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		double top, bottom;
		double top2, bottom2;
		GetActorExtents(actor, &top, &bottom);
		GetActorExtents(target, &top2, &bottom2);

		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 57.5625 && top2 > top && nDeltaAngle < DAngle15)
					aiNewState(actor, &eelSwoop);
				else if (nDist <= 57.5625 && nDeltaAngle < DAngle15)
					aiNewState(actor, &eelBite);
				else if (bottom2 > top && nDeltaAngle < DAngle15)
					aiNewState(actor, &eelSwoop);
				else if (top2 < top && nDeltaAngle < DAngle15)
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.Angles.Yaw += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (nDist <= 57.5625)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel * 0.5;
	});

}

static void eelMoveSwoop(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
		return;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x8000) && nDist <= 57.5625)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});

	actor->vel.Z = FixedToFloat(0x22222);
}

static void eelMoveAscend(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
		return;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x4000) && nDist <= 57.5625)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});

	actor->vel.Z = FixedToFloat(-0x8000);
}

void eelMoveToCeil(DBloodActor* actor)
{
	if (actor->spr.pos.Z - actor->xspr.TargetPos.Z < 0x10)
	{
		
		actor->dudeExtra.active = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &eelIdle);
	}
	else
		aiSetTarget(actor, DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz));
}

END_BLD_NS
