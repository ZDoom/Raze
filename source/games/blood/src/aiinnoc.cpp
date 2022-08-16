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

static void innocThinkSearch(DBloodActor*);
static void innocThinkGoto(DBloodActor*);
static void innocThinkChase(DBloodActor*);

AISTATE innocentIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE innocentSearch = { kAiStateSearch, 6, -1, 1800, NULL, aiMoveForward, innocThinkSearch, &innocentIdle };
AISTATE innocentChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, innocThinkChase, NULL };
AISTATE innocentRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &innocentChase };
AISTATE innocentGoto = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, innocThinkGoto, &innocentIdle };

static void innocThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void innocThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &innocentSearch);
	aiThinkTarget(actor);
}

static void innocThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &innocentGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &innocentSearch);
		return;
	}
	if (target->IsPlayerActor())
	{
		aiNewState(actor, &innocentSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x666 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &innocentIdle);
				return;
			}
		}
	}

	aiPlay3DSound(actor, 7000 + Random(6), AI_SFX_PRIORITY_1, -1);
	aiNewState(actor, &innocentGoto);
	actor->SetTarget(nullptr);
}


END_BLD_NS
