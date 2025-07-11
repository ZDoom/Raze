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
	double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
	double height2 = (pDudeInfoT->eyeHeight * target->spr.scale.Y);
	DVector3 vec(actor->spr.Angles.Yaw.ToVector() * 64, height - height2);

	actFireVector(actor, 0, 0, vec, kVectorGargSlash);
	double r1 = RandomF(50, 8);
	double r2 = RandomF(50, 8);
	actFireVector(actor, 0, 0, vec + DVector2(r1, r2), kVectorGargSlash);
	r1 = RandomF(50, 8);
	r2 = RandomF(50, 8);
	actFireVector(actor, 0, 0, vec + DVector2(r1, r2), kVectorGargSlash);
}

void ThrowFSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0., 0., actor->dudeSlope * 0.25 - 0.11444, kThingBone, 0xeeeee / 65536.);
}

void BlastSSeqCallback(int, DBloodActor* actor)
{
	wrand(); // ???
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto target = actor->GetTarget();
	double height = (actor->spr.scale.Y * getDudeInfo(actor->spr.type)->eyeHeight);
	DVector3 pos(actor->spr.pos.XY(), height);

	DVector3 Aim(actor->spr.Angles.Yaw.ToVector(), actor->dudeSlope);
	double nClosest = 0x7fffffff;

	BloodStatIterator it(kStatDude);
	while (auto actor2 = it.Next())
	{
		if (actor == actor2 || !(actor2->spr.flags & 8))
			continue;

		auto pos2 = actor2->spr.pos;
		double nDist = (pos2 - pos).Length();
		if (nDist == 0 || nDist > 0x280)
			continue;

		pos2 += actor->vel * nDist * (65536. / 0x1aaaaa);

		DVector3 tvec = pos;
		tvec += actor->spr.Angles.Yaw.ToVector() * nDist;
		tvec.Z += actor->dudeSlope * nDist;

		double tsr = nDist * 9.23828125;
		double top, bottom;
		GetActorExtents(actor2, &top, &bottom);
		if (tvec.Z - tsr > bottom || tvec.Z + tsr < top)
			continue;

		double nDist2 = (tvec - pos2).Length();
		if (nDist2 < nClosest)
		{
			DAngle nAngle = (pos2.XY() - pos.XY()).Angle();
			DAngle nDeltaAngle = absangle(nAngle, actor->spr.Angles.Yaw);
			if (nDeltaAngle <= DAngle45)
			{
				double tz1 = actor2->spr.pos.Z - actor->spr.pos.Z;

				if (cansee(pos, actor->sector(), pos2, actor2->sector()))
				{
					nClosest = nDist2;
					Aim = DVector3(nAngle.ToVector(), tz1 / nDist);

					// This does not make any sense...
					if (tz1 < -3.2 && tz1 > -48)
						Aim.Z += 0.5774;
					else if (tz1 <= -48)
						Aim.Z -= 0.45776;
				}
				else
					Aim.Z = tz1 / nDist;
			}
		}
	}
	// allow to fire missile in non-player targets
	if (target->IsPlayerActor() || gModernMap) {
		actFireMissile(actor, -7.5, 0, Aim, kMissileArcGargoyle);
		actFireMissile(actor, 7.5, 0, Aim, kMissileArcGargoyle);
	}

}

void ThrowSSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0., 0., actor->dudeSlope * 0.25 - 0.11444, kThingBone, Chance(0x6000) ? 19.2 : 17.066666);
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
		actor->xspr.goalAng += DAngle45;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &gargoyleTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			if (pPlayer->GetActor()->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->GetActor()->spr.pos;
			auto dvect = ppos.XY() - actor->spr.pos;
			auto pSector = pPlayer->GetActor()->sector();
			double nDist = dvect.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvect.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, ppos);
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
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
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
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += FixedToFloat(pDudeInfo->sideSpeed);
		else
			t2 -= FixedToFloat(pDudeInfo->sideSpeed);
	});

	actor->vel.Z = FixedToFloat(-0x1d555);
}

static void gargMoveDodgeDown(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	if (actor->xspr.dodgeDir == 0)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->xspr.dodgeDir > 0)
			t2 += FixedToFloat(pDudeInfo->sideSpeed);
		else
			t2 -= FixedToFloat(pDudeInfo->sideSpeed);
	});

	actor->vel.Z = 4.26666;
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

	DVector2 dxy = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle dxyAngle = dxy.Angle();
	aiChooseDirection(actor, dxyAngle);
	if (target->xspr.health == 0)
	{
		aiNewState(actor, &gargoyleFSearch);
		return;
	}
	if (target->IsPlayerActor() && powerupCheck(getPlayer(target->spr.type - kDudePlayer1), kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &gargoyleFSearch);
		return;
	}
	double nDist = dxy.Length();
	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dxyAngle);
		double height = pDudeInfo->eyeHeight * actor->spr.scale.Y;
		// Should be dudeInfo[target->spr.type-kDudeBase]
		double height2 = pDudeInfo->eyeHeight * target->spr.scale.Y;
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				double floorZ = getflorzofslopeptr(actor->sector(), actor->spr.pos);
				double floorDelta = floorZ - bottom;
				double heightDelta = height2 - height;
				bool angWithinRange = nDeltaAngle < DAngle15;
				switch (actor->spr.type)
				{
				case kDudeGargoyleFlesh:
					if (nDist < 0x180 && nDist > 0xc0 && angWithinRange)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dxy, 0), CLIPMASK1, 0);
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
					else if (nDist < 0x40 && angWithinRange)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dxy, 0), CLIPMASK1, 0);
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
					else if ((heightDelta > 32 || floorDelta > 32) && nDist < 0x140 && nDist > 0xa0)
					{
						aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &gargoyleSwoop);
					}
					else if ((heightDelta < 32 || floorDelta < 32) && angWithinRange)
						aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
					break;
				case kDudeGargoyleStone:
					if (nDist < 0x180 && nDist > 0xc0 && angWithinRange)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dxy, 0), CLIPMASK1, 0);
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
					else if (nDist < 0x40 && angWithinRange)
					{
						int hit = HitScan(actor, actor->spr.pos.Z, DVector3(dxy, 0), CLIPMASK1, 0);
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
					else if ((heightDelta > 32 || floorDelta > 32) && nDist < 0x140 && nDist > 0x80)
					{
						if (actor->spr.type == kDudeGargoyleFlesh)
							aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
						else
							aiPlay3DSound(actor, 1450, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &gargoyleSwoop);
					}
					else if ((heightDelta < 32 || floorDelta < 32) && angWithinRange)
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
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.Angles.Yaw += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if ((unsigned int)Random(64) < 32 && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		if (actor->GetTarget() == nullptr)
			t1 += nAccel;
		else
			t1 += nAccel * 0.5;
	});

}

static void gargMoveSlow(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x600) && nDist <= 0x40)
		return;
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
		t2 *= 0.5;
	});

	switch (actor->spr.type) {
	case kDudeGargoyleFlesh:
		actor->vel.Z = 4.26666;
		break;
	case kDudeGargoyleStone:
		actor->vel.Z = FixedToFloat(0x35555);
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
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x600) && nDist <= 0x40)
		return;

	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
		switch (actor->spr.type) {
		case kDudeGargoyleFlesh:
			actor->vel.Z = t1;
			break;
		case kDudeGargoyleStone:
			actor->vel.Z = t1;
			break;
		}
	});

}

static void gargMoveFly(DBloodActor* actor)
{
	if (!(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax)) {
		Printf(PRINT_HIGH, "actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	double nAccel = pDudeInfo->FrontSpeed() * 4;
	if (abs(nAng) > DAngle60)
	{
		actor->spr.Angles.Yaw += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	double nDist = dvec.Length();
	if (Chance(0x4000) && nDist <= 0x40)
		return;
	
	AdjustVelocity(actor, ADJUSTER{
		t1 += nAccel * 0.5;
		switch (actor->spr.type) {
		case kDudeGargoyleFlesh:
			actor->vel.Z = -t1;
			break;
		case kDudeGargoyleStone:
			actor->vel.Z = -t1;
			break;
		}
	});

}

END_BLD_NS
