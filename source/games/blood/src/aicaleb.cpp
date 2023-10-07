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

void SeqAttackCallback(DBloodActor* actor)
{
	DVector3 vect(actor->spr.Angles.Yaw.ToVector(), actor->dudeSlope);
	vect.X += Random2F(1500, 4);
	vect.Y += Random2F(1500, 4);
	vect.Z += Random2F(1500, 8);

	for (int i = 0; i < 2; i++)
	{
		double r3 = Random3F(500, 4);
		double r2 = Random3F(1000, 4);
		double r1 = Random3F(1000, 8);
		actFireVector(actor, 0, 0, vect + DVector3(r1, r2, r3), kVectorShell);
	}
	if (Chance(0x8000))
		sfxPlay3DSound(actor, 10000 + Random(5), -1, 0);
	if (Chance(0x8000))
		sfxPlay3DSound(actor, 1001, -1, 0);
	else
		sfxPlay3DSound(actor, 1002, -1, 0);
}

static void calebThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void calebThinkGoto(DBloodActor* actor)
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
			aiNewState(actor, NAME_tinycalebSwimSearch);
		else
			aiNewState(actor, NAME_tinycalebSearch);
	}
	aiThinkTarget(actor);
}

static void calebThinkChase(DBloodActor* actor)
{
	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (actor->GetTarget() == nullptr)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_tinycalebSwimSearch);
		else
			aiNewState(actor, NAME_tinycalebSearch);
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
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_tinycalebSwimSearch);
		else
		{
			aiPlay3DSound(actor, 11000 + Random(4), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_tinycalebSearch);
		}
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, NAME_tinycalebSwimSearch);
		else
			aiNewState(actor, NAME_tinycalebSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : target->spr.pos.Z - actor->spr.pos.Z / nDist;
				if (nDist < 89.5625 && abs(nDeltaAngle) < DAngle1 * 5)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dvec, 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, NAME_tinycalebSwimAttack);
						else
							aiNewState(actor, NAME_tinycalebAttack);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, NAME_tinycalebSwimAttack);
							else
								aiNewState(actor, NAME_tinycalebAttack);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, NAME_tinycalebSwimDodge);
							else
								aiNewState(actor, NAME_tinycalebDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, NAME_tinycalebSwimAttack);
						else
							aiNewState(actor, NAME_tinycalebAttack);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, NAME_tinycalebSwimGoto);
	else
		aiNewState(actor, NAME_tinycalebGoto);
	if (Chance(0x2000))
		sfxPlay3DSound(actor, 10000 + Random(5), -1, 0);
	actor->SetTarget(nullptr);
}

static void calebThinkSwimGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, NAME_tinycalebSwimSearch);
	aiThinkTarget(actor);
}

static void calebThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_tinycalebSwimGoto);
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
		aiNewState(actor, NAME_tinycalebSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, NAME_tinycalebSwimSearch);
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);

		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x40 && abs(nDeltaAngle) < DAngle15)
					aiNewState(actor, NAME_tinycalebSwimAttack);
				else
					aiNewState(actor, NAME_tinycalebSwimMoveIn);
			}
		}
		else
			aiNewState(actor, NAME_tinycalebSwimMoveIn);
		return;
	}
	aiNewState(actor, NAME_tinycalebSwimGoto);
	actor->SetTarget(nullptr);
}

static void calebMoveSwimChase(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
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

static void calebSwimUnused(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto target = actor->GetTarget();
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
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

	double dz = target->spr.pos.Z - actor->spr.pos.Z;
	actor->vel.Z = -dz / 256;
}

static void calebSwimMoveIn(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto target = actor->GetTarget();
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
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

	double dz = target->spr.pos.Z - actor->spr.pos.Z;
	actor->vel.Z = dz / 32;
}

END_BLD_NS
