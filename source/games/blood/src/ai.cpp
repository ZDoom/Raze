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
#include "savegamehelp.h"

#include "blood.h"

BEGIN_BLD_NS

void RecoilDude(DBloodActor* actor);

AISTATE genIdle = { kAiStateGenIdle, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE genRecoil = { kAiStateRecoil, 5, -1, 20, NULL, NULL, NULL, &genIdle };

const int dword_138BB0[5] = { 0x2000, 0x4000, 0x8000, 0xa000, 0xe000 };

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool dudeIsPlayingSeq(DBloodActor* actor, int nSeq)
{
	auto pSprite = &actor->s();
	if (pSprite->statnum == kStatDude && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
	{
		DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
        if (seqGetID(actor) == pDudeInfo->seqStartID + nSeq && seqGetStatus(actor) >= 0)
			return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiPlay3DSound(DBloodActor* actor, int a2, AI_SFX_PRIORITY a3, int a4)
{
	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	if (a3 == AI_SFX_PRIORITY_0)
		sfxPlay3DSound(actor, a2, a4, 2);
	else if (a3 > pDudeExtra->prio || pDudeExtra->time <= PlayClock)
	{
		sfxKill3DSound(actor, -1, -1);
		sfxPlay3DSound(actor, a2, a4, 0);
		pDudeExtra->prio = a3;
		pDudeExtra->time = PlayClock + 120;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiNewState(DBloodActor* actor, AISTATE* pAIState)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	pXSprite->stateTimer = pAIState->stateTicks;
	pXSprite->aiState = pAIState;
	int seqStartId = pDudeInfo->seqStartID;

	if (pAIState->seqId >= 0)
	{
		seqStartId += pAIState->seqId;
		if (getSequence(seqStartId))
			seqSpawn(seqStartId, actor, pAIState->funcId);
	}

	if (pAIState->enterFunc)
		pAIState->enterFunc(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool isImmune(DBloodActor* actor, int dmgType, int minScale)
{

	if (dmgType >= kDmgFall && dmgType < kDmgMax && actor->hasX() && actor->x().locked != 1)
	{
		int type = actor->s().type;
		if (type >= kThingBase && type < kThingMax)
			return (thingInfo[type - kThingBase].dmgControl[dmgType] <= minScale);
		else if (actor->IsDudeActor())
		{
			if (actor->IsPlayerActor()) return (gPlayer[type - kDudePlayer1].godMode || gPlayer[type - kDudePlayer1].damageControl[dmgType] <= minScale);
			else return (dudeInfo[type - kDudeBase].damageVal[dmgType] <= minScale);
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CanMove(DBloodActor* actor, DBloodActor* target, int nAngle, int nRange)
{
	auto pSprite = &actor->s();
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	int x = pSprite->x;
	int y = pSprite->y;
	int z = pSprite->z;
	HitScan(actor, z, bcos(nAngle), bsin(nAngle), 0, CLIPMASK0, nRange);
	int nDist = approxDist(x - gHitInfo.hitx, y - gHitInfo.hity);
	if (nDist - (pSprite->clipdist << 2) < nRange)
	{
		if (gHitInfo.hitactor == nullptr || target == nullptr || target != gHitInfo.hitactor)
			return false;
		return true;
	}
	x += MulScale(nRange, Cos(nAngle), 30);
	y += MulScale(nRange, Sin(nAngle), 30);
	int nSector = pSprite->sectnum;
	assert(nSector >= 0 && nSector < kMaxSectors);
	if (!FindSector(x, y, z, &nSector))
		return false;
	int floorZ = getflorzofslope(nSector, x, y);
	int nXSector = sector[nSector].extra;
    bool Underwater = 0; 
    bool Water = 0; 
    bool Depth = 0; 
    bool Crusher = 0;
	XSECTOR* pXSector = NULL;
	if (nXSector > 0)
	{
		pXSector = &xsector[nXSector];
		if (pXSector->Underwater)
			Underwater = 1;
		if (pXSector->Depth)
			Depth = 1;
		if (sector[nSector].type == kSectorDamage || pXSector->damageType > 0)
			Crusher = 1;
	}
	auto Upper = getUpperLink(nSector);
	auto Lower = getLowerLink(nSector);
	if (Upper != nullptr)
	{
		if (Upper->s().type == kMarkerUpWater || Upper->s().type == kMarkerUpGoo)
			Water = Depth = 1;
	}
	if (Lower != nullptr)
	{
		if (Lower->s().type == kMarkerLowWater || Lower->s().type == kMarkerLowGoo)
			Depth = 1;
	}
	switch (pSprite->type) {
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone:
	case kDudeBat:
		if (pSprite->clipdist > nDist)
			return 0;
		if (Depth)
		{
			// Ouch...
			if (Depth)
				return false;
			if (Crusher)
				return false;
		}
		break;
	case kDudeBoneEel:
		if (Water)
			return false;
		if (!Underwater)
			return false;
		if (Underwater)
			return true;
		break;
	case kDudeCerberusTwoHead:
	case kDudeCerberusOneHead:
		// by NoOne: a quick fix for Cerberus spinning in E3M7-like maps, where damage sectors is used.
		// It makes ignore danger if enemy immune to N damageType. As result Cerberus start acting like
		// in Blood 1.0 so it can move normally to player. It's up to you for adding rest of enemies here as
		// i don't think it will broke something in game.
		if (!cl_bloodvanillaenemies && !VanillaMode() && Crusher && isImmune(actor, pXSector->damageType, 16)) return true;
		[[fallthrough]];
	case kDudeZombieButcher:
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
	case kDudeSpiderMother:
	case kDudeHellHound:
	case kDudeRat:
	case kDudeInnocent:
		if (Crusher)
			return false;
		if (Depth || Underwater)
			return false;
		if (floorZ - bottom > 0x2000)
			return false;
		break;
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
	case kDudeModernCustomBurning:
		if ((Crusher && !nnExtIsImmune(actor, pXSector->damageType)) || ((Water || Underwater) && !canSwim(actor))) return false;
		return true;
		[[fallthrough]];
#endif
	case kDudeZombieAxeNormal:
	case kDudePhantasm:
	case kDudeGillBeast:
	default:
		if (Crusher)
			return false;
		if ((nXSector < 0 || (!xsector[nXSector].Underwater && !xsector[nXSector].Depth)) && floorZ - bottom > 0x2000)
			return false;
		break;
	}
	return 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiChooseDirection(DBloodActor* actor, int a3)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();

	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	int vc = ((a3 + 1024 - pSprite->ang) & 2047) - 1024;
	int nCos = Cos(pSprite->ang);
	int nSin = Sin(pSprite->ang);
	int dx = actor->xvel;
	int dy = actor->yvel;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int vsi = ((t1 * 15) >> 12) / 2;
	int v8 = 341;
	if (vc < 0)
		v8 = -341;
	if (CanMove(actor, actor->GetTarget(), pSprite->ang + vc, vsi))
		pXSprite->goalAng = pSprite->ang + vc;
	else if (CanMove(actor, actor->GetTarget(), pSprite->ang + vc / 2, vsi))
		pXSprite->goalAng = pSprite->ang + vc / 2;
	else if (CanMove(actor, actor->GetTarget(), pSprite->ang - vc / 2, vsi))
		pXSprite->goalAng = pSprite->ang - vc / 2;
	else if (CanMove(actor, actor->GetTarget(), pSprite->ang + v8, vsi))
		pXSprite->goalAng = pSprite->ang + v8;
	else if (CanMove(actor, actor->GetTarget(), pSprite->ang, vsi))
		pXSprite->goalAng = pSprite->ang;
	else if (CanMove(actor, actor->GetTarget(), pSprite->ang - v8, vsi))
		pXSprite->goalAng = pSprite->ang - v8;
	//else if (pSprite->flags&2)
		//pXSprite->goalAng = pSprite->ang+341;
	else // Weird..
		pXSprite->goalAng = pSprite->ang + 341;
	if (Chance(0x8000))
		pXSprite->dodgeDir = 1;
	else
		pXSprite->dodgeDir = -1;
	if (!CanMove(actor, actor->GetTarget(), pSprite->ang + pXSprite->dodgeDir * 512, 512))
	{
		pXSprite->dodgeDir = -pXSprite->dodgeDir;
		if (!CanMove(actor, actor->GetTarget(), pSprite->ang + pXSprite->dodgeDir * 512, 512))
			pXSprite->dodgeDir = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveForward(DBloodActor* actor)
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
    actor->xvel += MulScale(pDudeInfo->frontSpeed, Cos(pSprite->ang), 30);
    actor->yvel += MulScale(pDudeInfo->frontSpeed, Sin(pSprite->ang), 30);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveTurn(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveDodge(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	int nAng = ((pXSprite->goalAng + 1024 - pSprite->ang) & 2047) - 1024;
	int nTurnRange = (pDudeInfo->angSpeed << 2) >> 4;
	pSprite->ang = (pSprite->ang + ClipRange(nAng, -nTurnRange, nTurnRange)) & 2047;
	if (pXSprite->dodgeDir)
	{
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
	}
}

//---------------------------------------------------------------------------
//
// todo: split this up.
//
//---------------------------------------------------------------------------

void aiActivateDude(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	if (!pXSprite->state)
	{
		aiChooseDirection(actor, getangle(pXSprite->targetX - pSprite->x, pXSprite->targetY - pSprite->y));
		pXSprite->state = 1;
	}
	switch (pSprite->type)
	{
	case kDudePhantasm:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &ghostSearch);
		else
		{
			aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &ghostChase);
		}
		break;
	}
	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
	case kDudeCultistBeast:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &cultistSearch);
				if (Chance(0x8000))
				{
					if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);
					else aiPlay3DSound(actor, 1008 + Random(5), AI_SFX_PRIORITY_1, -1);
				}
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
			{
				if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(actor, 4003 + Random(4), AI_SFX_PRIORITY_1, -1);
				else aiPlay3DSound(actor, 1003 + Random(4), AI_SFX_PRIORITY_1, -1);
			}
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				if (pSprite->type == kDudeCultistTommy) aiNewState(actor, &fanaticChase);
				else aiNewState(actor, &cultistChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimChase);
				break;
			}
		}
		break;
	}
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
		{
			if (spriteIsUnderwater(actor, false))  aiGenDudeNewState(actor, &genDudeSearchW);
			else aiGenDudeNewState(actor, &genDudeSearchL);
		}
		else
		{
			if (Chance(0x4000)) playGenDudeSound(actor,kGenDudeSndTargetSpot);
			if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeChaseW);
			else aiGenDudeNewState(actor, &genDudeChaseL);
		}
		break;
	}
	case kDudeModernCustomBurning:
		if (actor->GetTarget() == nullptr) aiGenDudeNewState(actor, &genDudeBurnSearch);
		else aiGenDudeNewState(actor, &genDudeBurnChase);
		break;
#endif
	case kDudeCultistTommyProne:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 1;
		pSprite->type = kDudeCultistTommy;
		if (actor->GetTarget() == nullptr)
		{
			switch (pXSprite->medium)
			{
			case 0:
				aiNewState(actor, &cultistSearch);
				if (Chance(0x8000)) aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);

			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &cultistProneChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeCultistShotgunProne:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 1;
		pSprite->type = kDudeCultistShotgun;
		if (actor->GetTarget() == nullptr)
		{
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &cultistSearch);
				if (Chance(0x8000))
					aiPlay3DSound(actor, 1008 + Random(5), AI_SFX_PRIORITY_1, -1);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 1003 + Random(4), AI_SFX_PRIORITY_1, -1);
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &cultistProneChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &cultistSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeBurningCultist:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &cultistBurnSearch);
		else
			aiNewState(actor, &cultistBurnChase);
		break;
	case kDudeBat:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (!pSprite->flags)
			pSprite->flags = 9;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &batSearch);
		else
		{
			if (Chance(0xa000))
				aiPlay3DSound(actor, 2000, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &batChase);
		}
		break;
	}
	case kDudeBoneEel:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &eelSearch);
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 1501, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1500, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &eelChase);
		}
		break;
	}
	case kDudeGillBeast:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		XSECTOR* pXSector = NULL;
		if (sector[pSprite->sectnum].extra > 0)
			pXSector = &xsector[sector[pSprite->sectnum].extra];
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
		{
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, &gillBeastSwimSearch);
			else
				aiNewState(actor, &gillBeastSearch);
		}
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1701, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1700, AI_SFX_PRIORITY_1, -1);
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, &gillBeastSwimChase);
			else
				aiNewState(actor, &gillBeastChase);
		}
		break;
	}
	case kDudeZombieAxeNormal:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &zombieASearch);
		else
		{
			if (Chance(0xa000))
			{
				switch (Random(3))
				{
				default:
				case 0:
				case 3:
					aiPlay3DSound(actor, 1103, AI_SFX_PRIORITY_1, -1);
					break;
				case 1:
					aiPlay3DSound(actor, 1104, AI_SFX_PRIORITY_1, -1);
					break;
				case 2:
					aiPlay3DSound(actor, 1105, AI_SFX_PRIORITY_1, -1);
					break;
				}
			}
			aiNewState(actor, &zombieAChase);
		}
		break;
	}
	case kDudeZombieAxeBuried:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (pXSprite->aiState == &zombieEIdle) aiNewState(actor, &zombieEUp);
		break;
	}
	case kDudeZombieAxeLaying:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (pXSprite->aiState == &zombieSIdle) aiNewState(actor, &zombie13AC2C);
		break;
	}
	case kDudeZombieButcher:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &zombieFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1201, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1200, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &zombieFChase);
		}
		break;
	}
	case kDudeBurningZombieAxe:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &zombieABurnSearch);
		else
			aiNewState(actor, &zombieABurnChase);
		break;
	case kDudeBurningZombieButcher:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &zombieFBurnSearch);
		else
			aiNewState(actor, &zombieFBurnChase);
		break;
	case kDudeGargoyleFlesh: {
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &gargoyleFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &gargoyleFChase);
		}
		break;
	}
	case kDudeGargoyleStone:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &gargoyleFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1451, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1450, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &gargoyleFChase);
		}
		break;
	}
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:

#ifdef NOONE_EXTENSIONS
		// play gargoyle statue breaking animation if data1 = 1.
		if (gModernMap && pXSprite->data1 == 1)
		{
			if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &statueFBreakSEQ);
			else aiNewState(actor, &statueSBreakSEQ);
		}
		else
		{
			if (Chance(0x4000)) aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
			else aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);

			if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &gargoyleFMorph);
			else aiNewState(actor, &gargoyleSMorph);
		}
#else
		if (Chance(0x4000)) aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
		else aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);

		if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &gargoyleFMorph);
		else aiNewState(actor, &gargoyleSMorph);
#endif
		break;
	case kDudeCerberusTwoHead:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &cerberusSearch);
		else
		{
			aiPlay3DSound(actor, 2300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &cerberusChase);
		}
		break;
	case kDudeCerberusOneHead:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &cerberus2Search);
		else
		{
			aiPlay3DSound(actor, 2300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &cerberus2Chase);
		}
		break;
	case kDudeHellHound:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &houndSearch);
		else
		{
			aiPlay3DSound(actor, 1300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &houndChase);
		}
		break;
	case kDudeHand:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &handSearch);
		else
		{
			aiPlay3DSound(actor, 1900, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &handChase);
		}
		break;
	case kDudeRat:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &ratSearch);
		else
		{
			aiPlay3DSound(actor, 2100, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &ratChase);
		}
		break;
	case kDudeInnocent:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &innocentSearch);
		else
		{
			if (pXSprite->health > 0)
				aiPlay3DSound(actor, 7000 + Random(6), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &innocentChase);
		}
		break;
	case kDudeTchernobog:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &tchernobogSearch);
		else
		{
			aiPlay3DSound(actor, 2350 + Random(7), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &tchernobogChase);
		}
		break;
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
		pSprite->flags |= 2;
		pSprite->cstat &= ~8;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &spidSearch);
		else
		{
			aiPlay3DSound(actor, 1800, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &spidChase);
		}
		break;
	case kDudeSpiderMother:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->active = 1;
		pSprite->flags |= 2;
		pSprite->cstat &= ~8;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &spidSearch);
		else
		{
			aiPlay3DSound(actor, 1853 + Random(1), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &spidChase);
		}
		break;
	}
	case kDudeTinyCaleb:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &tinycalebSearch);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &tinycalebSwimSearch);
				break;
			}
		}
		else
		{
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &tinycalebChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &tinycalebSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeBeast:
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &beastSearch);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &beastSwimSearch);
				break;
			}
		}
		else
		{
			aiPlay3DSound(actor, 9009 + Random(2), AI_SFX_PRIORITY_1, -1);
			switch (pXSprite->medium)
			{
			case kMediumNormal:
				aiNewState(actor, &beastChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, &beastSwimChase);
				break;
			}
		}
		break;
	}
	case kDudePodGreen:
	case kDudePodFire:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &podSearch);
		else
		{
			if (pSprite->type == kDudePodFire)
				aiPlay3DSound(actor, 2453, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 2473, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &podChase);
		}
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, &tentacleSearch);
		else
		{
			aiPlay3DSound(actor, 2503, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, &tentacleChase);
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiSetTarget(DBloodActor* actor, int x, int y, int z)
{
	auto pXSprite = &actor->x();
	actor->SetTarget(nullptr);
	pXSprite->targetX = x;
	pXSprite->targetY = y;
	pXSprite->targetZ = z;
}

void aiSetTarget(DBloodActor* actor, DBloodActor* target)
{
	if (target == nullptr)
	{
		actor->SetTarget(nullptr);
		return;
	}
	auto pXSprite = &actor->x();
	spritetype* pTarget = &target->s();
	if (pTarget->type >= kDudeBase && pTarget->type < kDudeMax)
	{
		if (actor->GetOwner() != target)
		{
			actor->SetTarget(target);
			DUDEINFO* pDudeInfo = getDudeInfo(pTarget->type);
			pXSprite->targetX = pTarget->x;
			pXSprite->targetY = pTarget->y;
			pXSprite->targetZ = pTarget->z - ((pDudeInfo->eyeHeight * pTarget->yrepeat) << 2);
		}
	}
}

//---------------------------------------------------------------------------
//
// todo: split up and put most of its content in tables.
//
//---------------------------------------------------------------------------

int aiDamageSprite(DBloodActor* source, DBloodActor* actor, DAMAGE_TYPE nDmgType, int nDamage)
{
	auto pSprite = &actor->s();
	XSPRITE* pXSprite = &actor->x();

	if (!pXSprite->health)
		return 0;
	pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);
	actor->cumulDamage += nDamage;
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);

	if (source)
	{
		spritetype* pSource = &source->s();
		if (pSprite == pSource) return 0;
		else if (actor->GetTarget() == nullptr) // if no target, give the dude a target
		{
			aiSetTarget(actor, source);
			aiActivateDude(actor);
		}
		else if (source != actor->GetTarget()) // if found a new target, retarget
		{
			int nThresh = nDamage;
			if (pSprite->type == pSource->type)
				nThresh *= pDudeInfo->changeTargetKin;
			else
				nThresh *= pDudeInfo->changeTarget;
			if (Chance(nThresh))
			{
				aiSetTarget(actor, source);
				aiActivateDude(actor);
			}
		}

#ifdef NOONE_EXTENSIONS
		if (gModernMap) {

			// for enemies in patrol mode
			if (aiInPatrolState(pXSprite->aiState))
			{
                aiPatrolStop(actor, source, pXSprite->dudeAmbush);

				PLAYER* pPlayer = getPlayerById(pSource->type);
				if (!pPlayer) return nDamage;
				if (powerupCheck(pPlayer, kPwUpShadowCloak)) pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
				if (readyForCrit(source, actor)) 
				{
					nDamage += aiDamageSprite(actor, source, nDmgType, nDamage * (10 - gGameOptions.nDifficulty));
					if (pXSprite->health > 0)
					{
						int fullHp = (pXSprite->sysData2 > 0) ? ClipRange(pXSprite->sysData2 << 4, 1, 65535) : getDudeInfo(pSprite->type)->startHealth << 4;
						if (((100 * pXSprite->health) / fullHp) <= 75)
						{
							actor->cumulDamage += nDamage << 4; // to be sure any enemy will play the recoil animation
                            RecoilDude(actor);
						}
					}

                    DPrintf(DMSG_SPAMMY, "Player #%d does the critical damage to patrol dude #%d!", pPlayer->nPlayer + 1, actor->GetIndex());
				}

				return nDamage;
			}

			if (pSprite->type == kDudeModernCustomBurning)
			{
				if (Chance(0x2000) && actor->dudeExtra.time < PlayClock) {
					playGenDudeSound(actor,kGenDudeSndBurning);
					actor->dudeExtra.time = PlayClock + 360;
				}

				if (pXSprite->burnTime == 0) pXSprite->burnTime = 2400;
				if (spriteIsUnderwater(actor, false)) 
				{
					pSprite->type = kDudeModernCustom;
					pXSprite->burnTime = 0;
					pXSprite->health = 1; // so it can be killed with flame weapons while underwater and if already was burning dude before.
					aiGenDudeNewState(actor, &genDudeGotoW);
				}

				return nDamage;

			}

			if (pSprite->type == kDudeModernCustom)
			{
				GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
				if (nDmgType == kDamageBurn)
				{
					if (pXSprite->health > (uint32_t)pDudeInfo->fleeHealth) return nDamage;
					else if (pXSprite->txID <= 0 || getNextIncarnation(actor) == nullptr) 
					{
						removeDudeStuff(actor);

						if (pExtra->weaponType == kGenDudeWeaponKamikaze)
							doExplosion(actor, pXSprite->data1 - kTrapExploder);

						if (spriteIsUnderwater(actor)) 
						{
							pXSprite->health = 0;
							return nDamage;
						}

						if (pXSprite->burnTime <= 0)
							pXSprite->burnTime = 1200;

						if (pExtra->canBurn && pExtra->availDeaths[kDamageBurn] > 0) {

							aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
							playGenDudeSound(actor,kGenDudeSndBurning);
							pSprite->type = kDudeModernCustomBurning;

							if (pXSprite->data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
								pSprite->pal = 0;

							aiGenDudeNewState(actor, &genDudeBurnGoto);
							actHealDude(actor, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
							actor->dudeExtra.time = PlayClock + 360;
							evKillActor(actor, kCallbackFXFlameLick);

						}
					}
					else
					{
						actKillDude(actor, actor, kDamageFall, 65535);
					}
				} 
				else if (canWalk(actor) && !inDodge(pXSprite->aiState) && !inRecoil(pXSprite->aiState))
				{
					if (!dudeIsMelee(actor)) 
					{
						if (inIdle(pXSprite->aiState) || Chance(getDodgeChance(actor))) 
						{
							if (!spriteIsUnderwater(actor)) 
							{
								if (!canDuck(actor) || !dudeIsPlayingSeq(actor, 14))  aiGenDudeNewState(actor, &genDudeDodgeShortL);
								else aiGenDudeNewState(actor, &genDudeDodgeShortD);

								if (Chance(0x0200))
									playGenDudeSound(actor,kGenDudeSndGotHit);

							}
							else if (dudeIsPlayingSeq(actor, 13))
							{
								aiGenDudeNewState(actor, &genDudeDodgeShortW);
							}
						}
					}
					else if (Chance(0x0200))
					{
						playGenDudeSound(actor,kGenDudeSndGotHit);
					}
				}
				return nDamage;
			}
		}
#endif

		if (nDmgType == kDamageTesla)
		{
			DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
			pDudeExtra->teslaHit = 1;
		}
		else if (!VanillaMode()) // reset tesla hit state if received different type of damage
		{
			DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
			pDudeExtra->teslaHit = 0;
		}
		const bool fixRandomCultist = !cl_bloodvanillaenemies && (pSprite->inittype >= kDudeBase) && (pSprite->inittype < kDudeMax) && !VanillaMode(); // fix burning cultists randomly switching types underwater
		switch (pSprite->type)
		{
		case kDudeCultistTommy:
		case kDudeCultistShotgun:
		case kDudeCultistTesla:
		case kDudeCultistTNT:
			if (nDmgType != kDamageBurn)
			{
				if (!dudeIsPlayingSeq(actor, 14) && !pXSprite->medium)
					aiNewState(actor, &cultistDodge);
				else if (dudeIsPlayingSeq(actor, 14) && !pXSprite->medium)
					aiNewState(actor, &cultistProneDodge);
				else if (dudeIsPlayingSeq(actor, 13) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
					aiNewState(actor, &cultistSwimDodge);
			}
			else if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
			{
				pSprite->type = kDudeBurningCultist;
				aiNewState(actor, &cultistBurnGoto);
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1031 + Random(2), AI_SFX_PRIORITY_2, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
				evKillActor(actor, kCallbackFXFlameLick);
			}
			break;
		case kDudeInnocent:
			if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
			{
				pSprite->type = kDudeBurningInnocent;
				aiNewState(actor, &cultistBurnGoto);
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
				evKillActor(actor, kCallbackFXFlameLick);
			}
			break;
		case kDudeBurningCultist:
			if (Chance(0x4000) && actor->dudeExtra.time < PlayClock)
			{
				aiPlay3DSound(actor, 1031 + Random(2), AI_SFX_PRIORITY_2, -1);
				actor->dudeExtra.time = PlayClock + 360;
			}
			if (Chance(0x600) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
			{
				pSprite->type = kDudeCultistTommy;
				if (fixRandomCultist) // fix burning cultists randomly switching types underwater
					pSprite->type = pSprite->inittype; // restore back to spawned cultist type
				pXSprite->burnTime = 0;
				aiNewState(actor, &cultistSwimGoto);
			}
			else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
			{
				pSprite->type = kDudeCultistShotgun;
				if (fixRandomCultist) // fix burning cultists randomly switching types underwater
					pSprite->type = pSprite->inittype; // restore back to spawned cultist type
				pXSprite->burnTime = 0;
				aiNewState(actor, &cultistSwimGoto);
			}
			break;
		case kDudeGargoyleFlesh:
			aiNewState(actor, &gargoyleFChase);
			break;
		case kDudeZombieButcher:
			if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth) {
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
				pSprite->type = kDudeBurningZombieButcher;
				aiNewState(actor, &zombieFBurnGoto);
				actHealDude(actor, dudeInfo[42].startHealth, dudeInfo[42].startHealth);
				evKillActor(actor, kCallbackFXFlameLick);
			}
			break;
		case kDudeTinyCaleb:
			if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
			{
				if (!cl_bloodvanillaenemies && !VanillaMode()) // fix burning sprite for tiny caleb
				{
					pSprite->type = kDudeBurningTinyCaleb;
					aiNewState(actor, &tinycalebBurnGoto);
				}
				else
				{
					pSprite->type = kDudeBurningInnocent;
					aiNewState(actor, &cultistBurnGoto);
				}
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
				evKillActor(actor, kCallbackFXFlameLick);
			}
			break;
		case kDudeCultistBeast:
			if (pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
			{
				pSprite->type = kDudeBeast;
				aiPlay3DSound(actor, 9008, AI_SFX_PRIORITY_1, -1);
				aiNewState(actor, &beastMorphFromCultist);
				actHealDude(actor, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
			}
			break;
		case kDudeZombieAxeNormal:
		case kDudeZombieAxeBuried:
			if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
			{
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
				pSprite->type = kDudeBurningZombieAxe;
				aiNewState(actor, &zombieABurnGoto);
				actHealDude(actor, dudeInfo[41].startHealth, dudeInfo[41].startHealth);
				evKillActor(actor, kCallbackFXFlameLick);
			}
			break;
		}
	}
	return nDamage;
}

//---------------------------------------------------------------------------
//
// todo: split up and put most of its content in tables.
//
//---------------------------------------------------------------------------

void RecoilDude(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	char v4 = Chance(0x8000);
	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	if (pSprite->statnum == kStatDude && (pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
	{
		DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
		switch (pSprite->type)
		{
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustom:
		{
			GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
			int rChance = getRecoilChance(actor);
			if (pExtra->canElectrocute && pDudeExtra->teslaHit && !spriteIsUnderwater(actor, false))
			{
				if (Chance(rChance << 3) || (dudeIsMelee(actor) && Chance(rChance << 4))) aiGenDudeNewState(actor, &genDudeRecoilTesla);
				else if (pExtra->canRecoil && Chance(rChance)) aiGenDudeNewState(actor, &genDudeRecoilL);
				else if (canWalk(actor))
				{

					if (Chance(rChance >> 2)) aiGenDudeNewState(actor, &genDudeDodgeL);
					else if (Chance(rChance >> 1)) aiGenDudeNewState(actor, &genDudeDodgeShortL);

				}

			}
			else if (pExtra->canRecoil && Chance(rChance))
			{
				if (inDuck(pXSprite->aiState) && Chance(rChance >> 2)) aiGenDudeNewState(actor, &genDudeRecoilD);
				else if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeRecoilW);
				else aiGenDudeNewState(actor, &genDudeRecoilL);
			}

			short rState = inRecoil(pXSprite->aiState);
			if (rState > 0)
			{
				if (!canWalk(actor))
				{
					if (rState == 1) pXSprite->aiState->nextState = &genDudeChaseNoWalkL;
					else if (rState == 2) pXSprite->aiState->nextState = &genDudeChaseNoWalkD;
					else pXSprite->aiState->nextState = &genDudeChaseNoWalkW;

				}
				else if (!dudeIsMelee(actor) || Chance(rChance >> 2))
				{
					if (rState == 1) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeL : &genDudeDodgeShortL);
					else if (rState == 2) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeD : &genDudeDodgeShortD);
					else if (rState == 3) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeW : &genDudeDodgeShortW);
				}
				else if (rState == 1) pXSprite->aiState->nextState = &genDudeChaseL;
				else if (rState == 2) pXSprite->aiState->nextState = &genDudeChaseD;
				else pXSprite->aiState->nextState = &genDudeChaseW;

				playGenDudeSound(actor,kGenDudeSndGotHit);

			}

			pDudeExtra->teslaHit = 0;
			break;
		}
#endif
		case kDudeCultistTommy:
		case kDudeCultistShotgun:
		case kDudeCultistTesla:
		case kDudeCultistTNT:
		case kDudeCultistBeast:
			if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(actor, 4013 + Random(2), AI_SFX_PRIORITY_2, -1);
			else aiPlay3DSound(actor, 1013 + Random(2), AI_SFX_PRIORITY_2, -1);

			if (!v4 && pXSprite->medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit) aiNewState(actor, &cultistTeslaRecoil);
				else aiNewState(actor, &cultistRecoil);

			}
			else if (v4 && pXSprite->medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit) aiNewState(actor, &cultistTeslaRecoil);
				else if (gGameOptions.nDifficulty > 0) aiNewState(actor, &cultistProneRecoil);
				else aiNewState(actor, &cultistRecoil);
			}
			else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
				aiNewState(actor, &cultistSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, &cultistTeslaRecoil);
				else
					aiNewState(actor, &cultistRecoil);
			}
			break;
		case kDudeBurningCultist:
			aiNewState(actor, &cultistBurnGoto);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiGenDudeNewState(actor, &genDudeBurnGoto);
			break;
#endif
		case kDudeZombieButcher:
			aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, &zombieFTeslaRecoil);
			else
				aiNewState(actor, &zombieFRecoil);
			break;
		case kDudeZombieAxeNormal:
		case kDudeZombieAxeBuried:
			aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit && pXSprite->data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, &zombieATeslaRecoil);
			else if (pXSprite->data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, &zombieARecoil2);
			else
				aiNewState(actor, &zombieARecoil);
			break;
		case kDudeBurningZombieAxe:
			aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &zombieABurnGoto);
			break;
		case kDudeBurningZombieButcher:
			aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &zombieFBurnGoto);
			break;
		case kDudeGargoyleFlesh:
		case kDudeGargoyleStone:
			aiPlay3DSound(actor, 1402, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &gargoyleFRecoil);
			break;
		case kDudeCerberusTwoHead:
			aiPlay3DSound(actor, 2302 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit && pXSprite->data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, &cerberusTeslaRecoil);
			else
				aiNewState(actor, &cerberusRecoil);
			break;
		case kDudeCerberusOneHead:
			aiPlay3DSound(actor, 2302 + Random(2), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &cerberus2Recoil);
			break;
		case kDudeHellHound:
			aiPlay3DSound(actor, 1302, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, &houndTeslaRecoil);
			else
				aiNewState(actor, &houndRecoil);
			break;
		case kDudeTchernobog:
			aiPlay3DSound(actor, 2370 + Random(2), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &tchernobogRecoil);
			break;
		case kDudeHand:
			aiPlay3DSound(actor, 1902, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &handRecoil);
			break;
		case kDudeRat:
			aiPlay3DSound(actor, 2102, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &ratRecoil);
			break;
		case kDudeBat:
			aiPlay3DSound(actor, 2002, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &batRecoil);
			break;
		case kDudeBoneEel:
			aiPlay3DSound(actor, 1502, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &eelRecoil);
			break;
		case kDudeGillBeast: {
			XSECTOR* pXSector = NULL;
			if (sector[pSprite->sectnum].extra > 0)
				pXSector = &xsector[sector[pSprite->sectnum].extra];
			aiPlay3DSound(actor, 1702, AI_SFX_PRIORITY_2, -1);
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, &gillBeastSwimRecoil);
			else
				aiNewState(actor, &gillBeastRecoil);
			break;
		}
		case kDudePhantasm:
			aiPlay3DSound(actor, 1602, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, &ghostTeslaRecoil);
			else
				aiNewState(actor, &ghostRecoil);
			break;
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
			aiPlay3DSound(actor, 1802 + Random(1), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &spidDodge);
			break;
		case kDudeSpiderMother:
			aiPlay3DSound(actor, 1851 + Random(1), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, &spidDodge);
			break;
		case kDudeInnocent:
			aiPlay3DSound(actor, 7007 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, &innocentTeslaRecoil);
			else
				aiNewState(actor, &innocentRecoil);
			break;
		case kDudeTinyCaleb:
			if (pXSprite->medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, &tinycalebTeslaRecoil);
				else
					aiNewState(actor, &tinycalebRecoil);
			}
			else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
				aiNewState(actor, &tinycalebSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, &tinycalebTeslaRecoil);
				else
					aiNewState(actor, &tinycalebRecoil);
			}
			break;
		case kDudeBeast:
			aiPlay3DSound(actor, 9004 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (pXSprite->medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, &beastTeslaRecoil);
				else
					aiNewState(actor, &beastRecoil);
			}
			else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
				aiNewState(actor, &beastSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, &beastTeslaRecoil);
				else
					aiNewState(actor, &beastRecoil);
			}
			break;
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, &podRecoil);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, &tentacleRecoil);
			break;
		default:
			aiNewState(actor, &genRecoil);
			break;
		}
		pDudeExtra->teslaHit = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void aiThinkTarget(DBloodActor* actor)
{
	auto pXSprite = &actor->x();
	auto pSprite = &actor->s();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
            if (actor->GetOwner() == pPlayer->actor || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
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
                aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
				return;
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
				return;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiLookForTarget(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
            if (actor->GetOwner() == pPlayer->actor || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
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
                aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
				return;
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				aiSetTarget(actor, x, y, z);
				aiActivateDude(actor);
				return;
			}
		}
		if (pXSprite->state)
		{
			uint8_t sectmap[(kMaxSectors + 7) >> 3];
			const bool newSectCheckMethod = !cl_bloodvanillaenemies && !VanillaMode(); // use new sector checking logic
			GetClosestSpriteSectors(pSprite->sectnum, pSprite->x, pSprite->y, 400, sectmap, nullptr, newSectCheckMethod);

			BloodStatIterator it(kStatDude);
			while (DBloodActor* actor2 = it.Next())
			{
				spritetype* pSprite2 = &actor2->s();
				int dx = pSprite2->x - pSprite->x;
				int dy = pSprite2->y - pSprite->y;
				int nDist = approxDist(dx, dy);
				if (pSprite2->type == kDudeInnocent)
				{
					DUDEINFO* pDudeInfo = getDudeInfo(pSprite2->type);
					if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
						continue;
					aiSetTarget(actor, actor2);
					aiActivateDude(actor);
					return;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiProcessDudes(void)
{
	BloodStatIterator it(kStatDude);
	while (auto actor = it.Next())
	{
		auto pSprite = &actor->s();
		if (pSprite->flags & 32) continue;
		auto pXSprite = &actor->x();
		DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
		if (IsPlayerSprite(pSprite) || pXSprite->health == 0) continue;

		pXSprite->stateTimer = ClipLow(pXSprite->stateTimer - 4, 0);

		if (pXSprite->aiState)
		{
			if (pXSprite->aiState->moveFunc)
				pXSprite->aiState->moveFunc(actor);

			if (pXSprite->aiState->thinkFunc && (gFrameCount & 3) == (pSprite->index & 3)) // ouch, ouch! :(
				pXSprite->aiState->thinkFunc(actor);
		}

		switch (pSprite->type) {
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustom:
		case kDudeModernCustomBurning: {
			GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
				if (pExtra->slaveCount > 0) updateTargetOfSlaves(actor);
				if (pExtra->pLifeLeech != nullptr) updateTargetOfLeech(actor);
			if (pXSprite->stateTimer == 0 && pXSprite->aiState && pXSprite->aiState->nextState
				&& (pXSprite->aiState->stateTicks > 0 || seqGetStatus(actor) < 0))
			{
					aiGenDudeNewState(actor, pXSprite->aiState->nextState);
			}
			int hinder = ((pExtra->isMelee) ? 25 : 5) << 4;
			if (pXSprite->health <= 0 || hinder > actor->cumulDamage) break;
			pXSprite->data3 = actor->cumulDamage;
			RecoilDude(actor);
			break;
		}
#endif
		default:
			if (pXSprite->stateTimer == 0 && pXSprite->aiState && pXSprite->aiState->nextState) {
				if (pXSprite->aiState->stateTicks > 0)
					aiNewState(actor, pXSprite->aiState->nextState);
				else if (seqGetStatus(actor) < 0)
					aiNewState(actor, pXSprite->aiState->nextState);
			}

			if (pXSprite->health > 0 && ((pDudeInfo->hinderDamage << 4) <= actor->cumulDamage))
			{
				pXSprite->data3 = actor->cumulDamage;
				RecoilDude(actor);
			}
			break;
		}
	}

	it.Reset(kStatDude);
	while (auto actor = it.Next())
	{
		actor->cumulDamage = 0;
	}
}

void aiInit(void)
{
	BloodStatIterator it(kStatDude);
	while (auto actor = it.Next())
	{
		aiInitSprite(actor);
	}
}

void aiInitSprite(DBloodActor* actor)
{
	auto pSprite = &actor->s();
	auto pXSprite = &actor->x();
	int nSector = pSprite->sectnum;
	int nXSector = sector[nSector].extra;
	XSECTOR* pXSector = NULL;
	if (nXSector > 0)
		pXSector = &xsector[nXSector];
	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	pDudeExtra->teslaHit = 0;
	pDudeExtra->time = 0;
	pDudeExtraE->thinkTime = 0;
	pDudeExtraE->active = 0;

#ifdef NOONE_EXTENSIONS
	int stateTimer = -1;
	int targetX = 0, targetY = 0, targetZ = 0;
	DBloodActor* pTargetMarker = nullptr;

	// dude patrol init
	if (gModernMap)
	{
		// must keep it in case of loading save
		if (pXSprite->dudeFlag4 && actor->GetTarget() && actor->GetTarget()->s().type == kMarkerPath)
		{
			stateTimer = pXSprite->stateTimer;
			pTargetMarker = actor->GetTarget();
			targetX = pXSprite->targetX;
			targetY = pXSprite->targetY;
			targetZ = pXSprite->targetZ;
		}
	}
#endif

	switch (pSprite->type)
	{
	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
	case kDudeCultistBeast:
	{
		pDudeExtraE->active = 0;
		aiNewState(actor, &cultistIdle);
		break;
	}
	case kDudeCultistTommyProne:
	{
		pDudeExtraE->active = 0;
		aiNewState(actor, &fanaticProneIdle);
		break;
	}
	case kDudeCultistShotgunProne:
	{
		pDudeExtraE->active = 0;
		aiNewState(actor, &cultistProneIdle);
		break;
	}
	case kDudeZombieButcher: {
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &zombieFIdle);
		break;
	}
	case kDudeZombieAxeNormal: {
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &zombieAIdle);
		break;
	}
	case kDudeZombieAxeLaying:
	{
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &zombieSIdle);
		pSprite->flags &= ~1;
		break;
	}
	case kDudeZombieAxeBuried: {
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &zombieEIdle);
		break;
	}
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone: {
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 0;
		aiNewState(actor, &gargoyleFIdle);
		break;
	}
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:
		aiNewState(actor, &gargoyleStatueIdle);
		break;
	case kDudeCerberusTwoHead: {
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &cerberusIdle);
		break;
	}
	case kDudeCerberusOneHead: {
		if (!VanillaMode()) {
			pDudeExtraE->thinkTime = 0;
			aiNewState(actor, &cerberus2Idle);
			break;
		}
		aiNewState(actor, &genIdle);
		break;
	}
	case kDudeHellHound:
		aiNewState(actor, &houndIdle);
		break;
	case kDudeHand:
		aiNewState(actor, &handIdle);
		break;
	case kDudePhantasm:
	{
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 0;
		aiNewState(actor, &ghostIdle);
		break;
	}
	case kDudeInnocent:
		aiNewState(actor, &innocentIdle);
		break;
	case kDudeRat:
		aiNewState(actor, &ratIdle);
		break;
	case kDudeBoneEel:
	{
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 0;
		aiNewState(actor, &eelIdle);
		break;
	}
	case kDudeGillBeast:
		aiNewState(actor, &gillBeastIdle);
		break;
	case kDudeBat:
	{
		pDudeExtraE->thinkTime = 0;
		pDudeExtraE->active = 0;
		aiNewState(actor, &batIdle);
		break;
	}
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
	{
		pDudeExtraE->active = 0;
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &spidIdle);
		break;
	}
	case kDudeSpiderMother:
	{
		pDudeExtraE->active = 0;
		pDudeExtraE->birthCounter = 0;
		aiNewState(actor, &spidIdle);
		break;
	}
	case kDudeTchernobog:
	{
		pDudeExtraE->active = 0;
		pDudeExtraE->thinkTime = 0;
		aiNewState(actor, &tchernobogIdle);
		break;
	}
	case kDudeTinyCaleb:
		aiNewState(actor, &tinycalebIdle);
		break;
	case kDudeBeast:
		aiNewState(actor, &beastIdle);
		break;
	case kDudePodGreen:
	case kDudePodFire:
		aiNewState(actor, &podIdle);
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		aiNewState(actor, &tentacleIdle);
		break;
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
	case kDudeModernCustomBurning:
		if (!gModernMap) break;
		aiGenDudeInitSprite(actor);
		genDudePrepare(actor, kGenDudePropertyAll);
		break;
#endif
	default:
		aiNewState(actor, &genIdle);
		break;
	}
	aiSetTarget(actor, 0, 0, 0);
	pXSprite->stateTimer = 0;
	switch (pSprite->type)
	{
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
		if (pSprite->cstat & 8) pSprite->flags |= 9;
		else pSprite->flags = 15;
		break;
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone:
	case kDudePhantasm:
	case kDudeBoneEel:
	case kDudeBat:
		pSprite->flags |= 9;
		break;
	case kDudeGillBeast:
		if (pXSector && pXSector->Underwater) pSprite->flags |= 9;
		else pSprite->flags = 15;
		break;
	case kDudeZombieAxeBuried:
	case kDudeZombieAxeLaying:
		pSprite->flags = 7;
		break;
#ifdef NOONE_EXTENSIONS
	case kDudePodMother: // FakeDude type
		if (gModernMap) break;
		[[fallthrough]];
		// Allow put pods and tentacles on ceilings if sprite is y-flipped.
	case kDudePodGreen:
	case kDudeTentacleGreen:
	case kDudePodFire:
	case kDudeTentacleFire:
	case kDudeTentacleMother:
		if (gModernMap && (pSprite->cstat & CSTAT_SPRITE_YFLIP)) {
			if (!(pSprite->flags & kModernTypeFlag1)) // don't add autoaim for player if hitag 1 specified in editor.
				pSprite->flags = kHitagAutoAim;
			break;
		}
		[[fallthrough]];
		// go default
#endif
	default:
		pSprite->flags = 15;
		break;
	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		if (pXSprite->dudeFlag4)
		{
			// restore dude's path
			if (pTargetMarker)
			{
				actor->SetTarget(pTargetMarker);
				pXSprite->targetX = targetX;
				pXSprite->targetY = targetY;
				pXSprite->targetZ = targetZ;
			}

			// reset target spot progress
			pXSprite->data3 = 0;

			// make dude follow the markers
			bool uwater = spriteIsUnderwater(actor);
            if (actor->GetTarget() == nullptr || actor->GetTarget()->s().type != kMarkerPath) 
            {
                actor->SetTarget(nullptr);
                aiPatrolSetMarker(actor);
			}

            if (stateTimer > 0) 
            {
                if (uwater) aiPatrolState(actor, kAiStatePatrolWaitW);
                else if (pXSprite->unused1 & kDudeFlagCrouch) aiPatrolState(actor, kAiStatePatrolWaitC);
                else aiPatrolState(actor, kAiStatePatrolWaitL);
				pXSprite->stateTimer = stateTimer; // restore state timer
			}
            else if (uwater) aiPatrolState(actor, kAiStatePatrolMoveW);
            else if (pXSprite->unused1 & kDudeFlagCrouch) aiPatrolState(actor, kAiStatePatrolMoveC);
            else aiPatrolState(actor, kAiStatePatrolMoveL);
		}
	}
#endif
}

END_BLD_NS
