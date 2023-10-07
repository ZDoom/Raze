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

AISTATE genIdle = { kAiStateGenIdle, 0, nullptr, 0, NULL, NULL, NULL, NULL };
AISTATE genRecoil = { kAiStateRecoil, 5, nullptr, 20, NULL, NULL, NULL, &genIdle };

const int gCultTeslaFireChance[5] = { 0x2000, 0x4000, 0x8000, 0xa000, 0xe000 };

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool dudeIsPlayingSeq(DBloodActor* actor, int nSeq)
{
	if (actor->spr.statnum == kStatDude && actor->IsDudeActor())
	{
		DUDEINFO* pDudeInfo = getDudeInfo(actor);
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

void aiPlay3DSound(DBloodActor* actor, int soundid, AI_SFX_PRIORITY a3, int playchannel)
{
	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	if (a3 == AI_SFX_PRIORITY_0)
		sfxPlay3DSound(actor, soundid, playchannel, 2);
	else if (a3 > pDudeExtra->prio || pDudeExtra->time <= PlayClock)
	{
		sfxKill3DSound(actor, -1, -1);
		sfxPlay3DSound(actor, soundid, playchannel, 0);
		pDudeExtra->prio = a3;
		pDudeExtra->time = PlayClock + 120;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiNewState(DBloodActor* actor, FDefiningState* pAIState)
{
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	actor->xspr.stateTimer = pAIState->Tics;
	actor->xspr.aiState = pAIState;
	int seqStartId = pDudeInfo->seqStartID;

	if (pAIState->seqId >= 0)
	{
		seqStartId += pAIState->seqId;
		if (getSequence(seqStartId))
			seqSpawn(seqStartId, actor, pAIState->ActionFunc);
	}

	if (pAIState->EnterFunc)
		callActorFunction(pAIState->EnterFunc, actor);
}

void aiNewState(DBloodActor* actor, FName nAIState)
{
	auto cls = actor->GetClass();
	while (cls)
	{
		for (auto& state : static_cast<PClassActor*>(cls)->ActorInfo()->AIStates)
		{
			if (state.Label == nAIState)
			{
				aiNewState(actor, &state);
				return;
			}
		}
		cls = cls->ParentClass;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool isImmune(DBloodActor* actor, int dmgType, int minScale)
{

	if (dmgType >= kDmgFall && dmgType < kDmgMax && actor->hasX() && actor->xspr.locked != 1)
	{
		int type = actor->GetType();
		if (type >= kThingBase && type < kThingMax)
			return (actor->dmgControl[dmgType] <= minScale);
		else if (actor->IsDudeActor())
		{
			if (actor->IsPlayerActor()) return (getPlayer(type - kDudePlayer1)->godMode || getPlayer(type - kDudePlayer1)->damageControl[dmgType] <= minScale);
			else return (actor->dmgControl[dmgType] <= minScale);
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool CanMove(DBloodActor* actor, DBloodActor* target, DAngle nAngle, double nRange)
{
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	DVector3 pos = actor->spr.pos;
	DVector2 nAngVect = nAngle.ToVector();
	HitScan(actor, pos.Z, DVector3(nAngVect, 0) * 1024, CLIPMASK0, nRange);
	double nDist = (actor->spr.pos.XY() - gHitInfo.hitpos.XY()).Length();
	if (nDist - (actor->clipdist) < nRange / 16.) // this was actually comparing a Build unit value with a texel unit value!
	{
		if (gHitInfo.actor() == nullptr || target == nullptr || target != gHitInfo.actor())
			return false;
		return true;
	}
	pos.XY() += nRange / 16 * nAngVect; // see above - same weird mixup.
	auto pSector = actor->sector();
	assert(pSector);
	auto ps2 = pSector;
	updatesectorz(pos, &pSector);
	if (!pSector) return false;
	double floorZ = getflorzofslopeptr(pSector, pos);
	auto pXSector = pSector->hasX()? &pSector->xs() : nullptr;
	bool Underwater = 0; 
	bool Water = 0; 
	bool Depth = 0; 
	bool Crusher = 0;
	if (pXSector)
	{
		if (pXSector->Underwater)
			Underwater = 1;
		if (pXSector->Depth)
			Depth = 1;
		if (pSector->type == kSectorDamage || pXSector->damageType > 0)
			Crusher = 1;
	}
	auto Upper = barrier_cast<DBloodActor*>(pSector->upperLink);
	auto Lower = barrier_cast<DBloodActor*>(pSector->lowerLink);
	if (Upper != nullptr)
	{
		if (Upper->GetType() == kMarkerUpWater || Upper->GetType() == kMarkerUpGoo)
			Water = Depth = 1;
	}
	if (Lower != nullptr)
	{
		if (Lower->GetType() == kMarkerLowWater || Lower->GetType() == kMarkerLowGoo)
			Depth = 1;
	}
	switch (actor->GetType()) {
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone:
	case kDudeBat:
		if (actor->clipdist > nDist * 4)
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
		if (floorZ - bottom > 32)
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
		if ((pXSector == nullptr || (!pXSector->Underwater && !pXSector->Depth)) && floorZ - bottom > 32)
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

void aiChooseDirection(DBloodActor* actor, DAngle direction)
{
	assert(actor->IsDudeActor());
	DAngle vc = deltaangle(actor->spr.Angles.Yaw, direction);
	auto almost60deg = DAngle::fromBuild(341); // 60° does not work correctly - this is a little bit less, actually.
	DAngle v8 = vc.Sgn() == -1 ? -almost60deg : almost60deg;

	double range = actor->vel.XY().dot(actor->spr.Angles.Yaw.ToVector()) * 120;

	if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + vc, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + vc;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + vc / 2, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + vc / 2;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw - vc / 2, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw - vc / 2;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + v8, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw + v8;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw;
	else if (CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw - v8, range))
		actor->xspr.goalAng = actor->spr.Angles.Yaw - v8;
	//else if (actor->spr.flags&2)
		//actor->xspr.goalAng = actor->spr.angle+341;
	else // Weird..
		actor->xspr.goalAng = actor->spr.Angles.Yaw + almost60deg;
	if (Chance(0x8000))
		actor->xspr.dodgeDir = 1;
	else
		actor->xspr.dodgeDir = -1;

	actor->xspr.goalAng = actor->xspr.goalAng.Normalized360();
	if (!CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + DAngle90 * actor->xspr.dodgeDir, 512))
	{
		actor->xspr.dodgeDir = -actor->xspr.dodgeDir;
		if (!CanMove(actor, actor->GetTarget(), actor->spr.Angles.Yaw + DAngle90 * actor->xspr.dodgeDir, 512))
			actor->xspr.dodgeDir = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveForward(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	if (abs(nAng) > DAngle60)
		return;
	actor->vel.XY() += actor->spr.Angles.Yaw.ToVector() * pDudeInfo->FrontSpeed();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveTurn(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiMoveDodge(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto nAng = deltaangle(actor->spr.Angles.Yaw, actor->xspr.goalAng);
	auto nTurnRange = pDudeInfo->TurnRange();
	actor->spr.Angles.Yaw += clamp(nAng, -nTurnRange, nTurnRange);
	if (actor->xspr.dodgeDir)
	{
		AdjustVelocity(actor, ADJUSTER{
			if (actor->xspr.dodgeDir > 0)
				t2 += FixedToFloat(pDudeInfo->sideSpeed);
			else
				t2 -= FixedToFloat(pDudeInfo->sideSpeed);
		});
	}
}

//---------------------------------------------------------------------------
//
// todo: split this up.
//
//---------------------------------------------------------------------------

void aiActivateDude(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	if (!actor->xspr.state)
	{
		aiChooseDirection(actor, (actor->xspr.TargetPos - actor->spr.pos).Angle());
		actor->xspr.state = 1;
	}
	switch (actor->GetType())
	{
	case kDudePhantasm:
	{
		
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_ghostSearch);
		else
		{
			aiPlay3DSound(actor, 1600, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_ghostChase);
		}
		break;
	}
	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
	case kDudeCultistBeast:
	{
		
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_cultistSearch);
				if (Chance(0x8000))
				{
					if (actor->GetType() == kDudeCultistTommy) aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);
					else aiPlay3DSound(actor, 1008 + Random(5), AI_SFX_PRIORITY_1, -1);
				}
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
			{
				if (actor->GetType() == kDudeCultistTommy) aiPlay3DSound(actor, 4003 + Random(4), AI_SFX_PRIORITY_1, -1);
				else aiPlay3DSound(actor, 1003 + Random(4), AI_SFX_PRIORITY_1, -1);
			}
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				if (actor->GetType() == kDudeCultistTommy) aiNewState(actor, NAME_fanaticChase);
				else aiNewState(actor, NAME_cultistChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimChase);
				break;
			}
		}
		break;
	}
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustom:
	{
		
		actor->dudeExtra.active = 1;
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
		
		actor->dudeExtra.active = 1;
		actor->ChangeType(kDudeCultistTommy);
		if (actor->GetTarget() == nullptr)
		{
			switch (actor->xspr.medium)
			{
			case 0:
				aiNewState(actor, NAME_cultistSearch);
				if (Chance(0x8000)) aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 4008 + Random(5), AI_SFX_PRIORITY_1, -1);

			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_cultistProneChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeCultistShotgunProne:
	{
		
		actor->dudeExtra.active = 1;
		actor->ChangeType(kDudeCultistShotgun);
		if (actor->GetTarget() == nullptr)
		{
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_cultistSearch);
				if (Chance(0x8000))
					aiPlay3DSound(actor, 1008 + Random(5), AI_SFX_PRIORITY_1, -1);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimSearch);
				break;
			}
		}
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 1003 + Random(4), AI_SFX_PRIORITY_1, -1);
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_cultistProneChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_cultistSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeBurningCultist:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_cultistBurnSearch);
		else
			aiNewState(actor, NAME_cultistBurnChase);
		break;
	case kDudeBat:
	{
		
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (!actor->spr.flags)
			actor->spr.flags = 9;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_batSearch);
		else
		{
			if (Chance(0xa000))
				aiPlay3DSound(actor, 2000, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_batChase);
		}
		break;
	}
	case kDudeBoneEel:
	{
		
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_eelSearch);
		else
		{
			if (Chance(0x8000))
				aiPlay3DSound(actor, 1501, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1500, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_eelChase);
		}
		break;
	}
	case kDudeGillBeast:
	{
		
		XSECTOR* pXSector = actor->sector()->hasX()? &actor->sector()->xs() : nullptr;

		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
		{
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, NAME_gillBeastSwimSearch);
			else
				aiNewState(actor, NAME_gillBeastSearch);
		}
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1701, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1700, AI_SFX_PRIORITY_1, -1);
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, NAME_gillBeastSwimChase);
			else
				aiNewState(actor, NAME_gillBeastChase);
		}
		break;
	}
	case kDudeZombieAxeNormal:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_zombieASearch);
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
			aiNewState(actor, NAME_zombieAChase);
		}
		break;
	}
	case kDudeZombieAxeBuried:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->xspr.aiState->Label == NAME_zombieEIdle) aiNewState(actor, NAME_zombieEUp);
		break;
	}
	case kDudeZombieAxeLaying:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->xspr.aiState->Label == NAME_zombieSIdle) aiNewState(actor, NAME_zombieEStand);
		break;
	}
	case kDudeZombieButcher:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_zombieFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1201, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1200, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_zombieFChase);
		}
		break;
	}
	case kDudeBurningZombieAxe:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_zombieABurnSearch);
		else
			aiNewState(actor, NAME_zombieABurnChase);
		break;
	case kDudeBurningZombieButcher:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_zombieFBurnSearch);
		else
			aiNewState(actor, NAME_zombieFBurnChase);
		break;
	case kDudeGargoyleFlesh: {
		
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_gargoyleFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_gargoyleFChase);
		}
		break;
	}
	case kDudeGargoyleStone:
	{
		
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 1;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_gargoyleFSearch);
		else
		{
			if (Chance(0x4000))
				aiPlay3DSound(actor, 1451, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 1450, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_gargoyleFChase);
		}
		break;
	}
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:

#ifdef NOONE_EXTENSIONS
		// play gargoyle statue breaking animation if data1 = 1.
		if (gModernMap && actor->xspr.data1 == 1)
		{
			if (actor->GetType() == kDudeGargoyleStatueFlesh) aiNewState(actor, NAME_statueFBreakSEQ);
			else aiNewState(actor, NAME_statueSBreakSEQ);
		}
		else
		{
			if (Chance(0x4000)) aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
			else aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);

			if (actor->GetType() == kDudeGargoyleStatueFlesh) aiNewState(actor, NAME_gargoyleFMorph);
			else aiNewState(actor, NAME_gargoyleSMorph);
		}
#else
		if (Chance(0x4000)) aiPlay3DSound(actor, 1401, AI_SFX_PRIORITY_1, -1);
		else aiPlay3DSound(actor, 1400, AI_SFX_PRIORITY_1, -1);

		if (actor->GetType() == kDudeGargoyleStatueFlesh) aiNewState(actor, NAME_gargoyleFMorph);
		else aiNewState(actor, NAME_gargoyleSMorph);
#endif
		break;
	case kDudeCerberusTwoHead:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_cerberusSearch);
		else
		{
			aiPlay3DSound(actor, 2300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_cerberusChase);
		}
		break;
	case kDudeCerberusOneHead:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_cerberus2Search);
		else
		{
			aiPlay3DSound(actor, 2300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_cerberus2Chase);
		}
		break;
	case kDudeHellHound:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_houndSearch);
		else
		{
			aiPlay3DSound(actor, 1300, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_houndChase);
		}
		break;
	case kDudeHand:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_handSearch);
		else
		{
			aiPlay3DSound(actor, 1900, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_handChase);
		}
		break;
	case kDudeRat:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_ratSearch);
		else
		{
			aiPlay3DSound(actor, 2100, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_ratChase);
		}
		break;
	case kDudeInnocent:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_innocentSearch);
		else
		{
			if (actor->xspr.health > 0)
				aiPlay3DSound(actor, 7000 + Random(6), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_innocentChase);
		}
		break;
	case kDudeTchernobog:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_tchernobogSearch);
		else
		{
			aiPlay3DSound(actor, 2350 + Random(7), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_tchernobogChase);
		}
		break;
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
		actor->spr.flags |= 2;
		actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_spidSearch);
		else
		{
			aiPlay3DSound(actor, 1800, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_spidChase);
		}
		break;
	case kDudeSpiderMother:
	{
		
		actor->dudeExtra.active = 1;
		actor->spr.flags |= 2;
		actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_spidSearch);
		else
		{
			aiPlay3DSound(actor, 1853 + Random(1), AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_spidChase);
		}
		break;
	}
	case kDudeTinyCaleb:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_tinycalebSearch);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_tinycalebSwimSearch);
				break;
			}
		}
		else
		{
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_tinycalebChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_tinycalebSwimChase);
				break;
			}
		}
		break;
	}
	case kDudeBeast:
	{
		
		actor->dudeExtra.thinkTime = 1;
		if (actor->GetTarget() == nullptr)
		{
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_beastSearch);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_beastSwimSearch);
				break;
			}
		}
		else
		{
			aiPlay3DSound(actor, 9009 + Random(2), AI_SFX_PRIORITY_1, -1);
			switch (actor->xspr.medium)
			{
			case kMediumNormal:
				aiNewState(actor, NAME_beastChase);
				break;
			case kMediumWater:
			case kMediumGoo:
				aiNewState(actor, NAME_beastSwimChase);
				break;
			}
		}
		break;
	}
	case kDudePodGreen:
	case kDudePodFire:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_podSearch);
		else
		{
			if (actor->GetType() == kDudePodFire)
				aiPlay3DSound(actor, 2453, AI_SFX_PRIORITY_1, -1);
			else
				aiPlay3DSound(actor, 2473, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_podChase);
		}
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		if (actor->GetTarget() == nullptr)
			aiNewState(actor, NAME_tentacleSearch);
		else
		{
			aiPlay3DSound(actor, 2503, AI_SFX_PRIORITY_1, -1);
			aiNewState(actor, NAME_tentacleChase);
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void aiSetTarget(DBloodActor* actor, const DVector3& pos)
{
	actor->SetTarget(nullptr);
	actor->xspr.TargetPos = pos;
}

void aiSetTarget(DBloodActor* actor, DBloodActor* target)
{
	if (target == nullptr)
	{
		actor->SetTarget(nullptr);
		return;
	}
	if (target->GetType() >= kDudeBase && target->GetType() < kDudeMax)
	{
		if (actor->GetOwner() != target)
		{
			actor->SetTarget(target);
			DUDEINFO* pDudeInfo = getDudeInfo(target);
			double eyeHeight = (pDudeInfo->eyeHeight * target->spr.scale.Y);
			actor->xspr.TargetPos = target->spr.pos.plusZ(-eyeHeight);
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
	if (!actor->xspr.health)
		return 0;
	actor->xspr.health = ClipLow(actor->xspr.health - nDamage, 0);
	actor->cumulDamage += nDamage;
	DUDEINFO* pDudeInfo = getDudeInfo(actor);

	if (source)
	{
		if (actor == source) return 0;
		else if (actor->GetTarget() == nullptr) // if no target, give the dude a target
		{
			aiSetTarget(actor, source);
			aiActivateDude(actor);
		}
		else if (source != actor->GetTarget()) // if found a new target, retarget
		{
			int nThresh = nDamage;
			if (actor->GetType() == source->GetType())
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
			if (aiInPatrolState(actor->xspr.aiState))
			{
				aiPatrolStop(actor, source, actor->xspr.dudeAmbush);

				auto pPlayer = getPlayer(source->GetType());
				if (!pPlayer) return nDamage;
				//if (powerupCheck(pPlayer, kPwUpShadowCloak)) pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
				if (readyForCrit(source, actor)) 
				{
					nDamage += aiDamageSprite(actor, source, nDmgType, nDamage * (10 - gGameOptions.nDifficulty));
					if (actor->xspr.health > 0)
					{
						int fullHp = (actor->xspr.sysData2 > 0) ? ClipRange(actor->xspr.sysData2 << 4, 1, 65535) : getDudeInfo(actor)->startHealth << 4;
						if (((100 * actor->xspr.health) / fullHp) <= 75)
						{
							actor->cumulDamage += nDamage << 4; // to be sure any enemy will play the recoil animation
							RecoilDude(actor);
						}
					}

					DPrintf(DMSG_SPAMMY, "Player #%d does the critical damage to patrol dude #%d!", pPlayer->pnum + 1, actor->GetIndex());
				}

				return nDamage;
			}

			if (actor->GetType() == kDudeModernCustomBurning)
			{
				if (Chance(0x2000) && actor->dudeExtra.time < PlayClock) {
					playGenDudeSound(actor,kGenDudeSndBurning);
					actor->dudeExtra.time = PlayClock + 360;
				}

				if (actor->xspr.burnTime == 0) actor->xspr.burnTime = 2400;
				if (spriteIsUnderwater(actor, false)) 
				{
					actor->ChangeType(kDudeModernCustom);
					actor->xspr.burnTime = 0;
					actor->xspr.health = 1; // so it can be killed with flame weapons while underwater and if already was burning dude before.
					aiGenDudeNewState(actor, &genDudeGotoW);
				}

				return nDamage;

			}

			if (actor->GetType() == kDudeModernCustom)
			{
				GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
				if (nDmgType == kDamageBurn)
				{
					if (actor->xspr.health > (uint32_t)pDudeInfo->fleeHealth) return nDamage;
					else if (actor->xspr.txID <= 0 || getNextIncarnation(actor) == nullptr) 
					{
						removeDudeStuff(actor);

						if (pExtra->weaponType == kGenDudeWeaponKamikaze)
							doExplosion(actor, actor->xspr.data1 - kTrapExploder);

						if (spriteIsUnderwater(actor)) 
						{
							actor->xspr.health = 0;
							return nDamage;
						}

						if (actor->xspr.burnTime <= 0)
							actor->xspr.burnTime = 1200;

						if (pExtra->canBurn && pExtra->availDeaths[kDamageBurn] > 0) {

							aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
							playGenDudeSound(actor,kGenDudeSndBurning);
							actor->ChangeType(kDudeModernCustomBurning);

							if (actor->xspr.data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
								actor->spr.pal = 0;

							aiGenDudeNewState(actor, &genDudeBurnGoto);
							actHealDude(actor, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
							actor->dudeExtra.time = PlayClock + 360;
							evKillActor(actor, AF(fxFlameLick));

						}
					}
					else
					{
						actKillDude(actor, actor, kDamageFall, 65535);
					}
				} 
				else if (canWalk(actor) && !inDodge(actor->xspr.aiState) && !inRecoil(actor->xspr.aiState))
				{
					if (!dudeIsMelee(actor)) 
					{
						if (inIdle(actor->xspr.aiState) || Chance(getDodgeChance(actor))) 
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
		const bool fixRandomCultist = !cl_bloodvanillaenemies && (actor->spr.inittype >= kDudeBase) && (actor->spr.inittype < kDudeMax)  && (actor->spr.inittype != actor->GetType()) && !VanillaMode(); // fix burning cultists randomly switching types underwater
		switch (actor->GetType())
		{
		case kDudeCultistTommy:
		case kDudeCultistShotgun:
		case kDudeCultistTesla:
		case kDudeCultistTNT:
			if (nDmgType != kDamageBurn)
			{
				if (!dudeIsPlayingSeq(actor, 14) && !actor->xspr.medium)
					aiNewState(actor, NAME_cultistDodge);
				else if (dudeIsPlayingSeq(actor, 14) && !actor->xspr.medium)
					aiNewState(actor, NAME_cultistProneDodge);
				else if (dudeIsPlayingSeq(actor, 13) && (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo))
					aiNewState(actor, NAME_cultistSwimDodge);
			}
			else if (nDmgType == kDamageBurn && actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth/* && (actor->xspr.at17_6 != 1 || actor->xspr.at17_6 != 2)*/)
			{
				actor->ChangeType(kDudeBurningCultist);
				aiNewState(actor, NAME_cultistBurnGoto);
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1031 + Random(2), AI_SFX_PRIORITY_2, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
				evKillActor(actor, AF(fxFlameLick));
			}
			break;
		case kDudeInnocent:
			if (nDmgType == kDamageBurn && actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth/* && (actor->xspr.at17_6 != 1 || actor->xspr.at17_6 != 2)*/)
			{
				actor->ChangeType(kDudeBurningInnocent);
				aiNewState(actor, NAME_cultistBurnGoto);
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
				evKillActor(actor, AF(fxFlameLick));
			}
			break;
		case kDudeBurningCultist:
			if (Chance(0x4000) && actor->dudeExtra.time < PlayClock)
			{
				aiPlay3DSound(actor, 1031 + Random(2), AI_SFX_PRIORITY_2, -1);
				actor->dudeExtra.time = PlayClock + 360;
			}
			if (Chance(0x600) && (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo))
			{
				actor->ChangeType(kDudeCultistTommy);
				if (fixRandomCultist) // fix burning cultists randomly switching types underwater
					actor->ChangeType(actor->spr.inittype); // restore back to spawned cultist type
				actor->xspr.burnTime = 0;
				aiNewState(actor, NAME_cultistSwimGoto);
			}
			else if (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo)
			{
				actor->ChangeType(kDudeCultistShotgun);
				if (fixRandomCultist) // fix burning cultists randomly switching types underwater
					actor->ChangeType(actor->spr.inittype); // restore back to spawned cultist type
				actor->xspr.burnTime = 0;
				aiNewState(actor, NAME_cultistSwimGoto);
			}
			break;
		case kDudeGargoyleFlesh:
			aiNewState(actor, NAME_gargoyleFChase);
			break;
		case kDudeZombieButcher:
			if (nDmgType == kDamageBurn && actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth) {
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
				actor->ChangeType(kDudeBurningZombieButcher);
				aiNewState(actor, NAME_zombieFBurnGoto);
				actHealDude(actor, dudeInfo[42].startHealth, dudeInfo[42].startHealth);
				evKillActor(actor, AF(fxFlameLick));
			}
			break;
		case kDudeTinyCaleb:
			if (nDmgType == kDamageBurn && actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth/* && (actor->xspr.at17_6 != 1 || actor->xspr.at17_6 != 2)*/)
			{
				if (!cl_bloodvanillaenemies && !VanillaMode()) // fix burning sprite for tiny caleb
				{
					actor->ChangeType(kDudeBurningTinyCaleb);
					aiNewState(actor, NAME_tinycalebBurnGoto);
				}
				else
				{
					actor->ChangeType(kDudeBurningInnocent);
					aiNewState(actor, NAME_cultistBurnGoto);
				}
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				actor->dudeExtra.time = PlayClock + 360;
				actHealDude(actor, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
				evKillActor(actor, AF(fxFlameLick));
			}
			break;
		case kDudeCultistBeast:
			if (actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth)
			{
				actor->ChangeType(kDudeBeast);
				aiPlay3DSound(actor, 9008, AI_SFX_PRIORITY_1, -1);
				aiNewState(actor, NAME_beastMorphFromCultist);
				actHealDude(actor, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
			}
			break;
		case kDudeZombieAxeNormal:
		case kDudeZombieAxeBuried:
			if (nDmgType == kDamageBurn && actor->xspr.health <= (unsigned int)pDudeInfo->fleeHealth)
			{
				aiPlay3DSound(actor, 361, AI_SFX_PRIORITY_0, -1);
				aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
				actor->ChangeType(kDudeBurningZombieAxe);
				aiNewState(actor, NAME_zombieABurnGoto);
				actHealDude(actor, dudeInfo[41].startHealth, dudeInfo[41].startHealth);
				evKillActor(actor, AF(fxFlameLick));
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
	uint8_t v4 = Chance(0x8000);
	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	if (actor->spr.statnum == kStatDude && (actor->IsDudeActor()))
	{
		DUDEINFO* pDudeInfo = getDudeInfo(actor);
		switch (actor->GetType())
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
				if (inDuck(actor->xspr.aiState) && Chance(rChance >> 2)) aiGenDudeNewState(actor, &genDudeRecoilD);
				else if (spriteIsUnderwater(actor, false)) aiGenDudeNewState(actor, &genDudeRecoilW);
				else aiGenDudeNewState(actor, &genDudeRecoilL);
			}

			int rState = inRecoil(actor->xspr.aiState);
			if (rState > 0)
			{
				if (!canWalk(actor))
				{
					if (rState == 1) actor->xspr.aiState->nextState = &genDudeChaseNoWalkL;
					else if (rState == 2) actor->xspr.aiState->nextState = &genDudeChaseNoWalkD;
					else actor->xspr.aiState->nextState = &genDudeChaseNoWalkW;

				}
				else if (!dudeIsMelee(actor) || Chance(rChance >> 2))
				{
					if (rState == 1) actor->xspr.aiState->nextState = (Chance(rChance) ? &genDudeDodgeL : &genDudeDodgeShortL);
					else if (rState == 2) actor->xspr.aiState->nextState = (Chance(rChance) ? &genDudeDodgeD : &genDudeDodgeShortD);
					else if (rState == 3) actor->xspr.aiState->nextState = (Chance(rChance) ? &genDudeDodgeW : &genDudeDodgeShortW);
				}
				else if (rState == 1) actor->xspr.aiState->nextState = &genDudeChaseL;
				else if (rState == 2) actor->xspr.aiState->nextState = &genDudeChaseD;
				else actor->xspr.aiState->nextState = &genDudeChaseW;

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
			if (actor->GetType() == kDudeCultistTommy) aiPlay3DSound(actor, 4013 + Random(2), AI_SFX_PRIORITY_2, -1);
			else aiPlay3DSound(actor, 1013 + Random(2), AI_SFX_PRIORITY_2, -1);

			if (!v4 && actor->xspr.medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit) aiNewState(actor, NAME_cultistTeslaRecoil);
				else aiNewState(actor, NAME_cultistRecoil);

			}
			else if (v4 && actor->xspr.medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit) aiNewState(actor, NAME_cultistTeslaRecoil);
				else if (gGameOptions.nDifficulty > 0) aiNewState(actor, NAME_cultistProneRecoil);
				else aiNewState(actor, NAME_cultistRecoil);
			}
			else if (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo)
				aiNewState(actor, NAME_cultistSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, NAME_cultistTeslaRecoil);
				else
					aiNewState(actor, NAME_cultistRecoil);
			}
			break;
		case kDudeBurningCultist:
			aiNewState(actor, NAME_cultistBurnGoto);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiGenDudeNewState(actor, &genDudeBurnGoto);
			break;
#endif
		case kDudeZombieButcher:
			aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, NAME_zombieFTeslaRecoil);
			else
				aiNewState(actor, NAME_zombieFRecoil);
			break;
		case kDudeZombieAxeNormal:
		case kDudeZombieAxeBuried:
			aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit && actor->xspr.data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, NAME_zombieATeslaRecoil);
			else if (actor->xspr.data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, NAME_zombieARecoil2);
			else
				aiNewState(actor, NAME_zombieARecoil);
			break;
		case kDudeBurningZombieAxe:
			aiPlay3DSound(actor, 1106, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_zombieABurnGoto);
			break;
		case kDudeBurningZombieButcher:
			aiPlay3DSound(actor, 1202, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_zombieFBurnGoto);
			break;
		case kDudeGargoyleFlesh:
		case kDudeGargoyleStone:
			aiPlay3DSound(actor, 1402, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_gargoyleFRecoil);
			break;
		case kDudeCerberusTwoHead:
			aiPlay3DSound(actor, 2302 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit && actor->xspr.data3 > pDudeInfo->startHealth / 3)
				aiNewState(actor, NAME_cerberusTeslaRecoil);
			else
				aiNewState(actor, NAME_cerberusRecoil);
			break;
		case kDudeCerberusOneHead:
			aiPlay3DSound(actor, 2302 + Random(2), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_cerberus2Recoil);
			break;
		case kDudeHellHound:
			aiPlay3DSound(actor, 1302, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, NAME_houndTeslaRecoil);
			else
				aiNewState(actor, NAME_houndRecoil);
			break;
		case kDudeTchernobog:
			aiPlay3DSound(actor, 2370 + Random(2), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_tchernobogRecoil);
			break;
		case kDudeHand:
			aiPlay3DSound(actor, 1902, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_handRecoil);
			break;
		case kDudeRat:
			aiPlay3DSound(actor, 2102, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_ratRecoil);
			break;
		case kDudeBat:
			aiPlay3DSound(actor, 2002, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_batRecoil);
			break;
		case kDudeBoneEel:
			aiPlay3DSound(actor, 1502, AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_eelRecoil);
			break;
		case kDudeGillBeast: {
			XSECTOR* pXSector = actor->sector()->hasX() ? &actor->sector()->xs() : nullptr;

			aiPlay3DSound(actor, 1702, AI_SFX_PRIORITY_2, -1);
			if (pXSector && pXSector->Underwater)
				aiNewState(actor, NAME_gillBeastSwimRecoil);
			else
				aiNewState(actor, NAME_gillBeastRecoil);
			break;
		}
		case kDudePhantasm:
			aiPlay3DSound(actor, 1602, AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, NAME_ghostTeslaRecoil);
			else
				aiNewState(actor, NAME_ghostRecoil);
			break;
		case kDudeSpiderBrown:
		case kDudeSpiderRed:
		case kDudeSpiderBlack:
			aiPlay3DSound(actor, 1802 + Random(1), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_spidDodge);
			break;
		case kDudeSpiderMother:
			aiPlay3DSound(actor, 1851 + Random(1), AI_SFX_PRIORITY_2, -1);
			aiNewState(actor, NAME_spidDodge);
			break;
		case kDudeInnocent:
			aiPlay3DSound(actor, 7007 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (pDudeExtra->teslaHit)
				aiNewState(actor, NAME_innocentTeslaRecoil);
			else
				aiNewState(actor, NAME_innocentRecoil);
			break;
		case kDudeTinyCaleb:
			if (actor->xspr.medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, NAME_tinycalebTeslaRecoil);
				else
					aiNewState(actor, NAME_tinycalebRecoil);
			}
			else if (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo)
				aiNewState(actor, NAME_tinycalebSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, NAME_tinycalebTeslaRecoil);
				else
					aiNewState(actor, NAME_tinycalebRecoil);
			}
			break;
		case kDudeBeast:
			aiPlay3DSound(actor, 9004 + Random(2), AI_SFX_PRIORITY_2, -1);
			if (actor->xspr.medium == kMediumNormal)
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, NAME_beastTeslaRecoil);
				else
					aiNewState(actor, NAME_beastRecoil);
			}
			else if (actor->xspr.medium == kMediumWater || actor->xspr.medium == kMediumGoo)
				aiNewState(actor, NAME_beastSwimRecoil);
			else
			{
				if (pDudeExtra->teslaHit)
					aiNewState(actor, NAME_beastTeslaRecoil);
				else
					aiNewState(actor, NAME_beastRecoil);
			}
			break;
		case kDudePodGreen:
		case kDudePodFire:
			aiNewState(actor, NAME_podRecoil);
			break;
		case kDudeTentacleGreen:
		case kDudeTentacleFire:
			aiNewState(actor, NAME_tentacleRecoil);
			break;
		default:
			aiNewState(actor, NAME_genRecoil);
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			if (actor->GetOwner() == pPlayer->GetActor() || pPlayer->GetActor()->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->GetActor()->spr.pos;
			auto dvec = ppos.XY() - actor->spr.pos.XY();
			auto pSector = pPlayer->GetActor()->sector();

			double nDist = dvec.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;

			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvec.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
				return;
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				aiSetTarget(actor, ppos);
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
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			DBloodPlayer* pPlayer = getPlayer(p);
			if (actor->GetOwner() == pPlayer->GetActor() || pPlayer->GetActor()->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->GetActor()->spr.pos;
			auto dvec = ppos.XY() - actor->spr.pos.XY();
			auto pSector = pPlayer->GetActor()->sector();

			double nDist = dvec.Length();
			if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, dvec.Angle());
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, pPlayer->GetActor());
				aiActivateDude(actor);
				return;
			}
			else if (nDist < pDudeInfo->HearDist())
			{
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
				return;
			}
		}
		if (actor->xspr.state)
		{
			const bool newSectCheckMethod = !cl_bloodvanillaenemies && !VanillaMode(); // use new sector checking logic
			GetClosestSpriteSectors(actor->sector(), actor->spr.pos.XY(), 400, nullptr, newSectCheckMethod);

			BloodStatIterator it(kStatDude);
			while (DBloodActor* actor2 = it.Next())
			{
				double nDist = (actor2->spr.pos.XY() - actor->spr.pos.XY()).Length();
				if (actor2->GetType() == kDudeInnocent)
				{
					pDudeInfo = getDudeInfo(actor2);
					if (nDist > pDudeInfo->SeeDist() && nDist > pDudeInfo->HearDist())
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
		if (actor->spr.flags & 32) continue;
		DUDEINFO* pDudeInfo = getDudeInfo(actor);
		if (actor->IsPlayerActor() || actor->xspr.health == 0) continue;

		actor->xspr.stateTimer = ClipLow(actor->xspr.stateTimer - 4, 0);

		if (actor->xspr.aiState)
		{
			if (actor->xspr.aiState->MoveFunc)
				callActorFunction(actor->xspr.aiState->MoveFunc, actor);

			if (actor->xspr.aiState->TickFunc && (gFrameCount & 3) == (actor->GetIndex() & 3))
				callActorFunction(actor->xspr.aiState->TickFunc, actor);
		}

#ifdef NOONE_EXTENSIONS
		switch (actor->GetType()) {
		case kDudeModernCustom:
		case kDudeModernCustomBurning: {
			GENDUDEEXTRA* pExtra = &actor->genDudeExtra;
				if (pExtra->slaveCount > 0) updateTargetOfSlaves(actor);
				if (pExtra->pLifeLeech != nullptr) updateTargetOfLeech(actor);
			if (actor->xspr.stateTimer == 0 && actor->xspr.aiState && actor->xspr.aiState->nextState
				&& (actor->xspr.aiState->stateTicks > 0 || seqGetStatus(actor) < 0))
			{
					aiGenDudeNewState(actor, actor->xspr.aiState->nextState);
			}
			int hinder = ((pExtra->isMelee) ? 25 : 5) << 4;
			if (actor->xspr.health <= 0 || hinder > actor->cumulDamage) break;
			actor->xspr.data3 = actor->cumulDamage;
			RecoilDude(actor);
			break;
		}
		default:
#endif
			if (actor->xspr.stateTimer == 0 && actor->xspr.aiState && actor->xspr.aiState->NextState) {
				if (actor->xspr.aiState->Tics > 0)
					aiNewState(actor, actor->xspr.aiState->NextState);
				else if (seqGetStatus(actor) < 0)
					aiNewState(actor, actor->xspr.aiState->NextState);
			}

			if (actor->xspr.health > 0 && ((pDudeInfo->hinderDamage << 4) <= actor->cumulDamage))
			{
				actor->xspr.data3 = actor->cumulDamage;
				RecoilDude(actor);
			}
#ifdef NOONE_EXTENSIONS
			break;
		}
#endif
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
	XSECTOR* pXSector = actor->sector()->hasX() ? &actor->sector()->xs() : nullptr;

	DUDEEXTRA* pDudeExtra = &actor->dudeExtra;
	
	pDudeExtra->teslaHit = 0;
	pDudeExtra->time = 0;
	actor->dudeExtra.thinkTime = 0;
	actor->dudeExtra.active = 0;

#ifdef NOONE_EXTENSIONS
	unsigned int stateTimer = 0;
	DVector3 targetV(0,0,0);
	DBloodActor* pTargetMarker = nullptr;

	// dude patrol init
	if (gModernMap)
	{
		// must keep it in case of loading save
		if (actor->xspr.dudeFlag4 && actor->GetTarget() && actor->GetTarget()->GetType() == kMarkerPath)
		{
			stateTimer = actor->xspr.stateTimer;
			pTargetMarker = actor->GetTarget();
			targetV = actor->xspr.TargetPos;
		}
	}
#endif

	switch (actor->GetType())
	{
	case kDudeCultistTommy:
	case kDudeCultistShotgun:
	case kDudeCultistTesla:
	case kDudeCultistTNT:
	case kDudeCultistBeast:
	{
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_cultistIdle);
		break;
	}
	case kDudeCultistTommyProne:
	{
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_fanaticProneIdle);
		break;
	}
	case kDudeCultistShotgunProne:
	{
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_cultistProneIdle);
		break;
	}
	case kDudeZombieButcher: {
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_zombieFIdle);
		break;
	}
	case kDudeZombieAxeNormal: {
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_zombieAIdle);
		break;
	}
	case kDudeZombieAxeLaying:
	{
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_zombieSIdle);
		actor->spr.flags &= ~1;
		break;
	}
	case kDudeZombieAxeBuried: {
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_zombieEIdle);
		actor->spr.flags &= ~1;
		break;
	}
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone: {
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_gargoyleFIdle);
		break;
	}
	case kDudeGargoyleStatueFlesh:
	case kDudeGargoyleStatueStone:
		aiNewState(actor, NAME_gargoyleStatueIdle);
		break;
	case kDudeCerberusTwoHead: {
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_cerberusIdle);
		break;
	}
	case kDudeCerberusOneHead: {
		if (!VanillaMode()) {
			actor->dudeExtra.thinkTime = 0;
			aiNewState(actor, NAME_cerberus2Idle);
			break;
		}
		aiNewState(actor, NAME_genIdle);
		break;
	}
	case kDudeHellHound:
		aiNewState(actor, NAME_houndIdle);
		break;
	case kDudeHand:
		aiNewState(actor, NAME_handIdle);
		break;
	case kDudePhantasm:
	{
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_ghostIdle);
		break;
	}
	case kDudeInnocent:
		aiNewState(actor, NAME_innocentIdle);
		break;
	case kDudeRat:
		aiNewState(actor, NAME_ratIdle);
		break;
	case kDudeBoneEel:
	{
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_eelIdle);
		break;
	}
	case kDudeGillBeast:
		aiNewState(actor, NAME_gillBeastIdle);
		break;
	case kDudeBat:
	{
		actor->dudeExtra.thinkTime = 0;
		actor->dudeExtra.active = 0;
		aiNewState(actor, NAME_batIdle);
		break;
	}
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
	{
		actor->dudeExtra.active = 0;
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_spidIdle);
		break;
	}
	case kDudeSpiderMother:
	{
		actor->dudeExtra.active = 0;
		actor->dudeExtra.birthCounter = 0;
		aiNewState(actor, NAME_spidIdle);
		break;
	}
	case kDudeTchernobog:
	{
		actor->dudeExtra.active = 0;
		actor->dudeExtra.thinkTime = 0;
		aiNewState(actor, NAME_tchernobogIdle);
		break;
	}
	case kDudeTinyCaleb:
		aiNewState(actor, NAME_tinycalebIdle);
		break;
	case kDudeBeast:
		aiNewState(actor, NAME_beastIdle);
		break;
	case kDudePodGreen:
	case kDudePodFire:
		aiNewState(actor, NAME_podIdle);
		break;
	case kDudeTentacleGreen:
	case kDudeTentacleFire:
		aiNewState(actor, NAME_tentacleIdle);
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
		aiNewState(actor, NAME_genIdle);
		break;
	}
	aiSetTarget(actor, DVector3(0, 0, 0));
	actor->xspr.stateTimer = 0;
	switch (actor->GetType())
	{
	case kDudeSpiderBrown:
	case kDudeSpiderRed:
	case kDudeSpiderBlack:
		if (actor->spr.cstat & CSTAT_SPRITE_YFLIP) actor->spr.flags |= 9;
		else actor->spr.flags = 15;
		break;
	case kDudeGargoyleFlesh:
	case kDudeGargoyleStone:
	case kDudePhantasm:
	case kDudeBoneEel:
	case kDudeBat:
		actor->spr.flags |= 9;
		break;
	case kDudeGillBeast:
		if (pXSector && pXSector->Underwater) actor->spr.flags |= 9;
		else actor->spr.flags = 15;
		break;
	case kDudeZombieAxeBuried:
	case kDudeZombieAxeLaying:
		actor->spr.flags = 7;
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
		if (gModernMap && (actor->spr.cstat & CSTAT_SPRITE_YFLIP)) {
			if (!(actor->spr.flags & kModernTypeFlag1)) // don't add autoaim for player if hitag 1 specified in editor.
				actor->spr.flags = kHitagAutoAim;
			break;
		}
		[[fallthrough]];
		// go default
#endif
	default:
		actor->spr.flags = 15;
		break;
	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		if (actor->xspr.dudeFlag4)
		{
			// restore dude's path
			if (pTargetMarker)
			{
				actor->SetTarget(pTargetMarker);
				actor->xspr.TargetPos = targetV;
			}

			// reset target spot progress
			actor->xspr.data3 = 0;

			// make dude follow the markers
			bool uwater = spriteIsUnderwater(actor);
			if (actor->GetTarget() == nullptr || actor->GetTarget()->GetType() != kMarkerPath) 
			{
				actor->SetTarget(nullptr);
				aiPatrolSetMarker(actor);
			}

			if (stateTimer > 0) 
			{
				if (uwater) aiPatrolState(actor, kAiStatePatrolWaitW);
				else if (actor->xspr.modernFlags & kDudeFlagCrouch) aiPatrolState(actor, kAiStatePatrolWaitC);
				else aiPatrolState(actor, kAiStatePatrolWaitL);
				actor->xspr.stateTimer = stateTimer; // restore state timer
			}
			else if (uwater) aiPatrolState(actor, kAiStatePatrolMoveW);
			else if (actor->xspr.modernFlags & kDudeFlagCrouch) aiPatrolState(actor, kAiStatePatrolMoveC);
			else aiPatrolState(actor, kAiStatePatrolMoveL);
		}
	}
#endif
}


END_BLD_NS
