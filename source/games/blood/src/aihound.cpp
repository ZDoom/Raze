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

void houndBiteSeqCallback(DBloodActor* actor)
{
	if (!(actor->IsDudeActor())) {
		Printf(PRINT_HIGH, "actor->IsDudeActor()");
		return;
	}

	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DVector3 vec(actor->spr.Angles.Yaw.ToVector() * 64, target->spr.pos.Z - actor->spr.pos.Z);
	if (target->IsPlayerActor() || currentLevel->featureflags & kFeatureEnemyAttacks)
		actFireVector(actor, 0, 0, vec, kVectorHoundBite);
}

void houndBurnSeqCallback(DBloodActor* actor)
{
	actFireMissile(actor, 0, 0, DVector3(actor->spr.Angles.Yaw.ToVector(), 0), kMissileFlameHound);
}

void houndThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void houndThinkGoto(DBloodActor* actor)
{
	if (!(actor->IsDudeActor())) {
		Printf(PRINT_HIGH, "actor->IsDudeActor()");
		return;
	}

	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, NAME_houndSearch);
	aiThinkTarget(actor);
}

void houndThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_houndGoto);
		return;
	}
	if (!(actor->IsDudeActor())) {
		Printf(PRINT_HIGH, "actor->IsDudeActor()");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, NAME_houndSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, NAME_houndSearch);
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
				if (nDist < 0xb0 && nDist > 0x50 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_houndBurn);
				else if (nDist < 38.375 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_houndBite);
				return;
			}
		}
	}

	aiNewState(actor, NAME_houndGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
