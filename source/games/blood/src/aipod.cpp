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

void podPlaySound1(DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2503, -1, 0);
}

void podPlaySound2(DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2500, -1, 0);
}

void podAttack(DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dv = target->spr.pos - actor->spr.pos;
	dv.X += Random2F(1000, 4);
	dv.Y += Random2F(1000, 4);
	double nDist = dv.XY().Length();
	DBloodActor* pMissile = nullptr;
	switch (actor->GetType())
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
			seqSpawn(68, pMissile);
		break;
	case kDudePodFire:
		dv.Z += 31.25;
		if (pDudeInfo->SeeDist() * 1.6 < nDist)
		{
			sfxPlay3DSound(actor, 2454, -1, 0);
			pMissile = actFireThing(actor, 0., -500., dv.Z / 32768 - 0.22125, kThingPodFireBall, nDist * (2048. / 64800));
		}
		if (pMissile)
			seqSpawn(22, pMissile);
		break;
	}
	for (int i = 0; i < 4; i++)
		fxSpawnPodStuff(actor, 240);
}

void podExplode(DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2502, -1, 0);
	int nDist, nBurn;
	DAMAGE_TYPE dmgType;
	switch (actor->GetType()) {
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

void aiPodSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void aiPodMove(DBloodActor* actor)
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
		switch (actor->GetType()) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, NAME_podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, NAME_tentacleSearch);
			break;
		}
	}
	aiThinkTarget(actor);
}

void aiPodChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr) {
		switch (actor->GetType()) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, NAME_podMove);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, NAME_tentacleMove);
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

		switch (actor->GetType()) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, NAME_podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, NAME_tentacleSearch);
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
				if (nDeltaAngle < DAngle15 && target->GetType() != kDudePodGreen && target->GetType() != kDudePodFire) {
					switch (actor->GetType()) {
					case kDudePodGreen:
					case kDudePodFire:
						aiNewState(actor, NAME_podStartChase);
						break;
					case kDudeTentacleGreen:
					case kDudeTentacleFire:
						aiNewState(actor, NAME_tentacleStartChase);
						break;
					}
				}
				return;
			}
		}
	}

	switch (actor->GetType()) {
	case kDudePodGreen:
	case kDudePodFire:
		aiNewState(actor, NAME_podMove);
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		aiNewState(actor, NAME_tentacleMove);
		break;
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
