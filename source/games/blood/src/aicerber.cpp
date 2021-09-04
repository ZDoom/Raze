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

#include "compat.h"
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
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	int dz = pTarget->z - pSprite->z;
	actFireVector(actor, 350, -100, dx, dy, dz, kVectorCerberusHack);
	actFireVector(actor, -350, 0, dx, dy, dz, kVectorCerberusHack);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorCerberusHack);
}

void cerberusBurnSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int height = pDudeInfo->eyeHeight * pSprite->yrepeat;
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	int x = pSprite->x;
	int y = pSprite->y;
	int z = height; // ???
	TARGETTRACK tt1 = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
	Aim aim;
	aim.dx = bcos(pSprite->ang);
	aim.dy = bsin(pSprite->ang);
	aim.dz = actor->dudeSlope;
	int nClosest = 0x7fffffff;
	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		spritetype* pSprite2 = &actor2->s();
		if (pSprite == pSprite2 || !(pSprite2->flags & 8))
			continue;
		int x2 = pSprite2->x;
		int y2 = pSprite2->y;
		int z2 = pSprite2->z;
		int nDist = approxDist(x2 - x, y2 - y);
		if (nDist == 0 || nDist > 0x2800)
			continue;
		if (tt1.at10)
		{
			int t = DivScale(nDist, tt1.at10, 12);
            x2 += (actor2->xvel * t) >> 12;
            y2 += (actor2->yvel * t) >> 12;
            z2 += (actor2->zvel * t) >> 8;
		}
		int tx = x + MulScale(Cos(pSprite->ang), nDist, 30);
		int ty = y + MulScale(Sin(pSprite->ang), nDist, 30);
		int tz = z + MulScale(actor->dudeSlope, nDist, 10);
		int tsr = MulScale(9460, nDist, 10);
		int top, bottom;
		GetSpriteExtents(pSprite2, &top, &bottom);
		if (tz - tsr > bottom || tz + tsr < top)
			continue;
		int dx = (tx - x2) >> 4;
		int dy = (ty - y2) >> 4;
		int dz = (tz - z2) >> 8;
		int nDist2 = ksqrt(dx * dx + dy * dy + dz * dz);
		if (nDist2 < nClosest)
		{
			int nAngle = getangle(x2 - x, y2 - y);
			int nDeltaAngle = ((nAngle - pSprite->ang + 1024) & 2047) - 1024;
			if (abs(nDeltaAngle) <= tt1.at8)
			{
				int tz = pSprite2->z - pSprite->z;
				if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz, nDist, 10);
				}
				else
					aim.dz = tz;
			}
		}
	}
	switch (pSprite->type) {
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
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int height = pDudeInfo->eyeHeight * pSprite->yrepeat;

	int x = pSprite->x;
	int y = pSprite->y;
	int z = height; // ???
	TARGETTRACK tt1 = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
	Aim aim;
	int ax, ay, az;
	aim.dx = ax = bcos(pSprite->ang);
	aim.dy = ay = bsin(pSprite->ang);
	aim.dz = actor->dudeSlope;
	az = 0;
	int nClosest = 0x7fffffff;
	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		spritetype* pSprite2 = &actor2->s();
		if (pSprite == pSprite2 || !(pSprite2->flags & 8))
			continue;
		int x2 = pSprite2->x;
		int y2 = pSprite2->y;
		int z2 = pSprite2->z;
		int nDist = approxDist(x2 - x, y2 - y);
		if (nDist == 0 || nDist > 0x2800)
			continue;
		if (tt1.at10)
		{
			int t = DivScale(nDist, tt1.at10, 12);
            x2 += (actor->xvel * t) >> 12;
            y2 += (actor->yvel * t) >> 12;
            z2 += (actor->zvel * t) >> 8;
		}
		int tx = x + MulScale(Cos(pSprite->ang), nDist, 30);
		int ty = y + MulScale(Sin(pSprite->ang), nDist, 30);
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
			int nDeltaAngle = ((nAngle - pSprite->ang + 1024) & 2047) - 1024;
			if (abs(nDeltaAngle) <= tt1.at8)
			{
				DUDEINFO* pDudeInfo2 = getDudeInfo(pSprite2->type);
				int height = (pDudeInfo2->aimHeight * pSprite2->yrepeat) << 2;
				int tz = (z2 - height) - z;
				if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz, nDist, 10);
				}
				else
					aim.dz = tz;
			}
		}
	}
	switch (pSprite->type) {
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
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void cerberusThinkTarget(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	if (pDudeExtraE->active && pDudeExtraE->thinkTime < 10)
		pDudeExtraE->thinkTime++;
	else if (pDudeExtraE->thinkTime >= 10 && pDudeExtraE->active)
	{
		pXSprite->goalAng += 256;
        POINT3D* pTarget = &actor->basePoint;
		aiSetTarget(actor, pTarget->x, pTarget->y, pTarget->z);
		if (pSprite->type == kDudeCerberusTwoHead)
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
			if (pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			int x = pPlayer->pSprite->x;
			int y = pPlayer->pSprite->y;
			int z = pPlayer->pSprite->z;
			int nSector = pPlayer->pSprite->sectnum;
			int dx = x - pSprite->x;
			int dy = y - pSprite->y;
			int nDist = approxDist(dx, dy);
			if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
				continue;
			if (!cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z - ((pDudeInfo->eyeHeight * pSprite->yrepeat) << 2), pSprite->sectnum))
				continue;
			int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, pPlayer->actor());
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
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
	{
		switch (pSprite->type) {
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
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr) {
		switch (pSprite->type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusGoto);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Goto);
			break;
		}
		return;
	}

	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}

	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);

	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = pTarget->x - pSprite->x;
	int dy = pTarget->y - pSprite->y;
	aiChooseDirection(actor, getangle(dx, dy));

	if (pXTarget->health == 0) {
		switch (pSprite->type) {
		case kDudeCerberusTwoHead:
			aiNewState(actor, &cerberusSearch);
			break;
		case kDudeCerberusOneHead:
			aiNewState(actor, &cerberus2Search);
			break;
		}
		return;
	}

	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0) {
		switch (pSprite->type) {
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
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;
		if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery) {
				aiSetTarget(actor, actor->GetTarget());

				if (nDist < 0x1b00 && nDist > 0xd00 && abs(nDeltaAngle) < 85) {
					switch (pSprite->type) {
					case kDudeCerberusTwoHead:
						aiNewState(actor, &cerberusBurn);
						break;
					case kDudeCerberusOneHead:
						aiNewState(actor, &cerberus2Burn);
						break;
					}
				}

				else if (nDist < 0xb00 && nDist > 0x500 && abs(nDeltaAngle) < 85) {
					switch (pSprite->type) {
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
					int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
					switch (pSprite->type) {
					case kDudeCerberusTwoHead:
						switch (hit) {
						case -1:
							aiNewState(actor, &cerberusBite);
							break;
						case 3:
							if (pSprite->type != gHitInfo.hitactor->s().type && gHitInfo.hitactor->s().type != kDudeHellHound)
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
							if (pSprite->type != gHitInfo.hitactor->s().type && gHitInfo.hitactor->s().type != kDudeHellHound)
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

	switch (pSprite->type) {
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
