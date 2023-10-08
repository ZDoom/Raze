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


void BurnSeqCallback(DBloodActor*)
{
}

void burnThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	aiThinkTarget(actor);
}

void burnThinkGoto(DBloodActor* actor)
{
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();
	aiChooseDirection(actor, nAngle);
	if (nDist < 32 && absangle(actor->spr.Angles.Yaw, nAngle) < pDudeInfo->Periphery())
	{
		switch (actor->GetType())
		{
		case kDudeBurningCultist:
			aiNewState(actor, NAME_cultistBurnSearch);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, NAME_zombieABurnSearch);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, NAME_zombieFBurnSearch);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, NAME_innocentBurnSearch);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, NAME_beastBurnSearch);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, NAME_tinycalebBurnSearch);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, NAME_genDudeBurnSearch);
			break;
#endif
		}
	}
	aiThinkTarget(actor);
}

void burnThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		switch (actor->GetType())
		{
		case kDudeBurningCultist:
			aiNewState(actor, NAME_cultistBurnGoto);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, NAME_zombieABurnGoto);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, NAME_zombieFBurnGoto);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, NAME_innocentBurnGoto);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, NAME_beastBurnGoto);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, NAME_tinycalebBurnGoto);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, NAME_genDudeBurnGoto);
			break;
#endif
		}
		return;
	}
	assert(actor->IsDudeActor());
	DUDEINFO* pDudeInfo = getDudeInfo(actor);
	auto target = actor->GetTarget();

	auto dvec = target->spr.pos.XY() - actor->spr.pos.XY();
	DAngle nAngle = dvec.Angle();
	double nDist = dvec.Length();

	aiChooseDirection(actor, nAngle);
	if (target->xspr.health == 0)
	{
		switch (actor->GetType())
		{
		case kDudeBurningCultist:
			aiNewState(actor, NAME_cultistBurnSearch);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, NAME_zombieABurnSearch);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, NAME_zombieFBurnSearch);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, NAME_innocentBurnSearch);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, NAME_beastBurnSearch);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, NAME_tinycalebBurnSearch);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, NAME_genDudeBurnSearch);
			break;
#endif
		}
		return;
	}

	if (nDist <= pDudeInfo->SeeDist())
	{
		DAngle nDeltaAngle = absangle(actor->spr.Angles.Yaw, nAngle);
		double height = (pDudeInfo->eyeHeight * actor->spr.scale.Y);
		if (cansee(target->spr.pos, target->sector(), actor->spr.pos.plusZ(-height), actor->sector()))
		{
			if (nDist < pDudeInfo->SeeDist() && nDeltaAngle <= pDudeInfo->Periphery())
			{
				aiSetTarget(actor, actor->GetTarget());
				if (nDist < 51.1875 && nDeltaAngle < DAngle15)
				{
					switch (actor->GetType())
					{
					case kDudeBurningCultist:
						aiNewState(actor, NAME_cultistBurnAttack);
						break;
					case kDudeBurningZombieAxe:
						aiNewState(actor, NAME_zombieABurnAttack);
						break;
					case kDudeBurningZombieButcher:
						aiNewState(actor, NAME_zombieFBurnAttack);
						break;
					case kDudeBurningInnocent:
						aiNewState(actor, NAME_innocentBurnAttack);
						break;
					case kDudeBurningBeast:
						aiNewState(actor, NAME_beastBurnAttack);
						break;
					case kDudeBurningTinyCaleb:
						aiNewState(actor, NAME_tinycalebBurnAttack);
						break;
#ifdef NOONE_EXTENSIONS
					case kDudeModernCustomBurning:
						aiNewState(actor, NAME_genDudeBurnSearch);
						break;
#endif
					}
				}
				return;
			}
		}
	}

	switch (actor->GetType())
	{
	case kDudeBurningCultist:
		aiNewState(actor, NAME_cultistBurnGoto);
		break;
	case kDudeBurningZombieAxe:
		aiNewState(actor, NAME_zombieABurnGoto);
		break;
	case 242:
		aiNewState(actor, NAME_zombieFBurnGoto);
		break;
	case kDudeBurningInnocent:
		aiNewState(actor, NAME_innocentBurnGoto);
		break;
	case kDudeBurningBeast:
		aiNewState(actor, NAME_beastBurnGoto);
		break;
	case kDudeBurningTinyCaleb:
		aiNewState(actor, NAME_tinycalebBurnGoto);
		break;
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustomBurning:
		aiNewState(actor, NAME_genDudeBurnSearch);
		break;
#endif
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
