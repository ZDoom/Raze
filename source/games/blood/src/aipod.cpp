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

static void aiPodSearch(DBloodActor* actor);
static void aiPodMove(DBloodActor* actor);
static void aiPodChase(DBloodActor* actor);

AISTATE podIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE podMove = { kAiStateMove, 7, -1, 3600, NULL, aiMoveTurn, aiPodMove, &podSearch };
AISTATE podSearch = { kAiStateSearch, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, &podSearch };
AISTATE podStartChase = { kAiStateChase, 8, nPodStartChaseClient, 600, NULL, NULL, NULL, &podChase };
AISTATE podRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &podChase };
AISTATE podChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };
AISTATE tentacleIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tentacle13A6A8 = { kAiStateOther, 7, dword_279B3C, 0, NULL, NULL, NULL, &tentacle13A6C4 };
AISTATE tentacle13A6C4 = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacle13A6E0 = { kAiStateOther, 8, dword_279B40, 0, NULL, NULL, NULL, &tentacle13A6FC };
AISTATE tentacle13A6FC = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleIdle };
AISTATE tentacleMove = { kAiStateOther, 8, -1, 3600, NULL, aiMoveTurn, aiPodMove, &tentacleSearch };
AISTATE tentacleSearch = { kAiStateOther, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, NULL };
AISTATE tentacleStartChase = { kAiStateOther, 6, nTentacleStartSearchClient, 120, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };

void sub_6FF08(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2503, -1, 0);
}

void sub_6FF54(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2500, -1, 0);
}

void podAttack(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dv = target->spr.pos - actor->spr.pos;
	dv.X += Random2F(1000, 4);
	dv.Y += Random2F(1000, 4);
	double nDist = dv.XY().Length();
	DBloodActor* pMissile = nullptr;
	switch (actor->spr.type)
	{
	case kDudePodGreen:
		dv.Z += 31.25;
		if (pDudeInfo->SeeDist() * 1.6 < nDist)
		{
			if (Chance(0x8000))
				sfxPlay3DSound(actor, 2474, -1, 0);
			else
				sfxPlay3DSound(actor, 2475, -1, 0);
			pMissile = actFireThing(actor, 0., -500., dv.Z / 32768 - 0.22125, kThingPodGreenBall, nDist * (2048. / 64800));
		}
		if (pMissile)
			seqSpawn(68, pMissile, -1);
		break;
	case kDudePodFire:
		dv.Z += 31.25;
		if (pDudeInfo->SeeDist() * 1.6 < nDist)
		{
			sfxPlay3DSound(actor, 2454, -1, 0);
			pMissile = actFireThing(actor, 0., -500., dv.Z / 32768 - 0.22125, kThingPodFireBall, nDist * (2048. / 64800));
		}
		if (pMissile)
			seqSpawn(22, pMissile, -1);
		break;
	}
	for (int i = 0; i < 4; i++)
		fxSpawnPodStuff(actor, 240);
}

void sub_70284(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2502, -1, 0);
	int nDist, nBurn;
	DAMAGE_TYPE dmgType;
	switch (actor->spr.type) {
	case kDudeTentacleGreen:
	default: // ???
		nBurn = 0;
		dmgType = kDamageBullet;
		nDist = 50;
		break;
	case kDudeTentacleFire: // ???
		nBurn = (gGameOptions.nDifficulty * 120) >> 2;
		dmgType = kDamageExplode;
		nDist = 75;
		break;
	}
	actRadiusDamage(actor, actor->spr.pos, actor->sector(), nDist, 1, 5 * (1 + gGameOptions.nDifficulty), dmgType, 2, nBurn);
}

static void aiPodSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void aiPodMove(DBloodActor* actor)
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
	{
		switch (actor->spr.type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleSearch);
			break;
		}
	}
	aiThinkTarget(actor);
}

static void aiPodChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr) {
		switch (actor->spr.type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podMove);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleMove);
			break;
		}
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
	if (target->xspr.health == 0) {

		switch (actor->spr.type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleSearch);
			break;
		}
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
				if (nDeltaAngle < DAngle15 && target->spr.type != kDudePodGreen && target->spr.type != kDudePodFire) {
					switch (actor->spr.type) {
					case kDudePodGreen:
					case kDudePodFire:
						aiNewState(actor, &podStartChase);
						break;
					case kDudeTentacleGreen:
					case kDudeTentacleFire:
						aiNewState(actor, &tentacleStartChase);
						break;
					}
				}
				return;
			}
		}
	}

	switch (actor->spr.type) {
	case kDudePodGreen:
	case kDudePodFire:
		aiNewState(actor, &podMove);
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		aiNewState(actor, &tentacleMove);
		break;
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
