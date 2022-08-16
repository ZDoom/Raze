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

static void batThinkTarget(DBloodActor*);
static void batThinkSearch(DBloodActor*);
static void batThinkGoto(DBloodActor*);
static void batThinkPonder(DBloodActor*);
static void batMoveDodgeUp(DBloodActor*);
static void batMoveDodgeDown(DBloodActor*);
static void batThinkChase(DBloodActor*);
static void batMoveForward(DBloodActor*);
static void batMoveSwoop(DBloodActor*);
static void batMoveFly(DBloodActor*);
static void batMoveToCeil(DBloodActor*);


AISTATE batIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batFlyIdle = { kAiStateIdle, 6, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batChase = { kAiStateChase, 6, -1, 0, NULL, batMoveForward, batThinkChase, &batFlyIdle };
AISTATE batPonder = { kAiStateOther, 6, -1, 0, NULL, NULL, batThinkPonder, NULL };
AISTATE batGoto = { kAiStateMove, 6, -1, 600, NULL, batMoveForward, batThinkGoto, &batFlyIdle };
AISTATE batBite = { kAiStateChase, 7, nBatBiteClient, 60, NULL, NULL, NULL, &batPonder };
AISTATE batRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &batChase };
AISTATE batSearch = { kAiStateSearch, 6, -1, 120, NULL, batMoveForward, batThinkSearch, &batFlyIdle };
AISTATE batSwoop = { kAiStateOther, 6, -1, 60, NULL, batMoveSwoop, batThinkChase, &batChase };
AISTATE batFly = { kAiStateMove, 6, -1, 0, NULL, batMoveFly, batThinkChase, &batChase };
AISTATE batTurn = { kAiStateMove, 6, -1, 60, NULL, aiMoveTurn, NULL, &batChase };
AISTATE batHide = { kAiStateOther, 6, -1, 0, NULL, batMoveToCeil, batMoveForward, NULL };
AISTATE batDodgeUp = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeDown = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };

void batBiteSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(pTarget->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (pTarget->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	actFireVector(actor, 0, 0, dx, dy, height2 - height, kVectorBatBite);
}

static void batThinkTarget(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	if (pDudeExtraE->active && pDudeExtraE->thinkTime < 10)
		pDudeExtraE->thinkTime++;
	else if (pDudeExtraE->thinkTime >= 10 && pDudeExtraE->active)
	{
		pDudeExtraE->thinkTime = 0;
		actor->xspr.goalAng += 256;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &batTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
			if (pPlayer->actor->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			int x = pPlayer->actor->int_pos().X;
			int y = pPlayer->actor->int_pos().Y;
			int z = pPlayer->actor->int_pos().Z;
			auto pSector = pPlayer->actor->sector();
			int dx = x - actor->int_pos().X;
			int dy = y - actor->int_pos().Y;
			int nDist = approxDist(dx, dy);
			if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
				continue;
			if (!cansee(x, y, z, pSector, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - ((pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2), actor->sector()))
				continue;
			int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void batThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	batThinkTarget(actor);
}

static void batThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &batSearch);
	batThinkTarget(actor);
}

static void batThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	int dx = pTarget->int_pos().X - actor->int_pos().X;
	int dy = pTarget->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		int height2 = (getDudeInfo(pTarget->spr.type)->eyeHeight * pTarget->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->int_pos().X, pTarget->int_pos().Y, pTarget->int_pos().Z, pTarget->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < 0x3000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x5000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x2000 && nDist < 0x200 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x6000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x2000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height < 0x2000 && abs(nDeltaAngle) < 85 && nDist > 0x1400)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x4000)
				aiNewState(actor, &batDodgeDown);
			else
				aiNewState(actor, &batDodgeUp);
			return;
		}
	}
	aiNewState(actor, &batGoto);
	actor->SetTarget(nullptr);
}

static void batMoveDodgeUp(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int dx = actor->vel.X;
	int dy = actor->vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = -0x52aaa;
}

static void batMoveDodgeDown(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	if (actor->xspr.dodgeDir == 0)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int dx = actor->vel.X;
	int dy = actor->vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = 0x44444;
}

static void batThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	int dx = pTarget->int_pos().X - actor->int_pos().X;
	int dy = pTarget->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	if (pTarget->IsPlayerActor() && powerupCheck(&gPlayer[pTarget->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		// Should be dudeInfo[pTarget->spr.type-kDudeBase]
		int height2 = (pDudeInfo->eyeHeight * pTarget->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->int_pos().X, pTarget->int_pos().Y, pTarget->int_pos().Z, pTarget->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				int floorZ = getflorzofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y);
				if (height2 - height < 0x2000 && nDist < 0x200 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batBite);
				else if ((height2 - height > 0x5000 || floorZ - bottom > 0x5000) && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batSwoop);
				else if ((height2 - height < 0x3000 || floorZ - bottom < 0x3000) && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batFly);
				return;
			}
		}
		else
		{
			aiNewState(actor, &batFly);
			return;
		}
	}

	actor->SetTarget(nullptr);
	aiNewState(actor, &batHide);
}

static void batMoveForward(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->set_int_ang((actor->int_ang() + 256) & 2047);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if ((unsigned int)Random(64) < 32 && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void batMoveSwoop(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->int_ang() + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = 0x44444;
}

static void batMoveFly(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->set_int_ang((actor->int_ang() + 512) & 2047);
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = -0x2d555;
}

void batMoveToCeil(DBloodActor* actor)
{
	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = actor->int_pos().Z;
	if (z - actor->xspr.TargetPos.Z < 0x1000)
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &batIdle);
	}
	else
		aiSetTarget(actor, x, y, actor->sector()->int_ceilingz());
}

END_BLD_NS
