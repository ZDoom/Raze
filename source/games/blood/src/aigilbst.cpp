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

void GillBiteSeqCallback(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DVector3 vec(actor->spr.Angles.Yaw.ToVector() * 64, actor->spr.pos.Z - target->spr.pos.Z);
	vec.X += Random3F(2000, 8);
	vec.Y += Random3F(2000, 8);
	actFireVector(actor, 0, 0, vec, kVectorGillBite);
	actFireVector(actor, 0, 0, vec, kVectorGillBite);
	actFireVector(actor, 0, 0, vec, kVectorGillBite);
}

void gillThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void gillThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);

	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_gillBeastSwimSearch);
		else
			aiNewState(actor, NAME_gillBeastSearch);
	}
	aiThinkTarget(actor);
}

void gillThinkChase(DBloodActor* actor)
{
	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (actor->GetTarget() == nullptr)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_gillBeastSwimSearch);
		else
			aiNewState(actor, NAME_gillBeastSearch);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	auto dv = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dv.Angle();
	aiChooseDirection(actor, nAngle);
	if (actor->xspr.health == 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_gillBeastSwimSearch);
		else
			aiNewState(actor, NAME_gillBeastSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_gillBeastSwimSearch);
		else
			aiNewState(actor, NAME_gillBeastSearch);
		return;
	}
	double nDist = dv.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : (target->spr.pos.Z - actor->spr.pos.Z) / nDist;
				if (nDist < 57.5625 && abs(nDeltaAngle) < DAngle1 * 5)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv, 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, NAME_gillBeastSwimBite);
						else
							aiNewState(actor, NAME_gillBeastBite);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, NAME_gillBeastSwimBite);
							else
								aiNewState(actor, NAME_gillBeastBite);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, NAME_gillBeastSwimDodge);
							else
								aiNewState(actor, NAME_gillBeastDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, NAME_gillBeastSwimBite);
						else
							aiNewState(actor, NAME_gillBeastBite);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, NAME_gillBeastSwimGoto);
	else
		aiNewState(actor, NAME_gillBeastGoto);
	sfxPlay3DSound(actor, 1701, -1, 0);
	actor->SetTarget(nullptr);
}

void gillThinkSwimGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, NAME_gillBeastSwimSearch);
	aiThinkTarget(actor);
}

void gillThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_gillBeastSwimSearch);
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
	
	if (actor->xspr.health == 0)
	{
		aiNewState(actor, NAME_gillBeastSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, NAME_gillBeastSwimSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x40 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_gillBeastSwimBite);
				else
				{
					aiPlay3DSound(actor, 1700, AI_SFX_PRIORITY_1, -1);
					aiNewState(actor, NAME_gillBeastSwimMoveIn);
				}
			}
		}
		else
			aiNewState(actor, NAME_gillBeastSwimMoveIn);
		return;
	}
	aiNewState(actor, NAME_gillBeastSwimGoto);
	actor->SetTarget(nullptr);
}

void gillMoveSwimChase(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.Angles.Yaw += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Random(64) < 32 && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel * 0.25;
	});

}

void gillMoveSwimUnused(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x600) && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel;
	});

	actor->vel.Z = -(target->spr.pos.Z - actor->spr.pos.Z) / 256.;
}

void gillSwimMoveIn(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = FixedToFloat((pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2);
	if (abs(nAng) > DAngle60)
	{
		actor->spr.Angles.Yaw += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x4000) && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
	});

	actor->vel.Z = (target->spr.pos.Z - actor->spr.pos.Z) / 32.;
}

END_BLD_NS
