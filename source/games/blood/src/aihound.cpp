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

static void houndThinkSearch(DBloodActor*);
static void houndThinkGoto(DBloodActor*);
static void houndThinkChase(DBloodActor*);

AISTATE houndIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE houndSearch = { kAiStateMove, 8, -1, 1800, NULL, aiMoveForward, houndThinkSearch, &houndIdle };
AISTATE houndChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, houndThinkChase, NULL };
AISTATE houndRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &houndSearch };
AISTATE houndTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &houndSearch };
AISTATE houndGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, houndThinkGoto, &houndIdle };
AISTATE houndBite = { kAiStateChase, 6, nHoundBiteClient, 60, NULL, NULL, NULL, &houndChase };
AISTATE houndBurn = { kAiStateChase, 7, nHoundBurnClient, 60, NULL, NULL, NULL, &houndChase };

void houndBiteSeqCallback(int, DBloodActor* actor)
{
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
#ifdef NOONE_EXTENSIONS
	if (IsPlayerSprite(pTarget) || gModernMap) // allow to hit non-player targets
		actFireVector(actor, 0, 0, dx, dy, pTarget->z - pSprite->z, kVectorHoundBite);
#else
	if (IsPlayerSprite(pTarget))
		actFireVector(actor, 0, 0, dx, dy, pTarget->z - pSprite->z, kVectorHoundBite);
#endif
}

void houndBurnSeqCallback(int, DBloodActor* actor)
{
	spritetype* pSprite = &actor->s();
    actFireMissile(actor, 0, 0, bcos(pSprite->ang), bsin(pSprite->ang), 0, kMissileFlameHound);
}

static void houndThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void houndThinkGoto(DBloodActor* actor)
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
		aiNewState(actor, &houndSearch);
	aiThinkTarget(actor);
}

static void houndThinkChase(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &houndGoto);
		return;
	}
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	spritetype* pTarget = &actor->GetTarget()->s();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = pTarget->x - pSprite->x;
	int dy = pTarget->y - pSprite->y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0)
	{
		aiNewState(actor, &houndSearch);
		return;
	}
	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &houndSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;
		if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0xb00 && nDist > 0x500 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &houndBurn);
				else if (nDist < 0x266 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &houndBite);
				return;
			}
		}
	}

	aiNewState(actor, &houndGoto);
	actor->SetTarget(nullptr);
}

END_BLD_NS
