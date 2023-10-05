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

static void handThinkSearch(DBloodActor*);
static void handThinkGoto(DBloodActor*);
static void handThinkChase(DBloodActor*);

AISTATE handIdle = { kAiStateIdle, 0, nullptr, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE hand13A3B4 = { kAiStateOther, 0, nullptr, 0, NULL, NULL, NULL, NULL };
AISTATE handSearch = { kAiStateMove, 6, nullptr, 600, NULL, aiMoveForward, handThinkSearch, &handIdle };
AISTATE handChase = { kAiStateChase, 6, nullptr, 0, NULL, aiMoveForward, handThinkChase, NULL };
AISTATE handRecoil = { kAiStateRecoil, 5, nullptr, 0, NULL, NULL, NULL, &handSearch };
AISTATE handGoto = { kAiStateMove, 6, nullptr, 1800, NULL, aiMoveForward, handThinkGoto, &handIdle };
AISTATE handJump = { kAiStateChase, 7, &AF(HandJumpSeqCallback), 120, NULL, NULL, NULL, &handChase };

void HandJumpSeqCallback(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor())
	{
		auto pPlayer = getPlayer(target);
		if (!pPlayer->hand)
		{
			pPlayer->hand = 1;
			actPostSprite(actor, kStatFree);
		}
	}
}

static void handThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void handThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, &handSearch);
	aiThinkTarget(actor);
}

static void handThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &handGoto);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &handSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &handSearch);
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
				if (nDist < 35.1875 && nDeltaAngle < DAngle15 && gGameOptions.nGameType == 0)
					aiNewState(actor, &handJump);
				return;
			}
		}
	}

	aiNewState(actor, &handGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
