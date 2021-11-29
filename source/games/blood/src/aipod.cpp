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

static void aiPodSearch(DBloodActor* actor);
static void aiPodMove(DBloodActor* actor);
static void aiPodChase(DBloodActor* actor);

AISTATE podIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE podMove = { kAiStateMove, 7, -1, 3600, NULL, aiMoveTurn, aiPodMove, &podSearch };
AISTATE podSearch = { kAiStateSearch, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, &podSearch };
AISTATE podStartChase = { kAiStateChase, 8, nPodStartChaseClient, 600, NULL, NULL, NULL, &podChase };
AISTATE podRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &podChase };
AISTATE podChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };
AISTATE tentacleIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tentacle13A6A8 = { kAiStateOther, 7, dword_279B3C, 0, NULL, NULL, NULL, &tentacle13A6C4 };
AISTATE tentacle13A6C4 = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacle13A6E0 = { kAiStateOther, 8, dword_279B40, 0, NULL, NULL, NULL, &tentacle13A6FC };
AISTATE tentacle13A6FC = { kAiStateOther, -1, -1, 0, NULL, NULL, NULL, &tentacleIdle };
AISTATE tentacleMove = { kAiStateOther, 8, -1, 3600, NULL, aiMoveTurn, aiPodMove, &tentacleSearch };
AISTATE tentacleSearch = { kAiStateOther, 0, -1, 3600, NULL, aiMoveTurn, aiPodSearch, NULL };
AISTATE tentacleStartChase = { kAiStateOther, 6, nTentacleStartSearchClient, 120, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tentacleChase };
AISTATE tentacleChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveTurn, aiPodChase, NULL };

void sub_6FF08(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2503, -1, 0);
}

void sub_6FF54(int, DBloodActor* actor)
{
	sfxPlay3DSound(actor, 2500, -1, 0);
}

void podAttack(int, DBloodActor* actor)
{
	spritetype* pSprite = &actor->s();

	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();

	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int x = pTarget->x - pSprite->x;
	int y = pTarget->y - pSprite->y;
	int dz = pTarget->z - pSprite->z;
	x += Random2(1000);
	y += Random2(1000);
	int nDist = approxDist(x, y);
	int nDist2 = nDist / 540;
	DBloodActor* pMissile = nullptr;
	switch (pSprite->type)
	{
	case kDudePodGreen:
		dz += 8000;
		if (pDudeInfo->seeDist * 0.1 < nDist)
		{
			if (Chance(0x8000))
				sfxPlay3DSound(actor, 2474, -1, 0);
			else
				sfxPlay3DSound(actor, 2475, -1, 0);
			pMissile = actFireThing(actor, 0, -8000, dz / 128 - 14500, kThingPodGreenBall, (nDist2 << 23) / 120);
		}
		if (pMissile)
			seqSpawn(68, pMissile, -1);
		break;
	case kDudePodFire:
		dz += 8000;
		if (pDudeInfo->seeDist * 0.1 < nDist)
		{
			sfxPlay3DSound(actor, 2454, -1, 0);
			pMissile = actFireThing(actor, 0, -8000, dz / 128 - 14500, kThingPodFireBall, (nDist2 << 23) / 120);
		}
		if (pMissile)
			seqSpawn(22, pMissile, -1);
		break;
	}
	for (int i = 0; i < 4; i++)
        fxSpawnPodStuff(actor, 240);
}

void sub_70284(int, DBloodActor* actor)
{
	auto pSprite = &actor->s();
	sfxPlay3DSound(actor, 2502, -1, 0);
	int nDist, nBurn;
	DAMAGE_TYPE dmgType;
	switch (pSprite->type) {
	case kDudeTentacleGreen:
	default: // ???
		nBurn = 0;
		dmgType = kDamageBullet;
		nDist = 50;
		break;
	case kDudeTentacleFire: // ???
		nBurn = (gGameOptions.nDifficulty * 120) >> 2;
		dmgType = kDamageExplode;
		nDist = 75;
		break;
	}
	actRadiusDamage(actor, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, nDist, 1, 5 * (1 + gGameOptions.nDifficulty), dmgType, 2, nBurn);
}

static void aiPodSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void aiPodMove(DBloodActor* actor)
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
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery) {
		switch (pSprite->type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleSearch);
			break;
		}
	}
	aiThinkTarget(actor);
}

static void aiPodChase(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr) {
		switch (pSprite->type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podMove);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleMove);
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
	spritetype* pTarget = &actor->GetTarget()->s();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = pTarget->x - pSprite->x;
	int dy = pTarget->y - pSprite->y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0) {

		switch (pSprite->type) {
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podSearch);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleSearch);
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
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (abs(nDeltaAngle) < 85 && pTarget->type != kDudePodGreen && pTarget->type != kDudePodFire) {
					switch (pSprite->type) {
					case kDudePodGreen:
					case kDudePodFire:
						aiNewState(actor, &podStartChase);
						break;
					case kDudeTentacleGreen:
					case kDudeTentacleFire:
						aiNewState(actor, &tentacleStartChase);
						break;
					}
				}
				return;
			}
		}
	}

	switch (pSprite->type) {
	case kDudePodGreen:
	case kDudePodFire:
		aiNewState(actor, &podMove);
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		aiNewState(actor, &tentacleMove);
		break;
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
