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
	int dx = bcos(actor->spr.ang);
	int dy = bsin(actor->spr.ang);
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor())
		actFireVector(actor, 0, 0, dx, dy, target->int_pos().Z - actor->int_pos().Z, kVectorRatBite);
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
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
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

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &ratSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &ratSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x399 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &ratBite);
				return;
			}
		}
	}

	aiNewState(actor, &ratGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
