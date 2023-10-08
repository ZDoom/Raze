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

constexpr double Tchernnobog_XYOff = 350. / 16;

void tchernobogFire(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->xspr.burnTime == 0)
		evPostActor(target, 0, AF(fxFlameLick));
	actBurnSprite(actor->GetOwner(), target, 40);
	if (Chance(0x6000))
		aiNewState(actor, NAME_tchernobogBurn1);
}

void tchernobogBurnSeqCallback(DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	double height = actor->spr.scale.Y * pDudeInfo->eyeHeight * 0.25;
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	DVector3 pos(actor->spr.pos.XY(), height);

	DVector3 Aim(actor->spr.Angles.Yaw.ToVector(), actor->dudeSlope);
	double nClosest = 0x7fffffff;

	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		if (actor == actor2 || !(actor2->spr.flags & 8))
			continue;

		auto pos2 = actor2->spr.pos;
		double nDist = (pos2 - pos).Length();
		if (nDist == 0 || nDist > 0x280)
			continue;

		pos2 += actor->vel * nDist * (65536. / 0x1aaaaa);

		DVector3 tvec = pos;
		tvec.XY() += actor->spr.Angles.Yaw.ToVector() * nDist;
		tvec.Z += actor->dudeSlope * nDist;

		double tsr = nDist * 9.23828125;
		double top, bottom;
		GetActorExtents(actor2, &top, &bottom);
		if (tvec.Z - tsr > bottom || tvec.Z + tsr < top)
			continue;

		double nDist2 = (tvec - pos2).Length();
		if (nDist2 < nClosest)
		{
			DAngle nAngle = (pos2.XY() - pos.XY()).Angle();
			DAngle nDeltaAngle = absangle(nAngle, actor->spr.Angles.Yaw);
			if (nDeltaAngle <= DAngle45)
			{
				double tz1 = actor2->spr.pos.Z - actor->spr.pos.Z;

				if (cansee(pos, actor->sector(), pos2, actor2->sector()))
				{
					nClosest = nDist2;
					Aim.XY() = nAngle.ToVector();
					Aim.Z = tz1 / nDist;
				}
				else
					Aim.Z = tz1 / nDist;
			}
		}
	}
	actFireMissile(actor, -Tchernnobog_XYOff, 0., Aim, kMissileFireballTchernobog);
	actFireMissile(actor, Tchernnobog_XYOff, 0., Aim, kMissileFireballTchernobog);
}

void tchernobogBurnSeqCallback2(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	double height = actor->spr.scale.Y * pDudeInfo->eyeHeight * 0.25;
	
	DVector3 pos(actor->spr.pos.XY(), height);
	DVector3 Aim(actor->spr.Angles.Yaw.ToVector(), -actor->dudeSlope);
	DVector3 Aim2(Aim.XY(), 0);
	double nClosest = 0x7fffffff;

	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		auto pos2 = actor2->spr.pos;
		double nDist = (pos2 - pos).Length();
		if (nDist == 0 || nDist > 0x280)
			continue;

		pos2 += actor->vel * nDist * (65536. / 0x1aaaaa);

		DVector3 tvec = pos;
		tvec.XY() += actor->spr.Angles.Yaw.ToVector() * nDist;
		tvec.Z += actor->dudeSlope * nDist;

		double tsr = nDist * 9.23828125;
		double top, bottom;
		GetActorExtents(actor2, &top, &bottom);
		if (tvec.Z - tsr > bottom || tvec.Z + tsr < top)
			continue;

		double nDist2 = (tvec - pos2).Length();
		if (nDist2 < nClosest)
		{
			DAngle nAngle = (pos2.XY() - pos.XY()).Angle();
			DAngle nDeltaAngle = absangle(nAngle, actor->spr.Angles.Yaw);
			if (nDeltaAngle <= DAngle45)
			{
				double tz1 = actor2->spr.pos.Z - actor->spr.pos.Z;

				if (cansee(pos, actor->sector(), pos2, actor2->sector()))
				{
					nClosest = nDist2;
					Aim.XY() = nAngle.ToVector();
					Aim.Z = -tz1 / nDist;
				}
				else
					Aim.Z = -tz1 / nDist;
			}
		}
	}
	actFireMissile(actor, Tchernnobog_XYOff, 0, Aim, kMissileFireballTchernobog);
	actFireMissile(actor, -Tchernnobog_XYOff, 0, Aim2, kMissileFireballTchernobog);
}

void tchernobogThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void tchernobogThinkTarget(DBloodActor* actor)
{
	if (!(actor->IsDudeActor())) {
		Printf(PRINT_HIGH, "actor->IsDudeActor()");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	
	if (actor->dudeExtra.active && actor->dudeExtra.thinkTime < 10)
		actor->dudeExtra.thinkTime++;
	else if (actor->dudeExtra.thinkTime >= 10 && actor->dudeExtra.active)
	{
		actor->xspr.goalAng += DAngle45;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, NAME_tchernobogTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			if (pPlayer->GetActor()->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->GetActor()->spr.pos;
			auto dvect = ppos.XY() - actor->spr.pos;
			auto pSector = pPlayer->GetActor()->sector();
			DAngle nAngle = dvect.Angle();
			double nDist = dvect.Length();

			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
			if (nDist < pDudeInfo->SeeDist() && abs(nDeltaAngle) <= pDudeInfo->Periphery())
			{
				actor->dudeExtra.thinkTime = 0;
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				actor->dudeExtra.thinkTime = 0;
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

void tchernobogThinkGoto(DBloodActor* actor)
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
		aiNewState(actor, NAME_tchernobogSearch);
	aiThinkTarget(actor);
}

void tchernobogThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_tchernobogGoto);
		return;
	}
	if (!(actor->IsDudeActor())) {
		Printf(PRINT_HIGH, "actor->IsDudeActor()");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, NAME_tchernobogSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, NAME_tchernobogSearch);
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
				if (nDist < 0x1f0 && nDist > 0xd0 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_tchernobogFireAtk);
				else if (nDist < 0xd0 && nDist > 0xb0 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_tchernobogBurn1);
				else if (nDist < 0xb0 && nDist > 0x50 && nDeltaAngle < DAngle15)
					aiNewState(actor, NAME_tchernobogBurn2);
				return;
			}
		}
	}

	aiNewState(actor, NAME_tchernobogGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
