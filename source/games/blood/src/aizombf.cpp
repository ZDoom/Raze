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

void zombfHackSeqCallback(DBloodActor* actor)
{
	if (actor->GetType() != kDudeZombieButcher)
		return;
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	double height = pDudeInfo->eyeHeight * actor->spr.scale.Y * 0.25;
	DUDEINFO* pDudeInfoT = getDudeInfo(target);
	double height2 = pDudeInfoT->eyeHeight * target->spr.scale.Y * 0.25;
	actFireVector(actor, 0, 0, DVector3(actor->spr.Angles.Yaw.ToVector() * 64, height - height2), kVectorCleaver);
}

void PukeSeqCallback(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	DUDEINFO* pDudeInfoT = getDudeInfo(target);

	DVector2 dv = (actor->xspr.TargetPos.XY() - actor->spr.pos.XY()).Resized(64);

	double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
	double height2 = (pDudeInfoT->eyeHeight * target->spr.scale.Y);
	double z = (height - height2) * 0.25;

	sfxPlay3DSound(actor, 1203, 1, 0);
	actFireMissile(actor, 0, -z, DVector3(dv, 0), kMissilePukeGreen);
}

void ThrowSeqCallback(DBloodActor* actor)
{
	actFireMissile(actor, 0, 0, DVector3(actor->spr.Angles.Yaw.ToVector(), 0), kMissileButcherKnife);
}

void zombfThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void zombfThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, NAME_zombieFSearch);
	aiThinkTarget(actor);
}

void zombfThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_zombieFGoto);
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dv = target->spr.pos - actor->spr.pos;
	auto nAngle = dv.Angle();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, NAME_zombieFSearch);
		return;
	}
	if (target->IsPlayerActor() && (powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0 || powerupCheck(getPlayer(target), kPwUpDeathMaskUseless) > 0))
	{
		aiNewState(actor, NAME_zombieFSearch);
		return;
	}
	double nDist = dv.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x100 && nDist > 0xe0 && abs(nDeltaAngle) < DAngle15)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv.XY(), 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						aiNewState(actor, NAME_zombieFThrow);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
							aiNewState(actor, NAME_zombieFThrow);
						else
							aiNewState(actor, NAME_zombieFDodge);
						break;
					default:
						aiNewState(actor, NAME_zombieFThrow);
						break;
					}
				}
				else if (nDist < 0x140 && nDist > 0x60 && nDeltaAngle < DAngle15)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv.XY(), 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						aiNewState(actor, NAME_zombieFPuke);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
							aiNewState(actor, NAME_zombieFPuke);
						else
							aiNewState(actor, NAME_zombieFDodge);
						break;
					default:
						aiNewState(actor, NAME_zombieFPuke);
						break;
					}
				}
				else if (nDist < 0x40 && nDeltaAngle < DAngle15)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dv.XY(), 0), CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						aiNewState(actor, NAME_zombieFHack);
						break;
					case 3:
						if (actor->GetType() != gHitInfo.actor()->GetType())
							aiNewState(actor, NAME_zombieFHack);
						else
							aiNewState(actor, NAME_zombieFDodge);
						break;
					default:
						aiNewState(actor, NAME_zombieFHack);
						break;
					}
				}
				return;
			}
		}
	}

	aiNewState(actor, NAME_zombieFSearch);
	actor->SetTarget(nullptr);
}

END_BLD_NS
