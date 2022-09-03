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

static void houndThinkSearch(DBloodActor*);
static void houndThinkGoto(DBloodActor*);
static void houndThinkChase(DBloodActor*);

AISTATE houndIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE houndSearch = { kAiStateMove, 8, -1, 1800, NULL, aiMoveForward, houndThinkSearch, &houndIdle };
AISTATE houndChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, houndThinkChase, NULL };
AISTATE houndRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &houndSearch };
AISTATE houndTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &houndSearch };
AISTATE houndGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, houndThinkGoto, &houndIdle };
AISTATE houndBite = { kAiStateChase, 6, nHoundBiteClient, 60, NULL, NULL, NULL, &houndChase };
AISTATE houndBurn = { kAiStateChase, 7, nHoundBurnClient, 60, NULL, NULL, NULL, &houndChase };

void houndBiteSeqCallback(int, DBloodActor* actor)
{
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}

	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
#ifdef NOONE_EXTENSIONS
	if (target->IsPlayerActor() || gModernMap) // allow to hit non-player targets
		actFireVector(actor, 0, 0, dx, dy, target->int_pos().Z - actor->int_pos().Z, kVectorHoundBite);
#else
	if (target->IsPlayerActor())
		actFireVector(actor, 0, 0, dx, dy, target->spr.z - actor->spr.z, kVectorHoundBite);
#endif
}

void houndBurnSeqCallback(int, DBloodActor* actor)
{
	actFireMissile(actor, 0, 0, bcos(actor->int_ang()), bsin(actor->int_ang()), 0, kMissileFlameHound);
}

static void houndThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void houndThinkGoto(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}

	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &houndSearch);
	aiThinkTarget(actor);
}

static void houndThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &houndGoto);
		return;
	}
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &houndSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &houndSearch);
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
				if (nDist < 0xb00 && nDist > 0x500 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &houndBurn);
				else if (nDist < 0x266 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &houndBite);
				return;
			}
		}
	}

	aiNewState(actor, &houndGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
