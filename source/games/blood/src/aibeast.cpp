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
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	// Correct ?
	int dz = actor->int_pos().Z - target->int_pos().Z;
	dx += Random3(4000 - 700 * gGameOptions.nDifficulty);
	dy += Random3(4000 - 700 * gGameOptions.nDifficulty);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	sfxPlay3DSound(actor, 9012 + Random(2), -1, 0);
}

void StompSeqCallback(int, DBloodActor* actor)
{
	int angx = bcos(actor->int_ang());
	int angy = bsin(actor->int_ang());
	int x = actor->int_pos().X;
	int y = actor->int_pos().Y;
	int z = actor->int_pos().Z;
	const int vc = 400;
	auto pSector = actor->sector();
	int v1c = 5 + 2 * gGameOptions.nDifficulty;
	int v10 = 25 + 30 * gGameOptions.nDifficulty;
	const bool newSectCheckMethod = !cl_bloodvanillaenemies && !VanillaMode(); // use new sector checking logic
	auto sectorMap = GetClosestSpriteSectors(pSector, x, y, vc, nullptr, newSectCheckMethod);
	int hit = HitScan(actor, actor->int_pos().Z, angx, angy, 0, CLIPMASK1, 0);
	DBloodActor* actorh = nullptr;
	actHitcodeToData(hit, &gHitInfo, &actorh);

	BloodStatIterator it1(kStatDude);
	while (auto actor2 = it1.Next())
	{
		if (actor != actor2)
		{
			if (actor2->hasX())
			{
				if (actor2->spr.type == kDudeBeast)
					continue;
				if (actor2->spr.flags & 32)
					continue;
				if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, x, y, z, pSector, vc << 4))
				{
					int top, bottom;
					GetActorExtents(actor, &top, &bottom);
					if (abs(bottom - pSector->int_floorz()) == 0)
					{
						double nDist2 = (actor->spr.pos.XY() - actor2->spr.pos.XY()).Length();
						if (nDist2 <= vc)
						{
							int nDamage;
							if (nDist2 <= 0)
								nDamage = v1c + v10;
							else
								nDamage = v1c + v10 *  ((vc - nDist2) / vc);
							if (actor2->IsPlayerActor())
								gPlayer[actor2->spr.type - kDudePlayer1].quakeEffect += nDamage * 4;
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
		if (actor2->spr.flags & 32)
			continue;
		if (CheckSector(sectorMap, actor2) && CheckProximity(actor2, x, y, z, pSector, vc << 4))
		{
			if (actor2->xspr.locked)
				continue;

			double nDist2 = (actor->spr.pos.XY() - actor2->spr.pos.XY()).Length();
			if (nDist2 <= vc)
			{
				int nDamage;
				if (nDist2 <= 0)
					nDamage = v1c + v10;
				else
					nDamage = v1c + v10 * ((vc - nDist2) / vc);

				if (actor2->IsPlayerActor())
					gPlayer[actor2->spr.type - kDudePlayer1].quakeEffect += nDamage * 4;
				actDamageSprite(actor, actor2, kDamageFall, nDamage << 4);
			}
		}
	}
	sfxPlay3DSound(actor, 9015 + Random(2), -1, 0);
}

static void MorphToBeast(DBloodActor* actor)
{
	actHealDude(actor, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
	actor->spr.type = kDudeBeast;
}

static void beastThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

static void beastThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
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
	if (actor->GetTarget() == nullptr)
	{
		auto pSector = actor->sector();
		auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

		if (pXSector && pXSector->Underwater)
			aiNewState(actor, &beastSwimSearch);
		else
			aiNewState(actor, &beastSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));

	auto pSector = actor->sector();
	auto pXSector = pSector->hasX() ? &pSector->xs() : nullptr;

	if (target->xspr.health == 0)
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
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				actor->dudeSlope = nDist == 0 ? 0 : DivScale(target->int_pos().Z - actor->int_pos().Z, nDist, 10);
				if (nDist < 0x1400 && nDist > 0xa00 && abs(nDeltaAngle) < 85 && (target->spr.flags & 2)
					&& target->IsPlayerActor() && Chance(0x8000))
				{
					int hit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, CLIPMASK1, 0);
					if (target->xspr.health > (unsigned)gPlayerTemplate[0].startHealth / 2)
					{
						switch (hit)
						{
						case -1:
							if (!pXSector || !pXSector->Underwater)
								aiNewState(actor, &beastStomp);
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type)
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
					int hit = HitScan(actor, actor->int_pos().Z, dx, dy, 0, CLIPMASK1, 0);
					switch (hit)
					{
					case -1:
						if (pXSector && pXSector->Underwater)
							aiNewState(actor, &beastSwimSlash);
						else
							aiNewState(actor, &beastSlash);
						break;
					case 3:
						if (actor->spr.type != gHitInfo.actor()->spr.type)
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
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nAngle = getangle(dx, dy);
	int nDist = approxDist(dx, dy);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &beastSwimSearch);
	aiThinkTarget(actor);
}

static void beastThinkSwimChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &beastSwimGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();

	int dx = target->int_pos().X - actor->int_pos().X;
	int dy = target->int_pos().Y - actor->int_pos().Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
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
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->int_ang()) & 2047) - 1024;
		int height = pDudeInfo->eyeHeight + actor->int_pos().Z;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->int_pos().X, target->int_pos().Y, target->int_pos().Z, target->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - height, actor->sector()))
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
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	if (abs(nAng) > 341)
		return;
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (nDist <= 0x400 && Random(64) < 32)
		return;
	actor->vel.X += MulScale(pDudeInfo->frontSpeed, Cos(actor->int_ang()), 30);
	actor->vel.Y += MulScale(pDudeInfo->frontSpeed, Sin(actor->int_ang()), 30);
}

static void sub_628A0(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->set_int_ang((actor->int_ang() + 256) & 2047);
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int nDist = approxDist(dx, dy);
	if (Random(64) < 32 && nDist <= 0x400)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 2;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void sub_62AE0(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = actor->int_pos().Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->int_pos().Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->int_ang() + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int dz = z2 - z;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = -dz;
}

static void sub_62D7C(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int z = actor->int_pos().Z + getDudeInfo(actor->spr.type)->eyeHeight;
	int z2 = target->int_pos().Z + getDudeInfo(target->spr.type)->eyeHeight;
	int nAng = ((actor->xspr.goalAng + 1024 - actor->int_ang()) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->set_int_ang((actor->int_ang() + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->set_int_ang((actor->int_ang() + 512) & 2047);
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->int_pos().X;
	int dy = actor->xspr.TargetPos.Y - actor->int_pos().Y;
	int dz = (z2 - z) << 3;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = dz;
}

END_BLD_NS
