//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

*****************************************************************
NoOne: AI code for Custom Dude system.
*****************************************************************

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
#ifdef NOONE_EXTENSIONS
#include "ai.h"
#include "nnexts.h"
#include "globals.h"
#include "player.h"
#include "endgame.h"
#include "view.h"
//#include "aicdud.h"

BEGIN_BLD_NS

#define SEQOFFS(x) (kCdudeDefaultSeq + x)

#pragma pack(push, 1)
struct TARGET_INFO
{
	DBloodActor* pSpr;
	double nDist;
	DAngle nAng;
	DAngle nDang;
	int nCode;
};
#pragma pack(pop)

void resetTarget(DBloodActor* pSpr)            { pSpr->xspr.target = nullptr; }
void moveStop(DBloodActor* pSpr)                { pSpr->vel.XY().Zero(); }
static bool THINK_CLOCK(int nSpr, int nClock = 3)               { return ((gFrameCount & nClock) == (nSpr & nClock)); }
static int qsSortTargets(TARGET_INFO* ref1, TARGET_INFO* ref2)  { return ref1->nDist > ref2->nDist? 1 : ref1->nDist < ref2->nDist? -1 : 0; }
DAngle getTargetAng(DBloodActor* pSpr);


// This set of functions needs to be exported for scripting later to allow extension of this list.
static DBloodActor* weaponShotDummy(DCustomDude*, CUSTOMDUDE_WEAPON*, DVector3& offs, DVector3& vel) { return nullptr; }
static DBloodActor* weaponShotHitscan(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotMissile(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotThing(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotSummon(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotKamikaze(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotSpecialBeastStomp(DCustomDude* pDude, CUSTOMDUDE_WEAPON*, DVector3& offs, DVector3& vel);

DBloodActor* (*gWeaponShotFunc[])(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel) =
{
	weaponShotDummy,    // none
	weaponShotHitscan,
	weaponShotMissile,
	weaponShotThing,
	weaponShotSummon,   // vanilla dude
	weaponShotSummon,   // custom  dude
	weaponShotKamikaze,
	weaponShotSpecialBeastStomp,
};


// for kModernThingThrowableRock
static const short gCdudeDebrisPics[6] =
{
	2406, 2280, 2185, 2155, 2620, 3135
};

static DBloodActor* weaponShotHitscan(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
	const VECTORDATA* pVect = &gVectorData[pWeap->id];
	auto pSpr = pDude->pSpr;

	// ugly hack to make it fire at required distance was removed because we have a better solution! :P
	actFireVector(pSpr, pOffs.X, pOffs.Y, vel, (VECTOR_TYPE)pWeap->id, pWeap->GetDistance());

	return nullptr;
}

static DBloodActor* weaponShotMissile(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
	DBloodActor* pSpr = pDude->pSpr, *pShot;

	pShot = nnExtFireMissile(pSpr, pOffs.X, pOffs.Y, vel, pWeap->id);
	if (pShot)
	{
		nnExtOffsetSprite(pShot, DVector3(0, pOffs.Y, 0));

		if (pWeap->shot.clipdist)
			pShot->clipdist = pWeap->shot.clipdist;

		if (pWeap->HaveVelocity())
		{
			pShot->xspr.target = nullptr; // have to erase, so vanilla won't set velocity back
			nnExtScaleVelocity(pShot, pWeap->shot._velocity, vel);
		}

		pWeap->shot.appearance.Set(pShot);

		if (pWeap->shot.targetFollow != nullAngle)
		{
			pShot->xspr.goalAng = pWeap->shot.targetFollow;
			gFlwSpritesList.Push(MakeObjPtr(pShot));
			pShot->prevmarker = pSpr->xspr.target;
			pShot->xspr.target = nullptr; // have own target follow code
		}


		return pShot;
	}

	return nullptr;
}

static DBloodActor* weaponShotThing(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
	DBloodActor* pSpr = pDude->pSpr;
	DBloodActor* pLeech = pDude->pLeech, *pShot, *pTarget = pSpr->xspr.target;
	
	if (!pTarget)
		return nullptr;

	auto dv = pTarget->spr.pos - pSpr->spr.pos;
	auto nDist = dv.Length();
	double nSlope = FixedToFloat(12000);
	bool impact = true;
	int nHealth = 0;

	switch (pWeap->id)
	{
		case kModernThingEnemyLifeLeech:
		case kThingDroppedLifeLeech:
			if (!pDude->IsLeechBroken())
			{
				if (pLeech)
				{
					if (xsprIsFine(pLeech))
						nHealth = pLeech->xspr.health;

					pDude->LeechPickup(); // pickup it before throw
				}
			}
			break;
	}
	
	nSlope = (pWeap->HaveSlope()) ? pWeap->shot._slope : ((vel.Z * 2) - nSlope);

	// fixed point math sucks
	//nVel = divscale23(nDist / nDiv, 120);
	//double nVel = (nDist * worldtoint * 128 / 540) / 120.;
	double nVel = nDist * 0.031605;

	pShot = actFireThing(pSpr, -pOffs.X, pOffs.Z, nSlope, pWeap->id, nVel);
	if (pShot)
	{
		nnExtOffsetSprite(pShot, DVector3(0, pOffs.Y, 0));

		pShot->ownerActor            = pSpr;

		switch (pWeap->id)
		{
			case kModernThingTNTProx:
			case kThingArmedProxBomb:
			case kModernThingThrowableRock:
			case kModernThingEnemyLifeLeech:
			case kThingDroppedLifeLeech:
			case kThingBloodBits:
			case kThingBloodChunks:
				switch (pWeap->id)
				{
					case kModernThingThrowableRock:
						pShot->spr.setspritetexture(tileGetTextureID(gCdudeDebrisPics[Random(countof(gCdudeDebrisPics))]));
						pShot->spr.scale.X  = pShot->spr.scale.Y = (24 + Random(42)) * REPEAT_SCALE;
						pShot->spr.cstat    |= CSTAT_SPRITE_BLOCK;
						pShot->spr.pal      = 5;

						if (Chance(0x5000)) pShot->spr.cstat |= CSTAT_SPRITE_XFLIP;
						if (Chance(0x5000)) pShot->spr.cstat |= CSTAT_SPRITE_YFLIP;

						if (pShot->spr.scale.X > 60 * REPEAT_SCALE)       pShot->xspr.data1 = 43;
						else if (pShot->spr.scale.X > 40 * REPEAT_SCALE)  pShot->xspr.data1 = 33;
						else if (pShot->spr.scale.X > 30 * REPEAT_SCALE)  pShot->xspr.data1 = 23;
						else                           pShot->xspr.data1 = 12;
						break;
					case kThingArmedProxBomb:
					case kModernThingTNTProx:
						pShot->xspr.state = 0;
						pShot->xspr.Proximity = true;
						break;
					case kThingBloodBits:
					case kThingBloodChunks:
						DudeToGibCallback1(pShot);
						break;
					default:
						if (pLeech)
						{
							pShot->xspr.health = nHealth;
						}
						else
						{
							pShot->xspr.health = ((pShot->IntVar("defhealth") << 4) * ClipLow(gGameOptions.nDifficulty, 1)) >> 1;
						}

						pShot->spr.cstat        &= ~CSTAT_SPRITE_BLOCK;
						pShot->spr.pal          = 6;
						pShot->spr.clipdist     = 0;
						pShot->xspr.data3       = 512 / (gGameOptions.nDifficulty + 1);
						pShot->xspr.target      = pTarget;
						pShot->xspr.Proximity   = true;
						pShot->xspr.stateTimer  = 1;

						evPostActor(pShot, 80, AF(LeechStateTimer));
						pDude->pLeech = pShot;
						break;
				}
				impact = false;
				break;
			case kThingNapalmBall:
				pShot->spr.scale.X = pShot->spr.scale.Y = 24 * REPEAT_SCALE;
				pShot->xspr.data4 = 3 + Random2(2);
				pShot->xspr.Impact = true;
				break;
		}

		if (pWeap->shot.clipdist)               pShot->clipdist = pWeap->shot.clipdist * CLIPDIST_SCALE;
		if (pWeap->HaveVelocity())              nnExtScaleVelocity(pShot, pWeap->shot._velocity * 8, vel, 0x01);
		
		pWeap->shot.appearance.Set(pShot);

		if (pWeap->shot.targetFollow != nullAngle)
		{
			pShot->xspr.goalAng = pWeap->shot.targetFollow;
			pShot->prevmarker = pSpr->xspr.target;
			gFlwSpritesList.Push(MakeObjPtr(pShot));
		}

		if (pWeap->shot.impact > 1)
		{
			if (impact)
				pShot->xspr.Impact = (pShot->xspr.Impact != 0 && nDist <= 7680);
		}
		else
		{
			pShot->xspr.Impact = pWeap->shot.impact;
		}

		if (!pShot->xspr.Impact)
			evPostActor(pShot, 120 * Random(2) + 120, kCmdOn, pSpr);

		return pShot;
	}

	return nullptr;
}

static bool posObstructed(DVector3& pos, double nRadius)
{
	int i;
	for (i = sector.SSize() - 1; i >= 0; i--)
	{
		if (inside(pos.X, pos.Y, &sector[i]))
			break;
	}
	if (i < 0)
		return true;

	BloodSpriteIterator it;
	while (auto pSpr = it.Next())
	{
		if ((pSpr->spr.flags & kHitagFree) || (pSpr->spr.flags & kHitagRespawn)) continue;
		if ((pSpr->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_FACING)
			continue;

		if (!(pSpr->spr.cstat & CSTAT_SPRITE_BLOCK))
		{
			if (!pSpr->IsDudeActor() || !dudeIsAlive(pSpr))
				continue;
		}
		else
		{
			auto tex = TexMan.GetGameTexture(pSpr->spr.spritetexture());
			if (!tex) continue;
			float w = tex->GetDisplayWidth();
			float h = tex->GetDisplayHeight();

			if (w <= 0 || h <= 0)
				continue;
		}

		if (CheckProximityPoint(pSpr->spr.pos.X, pSpr->spr.pos.Y, pSpr->spr.pos.Z, pos.X, pos.Y, pos.Z, nRadius))
			return true;
	}

	return false;
}



static DBloodActor* weaponShotSummon(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel)
{
	DBloodActor* pShot, *pSpr = pDude->pSpr;

	DVector3 pos = pSpr->spr.pos;
	DAngle a = nullAngle;
	
	int nDude = pWeap->id;
	if (pWeap->type == kCdudeWeaponSummonCdude)
		nDude = kDudeModernCustom;

	DVector3 pOffs(offs.X, max(offs.Y, 800.), offs.Z);
	nnExtOffsetPos(pOffs, pSpr->spr.Angles.Yaw, pos);

	while (a < DAngle180)
	{
		if (!posObstructed(pos, 2.))
		{
			if ((pShot = nnExtSpawnDude(pSpr, nDude, pos)) != nullptr)
			{
				if (nDude == kDudeModernCustom)
					pShot->xspr.data1 = pWeap->id;

				if (pWeap->shot.clipdist)
					pShot->clipdist = pWeap->shot.clipdist * CLIPDIST_SCALE;

				if (pWeap->HaveVelocity())
					nnExtScaleVelocity(pShot, pWeap->shot._velocity, vel);

				pWeap->shot.appearance.Set(pShot);

				aiInitSprite(pShot);

				pShot->xspr.TargetPos  = pSpr->xspr.TargetPos;
				pShot->xspr.target     = pSpr->xspr.target;
				pShot->spr.Angles      = pSpr->spr.Angles;

				aiActivateDude(pShot);

				pDude->pSlaves.Push(MakeObjPtr(pShot));
				if (AllowedKillType(pShot))
					Level.addKillCount();

				return pShot;
			}
		}
		else
		{
			pos.XY() = rotatepoint(pSpr->spr.pos.XY(), pos.XY(), a);
			a += DAngle15;
			continue;
		}

		break;
	}

	return nullptr;
}

static DBloodActor* weaponShotKamikaze(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
	DBloodActor* pSpr = pDude->pSpr;
	DBloodActor* pShot = actSpawnSprite(pSpr->sector(), pSpr->spr.pos, kStatExplosion, true);

	if (pShot)
	{
		int nType = pWeap->id - kTrapExploder;
		const EXPLOSION* pExpl = &explodeInfo[nType];
		const EXPLOSION_EXTRA* pExtra = &gExplodeExtra[nType];

		pShot->spr.lotag = nType; // this may not call ChangeType!
		pShot->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		pShot->ownerActor = pSpr;
		pShot->spr.shade = -127;
		pShot->spr.scale.X = pShot->spr.scale.Y = pExpl->repeat * REPEAT_SCALE;
		pShot->spr.Angles.Yaw = pSpr->spr.Angles.Yaw;
		
		pShot->xspr.data1 = pExpl->ticks;
		pShot->xspr.data2 = pExpl->quakeEffect;
		pShot->xspr.data3 = pExpl->flashEffect;
		pShot->xspr.data4 = ClipLow((int)pWeap->GetDistance(), pExpl->radius); // stores actual distance as integer.

		seqSpawn(pExtra->seq, pShot);

		if (pExtra->ground)
		   pShot->spr.pos.Z = getflorzofslopeptr(pShot->sector(), pShot->spr.pos.X, pShot->spr.pos.Y);

		pWeap->shot.appearance.Set(pShot);

		clampSprite(pShot);
		nnExtOffsetSprite(pShot, pOffs); // offset after default sprite placement
	}

	if (pSpr->xspr.health)
	{
		pSpr->xspr.health = 0; // it supposed to attack once
		pDude->Kill(pSpr, kDamageExplode, 0x10000);
	}
	
	return pShot;
}

static DBloodActor* weaponShotSpecialBeastStomp(DCustomDude* pDude, CUSTOMDUDE_WEAPON* pWeapon, DVector3& pOffs, DVector3& vel)
{
	DBloodActor* pSpr = pDude->pSpr;
	
	int vc = 400;
	int v1c = 7 * gGameOptions.nDifficulty;
	int v10 = 55 * gGameOptions.nDifficulty;

	for (auto stat : { kStatDude, kStatThing })
	{
		BloodStatIterator it(stat);
		while (auto pSpr2 = it.Next())
		{
			if (pSpr2 == pSpr || !xsprIsFine(pSpr2) || pSpr2->ownerActor == pSpr)
				continue;

			if (CheckProximity(pSpr2, pSpr->spr.pos, pSpr->sector(), (int)pWeapon->GetDistance()))
			{
				int nDist2 = int((pSpr->spr.pos.XY() - pSpr2->spr.pos.XY()).Length() * worldtoint);
				if (nDist2 <= vc)
				{
					int nDamage;
					if (!nDist2)
						nDamage = v1c + v10;
					else
						nDamage = v1c + ((vc - nDist2) * v10) / vc;
						
					if (pSpr2->IsPlayerActor())
					{
						auto pPlayer = getPlayer(pSpr2);
						pPlayer->quakeEffect = ClipHigh(pPlayer->quakeEffect + (nDamage << 2), 1280);
					}

					actDamageSprite(pSpr, pSpr2, kDamageFall, nDamage << 4);
				}
			}
		}
	}

	return nullptr;
}


void weaponShot(DBloodActor* pSpr)
{
	if (!pSpr->hasX())
		return;

	auto pDude = cdudeGet(pSpr);
	CUSTOMDUDE_WEAPON* pCurWeap = pDude->pWeapon, * pWeap;
	DVector3 shotOffs, * pStyleOffs;

	int nShots, nTime;
	int i, j;

	DAngle sang; 
	DAngle tang; 
	bool hxof;
	int  hsht;
	bool styled;


	DVector3 dv1 (pSpr->spr.Angles.Yaw.ToVector() * 0.25, 0);  // todo: check the factor! If I read it correctly nnextCoSin returns Q18.14 vectors.
	DVector3 dv2, dv3;

	if (pCurWeap)
	{
		for (i = 0; i < pDude->numWeapons; i++)
		{
			pWeap = &pDude->weapons[i];
			if (pWeap->available)
			{
				if (pCurWeap != pWeap)
				{
					// check if this weapon could be used in conjunction with current
					if (!pCurWeap->sharedId || pCurWeap->sharedId != pWeap->sharedId)
						continue;
				}

				nShots = pWeap->GetNumshots();
				pWeap->ammo.Dec(nShots);
				styled = (nShots > 1 && pWeap->style.available);
				shotOffs = pWeap->shot.offset;

				if (styled)
				{
					pStyleOffs = &pWeap->style.offset; 
					hsht = nShots >> 1;
					sang = pWeap->style.angle / nShots;
					hxof = 0;
					tang = nullAngle;
				}

				dv1.Z = (pWeap->shot._slope == INT32_MAX) ?
					pDude->AdjustSlope(pSpr->xspr.target, pWeap->shot.offset.Z) : pWeap->shot._slope;

				for (j = nShots; j > 0; j--)
				{
					if (!styled || j == nShots)
					{

						dv3.X = Random3F(pWeap->dispersion[0]); // todo: check if same scale as dv1.
						dv3.Y = Random3F(pWeap->dispersion[0]);
						dv3.Z = Random3F(pWeap->dispersion[1]);
						dv2 = dv1 + dv3;
					}

					auto pShot = gWeaponShotFunc[pWeap->type](pDude, pWeap, shotOffs, dv2);
					if (pShot)
					{
						// override removal timer
						if ((nTime = pWeap->shot.remTime) >= 0)
						{
							evKillActor(pShot, AF(RemoveActor));
							if (nTime)
								evPostActor(pShot, nTime, AF(RemoveActor));
						}
					}

					// setup style
					if (styled)
					{
						if (double txof = pStyleOffs->X)
						{
							if (j <= hsht)
							{
								if (!hxof)
								{
									shotOffs.X = pWeap->shot.offset.X;
									hxof = 1;
								}

								txof = -txof;
							}

							shotOffs.X += txof;
						}

						shotOffs.Y += pStyleOffs->Y;
						shotOffs.Z += pStyleOffs->Z;

						if (pWeap->style.angle != nullAngle)
						{
							// for sprites
							if (pShot)
							{
								if (j <= hsht && sang > nullAngle)
								{
									sang = -sang;
									tang = nullAngle;
								}

								tang += sang;
								pShot->vel.XY() = rotatepoint(pShot->vel.XY(), pSpr->spr.pos.XY(), tang); // formula looks broken
								//pShot->vel.XY() = rotatepoint(pShot->vel.XY(), DVector2(0, 0), tang); // what it probably should be!
								pShot->spr.Angles.Yaw = pShot->vel.Angle();
							}
							// for hitscan
							else
							{
								if (j <= hsht && sang > nullAngle)
								{
									dv2.XY() = pSpr->spr.Angles.Yaw.ToVector() + dv3.XY();
									sang = -sang;
								}

								auto dv = rotatepoint(dv2.XY(), pSpr->spr.pos.XY(), sang); // formula looks broken
								//auto dv = rotatepoint(dv2, DVector2(0, 0), sang); // what it probably should be!
							}
						}
					}
				}

				pWeap->sound.Play(pSpr);
				if (pWeap->cooldown.Check())
					pWeap->available = 0;
			}
		}
	}
}


static int checkTarget(DCustomDude* pDude, DBloodActor* pTarget, TARGET_INFO* pOut)
{
	if (!pTarget)
		return -1;

	DBloodActor* pSpr = pDude->pSpr;
	if (pSpr->ownerActor == pTarget || pTarget->xspr.health <= 0)
		return -2;

	if (pTarget->IsPlayerActor())
	{
		auto pPlayer = getPlayer(pTarget);
		if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
			return -3;
	}

	auto dv = pSpr->spr.pos.XY() - pTarget->spr.pos.XY();
	double nDist = dv.Length();
	bool s = (nDist < pDude->_seeDist);
	bool h = (nDist < pDude->_hearDist);
	
	if (!s && !h)
		return -4;

	DUDEINFO* pInfo = pDude->pInfo;
	if (!cansee(pTarget->spr.pos, pTarget->sector(), pSpr->spr.pos.plusZ(-(pInfo->eyeHeight * pSpr->spr.scale.Y)), pSpr->sector()))
		return -5;

	DAngle nAng = dv.Angle();
	if (s)
	{
		auto nDang = absangle(nAng, pSpr->spr.Angles.Yaw);
		if (nDang <= pDude->periphery)
		{
			pOut->pSpr  = pTarget;
			pOut->nDist = nDist;
			pOut->nDang = nDang;
			pOut->nAng  = nAng;
			pOut->nCode = 1;
			return 1;
		}
	}
	
	if (h)
	{
		pOut->pSpr  = pTarget;
		pOut->nDist = nDist;
		pOut->nDang = nullAngle;
		pOut->nAng  = nAng;
		pOut->nCode = 2;
		return 2;
	}

	return -255;
}

void thinkTarget(DBloodActor* pSpr)
{
	int i; 
	DBloodActor* pTarget;
	TARGET_INFO targets[kMaxPlayers], *pInfo = targets;
	DCustomDude* pDude = cdudeGet(pSpr);
	int numTargets = 0;

	if (Chance(pDude->pInfo->alertChance))
	{
		for (i = connecthead; i >= 0; i = connectpoint2[i])
		{
			auto pPlayer = getPlayer(i);
			if (checkTarget(pDude, pPlayer->GetActor(), &targets[numTargets]) > 0)
				numTargets++;
		}

		if (numTargets)
		{
			if (numTargets > 1) // closest first
				qsort(targets, numTargets, sizeof(targets[0]), (int(*)(const void*, const void*))qsSortTargets);

			pTarget = pInfo->pSpr;
			if (pDude->pSpr->dudeExtra.active)
			{
				if (pSpr->xspr.target != pTarget || Chance(0x0400))
					pDude->PlaySound(kCdudeSndTargetSpot);
			}
			
			pSpr->xspr.goalAng = pInfo->nAng.Normalized360();
			if (pInfo->nCode == 1) aiSetTarget(pSpr, pTarget);
			else aiSetTarget(pSpr, pTarget->spr.pos);
			aiActivateDude(pSpr);
		}
	}
}

void thinkFlee(DBloodActor* pSpr)
{
	DAngle nAng = (pSpr->spr.pos.XY() - pSpr->xspr.TargetPos.XY()).Angle();
	DAngle nDang = absangle(nAng, pSpr->spr.Angles.Yaw);
	if (nDang > DAngle45)
		pSpr->xspr.goalAng = (nAng + (DAngle15 * Random2(2))).Normalized360();

	aiChooseDirection(pSpr, pSpr->xspr.goalAng);

}

void thinkSearch(DBloodActor* pSpr)
{
	aiChooseDirection(pSpr, pSpr->xspr.goalAng);
	thinkTarget(pSpr);
}

void maybeThinkSearch(DBloodActor* pSpr)
{
	// this originally edited the state's callback, but that's inherently non-serializable so another way had to be chosen.
	if (pSpr->chasehackflag)
	{
		aiChooseDirection(pSpr, pSpr->xspr.goalAng);
		thinkTarget(pSpr);
	}
}

void thinkChase(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr); 
	auto pHit = &gHitInfo; 
	DUDEINFO* pInfo = pDude->pInfo;
	double nSlope = 0;
	int thinkTime = THINK_CLOCK(pSpr->GetIndex());
	int turn2target = 0, interrupt = 0;
	int inAttack = pDude->IsAttacking();
	int changePos = 0;

	DBloodActor* pTarget = pSpr->xspr.target;
	if (!pTarget)
	{
		pDude->NewState(kCdudeStateSearch);
		return;
	}

	if (pTarget->ownerActor == pSpr || !pTarget->IsDudeActor() || !xsprIsFine(pTarget)) // target lost
	{
		pDude->NewState(kCdudeStateSearch);
		return;
	}

	if (pTarget->xspr.health <= 0) // target is dead
	{
		DBloodPlayer* pPlayer = nullptr;
		if ((!pTarget->IsPlayerActor()) || ((pPlayer = getPlayer(pTarget)) != nullptr && pPlayer->fragger == pSpr))
			pDude->PlaySound(kCdudeSndTargetDead);
		
		if (inAttack) pDude->NextState(kCdudeStateSearch);
		else pDude->NewState(kCdudeStateSearch);
		return;
	}

	if (pTarget->IsPlayerActor())
	{
		auto pPlayer = getPlayer(pTarget);
		if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
		{
			pDude->NewState(kCdudeStateSearch);
			return;
		}
	}

	// check target
	auto dv = pTarget->spr.pos.XY() - pSpr->spr.pos.XY();
	auto nDist = dv.Length();
	auto nDAng = absangle(dv.Angle(), pSpr->spr.Angles.Yaw);
	auto nHeigh = (pInfo->eyeHeight * pSpr->spr.scale.Y);

	if (thinkTime && !inAttack)
		aiChooseDirection(pSpr, dv.Angle());

	// is the target visible?
	if (nDist > pInfo->SeeDist() || !cansee(pTarget->spr.pos, pTarget->sector(), pSpr->spr.pos.plusZ(-nHeigh), pSpr->sector()))
	{
		if (inAttack) pDude->NextState(kCdudeStateSearch);
		else pDude->NewState(kCdudeStateSearch);
		return;
	}
	else if (nDAng > pInfo->Periphery())
	{
		if (inAttack) pDude->NextState(kCdudeStateChase);
		else pDude->NewState(kCdudeStateChase);
		return;
	}

	ARG_PICK_WEAPON pPickArg(pSpr, pTarget, nDist, nDAng);
	CUSTOMDUDE_WEAPON* pWeapon = pDude->pWeapon;
	if (pWeapon)
	{
		nSlope      = pDude->AdjustSlope(pSpr->xspr.target, pWeapon->shot.offset.Z);
		turn2target = pWeapon->turnToTarget;
		interrupt   = pWeapon->interruptable;
	}

	if (thinkTime && Chance(0x2000))
		pDude->PlaySound(kCdudeSndTargetChase);

	// in attack
	if (inAttack)
	{
		if (turn2target)
		{
			pSpr->xspr.goalAng = getTargetAng(pSpr);
			moveTurn(pSpr);
		}

		if (pSpr->xspr.aiState->stateTicks) // attack timer set
		{
			if (!pSpr->xspr.stateTimer)
			{
				pWeapon = pDude->PickWeapon(&pPickArg);
				if (pWeapon && pWeapon == pDude->pWeapon)
				{
					pDude->pWeapon = pWeapon;
					pDude->NewState(pWeapon->stateID);
				}
				else
					pDude->NewState(kCdudeStateChase);
			}
			else if (interrupt)
			{
				pDude->PickWeapon(&pPickArg);
				if (!pWeapon->available)
					pDude->NewState(kCdudeStateChase);
			}

			return;
		}

		if (!pDude->SeqPlaying()) // final frame
		{
			pWeapon = pDude->PickWeapon(&pPickArg);
			if (!pWeapon)
			{
				pDude->NewState(kCdudeStateChase);
				return;
			}
			else
			{
				pDude->pWeapon = pWeapon;
			}
		}
		else // playing previous animation
		{
			if (!interrupt)
			{
				if (!pWeapon)
				{
					pDude->NextState(kCdudeStateChase);
				}

				return;
			}
			else
			{
				pDude->PickWeapon(&pPickArg);
				if (!pWeapon->available)
				{
					pDude->NewState(kCdudeStateChase);
					return;
				}
			}
		}
	}
	else
	{
		// enter attack
		pWeapon = pDude->PickWeapon(&pPickArg);
		if (pWeapon)
			pDude->pWeapon = pWeapon;
	}

	if (pWeapon)
	{
		switch (pWeapon->type)
		{
			case kCdudeWeaponNone:
				if (pDude->CanMove()) pDude->NextState(kCdudeStateFlee);
				else pDude->NextState(kCdudeStateSearch);
				return;
			case kCdudeWeaponHitscan:
			case kCdudeWeaponMissile:
			case kCdudeWeaponThrow:
				if (pDude->CanMove())
				{
					HitScan(pSpr, pSpr->spr.pos.Z, DVector3(dv, nSlope), pWeapon->clipMask, nDist);
					DBloodActor* pHitSpr = pHit->actor();
					if (pHitSpr != pSpr->xspr.target && !pDude->AdjustSlope(nDist, &nSlope))
					{
						changePos = 1;
						if (pHitSpr)
						{
							if (pHitSpr->IsDudeActor())
							{
								if (pHitSpr->hasX())
								{
									if (pHitSpr->xspr.target == pSpr)
										return;

									if (pHitSpr->xspr.dodgeDir > 0)
										pSpr->xspr.dodgeDir = -pHitSpr->xspr.dodgeDir;
								}
							}
							else if (pHitSpr->ownerActor == pSpr) // projectiles, things, fx etc...
							{
								if (!pHitSpr->hasX() || !pHitSpr->xspr.health)
									changePos = 0;
							}
							
							if (changePos)
							{
								// prefer dodge
								if (pDude->dodge.onAimMiss.Allow())
								{
									pDude->NewState(kCdudeStateDodge, 30 * (Random(2) + 1));
									return;
								}
							}
						}

						if (changePos)
						{
							// prefer chase
							pDude->NewState(kCdudeStateChase);
							return;
						}
					}
				}
				[[fallthrough]];
			default:
				pDude->NewState(pWeapon->stateID);
				pDude->NextState(pWeapon->nextStateID);
				return;
		}
	}

	if (!pDude->CanMove())
		pDude->NextState(kCdudeStateSearch);
}

DAngle getTargetAng(DBloodActor* pSpr)
{
	DVector2 v;
	DBloodActor* pTarg = pSpr->xspr.target;
	if (pTarg)
	{
		v = pTarg->spr.pos.XY();
	}
	else
	{
		v = pSpr->xspr.TargetPos.XY();
	}

	return (v - pSpr->spr.pos.XY()).Angle();
}

void turnToTarget(DBloodActor* pSpr)
{
	pSpr->xspr.goalAng = pSpr->spr.Angles.Yaw = getTargetAng(pSpr);
}

void moveTurn(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	DAngle nVelTurn = DAngle::fromBuild(pDude->GetVelocity(kParVelocityTurn));
	DAngle nAng = absangle(pSpr->xspr.goalAng, pSpr->spr.Angles.Yaw);
	pSpr->spr.Angles.Yaw = (pSpr->spr.Angles.Yaw + clamp(nAng, -nVelTurn, nVelTurn)).Normalized360();
}

void moveDodge(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	moveTurn(pSpr);

	if (pSpr->xspr.dodgeDir && pDude->CanMove())
	{
		AdjustVelocity(pSpr, ADJUSTER
			{
				double nVelDodge = pDude->GetVelocityF(kParVelocityDodge);
				if (pSpr->xspr.dodgeDir > 0)
				{
					t2 += nVelDodge;
				}
				else
				{
					t2 -= nVelDodge;
				}

			});
	}
}

void moveKnockout(DBloodActor* pSpr)
{
	double zv = pSpr->vel.Z;
	pSpr->vel.Z = clamp(zv * (1 + 3. / 16), 1. / 16, 4.);
}

void moveForward(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	DAngle nVelTurn    = DAngle::fromBuild(pDude->GetVelocity(kParVelocityTurn));
	double nVelForward = pDude->GetVelocityF(kParVelocityForward);
	DAngle nAng = absangle(pSpr->xspr.goalAng, pSpr->spr.Angles.Yaw);
	pSpr->spr.Angles.Yaw = (pSpr->spr.Angles.Yaw + clamp(nAng, -nVelTurn, nVelTurn)).Normalized360();
	double z = 0;

	if (pDude->CanMove())
	{
		if (pDude->IsUnderwater())
		{
			DBloodActor* pTarget = pSpr->xspr.target;
			if (pTarget)
			{
				if (spriteIsUnderwater(pTarget, true))
					z = (pTarget->spr.pos.Z - pSpr->spr.pos.Z) + inttoworld * (10 << Random(12));
			}
			else
			{
				z = (pSpr->xspr.TargetPos.Z - pSpr->spr.pos.Z);
			}

			if (Chance(0x0500))
				z *= 2;

			pSpr->vel.Z += z / 256.;    // was directly adding Build z (Q24.8) to vel (Q16.16)
		}
		
		// don't move forward if trying to turn around
		if (abs(nAng) <= DAngle60)
		{
			pSpr->vel.XY() += pSpr->spr.Angles.Yaw.ToVector() * nVelForward;
		}
	}
}

void enterSleep(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	pDude->StatusSet(kCdudeStatusSleep);
	resetTarget(pSpr);
	moveStop(pSpr);

	// reduce distances while sleeping
	pDude->_seeDist      = kCdudeMinSeeDist;
	pDude->_hearDist     = kCdudeMinHearDist;
	pDude->periphery    = DAngle360;
}

void enterWake(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	if (pDude->StatusTest(kCdudeStatusSleep))
	{
		pDude->StatusRem(kCdudeStatusSleep);

		// restore distances when awaked
		pDude->_seeDist      = pDude->pInfo->SeeDist();
		pDude->_hearDist     = pDude->pInfo->HearDist();
		pDude->periphery    = pDude->pInfo->Periphery();
	}

	pDude->PlaySound(kCdudeSndWake);
}


void enterDying(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	if (pDude->mass > 48)
		pDude->mass = ClipLow(pDude->mass >> 2, 48);
}

void thinkDying(DBloodActor* pSpr)
{
	SPRITEHIT* pHit = &pSpr->hit;
	if (pHit->florhit.type == kHitNone && spriteIsUnderwater(pSpr, true))
		pSpr->vel.Z = max(pSpr->vel.Z, 1024.);
}

void enterDeath(DBloodActor* pSpr)
{
	// don't let the data fields gets overwritten!
	if (!(pSpr->spr.flags & kHitagRespawn))
		DudeToGibCallback1(pSpr);

	pSpr->ChangeType(kThingBloodChunks);
	actPostSprite(pSpr, kStatThing);
}

void enterMorph(DBloodActor* pSpr)
{
	DCustomDude* pDude = cdudeGet(pSpr);
	if (!pDude->IsMorphing())
	{
		pDude->PlaySound(kCdudeSndTransforming);
		pDude->StatusSet(kCdudeStatusMorph); // set morph status
		pSpr->xspr.locked = 1; // lock it while morphing

		pSpr->spr.flags &= ~kPhysMove;
		moveStop(pSpr);
		if (pSpr->xspr.aiState->seqId <= 0)
			seqKill(pSpr);
	}
}

void thinkMorph(DBloodActor* pSpr)
{
	bool triggerOn, triggerOff;
	DCustomDude* pDude = cdudeGet(pSpr);

	if (pDude->SeqPlaying())
	{
		moveStop(pSpr);
		return;
	}
	
	pDude->ClearEffectCallbacks();
	pDude->StatusRem(kCdudeStatusMorph);    // clear morph status
	pSpr->xspr.burnSource = -1;
	pSpr->xspr.burnTime = 0;
	pSpr->xspr.locked = 0;
	pSpr->xspr.scale = 0;

	if (auto ppNext = std::get_if<DBloodActor*>(&pDude->nextDude))
	{
		auto pNext = *ppNext;
		// classic morphing to already inserted sprite by TX ID

		pSpr->xspr.key = pSpr->xspr.dropMsg = 0;

		// save incarnation's going on and off options
		triggerOn = pNext->xspr.triggerOn, triggerOff = pNext->xspr.triggerOff;

		// then remove it from incarnation so it won't send the commands
		pNext->xspr.triggerOn = pNext->xspr.triggerOff = 0;

		// trigger dude death before morphing
		trTriggerSprite(pSpr, kCmdOff, pSpr);

		pSpr->ChangeType(pSpr->spr.inittype = pNext->GetType());
		pSpr->spr.flags = pNext->spr.flags;
		pSpr->spr.pal = pNext->spr.pal;
		pSpr->spr.shade = pNext->spr.shade;
		pSpr->clipdist = pNext->clipdist;
		pSpr->spr.scale = pNext->spr.scale;

		pSpr->xspr.txID = pNext->xspr.txID;
		pSpr->xspr.command = pNext->xspr.command;
		pSpr->xspr.triggerOn = triggerOn;
		pSpr->xspr.triggerOff = triggerOff;
		pSpr->xspr.busyTime = pNext->xspr.busyTime;
		pSpr->xspr.waitTime = pNext->xspr.waitTime;

		// inherit respawn properties
		pSpr->xspr.respawn = pNext->xspr.respawn;
		pSpr->xspr.respawnPending = pNext->xspr.respawnPending;

		pSpr->xspr.data1    = pNext->xspr.data1;                        // for v1 this is weapon id, v2 - descriptor id
		pSpr->xspr.data2    = pNext->xspr.data2;                        // for v1 this is seqBase id
		pSpr->xspr.data3    = pSpr->xspr.sysData1 = pNext->xspr.sysData1;   // for v1 this is soundBase id
		pSpr->xspr.data4    = pSpr->xspr.sysData2 = pNext->xspr.sysData2;   // start hp

		// inherit dude flags
		pSpr->xspr.dudeGuard = pNext->xspr.dudeGuard;
		pSpr->xspr.dudeDeaf = pNext->xspr.dudeDeaf;
		pSpr->xspr.dudeAmbush = pNext->xspr.dudeAmbush;
		pSpr->xspr.dudeFlag4 = pNext->xspr.dudeFlag4;
		pSpr->xspr.modernFlags = pNext->xspr.modernFlags;

		pSpr->xspr.dropMsg = pNext->xspr.dropMsg;
		pSpr->xspr.key = pNext->xspr.key;

		pSpr->xspr.Decoupled = pNext->xspr.Decoupled;
		pSpr->xspr.locked = pNext->xspr.locked;

		// set health
		pSpr->xspr.health = nnExtDudeStartHealth(pSpr, pSpr->xspr.data4);

		// restore values for incarnation
		pNext->xspr.triggerOn = triggerOn;
		pNext->xspr.triggerOff = triggerOff;
	}
	else
	{
		if (auto ppClass = std::get_if<PClass*>(&pDude->nextDude))
		{
			// morph to some vanilla dude
			pSpr->ChangeType(*ppClass);
			pSpr->clipdist = getDudeInfo(pSpr)->clipdist * CLIPDIST_SCALE;
			pSpr->xspr.data1 = 0;
		}
		// v2 morphing
		else if (auto pExt = std::get_if<int>(&pDude->nextDude))
		{
			// morph to another custom dude
			pSpr->xspr.data1 = *pExt;
		}

		pSpr->spr.inittype  = pSpr->GetType();
		pSpr->xspr.health   = nnExtDudeStartHealth(pSpr, 0);
		pSpr->xspr.data4    = pSpr->xspr.sysData2 = 0;
		pSpr->xspr.data2    = 0;
		pSpr->xspr.data3    = 0;
	}

	// clear init status
	pDude->initialized = 0;

	DBloodActor* pTarget = pSpr->xspr.target;        // save target
	aiInitSprite(pSpr);             // re-init sprite with all new settings

	switch (pSpr->GetType())
	{
		case kDudePodMother:        // fake dude
		case kDudeTentacleMother:   // fake dude
			break;
		default:
			if (pSpr->xspr.dudeFlag4) break;
			else if (pTarget) aiSetTarget(pSpr, pTarget); // try to restore target
			else aiSetTarget(pSpr, pSpr->spr.pos);
			aiActivateDude(pSpr); // finally activate it
			break;
	}
}

// get closest visible underwater sector it can fall in
void enterBurnSearchWater(DBloodActor* pSpr)
{
	double nClosest = FLT_MAX;
	
	auto p1 = pSpr->spr.pos.XY();
	double z1, z2;
	
	// this originally edited the state function, which is unsafe.
	pSpr->chasehackflag = false;
	if (!Chance(0x8000))
	{
		pSpr->chasehackflag = true; // try follow to the target
		return;
	}

	GetActorExtents(pSpr, &z1, &z2);

	for(int i = sector.SSize() - 1; i >= 0; i--)
	{
		auto sect = &sector[i];
		if (sect->upperLink == nullptr)
			continue;

		DBloodActor* pUp = barrier_cast<DBloodActor*>(sect->upperLink);
		DBloodActor* pLow = pUp->ownerActor;
		if (pLow && IsUnderwaterSector(pLow->sector()))
		{
			for(auto& wal : sect->walls)
			{

				if (!cansee(DVector3(p1, z1), pSpr->sector(), DVector3(wal.center(), z1), sect))
					continue;

				double sqDist = SquareDistToWall(p1.X, p1.Y, &wal);
				if (sqDist < nClosest)
				{
					pSpr->xspr.goalAng = (wal.center() - p1).Angle();
					nClosest = sqDist;
				}
			}
		}
	}

	if (Chance(0xB000) && pSpr->xspr.target)
	{
		DBloodActor* pTarget = pSpr->xspr.target;
		auto dv = (p1 - pTarget->spr.pos.XY());
		if (dv.LengthSquared() < nClosest)  // water sector is not closer than target
		{
			pSpr->xspr.goalAng = dv.Angle();
			pSpr->chasehackflag = true;
			return;
		}
	}
}

void cdudeDoExplosion(DCustomDude* pDude)
{
	static DVector3 nulvec;
	CUSTOMDUDE_WEAPON* pWeap = pDude->pWeapon;
	if (pWeap && pWeap->type == kCdudeWeaponKamikaze)
		weaponShotKamikaze(pDude, pWeap, pWeap->shot.offset, nulvec);
}

END_BLD_NS
#endif
