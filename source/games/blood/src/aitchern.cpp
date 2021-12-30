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
static void sub_72934(DBloodActor*);


AISTATE tchernobogIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, sub_725A4, NULL };
AISTATE tchernobogSearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, sub_72580, &tchernobogIdle };
AISTATE tchernobogChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, sub_72934, NULL };
AISTATE tchernobogRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tchernobogSearch };
AISTATE tcherno13A9B8 = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, sub_72850, &tchernobogIdle };
AISTATE tcherno13A9D4 = { kAiStateMove, 6, dword_279B54, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13A9F0 = { kAiStateChase, 6, dword_279B58, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13AA0C = { kAiStateChase, 7, dword_279B5C, 60, NULL, NULL, NULL, &tchernobogChase };
AISTATE tcherno13AA28 = { kAiStateChase, 8, -1, 60, NULL, aiMoveTurn, NULL, &tchernobogChase };

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

void sub_71BD4(int, DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int height = actor->spr.yrepeat * pDudeInfo->eyeHeight;
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	int x = actor->spr.pos.X;
	int y = actor->spr.pos.Y;
	int z = height;
	TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x100000 };
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
		int x2 = actor2->spr.pos.X;
		int y2 = actor2->spr.pos.Y;
		int z2 = actor2->spr.pos.Z;
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
				int tz1 = actor2->spr.pos.Z - actor->spr.pos.Z;
				if (cansee(x, y, z, actor->sector(), x2, y2, z2, actor2->sector()))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz1, nDist, 10);
				}
				else
					aim.dz = tz1;
			}
		}
	}
	actFireMissile(actor, -350, 0, aim.dx, aim.dy, aim.dz, kMissileFireballTchernobog);
	actFireMissile(actor, 350, 0, aim.dx, aim.dy, aim.dz, kMissileFireballTchernobog);
}

void sub_720AC(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int height = actor->spr.yrepeat * pDudeInfo->eyeHeight;
	int ax, ay, az;
	ax = bcos(actor->spr.ang);
	ay = bsin(actor->spr.ang);
	int x = actor->spr.pos.X;
	int y = actor->spr.pos.Y;
	int z = height;
	TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x100000 };
	Aim aim;
	aim.dx = ax;
	aim.dy = ay;
	aim.dz = actor->dudeSlope;
	int nClosest = 0x7fffffff;
	az = 0;
	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		if (actor == actor2 || !(actor2->spr.flags & 8))
			continue;
		int x2 = actor2->spr.pos.X;
		int y2 = actor2->spr.pos.Y;
		int z2 = actor2->spr.pos.Z;
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
				int tz1 = actor2->spr.pos.Z - actor->spr.pos.Z;
				if (cansee(x, y, z, actor->sector(), x2, y2, z2, actor2->sector()))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz1, nDist, 10);
				}
				else
					aim.dz = tz1;
			}
		}
	}
	actFireMissile(actor, 350, 0, aim.dx, aim.dy, -aim.dz, kMissileFireballTchernobog);
	actFireMissile(actor, -350, 0, ax, ay, az, kMissileFireballTchernobog);
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
		actor->xspr.goalAng += 256;
		POINT3D* pTarget = &actor->basePoint;
		aiSetTarget(actor, pTarget->X, pTarget->Y, pTarget->Z);
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
			int x = pPlayer->actor->spr.pos.X;
			int y = pPlayer->actor->spr.pos.Y;
			int z = pPlayer->actor->spr.pos.Z;
			auto pSector = pPlayer->actor->sector();
			int dx = x - actor->spr.pos.X;
			int dy = y - actor->spr.pos.Y;
			int nDist = approxDist(dx, dy);
			if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
				continue;
			if (!cansee(x, y, z, pSector, actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - ((pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2), actor->sector()))
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

static void sub_72850(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &tchernobogSearch);
	aiThinkTarget(actor);
}

static void sub_72934(DBloodActor* actor)
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

	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
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
				if (nDist < 0x1f00 && nDist > 0xd00 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &tcherno13AA0C);
				else if (nDist < 0xd00 && nDist > 0xb00 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &tcherno13A9D4);
				else if (nDist < 0xb00 && nDist > 0x500 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &tcherno13A9F0);
				return;
			}
		}
	}

	aiNewState(actor, &tcherno13A9B8);
	actor->SetTarget(nullptr);
}

END_BLD_NS
