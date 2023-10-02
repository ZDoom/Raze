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

static void ratThinkSearch(DBloodActor*);
static void ratThinkGoto(DBloodActor*);
static void ratThinkChase(DBloodActor*);

AISTATE ratIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE ratSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, ratThinkSearch, &ratIdle };
AISTATE ratChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, ratThinkChase, NULL };
AISTATE ratDodge = { kAiStateMove, 7, -1, 0, NULL, NULL, NULL, &ratChase };
AISTATE ratRecoil = { kAiStateRecoil, 7, -1, 0, NULL, NULL, NULL, &ratDodge };
AISTATE ratGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, ratThinkGoto, &ratIdle };
AISTATE ratBite = { kAiStateChase, 6, nRatBiteClient, 120, NULL, NULL, NULL, &ratChase };

void ratBiteSeqCallback(int, DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor())
	{
		DVector3 vec(actor->spr.Angles.Yaw.ToVector() * 64, target->spr.pos.Z - actor->spr.pos.Z);
		actFireVector(actor, 0, 0, vec, kVectorRatBite);
	}
}

static void ratThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void ratThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, &ratSearch);
	aiThinkTarget(actor);
}

static void ratThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &ratGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &ratSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target->spr.type - kDudePlayer1), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &ratSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && abs(nDeltaAngle) <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 57.5625 && nDeltaAngle < DAngle15)
					aiNewState(actor, &ratBite);
				return;
			}
		}
	}

	aiNewState(actor, &ratGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
