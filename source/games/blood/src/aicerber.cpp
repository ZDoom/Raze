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

void cerberusBiteSeqCallback(int, DBloodActor* actor)
{
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dz = target->int_pos().Z - actor->int_pos().Z;
	actFireVector(actor, 350, -100, dx, dy, dz, kVectorCerberusHack);
	actFireVector(actor, -350, 0, dx, dy, dz, kVectorCerberusHack);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorCerberusHack);
}

void cerberusBurnSeqCallback(int, DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int height = pDudeInfo->eyeHeight * actor->spr.yrepeat;
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = height; // ???
	TARGETTRACK tt1 = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
	Aim aim;
	aim.dx = bcos(actor->int_ang());
	aim.dy = bsin(actor->int_ang());
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
		if (tt1.at10)
		{
			int t = DivScale(nDist, tt1.at10, 12);
			x2 += (actor2->vel.X * t) >> 12;
			y2 += (actor2->vel.Y * t) >> 12;
			z2 += (actor2->vel.Z * t) >> 8;
		}
		int tx = x + MulScale(Cos(actor->int_ang()), nDist, 30);
		int ty = y + MulScale(Sin(actor->int_ang()), nDist, 30);
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
			int nDeltaAngle = ((nAngle - actor->int_ang() + 1024) & 2047) - 1024;
			if (abs(nDeltaAngle) <= tt1.at8)
			{
				int tz1 = actor2->int_pos().Z - actor->int_pos().Z;
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
	switch (actor->spr.type) {
	case kDudeCerberusTwoHead:
		actFireMissile(actor, -350, 0, aim.dx, aim.dy, aim.dz, kMissileFireballCerberus);
		actFireMissile(actor, 350, -100, aim.dx, aim.dy, aim.dz, kMissileFireballCerberus);
		break;
	case kDudeCerberusOneHead:
		actFireMissile(actor, 350, -100, aim.dx, aim.dy, aim.dz, kMissileFireballCerberus);
		break;
	}
}

void cerberusBurnSeqCallback2(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int height = pDudeInfo->eyeHeight * actor->spr.yrepeat;

	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = height; // ???
	TARGETTRACK tt1 = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
	Aim aim;
	int ax, ay, az;
	aim.dx = ax = bcos(actor->int_ang());
	aim.dy = ay = bsin(actor->int_ang());
	aim.dz = actor->dudeSlope;
	az = 0;
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
		if (tt1.at10)
		{
			int t = DivScale(nDist, tt1.at10, 12);
			x2 += (actor->vel.X * t) >> 12;
			y2 += (actor->vel.Y * t) >> 12;
			z2 += (actor->vel.Z * t) >> 8;
		}
		int tx = x + MulScale(Cos(actor->int_ang()), nDist, 30);
		int ty = y + MulScale(Sin(actor->int_ang()), nDist, 30);
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
			int nDeltaAngle = ((nAngle - actor->int_ang() + 1024) & 2047) - 1024;
			if (abs(nDeltaAngle) <= tt1.at8)
			{
				DUDEINFO* pDudeInfo2 = getDudeInfo(actor2->spr.type);
				int height1 = (pDudeInfo2->aimHeight * actor2->spr.yrepeat) << 2;
				int tz1 = (z2 - height1) - z;
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
	switch (actor->spr.type) {
	case kDudeCerberusTwoHead:
		actFireMissile(actor, 350, -100, aim.dx, aim.dy, -aim.dz, kMissileFlameHound);
		actFireMissile(actor, -350, 0, ax, ay, az, kMissileFlameHound);
		break;
	case kDudeCerberusOneHead:
		actFireMissile(actor, 350, -100, aim.dx, aim.dy, -aim.dz, kMissileFlameHound);
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
		actor->xspr.goalAng += 256;
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

static void cerberusThinkGoto(DBloodActor* actor)
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
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
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

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));

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

	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery) {
				aiSetTarget(actor, actor->GetTarget());

				if (nDist < 0x1b00 && nDist > 0xd00 && abs(nDeltaAngle) < 85) {
					switch (actor->spr.type) {
					case kDudeCerberusTwoHead:
						aiNewState(actor, &cerberusBurn);
						break;
					case kDudeCerberusOneHead:
						aiNewState(actor, &cerberus2Burn);
						break;
					}
				}

				else if (nDist < 0xb00 && nDist > 0x500 && abs(nDeltaAngle) < 85) {
					switch (actor->spr.type) {
					case kDudeCerberusTwoHead:
						aiNewState(actor, &cerberus3Burn);
						break;
					case kDudeCerberusOneHead:
						aiNewState(actor, &cerberus4Burn);
						break;
					}
				}
				else if (nDist < 0x200 && abs(nDeltaAngle) < 85)
				{
					int hit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, CLIPMASK1, 0);
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
