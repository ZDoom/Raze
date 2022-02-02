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

static void eelThinkTarget(DBloodActor*);
static void eelThinkSearch(DBloodActor*);
static void eelThinkGoto(DBloodActor*);
static void eelThinkPonder(DBloodActor*);
static void eelMoveDodgeUp(DBloodActor*);
static void eelMoveDodgeDown(DBloodActor*);
static void eelThinkChase(DBloodActor*);
static void eelMoveForward(DBloodActor*);
static void eelMoveSwoop(DBloodActor*);
static void eelMoveAscend(DBloodActor* actor);
static void eelMoveToCeil(DBloodActor*);


AISTATE eelIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelFlyIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, eelThinkTarget, NULL };
AISTATE eelChase = { kAiStateChase, 0, -1, 0, NULL, eelMoveForward, eelThinkChase, &eelIdle };
AISTATE eelPonder = { kAiStateOther, 0, -1, 0, NULL, NULL, eelThinkPonder, NULL };
AISTATE eelGoto = { kAiStateMove, 0, -1, 600, NULL, NULL, eelThinkGoto, &eelIdle };
AISTATE eelBite = { kAiStateChase, 7, nEelBiteClient, 60, NULL, NULL, NULL, &eelChase };
AISTATE eelRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &eelChase };
AISTATE eelSearch = { kAiStateSearch, 0, -1, 120, NULL, eelMoveForward, eelThinkSearch, &eelIdle };
AISTATE eelSwoop = { kAiStateOther, 0, -1, 60, NULL, eelMoveSwoop, eelThinkChase, &eelChase };
AISTATE eelFly = { kAiStateMove, 0, -1, 0, NULL, eelMoveAscend, eelThinkChase, &eelChase };
AISTATE eelTurn = { kAiStateMove, 0, -1, 60, NULL, aiMoveTurn, NULL, &eelChase };
AISTATE eelHide = { kAiStateOther, 0, -1, 0, NULL, eelMoveToCeil, eelMoveForward, NULL };
AISTATE eelDodgeUp = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeUp, NULL, &eelChase };
AISTATE eelDodgeDown = { kAiStateMove, 0, -1, 120, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };
AISTATE eelDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, eelMoveDodgeDown, NULL, &eelChase };

void eelBiteSeqCallback(int, DBloodActor* actor)
{
	/*
	 * workaround for
	 * actor->xspr.target >= 0 in file NBlood/source/blood/src/aiboneel.cpp at line 86
	 * The value of actor->xspr.target is -1.
	 * copied from lines 177:181
	 * resolves this case, but may cause other issues?
	 */
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelSearch);
		return;
	}

	auto target = actor->GetTarget();
	int dx = bcos(actor->spr.ang);
	int dy = bsin(actor->spr.ang);
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	actFireVector(actor, 0, 0, dx, dy, height2 - height, kVectorBoneelBite);
}

static void eelThinkTarget(DBloodActor* actor)
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
		POINT3D* pTarget = &actor->basePoint;
		aiSetTarget(actor, pTarget->X, pTarget->Y, pTarget->Z);
		aiNewState(actor, &eelTurn);
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
			int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void eelThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	eelThinkTarget(actor);
}

static void eelThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &eelSearch);
	eelThinkTarget(actor);
}

static void eelThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		int height2 = (getDudeInfo(target->spr.type)->eyeHeight * target->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < -0x2000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < 0xccc && nDist < 0x399 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeDown);
			else if (height2 - height < -0x2000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height < -0x2000 && abs(nDeltaAngle) < 85 && nDist > 0x1400)
				aiNewState(actor, &eelDodgeUp);
			else if (height2 - height > 0xccc)
				aiNewState(actor, &eelDodgeDown);
			else
				aiNewState(actor, &eelDodgeUp);
			return;
		}
	}
	aiNewState(actor, &eelGoto);
	actor->SetTarget(nullptr);
}

static void eelMoveDodgeUp(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
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
	actor->vel.Z = -0x8000;
}

static void eelMoveDodgeDown(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	if (actor->xspr.dodgeDir == 0)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
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

static void eelThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &eelGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &eelSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		int top2, bottom2;
		GetActorExtents(target, &top2, &bottom2);
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x399 && top2 > top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelSwoop);
				else if (nDist <= 0x399 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelBite);
				else if (bottom2 > top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelSwoop);
				else if (top2 < top && abs(nDeltaAngle) < 85)
					aiNewState(actor, &eelFly);
			}
		}
		return;
	}

	actor->SetTarget(nullptr);
	aiNewState(actor, &eelSearch);
}

static void eelMoveForward(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.ang = (actor->spr.ang + 256) & 2047;
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (nDist <= 0x399)
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
		t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void eelMoveSwoop(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > 341)
		return;
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x8000) && nDist <= 0x399)
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
	actor->vel.Z = 0x22222;
}

static void eelMoveAscend(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = (pDudeInfo->frontSpeed - (((4 - gGameOptions.nDifficulty) << 26) / 120) / 120) << 2;
	if (abs(nAng) > 341)
		return;
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x399)
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
	actor->vel.Z = -0x8000;
}

void eelMoveToCeil(DBloodActor* actor)
{
	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = actor->int_pos().Z;
	if (z - actor->xspr.TargetPos.Z < 0x1000)
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &eelIdle);
	}
	else
		aiSetTarget(actor, x, y, actor->sector()->int_ceilingz());
}

END_BLD_NS
