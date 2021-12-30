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

static void gargThinkTarget(DBloodActor*);
static void gargThinkSearch(DBloodActor*);
static void gargThinkGoto(DBloodActor*);
static void gargMoveDodgeUp(DBloodActor*);
static void gargMoveDodgeDown(DBloodActor*);
static void gargThinkChase(DBloodActor*);
static void entryFStatue(DBloodActor*);
static void entrySStatue(DBloodActor*);
static void gargMoveForward(DBloodActor*);
static void gargMoveSlow(DBloodActor*);
static void gargMoveSwoop(DBloodActor*);
static void gargMoveFly(DBloodActor*);
static void playStatueBreakSnd(DBloodActor*);

AISTATE gargoyleFIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, gargThinkTarget, NULL };
AISTATE gargoyleStatueIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE gargoyleFChase = { kAiStateChase, 0, -1, 0, NULL, gargMoveForward, gargThinkChase, &gargoyleFIdle };
AISTATE gargoyleFGoto = { kAiStateMove, 0, -1, 600, NULL, gargMoveForward, gargThinkGoto, &gargoyleFIdle };
AISTATE gargoyleFSlash = { kAiStateChase, 6, nSlashFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFThrow = { kAiStateChase, 6, nThrowFClient, 120, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleSThrow = { kAiStateChase, 6, nThrowSClient, 120, NULL, gargMoveForward, NULL, &gargoyleFChase };
AISTATE gargoyleSBlast = { kAiStateChase, 7, nBlastSClient, 60, NULL, gargMoveSlow, NULL, &gargoyleFChase };
AISTATE gargoyleFRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &gargoyleFChase };
AISTATE gargoyleFSearch = { kAiStateSearch, 0, -1, 120, NULL, gargMoveForward, gargThinkSearch, &gargoyleFIdle };
AISTATE gargoyleFMorph2 = { kAiStateOther, -1, -1, 0, entryFStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleFMorph = { kAiStateOther, 6, -1, 0, NULL, NULL, NULL, &gargoyleFMorph2 };
AISTATE gargoyleSMorph2 = { kAiStateOther, -1, -1, 0, entrySStatue, NULL, NULL, &gargoyleFIdle };
AISTATE gargoyleSMorph = { kAiStateOther, 6, -1, 0, NULL, NULL, NULL, &gargoyleSMorph2 };
AISTATE gargoyleSwoop = { kAiStateOther, 0, -1, 120, NULL, gargMoveSwoop, gargThinkChase, &gargoyleFChase };
AISTATE gargoyleFly = { kAiStateMove, 0, -1, 120, NULL, gargMoveFly, gargThinkChase, &gargoyleFChase };
AISTATE gargoyleTurn = { kAiStateMove, 0, -1, 120, NULL, aiMoveTurn, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeUp = { kAiStateMove, 0, -1, 60, NULL, gargMoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, gargMoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, gargMoveDodgeUp, NULL, &gargoyleFChase };
AISTATE gargoyleDodgeDown = { kAiStateMove, 0, -1, 120, NULL, gargMoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, gargMoveDodgeDown, NULL, &gargoyleFChase };
AISTATE gargoyleFDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, gargMoveDodgeDown, NULL, &gargoyleFChase };

AISTATE statueFBreakSEQ = { kAiStateOther, 5, -1, 0, entryFStatue, NULL, playStatueBreakSnd, &gargoyleFMorph2 };
AISTATE statueSBreakSEQ = { kAiStateOther, 5, -1, 0, entrySStatue, NULL, playStatueBreakSnd, &gargoyleSMorph2 };

static void playStatueBreakSnd(DBloodActor* actor) {

	aiPlay3DSound(actor, 313, AI_SFX_PRIORITY_1, -1);
}

void SlashFSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(target->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (target->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	int dz = height - height2;
	int dx = bcos(actor->spr.ang);
	int dy = bsin(actor->spr.ang);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGargSlash);
	int r1 = Random(50);
	int r2 = Random(50);
	actFireVector(actor, 0, 0, dx + r2, dy - r1, dz, kVectorGargSlash);
	r1 = Random(50);
	r2 = Random(50);
	actFireVector(actor, 0, 0, dx - r2, dy + r1, dz, kVectorGargSlash);
}

void ThrowFSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0, 0, actor->dudeSlope - 7500, kThingBone, 0xeeeee);
}

void BlastSSeqCallback(int, DBloodActor* actor)
{
	wrand(); // ???
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	int height = (actor->spr.yrepeat * getDudeInfo(actor->spr.type)->eyeHeight) << 2;
	int x = actor->spr.pos.X;
	int y = actor->spr.pos.Y;
	int z = height;
	TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
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
					if (tz1 > -0x333)
						aim.dz = DivScale(tz1, nDist, 10);
					else if (tz1 < -0x333 && tz1 > -0xb33)
						aim.dz = DivScale(tz1, nDist, 10) + 9460;
					else if (tz1 < -0xb33 && tz1 > -0x3000)
						aim.dz = DivScale(tz1, nDist, 10) + 9460;
					else if (tz1 < -0x3000)
						aim.dz = DivScale(tz1, nDist, 10) - 7500;
					else
						aim.dz = DivScale(tz1, nDist, 10);
				}
				else
					aim.dz = DivScale(tz1, nDist, 10);
			}
		}
	}
#ifdef NOONE_EXTENSIONS
	// allow to fire missile in non-player targets
	if (target->IsPlayerActor() || gModernMap) {
		actFireMissile(actor, -120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
		actFireMissile(actor, 120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
	}
#else
	if (target->IsPlayerActor()) {
		actFireMissile(actor, -120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
		actFireMissile(actor, 120, 0, aim.dx, aim.dy, aim.dz, kMissileArcGargoyle);
	}
#endif

}

void ThrowSSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0, 0, actor->dudeSlope - 7500, kThingBone, Chance(0x6000) ? 0x133333 : 0x111111);
}

static void gargThinkTarget(DBloodActor* actor)
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
		aiNewState(actor, &gargoyleTurn);
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

static void gargThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiLookForTarget(actor);
}

static void gargThinkGoto(DBloodActor* actor)
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
		aiNewState(actor, &gargoyleFSearch);
	aiThinkTarget(actor);
}

static void gargMoveDodgeUp(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int dx = actor->vel.X;
	int dy = actor->vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = -0x1d555;
}

static void gargMoveDodgeDown(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	if (actor->xspr.dodgeDir == 0)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int dx = actor->vel.X;
	int dy = actor->vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->vel.Z = 0x44444;
}

static void gargThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &gargoyleFGoto);
		return;
	}
	///assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto target = actor->GetTarget();

	int dx = target->spr.pos.X - actor->spr.pos.X;
	int dy = target->spr.pos.Y - actor->spr.pos.Y;
	aiChooseDirection(actor, getangle(dx, dy));
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &gargoyleFSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(&gPlayer[target->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &gargoyleFSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - actor->spr.ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		// Should be dudeInfo[target->spr.type-kDudeBase]
		int height2 = (pDudeInfo->eyeHeight * target->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos.X, target->spr.pos.Y, target->spr.pos.Z, target->sector(), actor->spr.pos.X, actor->spr.pos.Y, actor->spr.pos.Z - height, actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				int floorZ = getflorzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
				switch (actor->spr.type)
				{
				case kDudeGargoyleFlesh:
					if (nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							sfxPlay3DSound(actor, 1408, 0, 0);
							aiNewState(actor, &gargoyleFThrow);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeGargoyleStone)
							{
								sfxPlay3DSound(actor, 1408, 0, 0);
								aiNewState(actor, &gargoyleFThrow);
							}
							break;
						default:
							sfxPlay3DSound(actor, 1408, 0, 0);
							aiNewState(actor, &gargoyleFThrow);
							break;
						}
					}
					else if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							sfxPlay3DSound(actor, 1406, 0, 0);
							aiNewState(actor, &gargoyleFSlash);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeGargoyleStone)
							{
								sfxPlay3DSound(actor, 1406, 0, 0);
								aiNewState(actor, &gargoyleFSlash);
							}
							break;
						default:
							sfxPlay3DSound(actor, 1406, 0, 0);
							aiNewState(actor, &gargoyleFSlash);
							break;
						}
					}
					else if ((height2 - height > 0x2000 || floorZ - bottom > 0x2000) && nDist < 0x1400 && nDist > 0xa00)
					{
						aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &gargoyleSwoop);
					}
					else if ((height2 - height < 0x2000 || floorZ - bottom < 0x2000) && abs(nDeltaAngle) < 85)
						aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
					break;
				case kDudeGargoyleStone:
					if (nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							sfxPlay3DSound(actor, 1457, 0, 0);
							aiNewState(actor, &gargoyleSBlast);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeGargoyleFlesh)
							{
								sfxPlay3DSound(actor, 1457, 0, 0);
								aiNewState(actor, &gargoyleSBlast);
							}
							break;
						default:
							sfxPlay3DSound(actor, 1457, 0, 0);
							aiNewState(actor, &gargoyleSBlast);
							break;
						}
					}
					else if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							aiNewState(actor, &gargoyleFSlash);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (actor->spr.type != gHitInfo.actor()->spr.type && gHitInfo.actor()->spr.type != kDudeGargoyleFlesh)
								aiNewState(actor, &gargoyleFSlash);
							break;
						default:
							aiNewState(actor, &gargoyleFSlash);
							break;
						}
					}
					else if ((height2 - height > 0x2000 || floorZ - bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
					{
						if (actor->spr.type == kDudeGargoyleFlesh)
							aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
						else
							aiPlay3DSound(actor, 1450, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &gargoyleSwoop);
					}
					else if ((height2 - height < 0x2000 || floorZ - bottom < 0x2000) && abs(nDeltaAngle) < 85)
						aiPlay3DSound(actor, 1450, AI_SFX_PRIORITY_1, -1);
					break;
				}
			}
			return;
		}
		else
		{
			aiNewState(actor, &gargoyleFly);
			return;
		}
	}

	aiNewState(actor, &gargoyleFGoto);
	actor->SetTarget(nullptr);
}

static void entryFStatue(DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = &dudeInfo[6];
	actHealDude(actor, pDudeInfo->startHealth, pDudeInfo->startHealth);
	actor->spr.type = kDudeGargoyleFlesh;
}

static void entrySStatue(DBloodActor* actor)
{
	DUDEINFO* pDudeInfo = &dudeInfo[7];
	actHealDude(actor, pDudeInfo->startHealth, pDudeInfo->startHealth);
	actor->spr.type = kDudeGargoyleStone;
}

static void gargMoveForward(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.ang = (actor->spr.ang + 256) & 2047;
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nDist = approxDist(dx, dy);
	if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void gargMoveSlow(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 = nAccel >> 1;
	t2 >>= 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	switch (actor->spr.type) {
	case kDudeGargoyleFlesh:
		actor->vel.Z = 0x44444;
		break;
	case kDudeGargoyleStone:
		actor->vel.Z = 0x35555;
		break;
	}
}

static void gargMoveSwoop(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->xspr.goalAng = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	switch (actor->spr.type) {
	case kDudeGargoyleFlesh:
		actor->vel.Z = t1;
		break;
	case kDudeGargoyleStone:
		actor->vel.Z = t1;
		break;
	}
}

static void gargMoveFly(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	int nAng = ((actor->xspr.goalAng + 1024 - actor->spr.ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	actor->spr.ang = (actor->spr.ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > 341)
	{
		actor->spr.ang = (actor->spr.ang + 512) & 2047;
		return;
	}
	int dx = actor->xspr.TargetPos.X - actor->spr.pos.X;
	int dy = actor->xspr.TargetPos.Y - actor->spr.pos.Y;
	int nDist = approxDist(dx, dy);
	if (Chance(0x4000) && nDist <= 0x400)
		return;
	int nCos = Cos(actor->spr.ang);
	int nSin = Sin(actor->spr.ang);
	int vx = actor->vel.X;
	int vy = actor->vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->vel.X = DMulScale(t1, nCos, t2, nSin, 30);
	actor->vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	switch (actor->spr.type) {
	case kDudeGargoyleFlesh:
		actor->vel.Z = -t1;
		break;
	case kDudeGargoyleStone:
		actor->vel.Z = -t1;
		break;
	}
}

END_BLD_NS
