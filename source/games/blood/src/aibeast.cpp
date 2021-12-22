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

static void MorphToBeast(DBloodActor*);
static void beastThinkSearch(DBloodActor*);
static void beastThinkGoto(DBloodActor*);
static void beastThinkChase(DBloodActor*);
static void beastThinkSwimGoto(DBloodActor*);
static void beastThinkSwimChase(DBloodActor*);
static void beastMoveForward(DBloodActor*);
static void sub_628A0(DBloodActor*);
static void sub_62AE0(DBloodActor*);
static void sub_62D7C(DBloodActor*);

AISTATE beastIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastChase = { kAiStateChase, 8, -1, 0, NULL, beastMoveForward, beastThinkChase, NULL };
AISTATE beastDodge = { kAiStateMove, 8, -1, 60, NULL, aiMoveDodge, NULL, &beastChase };
AISTATE beastGoto = { kAiStateMove, 8, -1, 600, NULL, beastMoveForward, beastThinkGoto, &beastIdle };
AISTATE beastSlash = { kAiStateChase, 6, nSlashClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastStomp = { kAiStateChase, 7, nStompClient, 120, NULL, NULL, NULL, &beastChase };
AISTATE beastSearch = { kAiStateSearch, 8, -1, 120, NULL, beastMoveForward, beastThinkSearch, &beastIdle };
AISTATE beastRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &beastDodge };
AISTATE beastSwimIdle = { kAiStateIdle, 9, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE beastSwimChase = { kAiStateChase, 9, -1, 0, NULL, sub_628A0, beastThinkSwimChase, NULL };
AISTATE beastSwimDodge = { kAiStateMove, 9, -1, 90, NULL, aiMoveDodge, NULL, &beastSwimChase };
AISTATE beastSwimGoto = { kAiStateMove, 9, -1, 600, NULL, beastMoveForward, beastThinkSwimGoto, &beastSwimIdle };
AISTATE beastSwimSearch = { kAiStateSearch, 9, -1, 120, NULL, beastMoveForward, beastThinkSearch, &beastSwimIdle };
AISTATE beastSwimSlash = { kAiStateChase, 9, nSlashClient, 0, NULL, NULL, beastThinkSwimChase, &beastSwimChase };
AISTATE beastSwimRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &beastSwimDodge };
AISTATE beastMorphToBeast = { kAiStateOther, -1, -1, 0, MorphToBeast, NULL, NULL, &beastIdle };
AISTATE beastMorphFromCultist = { kAiStateOther, 2576, -1, 0, NULL, NULL, NULL, &beastMorphToBeast };
AISTATE beast138FB4 = { kAiStateOther, 9, -1, 120, NULL, sub_62AE0, beastThinkSwimChase, &beastSwimChase };
AISTATE beast138FD0 = { kAiStateOther, 9, -1, 0, NULL, sub_62D7C, beastThinkSwimChase, &beastSwimChase };
AISTATE beast138FEC = { kAiStateOther, 9, -1, 120, NULL, aiMoveTurn, NULL, &beastSwimChase };

void SlashSeqCallback(int, DBloodActor* actor)
{
	spritetype* pSprite = &actor->s();
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	// Correct ?
	int dz = pSprite->pos.Z - target->spr.pos.Z;
	dx += Random3(4000 - 700 * gGameOptions.nDifficulty);
	dy += Random3(4000 - 700 * gGameOptions.nDifficulty);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	sfxPlay3DSound(actor, 9012 + Random(2), -1, 0);
}

void StompSeqCallback(int, DBloodActor* actor)
{
	spritetype* pSprite = &actor->s();
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	int x = pSprite->pos.X;
	int y = pSprite->pos.Y;
	int z = pSprite->pos.Z;
	int vc = 400;
	auto pSector = pSprite->sector();
	int v1c = 5 + 2 * gGameOptions.nDifficulty;
	int v10 = 25 + 30 * gGameOptions.nDifficulty;
	const bool newSectCheckMethod = !cl_bloodvanillaenemies && !VanillaMode(); // use new sector checking logic
	auto sectorMap = GetClosestSpriteSectors(pSector, x, y, vc, nullptr, newSectCheckMethod);
	int hit = HitScan(actor, pSprite->pos.Z, dx, dy, 0, CLIPMASK1, 0);
	DBloodActor* actor2 = nullptr;
	actHitcodeToData(hit, &gHitInfo, &actor2);

	vc <<= 4;
	BloodStatIterator it1(kStatDude);
	while (auto actor2 = it1.Next())
	{
		if (actor != actor2)
		{
			spritetype* pSprite2 = &actor2->s();
            if (actor2->hasX())
			{
				if (pSprite2->type == kDudeBeast)
					continue;
				if (pSprite2->flags & 32)
					continue;
                if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, x, y, z, pSector, vc))
				{
					int top, bottom;
					GetActorExtents(actor, &top, &bottom);
					if (abs(bottom - pSector->floorz) == 0)
					{
						int dx = abs(pSprite->pos.X - pSprite2->pos.X);
						int dy = abs(pSprite->pos.Y - pSprite2->pos.Y);
						int nDist2 = ksqrt(dx * dx + dy * dy);
						if (nDist2 <= vc)
						{
							int nDamage;
							if (!nDist2)
								nDamage = v1c + v10;
							else
								nDamage = v1c + ((vc - nDist2) * v10) / vc;
							if (IsPlayerSprite(pSprite2))
								gPlayer[pSprite2->type - kDudePlayer1].quakeEffect += nDamage * 4;
							actDamageSprite(actor, actor2, kDamageFall, nDamage << 4);
						}
					}
				}
			}
		}
	}
	it1.Reset(kStatThing);
	while (auto actor2 = it1.Next())
	{
		spritetype* pSprite2 = &actor2->s();
		if (pSprite2->flags & 32)
			continue;
		if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, x, y, z, pSector, vc))
		{
			XSPRITE* pXSprite = &actor2->x();
			if (pXSprite->locked)
				continue;
			int dx = abs(pSprite->pos.X - pSprite2->pos.X);
			int dy = abs(pSprite->pos.Y - pSprite2->pos.Y);
			int nDist2 = ksqrt(dx * dx + dy * dy);
			if (nDist2 <= vc)
			{
				int nDamage;
				if (!nDist2)
					nDamage = v1c + v10;
				else
					nDamage = v1c + ((vc - nDist2) * v10) / vc;
				if (IsPlayerSprite(pSprite2))
					gPlayer[pSprite2->type - kDudePlayer1].quakeEffect += nDamage * 4;
				actDamageSprite(actor, actor2, kDamageFall, nDamage << 4);
			}
		}
	}
	sfxPlay3DSound(actor, 9015 + Random(2), -1, 0);
}

static void MorphToBeast(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	actHealDude(actor, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
	pSprite->type = kDudeBeast;
}

static void beastThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void beastThinkGoto(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	auto pSector = pSprite->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
	}
	aiThinkTarget(actor);
}

static void beastThinkChase(DBloodActor* actor)
{
	auto const pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		auto pSector = pSprite->sector();
		auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = target->spr.pos.X - pSprite->pos.X;
	int dy = target->spr.pos.Y - pSprite->pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));

	auto pSector = pSprite->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (pXTarget->health == 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->spr.sector(), pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z - height, pSprite->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0? 0 : DivScale(target->spr.pos.Z - pSprite->pos.Z, nDist, 10);
				if (nDist < 0x1400 && nDist > 0xa00 && abs(nDeltaAngle) < 85 && (target->spr.flags & 2)
					&& target->IsPlayerActor() && Chance(0x8000))
				{
					int hit = HitScan(actor, pSprite->pos.Z, dx, dy, 0, CLIPMASK1, 0);
					if (pXTarget->health > (unsigned)gPlayerTemplate[0].startHealth / 2)
					{
						switch (hit)
						{
						case -1:
							if (!pXSector || !pXSector->Underwater)
								aiNewState(actor, &beastStomp);
							break;
						case 3:
							if (pSprite->type != gHitInfo.actor()->spr.type)
							{
								if (!pXSector || !pXSector->Underwater)
									aiNewState(actor, &beastStomp);
							}
							else
							{
								if (pXSector && pXSector->Underwater)
									aiNewState(actor, &beastSwimDodge);
								else
									aiNewState(actor, &beastDodge);
							}
							break;
						default:
							if (!pXSector || !pXSector->Underwater)
								aiNewState(actor, &beastStomp);
							break;
						}
					}
				}
				if (nDist < 921 && abs(nDeltaAngle) < 28)
				{
					int hit = HitScan(actor, pSprite->pos.Z, dx, dy, 0, CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &beastSwimSlash);
						else
							aiNewState(actor, &beastSlash);
						break;
					case 3:
						if (pSprite->type != gHitInfo.actor()->spr.type)
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &beastSwimSlash);
							else
								aiNewState(actor, &beastSlash);
						}
						else
						{
							if (pXSector && pXSector->Underwater)
								aiNewState(actor, &beastSwimDodge);
							else
								aiNewState(actor, &beastDodge);
						}
						break;
					default:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &beastSwimSlash);
						else
							aiNewState(actor, &beastSlash);
						break;
					}
				}
			}
			return;
		}
	}

	if (pXSector && pXSector->Underwater)
		aiNewState(actor, &beastSwimGoto);
	else
		aiNewState(actor, &beastGoto);
	actor->SetTarget(nullptr);
}

static void beastThinkSwimGoto(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(pSprite->ang - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &beastSwimSearch);
	aiThinkTarget(actor);
}

static void beastThinkSwimChase(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &beastSwimGoto);
		return;
	}
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	XSPRITE* pXTarget = &actor->GetTarget()->x();
	int dx = target->spr.pos.X - pSprite->pos.X;
	int dy = target->spr.pos.Y - pSprite->pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (pXTarget->health == 0)
	{
		aiNewState(actor, &beastSwimSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &beastSwimSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = pDudeInfo->eyeHeight + pSprite->pos.Z;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->spr.sector(), pSprite->pos.X, pSprite->pos.Y, pSprite->pos.Z - height, pSprite->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &beastSwimSlash);
				else
				{
					aiPlay3DSound(actor, 9009 + Random(2), AI_SFX_PRIORITY_1, -1);
					aiNewState(actor, &beast138FD0);
				}
			}
		}
		else
			aiNewState(actor, &beast138FD0);
		return;
	}
	aiNewState(actor, &beastSwimGoto);
	actor->SetTarget(nullptr);
}

static void beastMoveForward(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	if (abs(nAng) > 341)
		return;
	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
	int nDist = approxDist(dx, dy);
	if (nDist <= 0x400 && Random(64) < 32)
		return;
    actor->xvel += MulScale(pDudeInfo->frontSpeed, Cos(pSprite->ang), 30);
    actor->yvel += MulScale(pDudeInfo->frontSpeed, Sin(pSprite->ang), 30);
}

static void sub_628A0(DBloodActor* actor)
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
	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
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

static void sub_62AE0(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = pSprite->pos.Z + getDudeInfo(pSprite->type)->eyeHeight;
	int z2 = target->spr.pos.Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		pXSprite->goalAng = (pSprite->ang + 512) & 2047;
		return;
	}
	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
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

static void sub_62D7C(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = pSprite->pos.Z + getDudeInfo(pSprite->type)->eyeHeight;
	int z2 = target->spr.pos.Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		pSprite->ang = (pSprite->ang + 512) & 2047;
		return;
	}
	int dx = pXSprite->targetX - pSprite->pos.X;
	int dy = pXSprite->targetY - pSprite->pos.Y;
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
