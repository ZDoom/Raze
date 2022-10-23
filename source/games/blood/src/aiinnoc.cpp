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
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.angle, nAngle) < pDudeInfo->Periphery())
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

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
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

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.angle, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && abs(nDeltaAngle) <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 102.375 && nDeltaAngle < DAngle15)
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
