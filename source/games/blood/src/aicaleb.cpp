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

static void calebThinkSearch(DBloodActor*);
static void calebThinkGoto(DBloodActor*);
static void calebThinkChase(DBloodActor*);
static void calebThinkSwimGoto(DBloodActor*);
static void calebThinkSwimChase(DBloodActor*);
static void sub_65D04(DBloodActor*);
static void sub_65F44(DBloodActor*);
static void sub_661E0(DBloodActor*);

AISTATE tinycalebIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tinycalebChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, calebThinkChase, NULL };
AISTATE tinycalebDodge = { kAiStateMove, 6, -1, 90, NULL, aiMoveDodge, NULL, &tinycalebChase };
AISTATE tinycalebGoto = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, calebThinkGoto, &tinycalebIdle };
AISTATE tinycalebAttack = { kAiStateChase, 0, nAttackClient, 120, NULL, NULL, NULL, &tinycalebChase };
AISTATE tinycalebSearch = { kAiStateSearch, 6, -1, 120, NULL, aiMoveForward, calebThinkSearch, &tinycalebIdle };
AISTATE tinycalebRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tinycalebDodge };
AISTATE tinycalebTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &tinycalebDodge };
AISTATE tinycalebSwimIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tinycalebSwimChase = { kAiStateChase, 8, -1, 0, NULL, sub_65D04, calebThinkSwimChase, NULL };
AISTATE tinycalebSwimDodge = { kAiStateMove, 8, -1, 90, NULL, aiMoveDodge, NULL, &tinycalebSwimChase };
AISTATE tinycalebSwimGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, calebThinkSwimGoto, &tinycalebSwimIdle };
AISTATE tinycalebSwimSearch = { kAiStateSearch, 8, -1, 120, NULL, aiMoveForward, calebThinkSearch, &tinycalebSwimIdle };
AISTATE tinycalebSwimAttack = { kAiStateChase, 10, nAttackClient, 0, NULL, NULL, NULL, &tinycalebSwimChase };
AISTATE tinycalebSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tinycalebSwimDodge };
AISTATE tinycaleb139660 = { kAiStateOther, 8, -1, 120, NULL, sub_65F44, calebThinkSwimChase, &tinycalebSwimChase };
AISTATE tinycaleb13967C = { kAiStateOther, 8, -1, 0, NULL, sub_661E0, calebThinkSwimChase, &tinycalebSwimChase };
AISTATE tinycaleb139698 = { kAiStateOther, 8, -1, 120, NULL, aiMoveTurn, NULL, &tinycalebSwimChase };

void SeqAttackCallback(int, DBloodActor* actor)
{
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	int dz = actor->dudeSlope;
	dx += Random2(1500);
	dy += Random2(1500);
	dz += Random2(1500);
	for (int i = 0; i < 2; i++)
	{
		int r1 = Random3(500);
		int r2 = Random3(1000);
		int r3 = Random3(1000);
		actFireVector(actor, 0, 0, dx + r3, dy + r2, dz + r1, kVectorShell);
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
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);

	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
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
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
		{
			aiPlay3DSound(actor, 11000 + Random(4), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &tinycalebSearch);
		}
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
		return;
	}

	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : DivScale(target->int_pos().Z - actor->int_pos().Z, nDist, 10);
				if (nDist < 0x599 && abs(nDeltaAngle) < 28)
				{
					int dx = dvec.X * worldtoint;
					int dy = dvec.Y * worldtoint;
					int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &tinycalebSwimAttack);
						else
							aiNewState(actor, &tinycalebAttack);
						break;
					case 3:
						if (actor->spr.type != gHitInfo.actor()->spr.type)
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &tinycalebSwimAttack);
							else
								aiNewState(actor, &tinycalebAttack);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &tinycalebSwimDodge);
							else
								aiNewState(actor, &tinycalebDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &tinycalebSwimAttack);
						else
							aiNewState(actor, &tinycalebAttack);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, &tinycalebSwimGoto);
	else
		aiNewState(actor, &tinycalebGoto);
	if (Chance(0x2000))
		sfxPlay3DSound(actor, 10000 + Random(5), -1, 0);
	actor->SetTarget(nullptr);
}

static void calebThinkSwimGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &tinycalebSwimSearch);
	aiThinkTarget(actor);
}

static void calebThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &tinycalebSwimGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, DAngle::fromBuild(nAngle));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &tinycalebSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &tinycalebSwimSearch);
		return;
	}

	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &tinycalebSwimAttack);
				else
					aiNewState(actor, &tinycaleb13967C);
			}
			else
				aiNewState(actor, &tinycaleb13967C);
		}
		return;
	}
	aiNewState(actor, &tinycalebSwimGoto);
	actor->SetTarget(nullptr);
}

static void sub_65D04(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.angle += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	if (Random(64) < 32 && nDist <= 0x400)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += FixedToFloat(nAccel);
		else
			t1 += FixedToFloat(nAccel * 0.25);
	});

}

static void sub_65F44(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto target = actor->GetTarget();
	int z = actor->int_pos().Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->int_pos().Z + getDudeInfo(target->spr.type)->eyeHeight;
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	int dz = z2 - z;
	if (Chance(0x600) && nDist <= 0x400)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += FixedToFloat(nAccel);
	});

	actor->set_int_bvel_z(-dz);
}

static void sub_661E0(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto target = actor->GetTarget();
	int z = actor->int_pos().Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->int_pos().Z + getDudeInfo(target->spr.type)->eyeHeight;
	auto nAng = deltaangle(actor->spr.angle, actor->xspr.goalAng);
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
	{
		actor->spr.angle += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	int nDist = approxDist(dvec);
	int dz = (z2 - z) << 3;
	if (Chance(0x4000) && nDist <= 0x400)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += FixedToFloat(nAccel * 0.5);
	});

	actor->set_int_bvel_z(dz);
}

END_BLD_NS
