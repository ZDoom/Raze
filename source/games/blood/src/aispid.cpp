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

static void spidThinkSearch(DBloodActor*);
static void spidThinkGoto(DBloodActor*);
static void spidThinkChase(DBloodActor*);


AISTATE spidIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE spidChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, spidThinkChase, NULL };
AISTATE spidDodge = { kAiStateMove, 7, -1, 90, NULL, aiMoveDodge, NULL, &spidChase };
AISTATE spidGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, spidThinkGoto, &spidIdle };
AISTATE spidSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, spidThinkSearch, &spidIdle };
AISTATE spidBite = { kAiStateChase, 6, nSpidBiteClient, 60, NULL, NULL, NULL, &spidChase };
AISTATE spidJump = { kAiStateChase, 8, nSpidJumpClient, 60, NULL, aiMoveForward, NULL, &spidChase };
AISTATE spidBirth = { kAiStateOther, 0, nSpidBirthClient, 60, NULL, NULL, NULL, &spidIdle };

static char spidBlindEffect(DBloodActor* dudeactor, int nBlind, int max)
{
	spritetype* pDude = &dudeactor->s();
	if (IsPlayerSprite(pDude))
	{
		nBlind <<= 4;
		max <<= 4;
		PLAYER* pPlayer = &gPlayer[pDude->type - kDudePlayer1];
		if (pPlayer->blindEffect < max)
		{
			pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect + nBlind, max);
			return 1;
		}
	}
	return 0;
}

void SpidBiteSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	dx += Random2(2000);
	dy += Random2(2000);
	int dz = Random2(2000);
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	auto const target = actor->GetTarget();
	spritetype* pTarget = &target->s();
	XSPRITE* pXTarget = &target->x();
	if (IsPlayerSprite(pTarget))
	{
		int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
		if (hit == 3 && gHitInfo.hitactor->IsPlayerActor())
		{
			dz += pTarget->z - pSprite->z;
			PLAYER* pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
			switch (pSprite->type)
			{
			case kDudeSpiderBrown:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				if (IsPlayerSprite(pTarget) && !pPlayer->godMode && powerupCheck(pPlayer, kPwUpDeathMask) <= 0 && Chance(0x4000))
					powerupActivate(pPlayer, kPwUpDeliriumShroom);
				break;
			case kDudeSpiderRed:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				if (Chance(0x5000)) spidBlindEffect(target, 4, 16);
				break;
			case kDudeSpiderBlack:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			case kDudeSpiderMother:
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);

				dx += Random2(2000);
				dy += Random2(2000);
				dz += Random2(2000);
				actFireVector(actor, 0, 0, dx, dy, dz, kVectorSpiderBite);
				spidBlindEffect(target, 8, 16);
				break;
			}
		}

	}
}

void SpidJumpSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	dx += Random2(200);
	dy += Random2(200);
	int dz = Random2(200);
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	if (IsPlayerSprite(pTarget)) {
		dz += pTarget->z - pSprite->z;
		switch (pSprite->type) {
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
                actor->xvel = IntToFixed(dx);
                actor->yvel = IntToFixed(dy);
                actor->zvel = IntToFixed(dz);
			break;
		}
	}
}

void SpidBirthSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);

	DBloodActor* spawned = nullptr;
	if (IsPlayerSprite(pTarget) && pDudeExtraE->birthCounter < 10)
	{
		if (nDist < 0x1a00 && nDist > 0x1400 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderRed, pSprite->clipdist, 0);
		else if (nDist < 0x1400 && nDist > 0xc00 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderBrown, pSprite->clipdist, 0);
		else if (nDist < 0xc00 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
			spawned = actSpawnDude(actor, kDudeSpiderBrown, pSprite->clipdist, 0);

		if (spawned)
		{
			pDudeExtraE->birthCounter++;
			spawned->SetOwner(spawned);
			gKillMgr.AddNewKill(1);
		}
	}

}

static void spidThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void spidThinkGoto(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &spidSearch);
	aiThinkTarget(actor);
}

static void spidThinkChase(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &spidGoto);
		return;
	}
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	spritetype* pTarget = &actor->GetTarget()->s();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = pTarget->x - pSprite->x;
	int dy = pTarget->y - pSprite->y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0)
	{
		aiNewState(actor, &spidSearch);
		return;
	}
	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &spidSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist) {
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;

		if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum)) {
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery) {
				aiSetTarget(actor, actor->GetTarget());

				switch (pSprite->type) {
				case kDudeSpiderRed:
					if (nDist < 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidBite);
					break;
				case kDudeSpiderBrown:
				case kDudeSpiderBlack:
					if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidJump);
					else if (nDist < 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidBite);
					break;
				case kDudeSpiderMother:
					if (nDist < 0x733 && nDist > 0x399 && abs(nDeltaAngle) < 85)
						aiNewState(actor, &spidJump);
					else if (Chance(0x8000))
						aiNewState(actor, &spidBirth);
					break;
				}

				return;
			}
		}
	}

	aiNewState(actor, &spidGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
