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

static void zombaThinkSearch(DBloodActor*);
static void zombaThinkGoto(DBloodActor*);
static void zombaThinkChase(DBloodActor*);
static void zombaThinkPonder(DBloodActor*);
static void myThinkTarget(DBloodActor*);
static void myThinkSearch(DBloodActor*);
static void entryEZombie(DBloodActor*);
static void entryAIdle(DBloodActor*);
static void entryEStand(DBloodActor*);


AISTATE zombieAIdle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, aiThinkTarget, NULL };
AISTATE zombieAChase = { kAiStateChase, 8, -1, 0, NULL, aiMoveForward, zombaThinkChase, NULL };
AISTATE zombieAPonder = { kAiStateOther, 0, -1, 0, NULL, aiMoveTurn, zombaThinkPonder, NULL };
AISTATE zombieAGoto = { kAiStateMove, 8, -1, 1800, NULL, aiMoveForward, zombaThinkGoto, &zombieAIdle };
AISTATE zombieAHack = { kAiStateChase, 6, nHackClient, 80, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieASearch = { kAiStateSearch, 8, -1, 1800, NULL, aiMoveForward, zombaThinkSearch, &zombieAIdle };
AISTATE zombieARecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieATeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieARecoil2 = { kAiStateRecoil, 1, -1, 360, NULL, NULL, NULL, &zombieAStand };
AISTATE zombieAStand = { kAiStateMove, 11, nStandClient, 0, NULL, NULL, NULL, &zombieAPonder };
AISTATE zombieEIdle = { kAiStateIdle, 12, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombieEUp2 = { kAiStateMove, 0, -1, 1, entryEZombie, NULL, NULL, &zombieASearch };
AISTATE zombieEUp = { kAiStateMove, 9, -1, 180, entryEStand, NULL, NULL, &zombieEUp2 };
AISTATE zombie2Idle = { kAiStateIdle, 0, -1, 0, entryAIdle, NULL, myThinkTarget, NULL };
AISTATE zombie2Search = { kAiStateSearch, 8, -1, 1800, NULL, NULL, myThinkSearch, &zombie2Idle };
AISTATE zombieSIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE zombie13AC2C = { kAiStateOther, 11, nStandClient, 0, entryEZombie, NULL, NULL, &zombieAPonder };

void HackSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int tx = pXSprite->targetX - actor->spr.pos.X;
	int ty = pXSprite->targetY - actor->spr.pos.Y;
	int nAngle = getangle(tx, ty);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	int dz = height - height2;
	int dx = bcos(nAngle);
	int dy = bsin(nAngle);
	sfxPlay3DSound(actor, 1101, 1, 0);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorAxe);
}

void StandSeqCallback(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 1102, -1, 0);
}

static void zombaThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiLookForTarget(actor);
}

static void zombaThinkGoto(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = pXSprite->targetX - actor->spr.pos.X;
	int dy = pXSprite->targetY - actor->spr.pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 921 && abs(actor->spr.ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &zombieASearch);
	aiThinkTarget(actor);
}

static void zombaThinkChase(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	if (target->IsPlayerActor() && (powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpDeathMaskUseless) > 0))
	{
		aiNewState(actor, &zombieAGoto);
		return;
	}
	// If the zombie gets whacked while rising from the grave it never executes this change and if it isn't done here at the very latest, will just aimlessly run around.
	if (!VanillaMode() && actor->spr.type == kDudeZombieAxeBuried)
		actor->spr.type = kDudeZombieAxeNormal;

	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - height, actor->spr.sector()))
		{
			if (abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &zombieAHack);
				return;
			}
		}
	}

	aiNewState(actor, &zombieAGoto);
	actor->SetTarget(nullptr);
}

static void zombaThinkPonder(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0)
	{
		aiNewState(actor, &zombieASearch);
		return;
	}
	if (target->IsPlayerActor() && (powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0 || powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpDeathMaskUseless) > 0))
	{
		aiNewState(actor, &zombieAGoto);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->spr.sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - height, actor->spr.sector()))
		{
			if (abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400)
				{
					if (abs(nDeltaAngle) < 85)
					{
						sfxPlay3DSound(actor, 1101, 1, 0);
						aiNewState(actor, &zombieAHack);
					}
					return;
				}
			}
		}
	}

	aiNewState(actor, &zombieAChase);
}

static void myThinkTarget(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		PLAYER* pPlayer = &gPlayer[p];
        auto owneractor = actor->GetOwner();
        if (owneractor == nullptr || owneractor == pPlayer->actor || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
			continue;
		int x = pPlayer->actor->spr.pos.X;
		int y = pPlayer->actor->spr.pos.Y;
		int z = pPlayer->actor->spr.pos.Z;
		auto pSector = pPlayer->actor->spr.sector();
		int dx = x - actor->spr.pos.X;
		int dy = y - actor->spr.pos.Y;
		int nDist = approxDist(dx, dy);
		if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
			continue;
		if (!cansee(x, y, z, pSector, actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - ((pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2), actor->spr.sector()))
			continue;
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void myThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	aiChooseDirection(actor, pXSprite->goalAng);
	myThinkTarget(actor);
}

static void entryEZombie(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	actor->spr.type = kDudeZombieAxeNormal;
	actor->spr.flags |= 1;
}

static void entryAIdle(DBloodActor* actor)
{
	actor->SetTarget(nullptr);
}

static void entryEStand(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	sfxPlay3DSound(actor, 1100, -1, 0);
	actor->spr.ang = getangle(pXSprite->targetX - actor->spr.pos.X, pXSprite->targetY - actor->spr.pos.Y);
}

END_BLD_NS
