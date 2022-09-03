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

AISTATE handIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE hand13A3B4 = { kAiStateOther, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE handSearch = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, handThinkSearch, &handIdle };
AISTATE handChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, handThinkChase, NULL };
AISTATE handRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &handSearch };
AISTATE handGoto = { kAiStateMove, 6, -1, 1800, NULL, aiMoveForward, handThinkGoto, &handIdle };
AISTATE handJump = { kAiStateChase, 7, nJumpClient, 120, NULL, NULL, NULL, &handChase };

void HandJumpSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor())
	{
		PLAYER* pPlayer = &gPlayer[target->spr.type - kDudePlayer1];
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
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
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
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &handSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &handSearch);
		return;
	}
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x233 && abs(nDeltaAngle) < 85 && gGameOptions.nGameType == 0)
					aiNewState(actor, &handJump);
				return;
			}
		}
	}

	aiNewState(actor, &handGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
