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



#include "blood.h"

BEGIN_BLD_NS


AISTATE cultistBurnIdle = { kAiStateIdle, 3, nullptr, 0, NULL, NULL, &AF(aiThinkTarget), NULL };
AISTATE cultistBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE cultistBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &cultistBurnSearch };
AISTATE cultistBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), &cultistBurnSearch };
AISTATE cultistBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &cultistBurnChase };

AISTATE zombieABurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE zombieABurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &zombieABurnSearch };
AISTATE zombieABurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), NULL };
AISTATE zombieABurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &zombieABurnChase };

AISTATE zombieFBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE zombieFBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &zombieFBurnSearch };
AISTATE zombieFBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), NULL };
AISTATE zombieFBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &zombieFBurnChase };

AISTATE innocentBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE innocentBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &zombieFBurnSearch };
AISTATE innocentBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), NULL };
AISTATE innocentBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &zombieFBurnChase };

AISTATE beastBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE beastBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &beastBurnSearch };
AISTATE beastBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), &beastBurnSearch };
AISTATE beastBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &beastBurnChase };

AISTATE tinycalebBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE tinycalebBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &tinycalebBurnSearch };
AISTATE tinycalebBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), &tinycalebBurnSearch };
AISTATE tinycalebBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &tinycalebBurnChase };

AISTATE genDudeBurnIdle = { kAiStateIdle, 3, nullptr, 0, NULL, NULL, &AF(aiThinkTarget), NULL };
AISTATE genDudeBurnChase = { kAiStateChase, 3, nullptr, 0, NULL, &AF(aiMoveForward), &AF(burnThinkChase), NULL };
AISTATE genDudeBurnGoto = { kAiStateMove, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkGoto), &genDudeBurnSearch };
AISTATE genDudeBurnSearch = { kAiStateSearch, 3, nullptr, 3600, NULL, &AF(aiMoveForward), &AF(burnThinkSearch), &genDudeBurnSearch };
AISTATE genDudeBurnAttack = { kAiStateChase, 3, &AF(BurnSeqCallback), 120, NULL, NULL, NULL, &genDudeBurnChase };

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
			aiNewState(actor, &cultistBurnSearch);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, &zombieABurnSearch);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, &zombieFBurnSearch);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, &innocentBurnSearch);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, &beastBurnSearch);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, &tinycalebBurnSearch);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, &genDudeBurnSearch);
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
			aiNewState(actor, &cultistBurnGoto);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, &zombieABurnGoto);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, &zombieFBurnGoto);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, &innocentBurnGoto);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, &beastBurnGoto);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, &tinycalebBurnGoto);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, &genDudeBurnGoto);
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
			aiNewState(actor, &cultistBurnSearch);
			break;
		case kDudeBurningZombieAxe:
			aiNewState(actor, &zombieABurnSearch);
			break;
		case kDudeBurningZombieButcher:
			aiNewState(actor, &zombieFBurnSearch);
			break;
		case kDudeBurningInnocent:
			aiNewState(actor, &innocentBurnSearch);
			break;
		case kDudeBurningBeast:
			aiNewState(actor, &beastBurnSearch);
			break;
		case kDudeBurningTinyCaleb:
			aiNewState(actor, &tinycalebBurnSearch);
			break;
#ifdef NOONE_EXTENSIONS
		case kDudeModernCustomBurning:
			aiNewState(actor, &genDudeBurnSearch);
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
						aiNewState(actor, &cultistBurnAttack);
						break;
					case kDudeBurningZombieAxe:
						aiNewState(actor, &zombieABurnAttack);
						break;
					case kDudeBurningZombieButcher:
						aiNewState(actor, &zombieFBurnAttack);
						break;
					case kDudeBurningInnocent:
						aiNewState(actor, &innocentBurnAttack);
						break;
					case kDudeBurningBeast:
						aiNewState(actor, &beastBurnAttack);
						break;
					case kDudeBurningTinyCaleb:
						aiNewState(actor, &tinycalebBurnAttack);
						break;
#ifdef NOONE_EXTENSIONS
					case kDudeModernCustomBurning:
						aiNewState(actor, &genDudeBurnSearch);
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
		aiNewState(actor, &cultistBurnGoto);
		break;
	case kDudeBurningZombieAxe:
		aiNewState(actor, &zombieABurnGoto);
		break;
	case 242:
		aiNewState(actor, &zombieFBurnGoto);
		break;
	case kDudeBurningInnocent:
		aiNewState(actor, &innocentBurnGoto);
		break;
	case kDudeBurningBeast:
		aiNewState(actor, &beastBurnGoto);
		break;
	case kDudeBurningTinyCaleb:
		aiNewState(actor, &tinycalebBurnGoto);
		break;
#ifdef NOONE_EXTENSIONS
	case kDudeModernCustomBurning:
		aiNewState(actor, &genDudeBurnSearch);
		break;
#endif
	}
	actor->SetTarget(nullptr);
}

END_BLD_NS
