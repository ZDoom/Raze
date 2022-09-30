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

static void cerberusThinkSearch(DBloodActor* actor);
static void cerberusThinkTarget(DBloodActor* actor);
static void cerberusThinkGoto(DBloodActor* actor);
static void cerberusThinkChase(DBloodActor* actor);


AISTATE cerberusIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, cerberusThinkTarget, NULL };
AISTATE cerberusSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, cerberusThinkSearch, &cerberusIdle };
AISTATE cerberusChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, cerberusThinkChase, NULL };
AISTATE cerberusRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cerberusSearch };
AISTATE cerberusTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &cerberusSearch };
AISTATE cerberusGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, cerberusThinkGoto, &cerberusIdle };
AISTATE cerberusBite = { kAiStateChase, 6, nCerberusBiteClient, 60, NULL, NULL, NULL, &cerberusChase };
AISTATE cerberusBurn = { kAiStateChase, 6, nCerberusBurnClient, 60, NULL, NULL, NULL, &cerberusChase };
AISTATE cerberus3Burn = { kAiStateChase, 6, nCerberusBurnClient2, 60, NULL, NULL, NULL, &cerberusChase };
AISTATE cerberus2Idle = { kAiStateIdle, 0, -1, 0, NULL, NULL, cerberusThinkTarget, NULL };
AISTATE cerberus2Search = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, cerberusThinkSearch, &cerberus2Idle };
AISTATE cerberus2Chase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, cerberusThinkChase, NULL };
AISTATE cerberus2Recoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &cerberus2Search };
AISTATE cerberus2Goto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, cerberusThinkGoto, &cerberus2Idle };
AISTATE cerberus2Bite = { kAiStateChase, 6, nCerberusBiteClient, 60, NULL, NULL, NULL, &cerberus2Chase };
AISTATE cerberus2Burn = { kAiStateChase, 6, nCerberusBurnClient, 60, NULL, NULL, NULL, &cerberus2Chase };
AISTATE cerberus4Burn = { kAiStateChase, 6, nCerberusBurnClient2, 60, NULL, NULL, NULL, &cerberus2Chase };
AISTATE cerberus139890 = { kAiStateOther, 7, -1, 120, NULL, aiMoveTurn, NULL, &cerberusChase };
AISTATE cerberus1398AC = { kAiStateOther, 7, -1, 120, NULL, aiMoveTurn, NULL, &cerberusChase };

static constexpr double Cerberus_XYOff = 350. / 16;
static constexpr double Cerberus_ZOff = 100. / 256;

void cerberusBiteSeqCallback(int, DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	DVector3 vec;
	vec.XY() = actor->spr.angle.ToVector() * 64;
	vec.Z = target->spr.pos.Z - actor->spr.pos.Z;
	actFireVector(actor, Cerberus_XYOff, -Cerberus_ZOff, vec, kVectorCerberusHack);
	actFireVector(actor, -Cerberus_XYOff, 0, vec, kVectorCerberusHack);
	actFireVector(actor, 0, 0, vec, kVectorCerberusHack);
}

void cerberusBurnSeqCallback(int, DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	double height = pDudeInfo->eyeHeight * actor->spr.yrepeat * REPEAT_SCALE * 0.25;
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	DVector3 pos(actor->spr.pos.XY(), height);
	//auto pos = actor->spr.pos.plusZ(height); //  what it probably should be

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
					Aim.Z = tz1 / 64.;
			}
		}
	}
	switch (actor->spr.type) {
	case kDudeCerberusTwoHead:
		actFireMissile(actor, -Cerberus_XYOff, 0, Aim, kMissileFireballCerberus);
		actFireMissile(actor, Cerberus_XYOff, -Cerberus_ZOff, Aim, kMissileFireballCerberus);
		break;
	case kDudeCerberusOneHead:
		actFireMissile(actor, Cerberus_XYOff, -Cerberus_ZOff, Aim, kMissileFireballCerberus);
		break;
	}
}

void cerberusBurnSeqCallback2(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	double height = pDudeInfo->eyeHeight * actor->spr.yrepeat * REPEAT_SCALE * 0.25;

	DVector3 pos(actor->spr.pos.XY(), height);
	//auto pos = actor->spr.pos.plusZ(height); //  what it probably should be

	DVector3 Aim(actor->spr.angle.ToVector(), actor->dudeSlope);
	DVector3 Aim2(Aim.XY(), 0);

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
				DUDEINFO* pDudeInfo2 = getDudeInfo(actor2->spr.type);
				double height1 = (pDudeInfo2->aimHeight * actor2->spr.yrepeat) * REPEAT_SCALE;
				double tz1 = actor2->spr.pos.Z - height - actor->spr.pos.Z;

				if (cansee(pos, actor->sector(), pos2, actor2->sector()))
				{
					nClosest = nDist2;
					Aim.XY() = nAngle.ToVector();
					Aim.Z = tz1 / nDist;
				}
				else
					Aim.Z = tz1 / 64.;
			}
		}
	}
	switch (actor->spr.type) {

	case kDudeCerberusTwoHead:
		actFireMissile(actor, Cerberus_XYOff, -Cerberus_ZOff, Aim, kMissileFlameHound);
		actFireMissile(actor, -Cerberus_XYOff, 0, Aim2, kMissileFlameHound);
		break;
	case kDudeCerberusOneHead:
		actFireMissile(actor, Cerberus_XYOff, -Cerberus_ZOff, Aim, kMissileFlameHound);
		break;
	}
}

static void cerberusThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void cerberusThinkTarget(DBloodActor* actor)
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
		if (actor->spr.type == kDudeCerberusTwoHead)
			aiNewState(actor, &cerberus139890);
		else
			aiNewState(actor, &cerberus1398AC);
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
			double nDist = dvect.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.angle, dvect.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
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

static void cerberusThinkGoto(DBloodActor* actor)
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
	{
		switch (actor->spr.type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusSearch);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Search);
			break;
		}
	}
	aiThinkTarget(actor);
}

static void cerberusThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr) {
		switch (actor->spr.type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusGoto);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Goto);
			break;
		}
		return;
	}

	///assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
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

	if (target->xspr.health == 0) {
		switch (actor->spr.type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusSearch);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Search);
			break;
		}
		return;
	}

	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0) {
		switch (actor->spr.type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusSearch);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Search);
			break;
		}
		return;
	}


	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.angle, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());

				if (nDist < 0x1b0 && nDist > 0xd0 && nDeltaAngle < DAngle15) 
				{
					switch (actor->spr.type) {
					case kDudeCerberusTwoHead:
						aiNewState(actor, &cerberusBurn);
						break;
					case kDudeCerberusOneHead:
						aiNewState(actor, &cerberus2Burn);
						break;
					}
				}

				else if (nDist < 0xb0 && nDist > 0x50 && nDeltaAngle < DAngle15)
				{
					switch (actor->spr.type) {
					case kDudeCerberusTwoHead:
						aiNewState(actor, &cerberus3Burn);
						break;
					case kDudeCerberusOneHead:
						aiNewState(actor, &cerberus4Burn);
						break;
					}
				}
				else if (nDist < 0x20 && nDeltaAngle < DAngle15)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dvec, 0), CLIPMASK1, 0);
					switch (actor->spr.type) {
					case kDudeCerberusTwoHead:
						switch (hit) {
						case -1:
							aiNewState(actor, &cerberusBite);
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeHellHound)
								aiNewState(actor, &cerberusBite);
							break;
						case 0:
						case 4:
							break;
						default:
							aiNewState(actor, &cerberusBite);
							break;
						}
						break;
					case kDudeCerberusOneHead:
						switch (hit) {
						case -1:
							aiNewState(actor, &cerberus2Bite);
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeHellHound)
								aiNewState(actor, &cerberus2Bite);
							break;
						case 0:
						case 4:
							break;
						default:
							aiNewState(actor, &cerberus2Bite);
							break;
						}
						break;
					}
				}
				return;
			}
		}
	}

	switch (actor->spr.type) {
	case kDudeCerberusTwoHead:
		aiNewState(actor, &cerberusGoto);
		break;
	case kDudeCerberusOneHead:
		aiNewState(actor, &cerberus2Goto);
		break;
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
