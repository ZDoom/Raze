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

static void calebThinkSearch(DBloodActor*);
static void calebThinkGoto(DBloodActor*);
static void calebThinkChase(DBloodActor*);
static void calebThinkSwimGoto(DBloodActor*);
static void calebThinkSwimChase(DBloodActor*);
static void sub_65D04(DBloodActor*);
static void sub_65F44(DBloodActor*);
static void sub_661E0(DBloodActor*);

AISTATE tinycalebIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tinycalebChase = { kAiStateChase, 6, -1, 0, NULL, aiMoveForward, calebThinkChase, NULL };
AISTATE tinycalebDodge = { kAiStateMove, 6, -1, 90, NULL, aiMoveDodge, NULL, &tinycalebChase };
AISTATE tinycalebGoto = { kAiStateMove, 6, -1, 600, NULL, aiMoveForward, calebThinkGoto, &tinycalebIdle };
AISTATE tinycalebAttack = { kAiStateChase, 0, nAttackClient, 120, NULL, NULL, NULL, &tinycalebChase };
AISTATE tinycalebSearch = { kAiStateSearch, 6, -1, 120, NULL, aiMoveForward, calebThinkSearch, &tinycalebIdle };
AISTATE tinycalebRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tinycalebDodge };
AISTATE tinycalebTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &tinycalebDodge };
AISTATE tinycalebSwimIdle = { kAiStateIdle, 10, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE tinycalebSwimChase = { kAiStateChase, 8, -1, 0, NULL, sub_65D04, calebThinkSwimChase, NULL };
AISTATE tinycalebSwimDodge = { kAiStateMove, 8, -1, 90, NULL, aiMoveDodge, NULL, &tinycalebSwimChase };
AISTATE tinycalebSwimGoto = { kAiStateMove, 8, -1, 600, NULL, aiMoveForward, calebThinkSwimGoto, &tinycalebSwimIdle };
AISTATE tinycalebSwimSearch = { kAiStateSearch, 8, -1, 120, NULL, aiMoveForward, calebThinkSearch, &tinycalebSwimIdle };
AISTATE tinycalebSwimAttack = { kAiStateChase, 10, nAttackClient, 0, NULL, NULL, NULL, &tinycalebSwimChase };
AISTATE tinycalebSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &tinycalebSwimDodge };
AISTATE tinycaleb139660 = { kAiStateOther, 8, -1, 120, NULL, sub_65F44, calebThinkSwimChase, &tinycalebSwimChase };
AISTATE tinycaleb13967C = { kAiStateOther, 8, -1, 0, NULL, sub_661E0, calebThinkSwimChase, &tinycalebSwimChase };
AISTATE tinycaleb139698 = { kAiStateOther, 8, -1, 120, NULL, aiMoveTurn, NULL, &tinycalebSwimChase };

void SeqAttackCallback(int, DBloodActor* actor)
{
	spritetype* pSprite = &actor->s();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	int dz = actor->dudeSlope;
	dx += Random2(1500);
	dy += Random2(1500);
	dz += Random2(1500);
	for (int i = 0; i < 2; i++)
	{
		int r1 = Random3(500);
		int r2 = Random3(1000);
		int r3 = Random3(1000);
		actFireVector(actor, 0, 0, dx + r3, dy + r2, dz + r1, kVectorShell);
	}
	if (Chance(0x8000))
		sfxPlay3DSound(actor, 10000 + Random(5), -1, 0);
	if (Chance(0x8000))
		sfxPlay3DSound(actor, 1001, -1, 0);
	else
		sfxPlay3DSound(actor, 1002, -1, 0);
}

static void calebThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void calebThinkGoto(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	XSECTOR* pXSector;
	int nXSector = sector[pSprite->sectnum].extra;
	if (nXSector > 0)
		pXSector = &xsector[nXSector];
	else
		pXSector = NULL;
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
	}
	aiThinkTarget(actor);
}

static void calebThinkChase(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		XSECTOR* pXSector;
		int nXSector = sector[pSprite->sectnum].extra;
		if (nXSector > 0)
			pXSector = &xsector[nXSector];
		else
			pXSector = NULL;
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
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
		XSECTOR* pXSector;
		int nXSector = sector[pSprite->sectnum].extra;
		if (nXSector > 0)
			pXSector = &xsector[nXSector];
		else
			pXSector = NULL;
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
		{
			aiPlay3DSound(actor, 11000 + Random(4), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &tinycalebSearch);
		}
		return;
	}
	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		XSECTOR* pXSector;
		int nXSector = sector[pSprite->sectnum].extra;
		if (nXSector > 0)
			pXSector = &xsector[nXSector];
		else
			pXSector = NULL;
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &tinycalebSwimSearch);
		else
			aiNewState(actor, &tinycalebSearch);
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
				actor->dudeSlope = nDist == 0 ? 0 : DivScale(pTarget->z-pSprite->z, nDist, 10);
				if (nDist < 0x599 && abs(nDeltaAngle) < 28)
				{
					XSECTOR* pXSector;
					int nXSector = sector[pSprite->sectnum].extra;
					if (nXSector > 0)
						pXSector = &xsector[nXSector];
					else
						pXSector = NULL;
					int hit = HitScan(actor, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &tinycalebSwimAttack);
						else
							aiNewState(actor, &tinycalebAttack);
						break;
					case 3:
						if (pSprite->type != gHitInfo.hitactor->s().type)
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &tinycalebSwimAttack);
							else
								aiNewState(actor, &tinycalebAttack);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &tinycalebSwimDodge);
							else
								aiNewState(actor, &tinycalebDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &tinycalebSwimAttack);
						else
							aiNewState(actor, &tinycalebAttack);
						break;
					}
				}
			}
			return;
		}
	}

	XSECTOR* pXSector;
	int nXSector = sector[pSprite->sectnum].extra;
	if (nXSector > 0)
		pXSector = &xsector[nXSector];
	else
		pXSector = NULL;
	if (pXSector && pXSector->Underwater)
		aiNewState(actor, &tinycalebSwimGoto);
	else
		aiNewState(actor, &tinycalebGoto);
	if (Chance(0x2000))
		sfxPlay3DSound(actor, 10000 + Random(5), -1, 0);
	actor->SetTarget(nullptr);
}

static void calebThinkSwimGoto(DBloodActor* actor)
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
		aiNewState(actor, &tinycalebSwimSearch);
	aiThinkTarget(actor);
}

static void calebThinkSwimChase(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &tinycalebSwimGoto);
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
		aiNewState(actor, &tinycalebSwimSearch);
		return;
	}
	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &tinycalebSwimSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = pDudeInfo->eyeHeight + pSprite->z;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &tinycalebSwimAttack);
				else
					aiNewState(actor, &tinycaleb13967C);
			}
		}
		return;
	}
	aiNewState(actor, &tinycalebSwimGoto);
	actor->SetTarget(nullptr);
}

static void sub_65D04(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		pSprite->ang = (pSprite->ang + 256) & 2047;
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int nDist = approxDist(dx, dy);
	if (Random(64) < 32 && nDist <= 0x400)
		return;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int vx = actor->xvel;
    int vy = actor->yvel;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 2;
    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void sub_65F44(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	spritetype* pTarget = &actor->GetTarget()->s();
	int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
	int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		pXSprite->goalAng = (pSprite->ang + 512) & 2047;
		return;
	}
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int dz = z2 - z;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int vx = actor->xvel;
    int vy = actor->yvel;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel;
    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel = -dz;
}

static void sub_661E0(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;

	spritetype* pTarget = &actor->GetTarget()->s();
	int z = pSprite->z + getDudeInfo(pSprite->type)->eyeHeight;
	int z2 = pTarget->z + getDudeInfo(pTarget->type)->eyeHeight;
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		pSprite->ang = (pSprite->ang + 512) & 2047;
		return;
	}
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int dz = (z2 - z) << 3;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x400)
		return;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int vx = actor->xvel;
    int vy = actor->yvel;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel = dz;
}

END_BLD_NS
