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

static void sub_72580(DBloodActor*);
static void sub_725A4(DBloodActor*);
static void sub_72850(DBloodActor*);
static void tchernobogThinkChase(DBloodActor*);


AISTATE tchernobogIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, sub_725A4, NULL };
AISTATE tchernobogSearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, sub_72580, &tchernobogIdle };
AISTATE tchernobogChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, tchernobogThinkChase, NULL };
AISTATE tchernobogRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tchernobogSearch };
AISTATE tcherno13A9B8 = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, sub_72850, &tchernobogIdle };
AISTATE tcherno13A9D4 = { kAiStateMove, 6, dword_279B54, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13A9F0 = { kAiStateChase, 6, dword_279B58, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13AA0C = { kAiStateChase, 7, dword_279B5C, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13AA28 = { kAiStateChase, 8, -1, 60, NULL, aiMoveTurn, NULL, &tchernobogChase };

static constexpr double Tchernnobog_XYOff = 350. / 16;

void sub_71A90(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->xspr.burnTime == 0)
		evPostActor(target, 0, kCallbackFXFlameLick);
	actBurnSprite(actor->GetOwner(), target, 40);
	if (Chance(0x6000))
		aiNewState(actor, &tcherno13A9D4);
}

void tchernobogBurnSeqCallback(int, DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	double height = actor->spr.scale.Y * pDudeInfo->eyeHeight * 0.25;
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	DVector3 pos(actor->spr.pos.XY(), height);

	DVector3 Aim(actor->spr.angle.ToVector(), actor->dudeSlope);
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

		pos += actor2->vel * nDist * (65536. / 0x1aaaaa);

		DVector3 tvec = pos;
		tvec.XY() += actor->spr.angle.ToVector() * nDist;
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
			DAngle nDeltaAngle = absangle(nAngle, actor->spr.angle);
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

void tchernobogBurnSeqCallback2(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	double height = actor->spr.scale.Y * pDudeInfo->eyeHeight * 0.25;
	
	DVector3 pos(actor->spr.pos.XY(), height);
	DVector3 Aim(actor->spr.angle.ToVector(), -actor->dudeSlope);
	DVector3 Aim2(Aim.XY(), 0);
	double nClosest = 0x7fffffff;

	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		auto pos2 = actor2->spr.pos;
		double nDist = (pos2 - pos).Length();
		if (nDist == 0 || nDist > 0x280)
			continue;

		pos += actor2->vel * nDist * (65536. / 0x1aaaaa);

		DVector3 tvec = pos;
		tvec.XY() += actor->spr.angle.ToVector() * nDist;
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
			DAngle nDeltaAngle = absangle(nAngle, actor->spr.angle);
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

static void sub_72580(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void sub_725A4(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	if (pDudeExtraE->active && pDudeExtraE->thinkTime < 10)
		pDudeExtraE->thinkTime++;
	else if (pDudeExtraE->thinkTime >= 10 && pDudeExtraE->active)
	{
		actor->xspr.goalAng += DAngle45;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &tcherno13AA28);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
			if (pPlayer->actor->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->actor->spr.pos;
			auto dvect = ppos.XY() - actor->spr.pos;
			auto pSector = pPlayer->actor->sector();
			DAngle nAngle = dvect.Angle();
			double nDist = dvect.Length();

			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.angle, nAngle);
			if (nDist < pDudeInfo->SeeDist() && abs(nDeltaAngle) <= pDudeInfo->Periphery())
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void sub_72850(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.angle, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, &tchernobogSearch);
	aiThinkTarget(actor);
}

static void tchernobogThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &tcherno13A9B8);
		return;
	}
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &tchernobogSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &tchernobogSearch);
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
				if (nDist < 0x1f0 && nDist > 0xd0 && nDeltaAngle < DAngle15)
					aiNewState(actor, &tcherno13AA0C);
				else if (nDist < 0xd0 && nDist > 0xb0 && nDeltaAngle < DAngle15)
					aiNewState(actor, &tcherno13A9D4);
				else if (nDist < 0xb0 && nDist > 0x50 && nDeltaAngle < DAngle15)
					aiNewState(actor, &tcherno13A9F0);
				return;
			}
		}
	}

	aiNewState(actor, &tcherno13A9B8);
	actor->SetTarget(nullptr);
}

END_BLD_NS
