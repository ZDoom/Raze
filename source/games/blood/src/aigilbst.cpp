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

static void gillThinkSearch(DBloodActor*);
static void gillThinkGoto(DBloodActor*);
static void gillThinkChase(DBloodActor*);
static void gillThinkSwimGoto(DBloodActor*);
static void gillThinkSwimChase(DBloodActor*);
static void sub_6CB00(DBloodActor*);
static void sub_6CD74(DBloodActor*);
static void sub_6D03C(DBloodActor*);


AISTATE gillBeastIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastChase = { kAiStateChase, 9, -1, 0, NULL, aiMoveForward, gillThinkChase, NULL };
AISTATE gillBeastDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastChase };
AISTATE gillBeastGoto = { kAiStateMove, 9, -1, 600, NULL, aiMoveForward, gillThinkGoto, &gillBeastIdle };
AISTATE gillBeastBite = { kAiStateChase, 6, nGillBiteClient, 120, NULL, NULL, NULL, &gillBeastChase };
AISTATE gillBeastSearch = { kAiStateMove, 9, -1, 120, NULL, aiMoveForward, gillThinkSearch, &gillBeastIdle };
AISTATE gillBeastRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gillBeastDodge };
AISTATE gillBeastSwimIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE gillBeastSwimChase = { kAiStateChase, 10, -1, 0, NULL, sub_6CB00, gillThinkSwimChase, NULL };
AISTATE gillBeastSwimDodge = { kAiStateMove, 10, -1, 90, NULL, aiMoveDodge, NULL, &gillBeastSwimChase };
AISTATE gillBeastSwimGoto = { kAiStateMove, 10, -1, 600, NULL, aiMoveForward, gillThinkSwimGoto, &gillBeastSwimIdle };
AISTATE gillBeastSwimSearch = { kAiStateSearch, 10, -1, 120, NULL, aiMoveForward, gillThinkSearch, &gillBeastSwimIdle };
AISTATE gillBeastSwimBite = { kAiStateChase, 7, nGillBiteClient, 0, NULL, NULL, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeastSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gillBeastSwimDodge };
AISTATE gillBeast13A138 = { kAiStateOther, 10, -1, 120, NULL, sub_6CD74, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A154 = { kAiStateOther, 10, -1, 0, NULL, sub_6D03C, gillThinkSwimChase, &gillBeastSwimChase };
AISTATE gillBeast13A170 = { kAiStateOther, 10, -1, 120, NULL, NULL, aiMoveTurn, &gillBeastSwimChase };

void GillBiteSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dx = bcos(actor->spr.ang);
	int dy = bsin(actor->spr.ang);
	int dz = actor->spr.pos.Z - target->spr.pos.Z;
	dx += Random3(2000);
	dy += Random3(2000);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGillBite);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGillBite);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGillBite);
}

static void gillThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void gillThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);

	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &gillBeastSwimSearch);
		else
			aiNewState(actor, &gillBeastSearch);
	}
	aiThinkTarget(actor);
}

static void gillThinkChase(DBloodActor* actor)
{
	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (actor->GetTarget() == nullptr)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &gillBeastSwimSearch);
		else
			aiNewState(actor, &gillBeastSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (actor->xspr.health == 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &gillBeastSwimSearch);
		else
			aiNewState(actor, &gillBeastSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &gillBeastSwimSearch);
		else
			aiNewState(actor, &gillBeastSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : DivScale(target->spr.pos.Z - actor->spr.pos.Z, nDist, 10);
				if (nDist < 921 && abs(nDeltaAngle) < 28)
				{
					int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &gillBeastSwimBite);
						else
							aiNewState(actor, &gillBeastBite);
						break;
					case 3:
						if (actor->spr.type != gHitInfo.actor()->spr.type)
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &gillBeastSwimBite);
							else
								aiNewState(actor, &gillBeastBite);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &gillBeastSwimDodge);
							else
								aiNewState(actor, &gillBeastDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &gillBeastSwimBite);
						else
							aiNewState(actor, &gillBeastBite);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, &gillBeastSwimGoto);
	else
		aiNewState(actor, &gillBeastGoto);
	sfxPlay3DSound(actor, 1701, -1, 0);
	actor->SetTarget(nullptr);
}

static void gillThinkSwimGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &gillBeastSwimSearch);
	aiThinkTarget(actor);
}

static void gillThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &gillBeastSwimSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (actor->xspr.health == 0)
	{
		aiNewState(actor, &gillBeastSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &gillBeastSwimSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = pDudeInfo->eyeHeight + actor->spr.pos.Z;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &gillBeastSwimBite);
				else
				{
					aiPlay3DSound(actor, 1700, AI_SFX_PRIORITY_1, -1);
					aiNewState(actor, &gillBeast13A154);
				}
			}
		}
		else
			aiNewState(actor, &gillBeast13A154);
		return;
	}
	aiNewState(actor, &gillBeastSwimGoto);
	actor->SetTarget(nullptr);
}

static void sub_6CB00(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.ang = (actor->spr.ang + 256) & 2047;
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nDist = approxDist(dx, dy);
	if (Random(64) < 32 && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 2;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void sub_6CD74(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = actor->spr.pos.Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->spr.pos.Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int dz = z2 - z;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = -dz;
}

static void sub_6D03C(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = actor->spr.pos.Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->spr.pos.Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 27) / 120) / 120) << 2;
	if (abs(nAng) > 341)
	{
		actor->spr.ang = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int dz = (z2 - z) << 3;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = dz;
}

END_BLD_NS
