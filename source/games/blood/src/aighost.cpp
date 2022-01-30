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

static void ghostThinkTarget(DBloodActor*);
static void ghostThinkSearch(DBloodActor*);
static void ghostThinkGoto(DBloodActor*);
static void ghostMoveDodgeUp(DBloodActor*);
static void ghostMoveDodgeDown(DBloodActor*);
static void ghostThinkChase(DBloodActor*);
static void ghostMoveForward(DBloodActor*);
static void ghostMoveSlow(DBloodActor*);
static void ghostMoveSwoop(DBloodActor*);
static void ghostMoveFly(DBloodActor*);


AISTATE ghostIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, ghostThinkTarget, NULL };
AISTATE ghostChase = { kAiStateChase, 0, -1, 0, NULL, ghostMoveForward, ghostThinkChase, &ghostIdle };
AISTATE ghostGoto = { kAiStateMove, 0, -1, 600, NULL, ghostMoveForward, ghostThinkGoto, &ghostIdle };
AISTATE ghostSlash = { kAiStateChase, 6, nGhostSlashClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostThrow = { kAiStateChase, 6, nGhostThrowClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostBlast = { kAiStateChase, 6, nGhostBlastClient, 120, NULL, ghostMoveSlow, NULL, &ghostChase };
AISTATE ghostRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostSearch = { kAiStateSearch, 0, -1, 120, NULL, ghostMoveForward, ghostThinkSearch, &ghostIdle };
AISTATE ghostSwoop = { kAiStateOther, 0, -1, 120, NULL, ghostMoveSwoop, ghostThinkChase, &ghostChase };
AISTATE ghostFly = { kAiStateMove, 0, -1, 0, NULL, ghostMoveFly, ghostThinkChase, &ghostChase };
AISTATE ghostTurn = { kAiStateMove, 0, -1, 120, NULL, aiMoveTurn, NULL, &ghostChase };
AISTATE ghostDodgeUp = { kAiStateMove, 0, -1, 60, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeDown = { kAiStateMove, 0, -1, 120, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };

void ghostSlashSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	int dz = height - height2;
	int dx = bcos(actor->spr.ang);
	int dy = bsin(actor->spr.ang);
	sfxPlay3DSound(actor, 1406, 0, 0);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGhost);
	int r1 = Random(50);
	int r2 = Random(50);
	actFireVector(actor, 0, 0, dx + r2, dy - r1, dz, kVectorGhost);
	r1 = Random(50);
	r2 = Random(50);
	actFireVector(actor, 0, 0, dx - r2, dy + r1, dz, kVectorGhost);
}

void ghostThrowSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0, 0, actor->dudeSlope - 7500, kThingBone, 0xeeeee);
}

void ghostBlastSeqCallback(int, DBloodActor* actor)
{
	wrand(); // ???
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int height = (actor->spr.yrepeat * getDudeInfo(actor->spr.type)->eyeHeight) << 2;
	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = height;
	TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
	Aim aim;
	aim.dx = bcos(actor->spr.ang);
	aim.dy = bsin(actor->spr.ang);
	aim.dz = actor->dudeSlope;
	int nClosest = 0x7fffffff;
	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		if (actor == actor2 || !(actor2->spr.flags & 8))
			continue;
		int x2 = actor2->int_pos().X;
		int y2 = actor2->int_pos().Y;
		int z2 = actor2->int_pos().Z;
		int nDist = approxDist(x2 - x, y2 - y);
		if (nDist == 0 || nDist > 0x2800)
			continue;
		if (tt.at10)
		{
			int t = DivScale(nDist, tt.at10, 12);
			x2 += (actor->vel.X * t) >> 12;
			y2 += (actor->vel.Y * t) >> 12;
			z2 += (actor->vel.Z * t) >> 8;
		}
		int tx = x + MulScale(Cos(actor->spr.ang), nDist, 30);
		int ty = y + MulScale(Sin(actor->spr.ang), nDist, 30);
		int tz = z + MulScale(actor->dudeSlope, nDist, 10);
		int tsr = MulScale(9460, nDist, 10);
		int top, bottom;
		GetActorExtents(actor2, &top, &bottom);
		if (tz - tsr > bottom || tz + tsr < top)
			continue;
		int dx = (tx - x2) >> 4;
		int dy = (ty - y2) >> 4;
		int dz = (tz - z2) >> 8;
		int nDist2 = ksqrt(dx * dx + dy * dy + dz * dz);
		if (nDist2 < nClosest)
		{
			int nAngle = getangle(x2 - x, y2 - y);
			int nDeltaAngle = ((nAngle - actor->spr.ang + 1024) & 2047) - 1024;
			if (abs(nDeltaAngle) <= tt.at8)
			{
				int tz1 = actor2->int_pos().Z - actor->int_pos().Z;
				if (cansee(x, y, z, actor->sector(), x2, y2, z2, actor2->sector()))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz1, nDist, 10);
					if (tz1 > -0x333)
						aim.dz = DivScale(tz1, nDist, 10);
					else if (tz1 < -0x333 && tz1 > -0xb33)
						aim.dz = DivScale(tz1, nDist, 10) + 9460;
					else if (tz1 < -0xb33 && tz1 > -0x3000)
						aim.dz = DivScale(tz1, nDist, 10) + 9460;
					else if (tz1 < -0x3000)
						aim.dz = DivScale(tz1, nDist, 10) - 7500;
					else
						aim.dz = DivScale(tz1, nDist, 10);
				}
				else
					aim.dz = DivScale(tz1, nDist, 10);
			}
		}
	}
#ifdef NOONE_EXTENSIONS
	// allow fire missile in non-player targets if not a demo
	if (target->IsPlayerActor() || gModernMap) {
		sfxPlay3DSound(actor, 489, 0, 0);
		actFireMissile(actor, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
	}
#else
	if (target->IsPlayerActor()) {
		sfxPlay3DSound(actor, 489, 0, 0);
		actFireMissile(actor, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
	}
#endif
}

static void ghostThinkTarget(DBloodActor* actor)
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
		actor->xspr.goalAng += 256;
		POINT3D* pTarget = &actor->basePoint;
		aiSetTarget(actor, pTarget->X, pTarget->Y, pTarget->Z);
		aiNewState(actor, &ghostTurn);
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
				return;
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
				return;
			}
		}
	}
}

static void ghostThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void ghostThinkGoto(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &ghostSearch);
	aiThinkTarget(actor);
}

static void ghostMoveDodgeUp(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
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
	actor->vel.Z = -0x1d555;
}

static void ghostMoveDodgeDown(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
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

static void ghostThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &ghostGoto);
		return;
	}
	///assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &ghostSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &ghostSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		// Should be dudeInfo[target->spr.type-kDudeBase]
		int height2 = (pDudeInfo->eyeHeight * target->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				int floorZ = getflorzofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y);
				switch (actor->spr.type) {
				case kDudePhantasm:
					if (nDist < 0x2000 && nDist > 0x1000 && abs(nDeltaAngle) < 85) {
						int hit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							aiNewState(actor, &ghostBlast);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudePhantasm)
								aiNewState(actor, &ghostBlast);
							break;
						default:
							aiNewState(actor, &ghostBlast);
							break;
						}
					}
					else if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					{
						int hit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							aiNewState(actor, &ghostSlash);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudePhantasm)
								aiNewState(actor, &ghostSlash);
							break;
						default:
							aiNewState(actor, &ghostSlash);
							break;
						}
					}
					else if ((height2 - height > 0x2000 || floorZ - bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
					{
						aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &ghostSwoop);
					}
					else if ((height2 - height < 0x2000 || floorZ - bottom < 0x2000) && abs(nDeltaAngle) < 85)
						aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
					break;
				}
			}
			return;
		}
		else
		{
			aiNewState(actor, &ghostFly);
			return;
		}
	}

	aiNewState(actor, &ghostGoto);
	actor->SetTarget(nullptr);
}

static void ghostMoveForward(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.ang = (actor->spr.ang + 256) & 2047;
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
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

static void ghostMoveSlow(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 = nAccel >> 1;
	t2 >>= 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	switch (actor->spr.type) {
	case kDudePhantasm:
		actor->vel.Z = 0x44444;
		break;
	}
}

static void ghostMoveSwoop(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
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
	switch (actor->spr.type) {
	case kDudePhantasm:
		actor->vel.Z = t1;
		break;
	}
}

static void ghostMoveFly(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->spr.ang = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
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
	switch (actor->spr.type) {
	case kDudePhantasm:
		actor->vel.Z = -t1;
		break;
	}
}

END_BLD_NS
