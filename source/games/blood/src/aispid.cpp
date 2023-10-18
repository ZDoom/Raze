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

static void spidBlindEffect(DBloodActor* actor, int nBlind, int max)
{
	if (actor->IsPlayerActor())
	{
		nBlind <<= 4;
		max <<= 4;
		auto pPlayer = getPlayer(actor);
		if (pPlayer->blindEffect < max)
		{
			pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect + nBlind, max);
		}
	}
}

void SpidBiteSeqCallback(DBloodActor* actor)
{
	DVector3 vec(actor->spr.Angles.Yaw.ToVector(), 0);

	vec.X += Random2F(2000, 14);
	vec.Y += Random2F(2000, 14);
	vec.Z = Random2F(2000, 14);
	assert(actor->IsDudeActor());
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto const target = actor->GetTarget();
	if (target->IsPlayerActor())
	{
		int hit = HitScan(actor, actor->spr.pos.Z, DVector3(vec.XY(), 0), CLIPMASK1, 0);
		if (hit == 3 && gHitInfo.actor()->IsPlayerActor())
		{
			vec.Z += target->spr.pos.Z - actor->spr.pos.Z;
			auto pPlayer = getPlayer(target);
			switch (actor->GetType())
			{
			case kDudeSpiderBrown:
				actFireVector(actor, 0, 0, vec, kVectorSpiderBite);
				if (target->IsPlayerActor() && !pPlayer->godMode && powerupCheck(pPlayer, kPwUpDeathMask) <= 0 && Chance(0x4000))
					powerupActivate(pPlayer, kPwUpDeliriumShroom);
				break;
			case kDudeSpiderRed:
				actFireVector(actor, 0, 0, vec, kVectorSpiderBite);
				if (Chance(0x5000)) spidBlindEffect(target, 4, 16);
				break;
			case kDudeSpiderBlack:
				actFireVector(actor, 0, 0, vec, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			case kDudeSpiderMother:
				actFireVector(actor, 0, 0, vec, kVectorSpiderBite);

				vec.X += Random2F(2000, 14);
				vec.Y += Random2F(2000, 14);
				vec.Z += Random2F(2000, 14);
				actFireVector(actor, 0, 0, vec, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			}
		}

	}
}

void SpidJumpSeqCallback(DBloodActor* actor)
{
	DVector3 vec(actor->spr.Angles.Yaw.ToVector(), 0);

	vec.X += Random2F(200, 14);
	vec.Y += Random2F(200, 14);
	vec.Z = Random2F(200, 14);

	assert(actor->IsDudeActor());
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	if (target->IsPlayerActor()) {
		vec.Z += target->spr.pos.Z - actor->spr.pos.Z;
		switch (actor->GetType()) {
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
			// This value seems a bit extreme, but that's how it was...
			actor->vel = vec * 16384;
			break;
		}
	}
}

void SpidBirthSeqCallback(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();

	DBloodActor* spawned = nullptr;
	if (target->IsPlayerActor() && actor->dudeExtra.birthCounter < 10)
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		if (nDist < 0x1a0 && nDist > 0x140 && nDeltaAngle < pDudeInfo->Periphery())
			spawned = actSpawnDude(actor, kDudeSpiderRed, actor->clipdist * 0.25);
		else if (nDist < 0x140 && nDist > 0xc0 && nDeltaAngle < pDudeInfo->Periphery())
			spawned = actSpawnDude(actor, kDudeSpiderBrown, actor->clipdist * 0.25);
		else if (nDist < 0xc0 && nDeltaAngle < pDudeInfo->Periphery())
			spawned = actSpawnDude(actor, kDudeSpiderBrown, actor->clipdist * 0.25);

		if (spawned)
		{
			actor->dudeExtra.birthCounter++;
			spawned->SetOwner(spawned);
			if (AllowedKillType(spawned)) Level.addKillCount();
		}
	}

}

void spidThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void spidThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
		aiNewState(actor, NAME_spidSearch);
	aiThinkTarget(actor);
}

void spidThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, NAME_spidGoto);
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
		aiNewState(actor, NAME_spidSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, NAME_spidSearch);
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

				switch (actor->GetType()) {
				case kDudeSpiderRed:
					if (nDist < 57.5625 && nDeltaAngle < DAngle15)
						aiNewState(actor, NAME_spidBite);
					break;
				case kDudeSpiderBrown:
				case kDudeSpiderBlack:
					if (nDist < 115.1875 && nDist > 57.5625 && nDeltaAngle < DAngle15)
						aiNewState(actor, NAME_spidJump);
					else if (nDist < 57.5625 && nDeltaAngle < DAngle15)
						aiNewState(actor, NAME_spidBite);
					break;
				case kDudeSpiderMother:
					if (nDist < 115.1875 && nDist > 57.5625 && nDeltaAngle < DAngle15)
						aiNewState(actor, NAME_spidJump);
					else if (Chance(0x8000))
						aiNewState(actor, NAME_spidBirth);
					break;
				}

				return;
			}
		}
	}

	aiNewState(actor, NAME_spidGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
