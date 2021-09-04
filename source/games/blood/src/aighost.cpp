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

static void ghostThinkTarget(DBloodActor*);
static void ghostThinkSearch(DBloodActor*);
static void ghostThinkGoto(DBloodActor*);
static void ghostMoveDodgeUp(DBloodActor*);
static void ghostMoveDodgeDown(DBloodActor*);
static void ghostThinkChase(DBloodActor*);
static void ghostMoveForward(DBloodActor*);
static void ghostMoveSlow(DBloodActor*);
static void ghostMoveSwoop(DBloodActor*);
static void ghostMoveFly(DBloodActor*);


AISTATE ghostIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, ghostThinkTarget, NULL };
AISTATE ghostChase = { kAiStateChase, 0, -1, 0, NULL, ghostMoveForward, ghostThinkChase, &ghostIdle };
AISTATE ghostGoto = { kAiStateMove, 0, -1, 600, NULL, ghostMoveForward, ghostThinkGoto, &ghostIdle };
AISTATE ghostSlash = { kAiStateChase, 6, nGhostSlashClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostThrow = { kAiStateChase, 6, nGhostThrowClient, 120, NULL, NULL, NULL, &ghostChase };
AISTATE ghostBlast = { kAiStateChase, 6, nGhostBlastClient, 120, NULL, ghostMoveSlow, NULL, &ghostChase };
AISTATE ghostRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostTeslaRecoil = { kAiStateRecoil, 4, -1, 0, NULL, NULL, NULL, &ghostChase };
AISTATE ghostSearch = { kAiStateSearch, 0, -1, 120, NULL, ghostMoveForward, ghostThinkSearch, &ghostIdle };
AISTATE ghostSwoop = { kAiStateOther, 0, -1, 120, NULL, ghostMoveSwoop, ghostThinkChase, &ghostChase };
AISTATE ghostFly = { kAiStateMove, 0, -1, 0, NULL, ghostMoveFly, ghostThinkChase, &ghostChase };
AISTATE ghostTurn = { kAiStateMove, 0, -1, 120, NULL, aiMoveTurn, NULL, &ghostChase };
AISTATE ghostDodgeUp = { kAiStateMove, 0, -1, 60, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeUpLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeUp, NULL, &ghostChase };
AISTATE ghostDodgeDown = { kAiStateMove, 0, -1, 120, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownRight = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };
AISTATE ghostDodgeDownLeft = { kAiStateMove, 0, -1, 90, NULL, ghostMoveDodgeDown, NULL, &ghostChase };

void ghostSlashSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	DUDEINFO* pDudeInfoT = getDudeInfo(pTarget->type);
	int height = (pSprite->yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (pTarget->yrepeat * pDudeInfoT->eyeHeight) << 2;
	int dz = height - height2;
	int dx = bcos(pSprite->ang);
	int dy = bsin(pSprite->ang);
	sfxPlay3DSound(actor, 1406, 0, 0);
	actFireVector(actor, 0, 0, dx, dy, dz, kVectorGhost);
	int r1 = Random(50);
	int r2 = Random(50);
	actFireVector(actor, 0, 0, dx + r2, dy - r1, dz, kVectorGhost);
	r1 = Random(50);
	r2 = Random(50);
	actFireVector(actor, 0, 0, dx - r2, dy + r1, dz, kVectorGhost);
}

void ghostThrowSeqCallback(int, DBloodActor* actor)
{
	actFireThing(actor, 0, 0, actor->dudeSlope - 7500, kThingBone, 0xeeeee);
}

void ghostBlastSeqCallback(int, DBloodActor* actor)
{
	XSPRITE* pXSprite = &actor->x();
	spritetype* pSprite = &actor->s();
	wrand(); // ???
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	spritetype* pTarget = &actor->GetTarget()->s();
	int height = (pSprite->yrepeat * getDudeInfo(pSprite->type)->eyeHeight) << 2;
	int dx = pXSprite->targetX - pSprite->x;
	int dy = pXSprite->targetY - pSprite->y;
	int x = pSprite->x;
	int y = pSprite->y;
	int z = height;
	TARGETTRACK tt = { 0x10000, 0x10000, 0x100, 0x55, 0x1aaaaa };
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
		if (tt.at10)
		{
			int t = DivScale(nDist, tt.at10, 12);
            x2 += (actor->xvel * t) >> 12;
            y2 += (actor->yvel * t) >> 12;
            z2 += (actor->zvel * t) >> 8;
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
			if (abs(nDeltaAngle) <= tt.at8)
			{
				int tz = pSprite2->z - pSprite->z;
				if (cansee(x, y, z, pSprite->sectnum, x2, y2, z2, pSprite2->sectnum))
				{
					nClosest = nDist2;
					aim.dx = bcos(nAngle);
					aim.dy = bsin(nAngle);
					aim.dz = DivScale(tz, nDist, 10);
					if (tz > -0x333)
						aim.dz = DivScale(tz, nDist, 10);
					else if (tz < -0x333 && tz > -0xb33)
						aim.dz = DivScale(tz, nDist, 10) + 9460;
					else if (tz < -0xb33 && tz > -0x3000)
						aim.dz = DivScale(tz, nDist, 10) + 9460;
					else if (tz < -0x3000)
						aim.dz = DivScale(tz, nDist, 10) - 7500;
					else
						aim.dz = DivScale(tz, nDist, 10);
				}
				else
					aim.dz = DivScale(tz, nDist, 10);
			}
		}
	}
#ifdef NOONE_EXTENSIONS
	// allow fire missile in non-player targets if not a demo
	if (IsPlayerSprite(pTarget) || gModernMap) {
		sfxPlay3DSound(actor, 489, 0, 0);
            actFireMissile(actor, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
	}
#else
	if (IsPlayerSprite(pTarget)) {
		sfxPlay3DSound(actor, 489, 0, 0);
            actFireMissile(actor, 0, 0, aim.dx, aim.dy, aim.dz, kMissileEctoSkull);
	}
#endif
}

static void ghostThinkTarget(DBloodActor* actor)
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
		aiNewState(actor, &ghostTurn);
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
				return;
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				pDudeExtraE->thinkTime = 0;
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
				return;
			}
		}
	}
}

static void ghostThinkSearch(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	aiChooseDirection(actor, pXSprite->goalAng);
	aiThinkTarget(actor);
}

static void ghostThinkGoto(DBloodActor* actor)
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
		aiNewState(actor, &ghostSearch);
	aiThinkTarget(actor);
}

static void ghostMoveDodgeUp(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	int nSprite = pSprite->index;
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int dx = actor->xvel;
    int dy = actor->yvel;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (pXSprite->dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel = -0x1d555;
}

static void ghostMoveDodgeDown(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	int nSprite = pSprite->index;
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	if (pXSprite->dodgeDir == 0)
		return;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int dx = actor->xvel;
    int dy = actor->yvel;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (pXSprite->dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
    actor->zvel = 0x44444;
}

static void ghostThinkChase(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &ghostGoto);
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
		aiNewState(actor, &ghostSearch);
		return;
	}
	if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &ghostSearch);
		return;
	}
	int nDist = approxDist(dx, dy);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = ((getangle(dx, dy) + 1024 - pSprite->ang) & 2047) - 1024;
		int height = (pDudeInfo->eyeHeight * pSprite->yrepeat) << 2;
		// Should be dudeInfo[pTarget->type-kDudeBase]
		int height2 = (pDudeInfo->eyeHeight * pTarget->yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				int floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
				switch (pSprite->type) {
				case kDudePhantasm:
					if (nDist < 0x2000 && nDist > 0x1000 && abs(nDeltaAngle) < 85) {
                        int hit = HitScan(actor, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							aiNewState(actor, &ghostBlast);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (pSprite->type != gHitInfo.hitactor->s().type && gHitInfo.hitactor->s().type != kDudePhantasm)
								aiNewState(actor, &ghostBlast);
							break;
						default:
							aiNewState(actor, &ghostBlast);
							break;
						}
					}
					else if (nDist < 0x400 && abs(nDeltaAngle) < 85)
					{
                        int hit = HitScan(actor, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
						switch (hit)
						{
						case -1:
							aiNewState(actor, &ghostSlash);
							break;
						case 0:
						case 4:
							break;
						case 3:
							if (pSprite->type != gHitInfo.hitactor->s().type && gHitInfo.hitactor->s().type != kDudePhantasm)
								aiNewState(actor, &ghostSlash);
							break;
						default:
							aiNewState(actor, &ghostSlash);
							break;
						}
					}
					else if ((height2 - height > 0x2000 || floorZ - bottom > 0x2000) && nDist < 0x1400 && nDist > 0x800)
					{
						aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
						aiNewState(actor, &ghostSwoop);
					}
					else if ((height2 - height < 0x2000 || floorZ - bottom < 0x2000) && abs(nDeltaAngle) < 85)
						aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
					break;
				}
			}
			return;
		}
		else
		{
			aiNewState(actor, &ghostFly);
			return;
		}
	}

	aiNewState(actor, &ghostGoto);
	actor->SetTarget(nullptr);
}

static void ghostMoveForward(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
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
	if ((unsigned int)Random(64) < 32 && nDist <= 0x400)
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
		t1 += nAccel >> 1;
    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void ghostMoveSlow(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
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
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
		return;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
    int vx = actor->xvel;
    int vy = actor->yvel;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 = nAccel >> 1;
	t2 >>= 1;
    actor->xvel = DMulScale(t1, nCos, t2, nSin, 30);
    actor->yvel = DMulScale(t1, nSin, -t2, nCos, 30);
	switch (pSprite->type) {
	case kDudePhantasm:
            actor->zvel = 0x44444;
		break;
	}
}

static void ghostMoveSwoop(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
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
	int nDist = approxDist(dx, dy);
	if (Chance(0x600) && nDist <= 0x400)
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
	switch (pSprite->type) {
	case kDudePhantasm:
            actor->zvel = t1;
		break;
	}
}

static void ghostMoveFly(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	///assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!(pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
		Printf(PRINT_HIGH, "pSprite->type >= kDudeBase && pSprite->type < kDudeMax");
		return;
	}
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
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
	switch (pSprite->type) {
	case kDudePhantasm:
            actor->zvel = -t1;
		break;
	}
}

END_BLD_NS
