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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build.h"

#include "blood.h"
#include "bloodactor.h"

BEGIN_BLD_NS

void FirePitchfork(int, DBloodPlayer* pPlayer);
void FireSpray(int, DBloodPlayer* pPlayer);
void ThrowCan(int, DBloodPlayer* pPlayer);
void DropCan(int, DBloodPlayer* pPlayer);
void ExplodeCan(int, DBloodPlayer* pPlayer);
void ThrowBundle(int, DBloodPlayer* pPlayer);
void DropBundle(int, DBloodPlayer* pPlayer);
void ExplodeBundle(int, DBloodPlayer* pPlayer);
void ThrowProx(int, DBloodPlayer* pPlayer);
void DropProx(int, DBloodPlayer* pPlayer);
void ThrowRemote(int, DBloodPlayer* pPlayer);
void DropRemote(int, DBloodPlayer* pPlayer);
void FireRemote(int, DBloodPlayer* pPlayer);
void FireShotgun(int nTrigger, DBloodPlayer* pPlayer);
void EjectShell(int, DBloodPlayer* pPlayer);
void FireTommy(int nTrigger, DBloodPlayer* pPlayer);
void FireSpread(int nTrigger, DBloodPlayer* pPlayer);
void AltFireSpread(int nTrigger, DBloodPlayer* pPlayer);
void AltFireSpread2(int nTrigger, DBloodPlayer* pPlayer);
void FireFlare(int nTrigger, DBloodPlayer* pPlayer);
void AltFireFlare(int nTrigger, DBloodPlayer* pPlayer);
void FireVoodoo(int nTrigger, DBloodPlayer* pPlayer);
void AltFireVoodoo(int nTrigger, DBloodPlayer* pPlayer);
void DropVoodoo(int nTrigger, DBloodPlayer* pPlayer);
void FireTesla(int nTrigger, DBloodPlayer* pPlayer);
void AltFireTesla(int nTrigger, DBloodPlayer* pPlayer);
void FireNapalm(int nTrigger, DBloodPlayer* pPlayer);
void FireNapalm2(int nTrigger, DBloodPlayer* pPlayer);
void AltFireNapalm(int nTrigger, DBloodPlayer* pPlayer);
void FireLifeLeech(int nTrigger, DBloodPlayer* pPlayer);
void AltFireLifeLeech(int nTrigger, DBloodPlayer* pPlayer);
void FireBeast(int nTrigger, DBloodPlayer* pPlayer);

typedef void(*QAVTypeCast)(int, void*);

void (*qavClientCallback[])(int, void*) =
{
(QAVTypeCast)FirePitchfork,
(QAVTypeCast)FireSpray,
(QAVTypeCast)ThrowCan,
(QAVTypeCast)DropCan,
(QAVTypeCast)ExplodeCan,
(QAVTypeCast)ThrowBundle,
(QAVTypeCast)DropBundle,
(QAVTypeCast)ExplodeBundle,
(QAVTypeCast)ThrowProx,
(QAVTypeCast)DropProx,
(QAVTypeCast)ThrowRemote,
(QAVTypeCast)DropRemote,
(QAVTypeCast)FireRemote,
(QAVTypeCast)FireShotgun,
(QAVTypeCast)EjectShell,
(QAVTypeCast)FireTommy,
(QAVTypeCast)AltFireSpread2,
(QAVTypeCast)FireSpread,
(QAVTypeCast)AltFireSpread,
(QAVTypeCast)FireFlare,
(QAVTypeCast)AltFireFlare,
(QAVTypeCast)FireVoodoo,
(QAVTypeCast)AltFireVoodoo,
(QAVTypeCast)FireTesla,
(QAVTypeCast)AltFireTesla,
(QAVTypeCast)FireNapalm,
(QAVTypeCast)FireNapalm2,
(QAVTypeCast)FireLifeLeech,
(QAVTypeCast)FireBeast,
(QAVTypeCast)AltFireLifeLeech,
(QAVTypeCast)DropVoodoo,
(QAVTypeCast)AltFireNapalm,
};

enum
{
	nClientFirePitchfork,
	nClientFireSpray,
	nClientThrowCan,
	nClientDropCan,
	nClientExplodeCan,
	nClientThrowBundle,
	nClientDropBundle,
	nClientExplodeBundle,
	nClientThrowProx,
	nClientDropProx,
	nClientThrowRemote,
	nClientDropRemote,
	nClientFireRemote,
	nClientFireShotgun,
	nClientEjectShell,
	nClientFireTommy,
	nClientAltFireSpread2,
	nClientFireSpread,
	nClientAltFireSpread,
	nClientFireFlare,
	nClientAltFireFlare,
	nClientFireVoodoo,
	nClientAltFireVoodoo,
	nClientFireTesla,
	nClientAltFireTesla,
	nClientFireNapalm,
	nClientFireNapalm2,
	nClientFireLifeLeech,
	nClientFireBeast,
	nClientAltFireLifeLeech,
	nClientDropVoodoo,
	nClientAltFireNapalm,
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

double getThrowPower(DBloodPlayer* pPlayer)
{
	return pPlayer->throwPower * 23.46666 + 6.4;
}

void setThrowPower(DBloodPlayer* pPlayer)
{
	pPlayer->throwPower = min((PlayClock - pPlayer->throwTime) / 240., 1.);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool checkLitSprayOrTNT(DBloodPlayer* pPlayer)
{
	switch (pPlayer->curWeapon)
	{
	case kWeapSpraycan:
		switch (pPlayer->weaponState)
		{
		case 5:
		case 6:
			return 1;
		case 7:
			if (VanillaMode())
				return 0;
			return 1;
		}
		break;
	case kWeapDynamite:
		switch (pPlayer->weaponState)
		{
		case 4:
		case 5:
		case 6:
			return 1;
		}
		break;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool BannedUnderwater(int nWeapon)
{
	return nWeapon == kWeapSpraycan || nWeapon == kWeapDynamite;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool CheckWeaponAmmo(DBloodPlayer* pPlayer, int weapon, int ammotype, int count)
{
	if (gInfiniteAmmo)
		return 1;
	if (ammotype == -1)
		return 1;
	if (weapon == kWeapRemote && pPlayer->weaponAmmo == 11 && pPlayer->weaponState == 11)
		return 1;
	if (weapon == kWeapLifeLeech && pPlayer->GetActor()->xspr.health > 0)
		return 1;
	return pPlayer->ammoCount[ammotype] >= count;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool CheckAmmo(DBloodPlayer* pPlayer, int ammotype, int count)
{
	if (gInfiniteAmmo)
		return 1;
	if (ammotype == -1)
		return 1;
	if (pPlayer->curWeapon == kWeapRemote && pPlayer->weaponAmmo == 11 && pPlayer->weaponState == 11)
		return 1;
	if (pPlayer->curWeapon == kWeapLifeLeech && pPlayer->GetActor()->xspr.health >= unsigned(count << 4))
		return 1;
	return pPlayer->ammoCount[ammotype] >= count;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool checkAmmo2(const DBloodPlayer* pPlayer, int ammotype, int amount)
{
	if (gInfiniteAmmo)
		return 1;
	if (ammotype == -1)
		return 1;
	return pPlayer->ammoCount[ammotype] >= amount;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SpawnBulletEject(DBloodPlayer* pPlayer, double dist, int rdist)
{
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
	pPlayer->zView = pPlayer->GetActor()->spr.pos.Z - pPosture->eyeAboveZ;
	double dz = pPlayer->zWeapon - (pPlayer->zWeapon - pPlayer->zView) * 0.5;
	fxSpawnEjectingBrass(pPlayer->GetActor(), dz, dist, rdist);
}

void SpawnShellEject(DBloodPlayer* pPlayer, double dist, int rdist)
{
	POSTURE* pPosture = &pPlayer->pPosture[pPlayer->lifeMode][pPlayer->posture];
	pPlayer->zView = pPlayer->GetActor()->spr.pos.Z - pPosture->eyeAboveZ;
	double t = pPlayer->zWeapon - pPlayer->zView;
	double dz = pPlayer->zWeapon - t + (t * 0.25);
	fxSpawnEjectingShell(pPlayer->GetActor(), dz, dist, rdist);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponInit(void)
{
	auto doInit = [](const int base)
	{
		for (int i = base; i < (kQAVEnd + base); i++)
		{
			auto pQAV = getQAV(i);
			if (!pQAV)
				I_Error("Could not load QAV %d\n", i);
		}
	};

	doInit(0);
	doInit(10000);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponPrecache()
{
	auto doPrecache = [](const int base)
	{
		for (int i = base; i < (kQAVEnd + base); i++)
		{
			auto pQAV = getQAV(i);
			if (pQAV)
				pQAV->Precache();
		}
	};

	doPrecache(0);
	doPrecache(10000);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponDraw(DBloodPlayer* pPlayer, int shade, double xpos, double ypos, int palnum, DAngle angle)
{
	assert(pPlayer != NULL);
	if (pPlayer->weaponQav == kQAVNone)
		return;
	auto pQAV = getQAV(pPlayer->weaponQav);
	int duration;
	double interpfrac;

	qavProcessTimer(pPlayer, pQAV, &duration, &interpfrac, pPlayer->weaponState == -1, pPlayer->curWeapon == kWeapShotgun && pPlayer->weaponState == 7);

	pQAV->x = xpos;
	pQAV->y = ypos;
	int flags = 2;
	int nInv = powerupCheck(pPlayer, kPwUpShadowCloak);
	if (nInv >= 120 * 8 || (nInv != 0 && (PlayClock & 32)))
	{
		shade = -128;
		flags |= 1;
	}
	pQAV->Draw(duration, flags, shade, palnum, true, interpfrac, angle);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponPlay(DBloodPlayer* pPlayer)
{
	assert(pPlayer != NULL);
	if (pPlayer->weaponQav == kQAVNone)
		return;
	auto pQAV = getQAV(pPlayer->weaponQav);
	int nTicks = pQAV->duration - pPlayer->weaponTimer;
	pQAV->Play(nTicks - 4, nTicks, pPlayer->qavCallback, pPlayer);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void StartQAV(DBloodPlayer* pPlayer, int nWeaponQAV, int callback = -1, bool looped = false)
{
	assert(nWeaponQAV < kQAVEnd);
	auto res_id = qavGetCorrectID(nWeaponQAV);
	auto pQAV = getQAV(res_id);
	pPlayer->weaponQav = res_id;
	pPlayer->weaponTimer = pQAV->duration;
	pPlayer->qavCallback = callback;
	pPlayer->qavLoop = looped;
	pPlayer->qavLastTick = I_GetTime(pQAV->ticrate);
	pPlayer->qavTimer = pQAV->duration;
	//pQAV->Preload();
	WeaponPlay(pPlayer);
	pPlayer->weaponTimer -= 4;
}

static void SetQAV(DBloodPlayer* pPlayer, int nWeaponQAV)
{
	assert(nWeaponQAV < kQAVEnd);
	pPlayer->weaponQav = qavGetCorrectID(nWeaponQAV);
	pPlayer->qavTimer = 0;
	pPlayer->qavLastTick = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

struct WEAPONTRACK
{
	int aimSpeedHorz;
	int aimSpeedVert;
	int angleRange;
	int thingAngle;
	int seeker;
	bool bIsProjectile;
};

WEAPONTRACK gWeaponTrack[] = {
	{ 0, 0, 0, 0, 0, false },
	{ 0x6000, 0x6000, 0x71, 0x55, 0x111111, false },
	{ 0x8000, 0x8000, 0x71, 0x55, 0x2aaaaa, true },
	{ 0x10000, 0x10000, 0x38, 0x1c, 0, false },
	{ 0x6000, 0x8000, 0x38, 0x1c, 0, false },
	{ 0x6000, 0x6000, 0x38, 0x1c, 0x2aaaaa, true },
	{ 0x6000, 0x6000, 0x71, 0x55, 0, true },
	{ 0x6000, 0x6000, 0x71, 0x38, 0, true },
	{ 0x8000, 0x10000, 0x71, 0x55, 0x255555, true },
	{ 0x10000, 0x10000, 0x71, 0, 0, true },
	{ 0x10000, 0x10000, 0xaa, 0, 0, false },
	{ 0x6000, 0x6000, 0x71, 0x55, 0, true },
	{ 0x6000, 0x6000, 0x71, 0x55, 0, true },
	{ 0x6000, 0x6000, 0x71, 0x55, 0, false },
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateAimVector(DBloodPlayer* pPlayer)
{
	assert(pPlayer != NULL);
	auto plActor = pPlayer->GetActor();
	DVector3 pos(plActor->spr.pos.XY(), pPlayer->zWeapon);
	DVector3 Aim(plActor->spr.Angles.Yaw.ToVector(), pPlayer->slope);

	WEAPONTRACK* pWeaponTrack = &gWeaponTrack[pPlayer->curWeapon];
	DBloodActor* targetactor = nullptr;
	pPlayer->aimTargetsCount = 0;
	int autoaim = Autoaim(pPlayer->pnum);
	if (autoaim == 1 || (autoaim == 2 && !pWeaponTrack->bIsProjectile) || pPlayer->curWeapon == kWeapVoodooDoll || pPlayer->curWeapon == kWeapLifeLeech)
	{
		double nClosest = 0x7fffffff;
		BloodStatIterator it(kStatDude);
		while (auto actor = it.Next())
		{
			if (plActor == actor)
				continue;
			if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, actor))
				continue;
			if (actor->spr.flags & 32)
				continue;
			if (!(actor->spr.flags & 8))
				continue;

			auto pos2 = actor->spr.pos;
			double nDist = (pos2.XY() - pos.XY()).Length();
			if (nDist == 0 || nDist > 3200)
				continue;

			if (pWeaponTrack->seeker)
			{
				double t = nDist * 4096 / pWeaponTrack->seeker;
				pos2 += actor->vel * t;
			}
			DVector3 lpos = pos + DVector3(plActor->spr.Angles.Yaw.ToVector(), pPlayer->slope) * nDist;

			double zRange = nDist * (9460 / 16384.);
			double top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (lpos.Z - zRange > bottom || lpos.Z + zRange < top)
				continue;

			DAngle angle = (pos2 - pos).Angle();
			DAngle deltaangle = absangle(angle, plActor->spr.Angles.Yaw);
			if (deltaangle > DAngle::fromBuild(pWeaponTrack->angleRange))
				continue;
			if (pPlayer->aimTargetsCount < 16 && cansee(pos, plActor->sector(), pos2, actor->sector()))
				pPlayer->aimTargets[pPlayer->aimTargetsCount++] = actor;

			double nDist2 = (lpos - pos2).Length();
			if (nDist2 >= nClosest)
				continue;

			if (cansee(pos, plActor->sector(), pos2, actor->sector()))
			{
				double center = (actor->spr.scale.Y * actor->aimHeight());
				double dzCenter = (pos2.Z - center) - pos.Z;

				nClosest = nDist2;
				Aim.XY() = angle.ToVector();
				Aim.Z = dzCenter / nDist;
				targetactor = actor;
			}
		}
		if (pWeaponTrack->thingAngle > 0)
		{
			BloodStatIterator itr(kStatThing);
			while (auto actor = itr.Next())
			{
				if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, actor))
					continue;
				if (!(actor->spr.flags & 8))
					continue;
				auto pos2 = actor->spr.pos;
				auto dv = pos2 - pos;

				double nDist = dv.Length();
				if (nDist == 0 || nDist > 3200)
					continue;

				DVector3 lpos = pos + DVector3(plActor->spr.Angles.Yaw.ToVector(), pPlayer->slope) * nDist;
				double zRange = nDist * (9460 / 16384.);

				double top, bottom;
				GetActorExtents(actor, &top, &bottom);
				if (lpos.Z - zRange > bottom || lpos.Z + zRange < top)
					continue;

				DAngle angle = dv.Angle();
				DAngle deltaangle = absangle(angle, plActor->spr.Angles.Yaw);
				if (deltaangle > DAngle::fromBuild(pWeaponTrack->thingAngle))
					continue;

				if (pPlayer->aimTargetsCount < 16 && cansee(pos, plActor->sector(), pos2, actor->sector()))
					pPlayer->aimTargets[pPlayer->aimTargetsCount++] = actor;

				double nDist2 = (lpos - pos2).Length();
				if (nDist2 >= nClosest)
					continue;
				if (cansee(pos, plActor->sector(), pos2, actor->sector()))
				{
					nClosest = nDist2;
					Aim.XY() = angle.ToVector();
					Aim.Z = dv.Z / nDist;
					targetactor = actor;
				}
			}
		}
	}
	DVector3 Aim2(Aim);
	Aim2.XY() = Aim2.XY().Rotated(-plActor->spr.Angles.Yaw);
	Aim2.Z -= pPlayer->slope;

	pPlayer->relAim.X = interpolatedvalue(pPlayer->relAim.X, Aim2.X, FixedToFloat(pWeaponTrack->aimSpeedHorz));
	pPlayer->relAim.Y = interpolatedvalue(pPlayer->relAim.Y, Aim2.Y, FixedToFloat(pWeaponTrack->aimSpeedHorz));
	pPlayer->relAim.Z = interpolatedvalue(pPlayer->relAim.Z, Aim2.Z, FixedToFloat(pWeaponTrack->aimSpeedVert));
	pPlayer->aim = pPlayer->relAim;
	pPlayer->aim.XY() = pPlayer->aim.XY().Rotated(plActor->spr.Angles.Yaw);
	pPlayer->aim.Z += pPlayer->slope;
	pPlayer->aimTarget = targetactor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

struct t_WeaponModes
{
	int update;
	int ammoType;
};

t_WeaponModes weaponModes[] = {
	{ 0, -1 },
	{ 1, -1 },
	{ 1, 1 },
	{ 1, 2 },
	{ 1, 3 },
	{ 1, 4 },
	{ 1, 5 },
	{ 1, 6 },
	{ 1, 7 },
	{ 1, 8 },
	{ 1, 9 },
	{ 1, 10 },
	{ 1, 11 },
	{ 0, -1 },
};

void WeaponRaise(DBloodPlayer* pPlayer)
{
	assert(pPlayer != NULL);
	int prevWeapon = pPlayer->curWeapon;
	pPlayer->curWeapon = pPlayer->newWeapon;
	pPlayer->newWeapon = kWeapNone;
	pPlayer->weaponAmmo = weaponModes[pPlayer->curWeapon].ammoType;
	switch (pPlayer->curWeapon)
	{
	case kWeapPitchFork:
		pPlayer->weaponState = 0;
		StartQAV(pPlayer, kQAVFORKUP);
		break;
	case kWeapSpraycan:
		if (pPlayer->weaponState == 2)
		{
			pPlayer->weaponState = 3;
			StartQAV(pPlayer, kQAVCANPREF);
		}
		else
		{
			pPlayer->weaponState = 0;
			StartQAV(pPlayer, kQAVLITEOPEN);
		}
		break;
	case kWeapDynamite:
		if (gInfiniteAmmo || checkAmmo2(pPlayer, 5, 1))
		{
			pPlayer->weaponState = 3;
			if (prevWeapon == kWeapSpraycan)
				StartQAV(pPlayer, kQAVBUNUP);
			else
				StartQAV(pPlayer, kQAVBUNUP2);
		}
		break;
	case kWeapProximity:
		if (gInfiniteAmmo || checkAmmo2(pPlayer, 10, 1))
		{
			pPlayer->weaponState = 7;
			StartQAV(pPlayer, kQAVPROXUP);
		}
		break;
	case kWeapRemote:
		if (gInfiniteAmmo || checkAmmo2(pPlayer, 11, 1))
		{
			pPlayer->weaponState = 10;
			StartQAV(pPlayer, kQAVREMUP2);
		}
		else
		{
			StartQAV(pPlayer, kQAVREMUP3);
			pPlayer->weaponState = 11;
		}
		break;
	case kWeapShotgun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns))
		{
			if (gInfiniteAmmo || pPlayer->ammoCount[2] >= 4)
				StartQAV(pPlayer, kQAV2SHOTUP);
			else
				StartQAV(pPlayer, kQAVSHOTUP);
			if (gInfiniteAmmo || pPlayer->ammoCount[2] >= 4)
				pPlayer->weaponState = 7;
			else if (pPlayer->ammoCount[2] > 1)
				pPlayer->weaponState = 3;
			else if (pPlayer->ammoCount[2] > 0)
				pPlayer->weaponState = 2;
			else
				pPlayer->weaponState = 1;
		}
		else
		{
			if (gInfiniteAmmo || pPlayer->ammoCount[2] > 1)
				pPlayer->weaponState = 3;
			else if (pPlayer->ammoCount[2] > 0)
				pPlayer->weaponState = 2;
			else
				pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVSHOTUP);
		}
		break;
	case kWeapTommyGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 3, 2))
		{
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAV2TOMUP);
		}
		else
		{
			pPlayer->weaponState = 0;
			StartQAV(pPlayer, kQAVTOMUP);
		}
		break;
	case kWeapVoodooDoll:
		if (gInfiniteAmmo || checkAmmo2(pPlayer, 9, 1))
		{
			pPlayer->weaponState = 2;
			StartQAV(pPlayer, kQAVVDUP);
		}
		break;
	case kWeapFlareGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 1, 2))
		{
			StartQAV(pPlayer, kQAVFLAR2UP);
			pPlayer->weaponState = 3;
		}
		else
		{
			StartQAV(pPlayer, kQAVFLARUP);
			pPlayer->weaponState = 2;
		}
		break;
	case kWeapTeslaCannon:
		if (checkAmmo2(pPlayer, 7, 1))
		{
			pPlayer->weaponState = 2;
			if (powerupCheck(pPlayer, kPwUpTwoGuns))
				StartQAV(pPlayer, kQAV2SGUNUP);
			else
				StartQAV(pPlayer, kQAVSGUNUP);
		}
		else
		{
			pPlayer->weaponState = 3;
			StartQAV(pPlayer, kQAVSGUNUP);
		}
		break;
	case kWeapNapalm:
		if (powerupCheck(pPlayer, kPwUpTwoGuns))
		{
			StartQAV(pPlayer, kQAV2NAPUP);
			pPlayer->weaponState = 3;
		}
		else
		{
			StartQAV(pPlayer, kQAVNAPUP);
			pPlayer->weaponState = 2;
		}
		break;
	case kWeapLifeLeech:
		pPlayer->weaponState = 2;
		StartQAV(pPlayer, kQAVSTAFUP);
		break;
	case kWeapBeast:
		pPlayer->weaponState = 2;
		StartQAV(pPlayer, kQAVBSTUP);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponLower(DBloodPlayer* pPlayer)
{
	assert(pPlayer != NULL);
	if (checkLitSprayOrTNT(pPlayer))
		return;
	pPlayer->throwPower = 0;
	int prevState = pPlayer->weaponState;
	switch (pPlayer->curWeapon)
	{
	case kWeapPitchFork:
		StartQAV(pPlayer, kQAVFORKDOWN);
		break;
	case kWeapSpraycan:
		sfxKill3DSound(pPlayer->GetActor(), -1, 441);
		switch (prevState)
		{
		case 1:
			if (VanillaMode())
			{
				StartQAV(pPlayer, kQAVLITECLO2);
			}
			else
			{
				if (pPlayer->newWeapon == kWeapDynamite) // do not put away lighter if TNT was selected while throwing a spray can
				{
					pPlayer->weaponState = 2;
					StartQAV(pPlayer, kQAVCANDOWN);
					WeaponRaise(pPlayer);
					return;
				}
			}
			break;
		case 2:
			pPlayer->weaponState = 1;
			WeaponRaise(pPlayer);
			return;
		case 4:
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVCANDOWN);
			if (VanillaMode())
			{
				pPlayer->newWeapon = kWeapNone;
				WeaponLower(pPlayer);
			}
			else
			{
				if (pPlayer->newWeapon == kWeapDynamite)
				{
					pPlayer->weaponState = 2;
					StartQAV(pPlayer, kQAVCANDOWN);
					return;
				}
				else
				{
					WeaponLower(pPlayer);
				}
			}
			break;
		case 3:
			if (pPlayer->newWeapon == kWeapDynamite)
			{
				pPlayer->weaponState = 2;
				StartQAV(pPlayer, kQAVCANDOWN);
				return;
			}
			else if (pPlayer->newWeapon == kWeapSpraycan)
			{
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVCANDOWN);
				pPlayer->newWeapon = kWeapNone;
				WeaponLower(pPlayer);
			}
			else
			{
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVCANDOWN);
			}
			break;
		case 7: // throwing ignited alt fire spray
			if (VanillaMode() || (pPlayer->newWeapon != 0))
				break;
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVCANDOWN);
			break;
		}
		break;
	case kWeapDynamite:
		switch (prevState)
		{
		case 1:
			if (VanillaMode())
			{
				StartQAV(pPlayer, kQAVLITECLO2);
			}
			else
			{
				if (pPlayer->newWeapon == kWeapSpraycan) // do not put away lighter if TNT was selected while throwing a spray can
				{
					pPlayer->weaponState = 2;
					StartQAV(pPlayer, kQAVCANDOWN);
					WeaponRaise(pPlayer);
					return;
				}
			}
			break;
		case 2:
			WeaponRaise(pPlayer);
			break;
		case 3:
			if (pPlayer->newWeapon == kWeapSpraycan)
			{
				pPlayer->weaponState = 2;
				StartQAV(pPlayer, kQAVBUNDOWN);
			}
			else
			{
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVBUNDOWN2);
			}
			break;
		default:
			break;
		}
		break;
	case kWeapProximity:
		switch (prevState)
		{
		case 7:
			StartQAV(pPlayer, kQAVPROXDOWN);
			break;
		}
		break;
	case kWeapRemote:
		switch (prevState)
		{
		case 10:
			StartQAV(pPlayer, kQAVREMDOWN2);
			break;
		case 11:
			StartQAV(pPlayer, kQAVREMDOWN3);
			break;
		}
		break;
	case kWeapShotgun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns))
			StartQAV(pPlayer, kQAV2SHOTDWN);
		else
			StartQAV(pPlayer, kQAVSHOTDOWN);
		break;
	case kWeapTommyGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns) && pPlayer->weaponState == 1)
			StartQAV(pPlayer, kQAV2TOMDOWN);
		else
			StartQAV(pPlayer, kQAVTOMDOWN);
		break;
	case kWeapFlareGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns) && pPlayer->weaponState == 3)
			StartQAV(pPlayer, kQAVFLAR2DWN);
		else
			StartQAV(pPlayer, kQAVFLARDOWN);
		break;
	case kWeapVoodooDoll:
		StartQAV(pPlayer, kQAVVDDOWN);
		break;
	case kWeapTeslaCannon:
		if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
			StartQAV(pPlayer, kQAV2SGUNDWN);
		else
			StartQAV(pPlayer, kQAVSGUNDOWN);
		break;
	case kWeapNapalm:
		if (powerupCheck(pPlayer, kPwUpTwoGuns))
			StartQAV(pPlayer, kQAV2NAPDOWN);
		else
			StartQAV(pPlayer, kQAVNAPDOWN);
		break;
	case kWeapLifeLeech:
		StartQAV(pPlayer, kQAVSTAFDOWN);
		break;
	case kWeapBeast:
		StartQAV(pPlayer, kQAVBSTDOWN);
		break;
	}
	pPlayer->curWeapon = kWeapNone;
	pPlayer->qavLoop = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponUpdateState(DBloodPlayer* pPlayer)
{
	static int lastWeapon = 0;
	static int lastState = 0;
	int va = pPlayer->curWeapon;
	int vb = pPlayer->weaponState;
	if (va != lastWeapon || vb != lastState)
	{
		lastWeapon = va;
		lastState = vb;
	}
	switch (lastWeapon)
	{
	case kWeapPitchFork:
		SetQAV(pPlayer, kQAVFORKIDLE);
		break;
	case kWeapSpraycan:
		switch (vb)
		{
		case 0:
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVLITEFLAM);
			break;
		case 1:
			if (CheckAmmo(pPlayer, 6, 1))
			{
				pPlayer->weaponState = 3;
				StartQAV(pPlayer, kQAVCANPREF);
			}
			else
				SetQAV(pPlayer, kQAVLITEIDLE);
			break;
		case 3:
			SetQAV(pPlayer, kQAVCANIDLE);
			break;
		case 4:
			if (CheckAmmo(pPlayer, 6, 1))
			{
				SetQAV(pPlayer, kQAVCANIDLE);
				pPlayer->weaponState = 3;
			}
			else
			{
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVCANDOWN);
			}
			sfxKill3DSound(pPlayer->GetActor(), -1, 441);
			break;
		}
		break;
	case kWeapDynamite:
		switch (vb)
		{
		case 1:
			if (pPlayer->weaponAmmo == 5 && CheckAmmo(pPlayer, 5, 1))
			{
				pPlayer->weaponState = 3;
				StartQAV(pPlayer, kQAVBUNUP);
			}
			break;
		case 0:
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVLITEFLAM);
			break;
		case 2:
			if (pPlayer->ammoCount[5] > 0)
			{
				pPlayer->weaponState = 3;
				StartQAV(pPlayer, kQAVBUNUP);
			}
			else
				SetQAV(pPlayer, kQAVLITEIDLE);
			break;
		case 3:
			SetQAV(pPlayer, kQAVBUNIDLE);
			break;
		}
		break;
	case kWeapProximity:
		switch (vb)
		{
		case 7:
			SetQAV(pPlayer, kQAVPROXIDLE);
			break;
		case 8:
			pPlayer->weaponState = 7;
			StartQAV(pPlayer, kQAVPROXUP);
			break;
		}
		break;
	case kWeapRemote:
		switch (vb)
		{
		case 10:
			SetQAV(pPlayer, kQAVREMIDLE1);
			break;
		case 11:
			SetQAV(pPlayer, kQAVREMIDLE2);
			break;
		case 12:
			if (pPlayer->ammoCount[11] > 0)
			{
				pPlayer->weaponState = 10;
				StartQAV(pPlayer, kQAVREMUP2);
			}
			else
				pPlayer->weaponState = -1;
			break;
		}
		break;
	case kWeapShotgun:
		switch (vb)
		{
		case 6:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && (gInfiniteAmmo || CheckAmmo(pPlayer, 2, 4)))
				pPlayer->weaponState = 7;
			else
				pPlayer->weaponState = 1;
			break;
		case 7:
			SetQAV(pPlayer, kQAV2SHOTI);
			break;
		case 1:
			if (CheckAmmo(pPlayer, 2, 1))
			{
				sfxPlay3DSound(pPlayer->GetActor(), 410, 3, 2);
				StartQAV(pPlayer, kQAVSHOTL1, nClientEjectShell);
				if (gInfiniteAmmo || pPlayer->ammoCount[2] > 1)
					pPlayer->weaponState = 3;
				else
					pPlayer->weaponState = 2;
			}
			else
				SetQAV(pPlayer, kQAVSHOTI3);
			break;
		case 2:
			SetQAV(pPlayer, kQAVSHOTI2);
			break;
		case 3:
			SetQAV(pPlayer, kQAVSHOTI1);
			break;
		}
		break;
	case kWeapTommyGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 3, 2))
		{
			SetQAV(pPlayer, kQAV2TOMIDLE);
			pPlayer->weaponState = 1;
		}
		else
		{
			SetQAV(pPlayer, kQAVTOMIDLE);
			pPlayer->weaponState = 0;
		}
		break;
	case kWeapFlareGun:
		if (powerupCheck(pPlayer, kPwUpTwoGuns))
		{
			if (vb == 3 && checkAmmo2(pPlayer, 1, 2))
				SetQAV(pPlayer, kQAVFLAR2I);
			else
			{
				SetQAV(pPlayer, kQAVFLARIDLE);
				pPlayer->weaponState = 2;
			}
		}
		else
			SetQAV(pPlayer, kQAVFLARIDLE);
		break;
	case kWeapVoodooDoll:
		if (pPlayer->GetActor()->xspr.height < 256 && pPlayer->swayHeight != 0)
			StartQAV(pPlayer, kQAVVDIDLE2);
		else
			SetQAV(pPlayer, kQAVVDIDLE1);
		break;
	case kWeapTeslaCannon:
		switch (vb)
		{
		case 2:
			if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
				SetQAV(pPlayer, kQAV2SGUNIDL);
			else
				SetQAV(pPlayer, kQAVSGUNIDL1);
			break;
		case 3:
			SetQAV(pPlayer, kQAVSGUNIDL2);
			break;
		}
		break;
	case kWeapNapalm:
		switch (vb)
		{
		case 3:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && (gInfiniteAmmo || CheckAmmo(pPlayer, 4, 4)))
				SetQAV(pPlayer, kQAV2NAPIDLE);
			else
				SetQAV(pPlayer, kQAVNAPIDLE);
			break;
		case 2:
			SetQAV(pPlayer, kQAVNAPIDLE);
			break;
		}
		break;
	case kWeapLifeLeech:
		switch (vb)
		{
		case 2:
			SetQAV(pPlayer, kQAVSTAFIDL1);
			break;
		}
		break;
	case kWeapBeast:
		SetQAV(pPlayer, kQAVBSTIDLE);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FirePitchfork(int, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	double r1 = Random2F(2000, 14);
	double r2 = Random2F(2000, 14);
	double r3 = Random2F(2000, 14);
	for (int i = 0; i < 4; i++)
		actFireVector(actor, (2 * i - 3) * 2.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r1, r2, r3), kVectorTine);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireSpray(int, DBloodPlayer* pPlayer)
{
	playerFireMissile(pPlayer, 0, pPlayer->aim, kMissileFlameSpray);
	UseAmmo(pPlayer, 6, 4);
	if (CheckAmmo(pPlayer, 6, 1))
		sfxPlay3DSound(pPlayer->GetActor(), 441, 1, 2);
	else
		sfxKill3DSound(pPlayer->GetActor(), -1, 441);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowCan(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), -1, 441);
	double nSpeed = getThrowPower(pPlayer);
	sfxPlay3DSound(pPlayer->GetActor(), 455, 1, 0);
	auto spawned = playerFireThing(pPlayer, 0, -9460 / 65536., kThingArmedSpray, nSpeed);
	if (spawned)
	{
		sfxPlay3DSound(spawned, 441, 0, 0);
		spawned->spr.shade = -128;
		evPostActor(spawned, pPlayer->fuseTime, kCmdOn, pPlayer->GetActor());
		spawned->xspr.Impact = 1;
		UseAmmo(pPlayer, 6, 480/*gAmmoItemData[0].count*/);
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropCan(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), -1, 441);
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedSpray, 0);
	if (spawned)
	{
		evPostActor(spawned, pPlayer->fuseTime, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 6, 480/*gAmmoItemData[0].count*/);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ExplodeCan(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), -1, 441);
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedSpray, 0);
	if (spawned)
	{
		evPostActor(spawned, 0, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 6, 480/*gAmmoItemData[0].count*/);
		StartQAV(pPlayer, kQAVCANBOOM);
		pPlayer->curWeapon = kWeapNone;
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowBundle(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), 16, -1);
	double nSpeed = getThrowPower(pPlayer);
	sfxPlay3DSound(pPlayer->GetActor(), 455, 1, 0);
	auto spawned = playerFireThing(pPlayer, 0, -9460 / 65536., kThingArmedTNTBundle, nSpeed);
	if (spawned)
	{
		if (pPlayer->fuseTime < 0)
			spawned->xspr.Impact = 1;
		else
			evPostActor(spawned, pPlayer->fuseTime, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 5, 1);
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropBundle(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), 16, -1);
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedTNTBundle, 0);
	if (spawned)
	{
		evPostActor(spawned, pPlayer->fuseTime, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 5, 1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ExplodeBundle(int, DBloodPlayer* pPlayer)
{
	sfxKill3DSound(pPlayer->GetActor(), 16, -1);
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedTNTBundle, 0);
	if (spawned)
	{
		evPostActor(spawned, 0, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 5, 1);
		StartQAV(pPlayer, kQAVDYNEXPLO);
		pPlayer->curWeapon = kWeapNone;
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowProx(int, DBloodPlayer* pPlayer)
{
	double nSpeed = getThrowPower(pPlayer);
	sfxPlay3DSound(pPlayer->GetActor(), 455, 1, 0);
	auto spawned = playerFireThing(pPlayer, 0, -9460 / 65536., kThingArmedProxBomb, nSpeed);
	if (spawned)
	{
		evPostActor(spawned, 240, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 10, 1);
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropProx(int, DBloodPlayer* pPlayer)
{
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedProxBomb, 0);
	if (spawned)
	{
		evPostActor(spawned, 240, kCmdOn, pPlayer->GetActor());
		UseAmmo(pPlayer, 10, 1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ThrowRemote(int, DBloodPlayer* pPlayer)
{
	double nSpeed = getThrowPower(pPlayer);
	sfxPlay3DSound(pPlayer->GetActor(), 455, 1, 0);
	auto spawned = playerFireThing(pPlayer, 0, -9460 / 65536., kThingArmedRemoteBomb, nSpeed);
	if (spawned)
	{
		spawned->xspr.rxID = 90 + (pPlayer->GetActor()->GetType() - kDudePlayer1);
		UseAmmo(pPlayer, 11, 1);
		pPlayer->throwPower = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropRemote(int, DBloodPlayer* pPlayer)
{
	auto spawned = playerFireThing(pPlayer, 0, 0, kThingArmedRemoteBomb, 0);
	if (spawned)
	{
		spawned->xspr.rxID = 90 + (pPlayer->GetActor()->GetType() - kDudePlayer1);
		UseAmmo(pPlayer, 11, 1);
	}
}

void FireRemote(int, DBloodPlayer* pPlayer)
{
	evSendGame(90 + (pPlayer->GetActor()->GetType() - kDudePlayer1), kCmdOn);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

enum { kMaxShotgunBarrels = 4 };

void FireShotgun(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	assert(nTrigger > 0 && nTrigger <= kMaxShotgunBarrels);
	if (nTrigger == 1)
	{
		sfxPlay3DSound(pPlayer->GetActor(), 411, 2, 0);
		pPlayer->tiltEffect = 30;
		pPlayer->visibility = 20;
	}
	else
	{
		sfxPlay3DSound(pPlayer->GetActor(), 412, 2, 0);
		pPlayer->tiltEffect = 50;
		pPlayer->visibility = 40;
	}
	int n = nTrigger << 4;
	for (int i = 0; i < n; i++)
	{
		double r1, r2, r3;
		VECTOR_TYPE nType;
		if (nTrigger == 1)
		{
			r1 = Random3F(1500, 14);
			r2 = Random3F(1500, 14);
			r3 = Random3F(500, 14);
			nType = kVectorShell;
		}
		else
		{
			r1 = Random3F(2500, 14);
			r2 = Random3F(2500, 14);
			r3 = Random3F(1500, 14);
			nType = kVectorShellAP;
		}
		actFireVector(actor, 0, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r1, r2, r3), nType);
	}
	UseAmmo(pPlayer, pPlayer->weaponAmmo, nTrigger);
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void EjectShell(int, DBloodPlayer* pPlayer)
{
	SpawnShellEject(pPlayer, 25 / 16., 35);
	SpawnShellEject(pPlayer, 3, 35);
}

void FireTommy(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	sfxPlay3DSound(pPlayer->GetActor(), 431, -1, 0);
	switch (nTrigger)
	{
	case 1:
	{
		double r1 = Random3F(400, 14);
		double r2 = Random3F(1200, 14);
		double r3 = Random3F(1200, 14);
		actFireVector(actor, 0, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r3, r2, r1), kVectorTommyRegular);
		SpawnBulletEject(pPlayer, -15 / 16., -45);
		pPlayer->visibility = 20;
		break;
	}
	case 2:
	{
		double r1 = Random3F(400, 14);
		double r2 = Random3F(1200, 14);
		double r3 = Random3F(1200, 14);
		actFireVector(actor, -7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r3, r2, r1), kVectorTommyRegular);
		SpawnBulletEject(pPlayer, -140 / 16., -45);
		r1 = Random3(400);
		r2 = Random3(1200);
		r3 = Random3(1200);
		actFireVector(actor, 7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r3, r2, r1), kVectorTommyRegular);
		SpawnBulletEject(pPlayer, 140 / 16., 45);
		pPlayer->visibility = 30;
		break;
	}
	}
	UseAmmo(pPlayer, pPlayer->weaponAmmo, nTrigger);
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

enum { kMaxSpread = 14 };
static constexpr DAngle DAngle10 = DAngle::fromDeg(10);
static constexpr DAngle DAngle20 = DAngle::fromDeg(20);

void FireSpread(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	assert(nTrigger > 0 && nTrigger <= kMaxSpread);
	DVector3 aim = pPlayer->aim;
	DAngle angle = ((aim.XY()).Angle() + ((DAngle20 * (nTrigger - 1)) / kMaxSpread - DAngle10));
	DVector3 dv = DVector3(angle.ToVector(), aim.Z);

	sfxPlay3DSound(pPlayer->GetActor(), 431, -1, 0);
	double r1, r2, r3;
	r1 = Random3F(300, 14);
	r2 = Random3F(600, 14);
	r3 = Random3F(600, 14);
	actFireVector(actor, 0, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
	int ri = Random2(90);
	r2 = Random2F(30, 4);
	SpawnBulletEject(pPlayer, r2, ri);
	pPlayer->visibility = 20;
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireSpread(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	assert(nTrigger > 0 && nTrigger <= kMaxSpread);
	DVector3 aim = pPlayer->aim;
	DAngle angle = ((aim.XY()).Angle() + ((DAngle20 * (nTrigger - 1)) / kMaxSpread - DAngle10));
	DVector3 dv = DVector3(angle.ToVector(), aim.Z);

	sfxPlay3DSound(pPlayer->GetActor(), 431, -1, 0);
	double r1, r2, r3;
	r1 = Random3F(300, 14);
	r2 = Random3F(600, 14);
	r3 = Random3F(600, 14);
	actFireVector(actor, -7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
	int ri = Random2(45);
	r2 = Random2F(120, 4);
	SpawnBulletEject(pPlayer, r2, ri);
	r1 = Random3F(300, 14);
	r2 = Random3F(600, 14);
	r3 = Random3F(600, 14);
	actFireVector(actor, 7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
	ri = Random2(-45);
	r2 = Random2F(-120, 4);
	SpawnBulletEject(pPlayer, r2, ri);
	pPlayer->tiltEffect = 20;
	pPlayer->visibility = 30;
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 2);
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireSpread2(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	assert(nTrigger > 0 && nTrigger <= kMaxSpread);
	DVector3 aim = pPlayer->aim;
	DAngle angle = ((aim.XY()).Angle() + ((DAngle20 * (nTrigger - 1)) / kMaxSpread - DAngle10));
	DVector3 dv = DVector3(angle.ToVector(), aim.Z);

	sfxPlay3DSound(pPlayer->GetActor(), 431, -1, 0);
	if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 3, 2))
	{
		double r1, r2, r3;
		r1 = Random3F(300, 14);
		r2 = Random3F(600, 14);
		r3 = Random3F(600, 14);
		actFireVector(actor, -7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
		int ri = Random2(45);
		r2 = Random2F(120, 4);
		SpawnBulletEject(pPlayer, r2, ri);
		r1 = Random3F(300, 14);
		r2 = Random3F(600, 14);
		r3 = Random3F(600, 14);
		actFireVector(actor, 7.5, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
		ri = Random2(-45);
		r2 = Random2F(-120, 4);
		SpawnBulletEject(pPlayer, r2, ri);
		pPlayer->tiltEffect = 30;
		pPlayer->visibility = 45;
		UseAmmo(pPlayer, pPlayer->weaponAmmo, 2);
	}
	else
	{
		double r1, r2, r3;
		r1 = Random3F(300, 14);
		r2 = Random3F(600, 14);
		r3 = Random3F(600, 14);
		actFireVector(actor, 0, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, dv + DVector3(r3, r2, r1), kVectorTommyAP);
		int ri = Random2(90);
		r2 = Random2F(30, 4);
		SpawnBulletEject(pPlayer, r2, ri);
		pPlayer->tiltEffect = 20;
		pPlayer->visibility = 30;
		UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);
	}
	pPlayer->flashEffect = 1;
	if (!checkAmmo2(pPlayer, 3, 1))
	{
		WeaponLower(pPlayer);
		pPlayer->weaponState = -1;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireFlare(int nTrigger, DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	double offset = 0;
	switch (nTrigger)
	{
	case 2:
		offset = -7.5;
		break;
	case 3:
		offset = 7.5;
		break;
	}
	playerFireMissile(pPlayer, offset, pPlayer->aim, kMissileFlareRegular);
	UseAmmo(pPlayer, 1, 1);
	sfxPlay3DSound(pPlayer->GetActor(), 420, 2, 0);
	pPlayer->visibility = 30;
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireFlare(int nTrigger, DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	double offset = 0;
	switch (nTrigger)
	{
	case 2:
		offset = -7.5;
		break;
	case 3:
		offset = 7.5;
		break;
	}
	playerFireMissile(pPlayer, offset, pPlayer->aim, kMissileFlareAlt);
	UseAmmo(pPlayer, 1, 8);
	sfxPlay3DSound(pPlayer->GetActor(), 420, 2, 0);
	pPlayer->visibility = 45;
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireVoodoo(int nTrigger, DBloodPlayer* pPlayer)
{
	nTrigger--;
	DBloodActor* actor = pPlayer->GetActor();
	auto plActor = pPlayer->GetActor();
	if (nTrigger == 4)
	{
		actDamageSprite(actor, actor, kDamageBullet, 1 << 4);
		return;
	}
	DBloodActor* targetactor = pPlayer->voodooTarget;
	if (!targetactor) return;
	if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, targetactor))
		return;
	switch (nTrigger)
	{
	case 0:
	{
		sfxPlay3DSound(actor, 460, 2, 0);
		fxSpawnBlood(targetactor, 17 << 4);
		int nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, 17 << 4);
		UseAmmo(pPlayer, 9, nDamage / 4);
		break;
	}
	case 1:
	{
		sfxPlay3DSound(actor, 460, 2, 0);
		fxSpawnBlood(targetactor, 17 << 4);
		int nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, 9 << 4);
		if (targetactor->IsPlayerActor())
			WeaponLower(getPlayer(targetactor));
		UseAmmo(pPlayer, 9, nDamage / 4);
		break;
	}
	case 3:
	{
		sfxPlay3DSound(actor, 463, 2, 0);
		fxSpawnBlood(targetactor, 17 << 4);
		int nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, 49 << 4);
		UseAmmo(pPlayer, 9, nDamage / 4);
		break;
	}
	case 2:
	{
		sfxPlay3DSound(actor, 460, 2, 0);
		fxSpawnBlood(targetactor, 17 << 4);
		int nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, 11 << 4);
		if (targetactor->IsPlayerActor())
		{
			auto pOtherPlayer = getPlayer(targetactor);
			pOtherPlayer->blindEffect = 128;
		}
		UseAmmo(pPlayer, 9, nDamage / 4);
		break;
	}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireVoodoo(int nTrigger, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	if (nTrigger == 2) {

		// by NoOne: trying to simulate v1.0x voodoo here.
		// dunno how exactly it works, but at least it not spend all the ammo on alt fire
		if (gGameOptions.weaponsV10x && !VanillaMode()) {
			int nCount = ClipHigh(pPlayer->ammoCount[9], pPlayer->aimTargetsCount);
			if (nCount > 0)
			{
				for (int i = 0; i < pPlayer->aimTargetsCount; i++)
				{
					DBloodActor* targetactor = pPlayer->aimTargets[i];
					if (!targetactor) continue;
					if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, targetactor))
						continue;
					double nDist = (targetactor->spr.pos.XY() - pPlayer->GetActor()->spr.pos.XY()).Length();
					if (nDist > 0 && nDist < 3200)
					{
						int vc = pPlayer->ammoCount[9] >> 3;
						int v8 = pPlayer->ammoCount[9] << 1;
						int nDamage = (v8 + Random(vc)) << 4;
						nDamage = int((nDamage * ((3200 - nDist) + 1/16.)) / 3200);
						nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, nDamage);

						if (targetactor->IsPlayerActor())
						{
							auto pOtherPlayer = getPlayer(targetactor);
							if (!pOtherPlayer->godMode || !powerupCheck(pOtherPlayer, kPwUpDeathMask))
								powerupActivate(pOtherPlayer, kPwUpDeliriumShroom);
						}
						fxSpawnBlood(targetactor, 0);
					}
				}
			}

			UseAmmo(pPlayer, 9, 20);
			pPlayer->weaponState = 0;
			return;
		}

		//int nAmmo = pPlayer->ammCount[9];
		int nCount = ClipHigh(pPlayer->ammoCount[9], pPlayer->aimTargetsCount);
		if (nCount > 0)
		{
			int v4 = pPlayer->ammoCount[9] - (pPlayer->ammoCount[9] / nCount) * nCount;
			for (int i = 0; i < pPlayer->aimTargetsCount; i++)
			{
				DBloodActor* targetactor = pPlayer->aimTargets[i];
				if (!targetactor) continue;
				if (!gGameOptions.bFriendlyFire && IsTargetTeammate(pPlayer, targetactor))
					continue;
				if (v4 > 0)
					v4--;
				double nDist = (targetactor->spr.pos.XY() - pPlayer->GetActor()->spr.pos.XY()).Length();
				if (nDist > 0 && nDist < 3200)
				{
					int vc = pPlayer->ammoCount[9] >> 3;
					int v8 = pPlayer->ammoCount[9] << 1;
					int nDamage = (v8 + Random2(vc)) << 4;
					nDamage = int((nDamage * ((3200 - nDist) + 1/16.)) / 3200);
					nDamage = actDamageSprite(actor, targetactor, kDamageSpirit, nDamage);
					UseAmmo(pPlayer, 9, nDamage);
					if (targetactor->IsPlayerActor())
					{
						auto pOtherPlayer = getPlayer(targetactor);
						if (!pOtherPlayer->godMode || !powerupCheck(pOtherPlayer, kPwUpDeathMask))
							powerupActivate(pOtherPlayer, kPwUpDeliriumShroom);
					}
					fxSpawnBlood(targetactor, 0);
				}
			}
		}
		UseAmmo(pPlayer, 9, pPlayer->ammoCount[9]);
		pPlayer->hasWeapon[kWeapVoodooDoll] = 0;
		pPlayer->weaponState = -1;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropVoodoo(int, DBloodPlayer* pPlayer)
{
	sfxPlay3DSound(pPlayer->GetActor(), 455, 2, 0);
	auto spawned = playerFireThing(pPlayer, 0, -4730 / 65536., kThingVoodooHead, 12.8);
	if (spawned)
	{
		spawned->xspr.data1 = pPlayer->ammoCount[9];
		evPostActor(spawned, 90, AF(DropVoodooCb));
		UseAmmo(pPlayer, 6, 480/*gAmmoItemData[0].count*/);
		UseAmmo(pPlayer, 9, pPlayer->ammoCount[9]);
		pPlayer->hasWeapon[kWeapVoodooDoll] = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

struct TeslaMissile
{
	float offset; // offset
	int id; // id
	int ammouse; // ammo use
	int sound; // sound
	int light; // light
	int flash; // weapon flash
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireTesla(int nTrigger, DBloodPlayer* pPlayer)
{
	TeslaMissile teslaMissile[6] =
	{
		{ 0, 306, 1, 470, 20, 1 },
		{ -8.75f, 306, 1, 470, 30, 1 },
		{ 8.75f, 306, 1, 470, 30, 1 },
		{ 0, 302, 35, 471, 40, 1 },
		{ -8.75f, 302, 35, 471, 50, 1 },
		{ 8.75f, 302, 35, 471, 50, 1 },
	};
	if (nTrigger > 0 && nTrigger <= 6)
	{
		nTrigger--;
		auto plActor = pPlayer->GetActor();
		TeslaMissile* pMissile = &teslaMissile[nTrigger];
		if (!checkAmmo2(pPlayer, 7, pMissile->ammouse))
		{
			pMissile = &teslaMissile[0];
			if (!checkAmmo2(pPlayer, 7, pMissile->ammouse))
			{
				pPlayer->weaponState = -1;
				SetQAV(pPlayer, kQAVSGUNIDL2);
				pPlayer->flashEffect = 0;
				return;
			}
		}
		playerFireMissile(pPlayer, pMissile->offset, pPlayer->aim, pMissile->id);
		UseAmmo(pPlayer, pPlayer->weaponAmmo, pMissile->ammouse);
		sfxPlay3DSound(pPlayer->GetActor(), pMissile->sound, 1, 0);
		pPlayer->visibility = pMissile->light;
		pPlayer->flashEffect = pMissile->flash;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireTesla(int, DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	playerFireMissile(pPlayer, 0., pPlayer->aim, kMissileTeslaAlt);
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 35);
	sfxPlay3DSound(pPlayer->GetActor(), 471, 2, 0);
	pPlayer->visibility = 40;
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireNapalm(int nTrigger, DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	double offset = 0;
	switch (nTrigger)
	{
	case 2:
		offset = -3.125;
		break;
	case 3:
		offset = 3.125;
		break;
	}
	playerFireMissile(pPlayer, offset, pPlayer->aim, kMissileFireballNapalm);
	sfxPlay3DSound(pPlayer->GetActor(), 480, 2, 0);
	UseAmmo(pPlayer, 4, 1);
	pPlayer->flashEffect = 1;
}

void FireNapalm2(int, DBloodPlayer* pPlayer)
{
	auto plActor = pPlayer->GetActor();
	playerFireMissile(pPlayer, -7.5, pPlayer->aim, kMissileFireballNapalm);
	playerFireMissile(pPlayer, 7.5, pPlayer->aim, kMissileFireballNapalm);
	sfxPlay3DSound(pPlayer->GetActor(), 480, 2, 0);
	UseAmmo(pPlayer, 4, 2);
	pPlayer->flashEffect = 1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireNapalm(int, DBloodPlayer* pPlayer)
{
	auto missile = playerFireThing(pPlayer, 0, -4730 / 65536., kThingNapalmBall, 18.13333);
	if (missile)
	{
		missile->xspr.data4 = ClipHigh(pPlayer->ammoCount[4], 12);
		UseAmmo(pPlayer, 4, missile->xspr.data4);
		seqSpawn(22, missile);
		actBurnSprite(pPlayer->GetActor(), missile, 600);
		evPostActor(missile, 0, AF(fxFlameLick));
		sfxPlay3DSound(missile, 480, 2, 0);
		pPlayer->visibility = 30;
		pPlayer->flashEffect = 1;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireLifeLeech(int nTrigger, DBloodPlayer* pPlayer)
{
	if (!CheckAmmo(pPlayer, 8, 1))
		return;
	double r1 = Random2F(2000, 14);
	double r2 = Random2F(2000, 14);
	double r3 = Random2F(1000, 14);
	DBloodActor* actor = pPlayer->GetActor();
	auto missileActor = playerFireMissile(pPlayer, 0, pPlayer->aim + DVector3(r1, r2, r3), 315);
	if (missileActor)
	{
		missileActor->SetTarget(pPlayer->aimTarget);
		missileActor->spr.Angles.Yaw = ((nTrigger == 2) ? DAngle180 : nullAngle);
	}
	if (checkAmmo2(pPlayer, 8, 1))
		UseAmmo(pPlayer, 8, 1);
	else
		actDamageSprite(actor, actor, kDamageSpirit, 16);
	pPlayer->visibility = ClipHigh(pPlayer->visibility + 5, 50);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AltFireLifeLeech(int, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	sfxPlay3DSound(pPlayer->GetActor(), 455, 2, 0);
	auto missile = playerFireThing(pPlayer, 0, -4730 / 65536., kThingDroppedLifeLeech, 1.6);
	if (missile)
	{
		missile->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1;
		missile->xspr.Push = 1;
		missile->xspr.Proximity = 1;
		missile->xspr.DudeLockout = 1;
		missile->xspr.stateTimer = 1;
		evPostActor(missile, 120, AF(LeechStateTimer));
		if (gGameOptions.nGameType <= 1)
		{
			int nAmmo = pPlayer->ammoCount[8];
			if (nAmmo < 25 && pPlayer->GetActor()->xspr.health > unsigned((25 - nAmmo) << 4))
			{
				actDamageSprite(actor, actor, kDamageSpirit, ((25 - nAmmo) << 4));
				nAmmo = 25;
			}
			missile->xspr.data3 = nAmmo;
			UseAmmo(pPlayer, 8, nAmmo);
		}
		else
		{
			missile->xspr.data3 = pPlayer->ammoCount[8];
			pPlayer->ammoCount[8] = 0;
		}
		pPlayer->hasWeapon[kWeapLifeLeech] = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FireBeast(int, DBloodPlayer* pPlayer)
{
	DBloodActor* actor = pPlayer->GetActor();
	double r1 = Random2F(2000, 14);
	double r2 = Random2F(2000, 14);
	double r3 = Random2F(2000, 14);
	actFireVector(actor, 0, pPlayer->zWeapon - pPlayer->GetActor()->spr.pos.Z, pPlayer->aim + DVector3(r1, r2, r3), kVectorBeastSlash);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const uint8_t gWeaponUpgrade[][13] = {
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },
	{ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
};

int WeaponUpgrade(DBloodPlayer* pPlayer, int newWeapon)
{
	int weaponswitch = WeaponSwitch(pPlayer->pnum);
	int weapon = pPlayer->curWeapon;
	if (!checkLitSprayOrTNT(pPlayer) && (weaponswitch & 1) && (gWeaponUpgrade[pPlayer->curWeapon][newWeapon] || (weaponswitch & 2)))
		weapon = newWeapon;
	return weapon;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const int OrderNext[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 1 };
static const int OrderPrev[] = { 12, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 1 };

static int WeaponFindNext(DBloodPlayer* pPlayer, int* a2, int bDir)
{
	int weapon = pPlayer->curWeapon;
	do
	{
		if (bDir)
			weapon = OrderNext[weapon];
		else
			weapon = OrderPrev[weapon];
		if (weaponModes[weapon].update && pPlayer->hasWeapon[weapon])
		{
			if (weapon == kWeapLifeLeech)
			{
				if (CheckAmmo(pPlayer, weaponModes[weapon].ammoType, 1))
					break;
			}
			else
			{
				if (checkAmmo2(pPlayer, weaponModes[weapon].ammoType, 1))
					break;
			}
		}
	} while (weapon != pPlayer->curWeapon);
	if (weapon == pPlayer->curWeapon)
	{
		if (!weaponModes[weapon].update || !CheckAmmo(pPlayer, weaponModes[weapon].ammoType, 1))
			weapon = kWeapPitchFork;
	}
	if (a2)
		*a2 = 0;
	return weapon;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int WeaponFindLoaded(DBloodPlayer* pPlayer, int* a2)
{
	int v4 = 1;
	int v14 = 0;
	if (weaponModes[pPlayer->curWeapon].update > 1)
	{
		for (int i = 0; i < weaponModes[pPlayer->curWeapon].update; i++)
		{
			if (CheckAmmo(pPlayer, weaponModes[pPlayer->curWeapon].ammoType, 1))
			{
				v14 = i;
				v4 = pPlayer->curWeapon;
				break;
			}
		}
	}
	if (v4 == kWeapPitchFork)
	{
		int vc = 0;
		for (int i = 0; i < 14; i++)
		{
			int weapon = pPlayer->weaponOrder[vc][i];
			if (pPlayer->hasWeapon[weapon])
			{
				for (int j = 0; j < weaponModes[weapon].update; j++)
				{
					if (CheckWeaponAmmo(pPlayer, weapon, weaponModes[weapon].ammoType, 1))
					{
						if (a2)
							*a2 = j;
						return weapon;
					}
				}
			}
		}
	}
	if (a2)
		*a2 = v14;
	return v4;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int processSprayCan(DBloodPlayer* pPlayer)
{
	const char bUseShootAsThrow = !VanillaMode() && (pPlayer->cmd.ucmd.actions & SB_FIRE);
	switch (pPlayer->weaponState)
	{
	case 5:
		if (!(pPlayer->cmd.ucmd.actions & SB_ALTFIRE) || bUseShootAsThrow)
			pPlayer->weaponState = 6;
		return 1;
	case 6:
		if ((pPlayer->cmd.ucmd.actions & SB_ALTFIRE) && !bUseShootAsThrow)
		{
			pPlayer->weaponState = 3;
			pPlayer->fuseTime = pPlayer->weaponTimer;
			StartQAV(pPlayer, kQAVCANDROP, nClientDropCan);
		}
		else if (pPlayer->cmd.ucmd.actions & SB_FIRE)
		{
			pPlayer->weaponState = 7;
			pPlayer->fuseTime = 0;
			pPlayer->throwTime = PlayClock;
		}
		return 1;
	case 7:
	{
		setThrowPower(pPlayer);
		if (!(pPlayer->cmd.ucmd.actions & SB_FIRE))
		{
			if (!pPlayer->fuseTime)
				pPlayer->fuseTime = pPlayer->weaponTimer;
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVCANTHRO, nClientThrowCan);
		}
		return 1;
	}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool processTNT(DBloodPlayer* pPlayer)
{
	const char bUseShootAsThrow = !VanillaMode() && (pPlayer->cmd.ucmd.actions & SB_FIRE);
	switch (pPlayer->weaponState)
	{
	case 4:
		if (!(pPlayer->cmd.ucmd.actions & SB_ALTFIRE) || bUseShootAsThrow)
			pPlayer->weaponState = 5;
		return 1;
	case 5:
		if ((pPlayer->cmd.ucmd.actions & SB_ALTFIRE) && !bUseShootAsThrow)
		{
			pPlayer->weaponState = 1;
			pPlayer->fuseTime = pPlayer->weaponTimer;
			StartQAV(pPlayer, kQAVBUNDROP, nClientDropBundle);
		}
		else if (pPlayer->cmd.ucmd.actions & SB_FIRE)
		{
			pPlayer->weaponState = 6;
			pPlayer->fuseTime = 0;
			pPlayer->throwTime = PlayClock;
		}
		return 1;
	case 6:
	{
		setThrowPower(pPlayer);
		if (!(pPlayer->cmd.ucmd.actions & SB_FIRE))
		{
			if (!pPlayer->fuseTime)
				pPlayer->fuseTime = pPlayer->weaponTimer;
			pPlayer->weaponState = 1;
			StartQAV(pPlayer, kQAVBUNTHRO, nClientThrowBundle);
		}
		return 1;
	}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool processProxy(DBloodPlayer* pPlayer)
{
	switch (pPlayer->weaponState)
	{
	case 9:
		setThrowPower(pPlayer);
		pPlayer->weaponTimer = 0;
		pPlayer->qavTimer = 0;
		if (!(pPlayer->cmd.ucmd.actions & SB_FIRE))
		{
			pPlayer->weaponState = 8;
			StartQAV(pPlayer, kQAVPROXTHRO, nClientThrowProx);
		}
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool processRemote(DBloodPlayer* pPlayer)
{
	switch (pPlayer->weaponState)
	{
	case 13:
		setThrowPower(pPlayer);
		if (!(pPlayer->cmd.ucmd.actions & SB_FIRE))
		{
			pPlayer->weaponState = 11;
			StartQAV(pPlayer, kQAVREMTHRO, nClientThrowRemote);
		}
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool processLeech(DBloodPlayer* pPlayer)
{
	switch (pPlayer->weaponState)
	{
	case 4:
		pPlayer->weaponState = 6;
		StartQAV(pPlayer, kQAVSTAFIRE1, nClientFireLifeLeech, 1);
		return 1;
	case 6:
		if (!(pPlayer->cmd.ucmd.actions & SB_ALTFIRE))
		{
			pPlayer->weaponState = 2;
			StartQAV(pPlayer, kQAVSTAFPOST);
			return 1;
		}
		break;
	case 8:
		pPlayer->weaponState = 2;
		StartQAV(pPlayer, kQAVSTAFPOST);
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool processTesla(DBloodPlayer* pPlayer)
{
	switch (pPlayer->weaponState)
	{
	case 4:
		pPlayer->weaponState = 5;
		if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
			StartQAV(pPlayer, kQAV2SGUNFIR, nClientFireTesla, 1);
		else
			StartQAV(pPlayer, kQAVSGUNFIR1, nClientFireTesla, 1);
		return 1;
	case 5:
		if (!(pPlayer->cmd.ucmd.actions & SB_FIRE))
		{
			pPlayer->weaponState = 2;
			if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
				StartQAV(pPlayer, kQAV2SGUNPST);
			else
				StartQAV(pPlayer, kQAVSGUNPOST);
			return 1;
		}
		break;
	case 7:
		pPlayer->weaponState = 2;
		if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
			StartQAV(pPlayer, kQAV2SGUNPST);
		else
			StartQAV(pPlayer, kQAVSGUNPOST);
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void WeaponProcess(DBloodPlayer* pPlayer) {

	pPlayer->flashEffect = ClipLow(pPlayer->flashEffect - 1, 0);

#ifdef NOONE_EXTENSIONS
	if (gPlayerCtrl[pPlayer->pnum].qavScene.initiator != nullptr && pPlayer->GetActor()->xspr.health > 0) {
		playerQavSceneProcess(pPlayer, &gPlayerCtrl[pPlayer->pnum].qavScene);
		UpdateAimVector(pPlayer);
		return;
	}
#endif

	int newweap = pPlayer->cmd.ucmd.getNewWeapon();
	if (newweap > 0 && newweap <= WeaponSel_MaxBlood) pPlayer->newWeapon = newweap;

	if (pPlayer->GetActor()->xspr.health == 0)
	{
		pPlayer->qavLoop = 0;
		sfxKill3DSound(pPlayer->GetActor(), 1, -1);
	}
	if (pPlayer->isUnderwater && BannedUnderwater(pPlayer->curWeapon))
	{
		if (checkLitSprayOrTNT(pPlayer))
		{
			if (pPlayer->curWeapon == kWeapSpraycan)
			{
				pPlayer->fuseTime = pPlayer->weaponTimer;
				DropCan(1, pPlayer);
				pPlayer->weaponState = 3;
			}
			else if (pPlayer->curWeapon == kWeapDynamite)
			{
				pPlayer->fuseTime = pPlayer->weaponTimer;
				DropBundle(1, pPlayer);
				pPlayer->weaponState = 3;
			}
		}
		WeaponLower(pPlayer);
		pPlayer->throwPower = 0;
	}
	WeaponPlay(pPlayer);
	UpdateAimVector(pPlayer);
	pPlayer->weaponTimer -= 4;
	bool bShoot = pPlayer->cmd.ucmd.actions & SB_FIRE;
	bool bShoot2 = pPlayer->cmd.ucmd.actions & SB_ALTFIRE;
	const int prevNewWeaponVal = pPlayer->cmd.ucmd.getNewWeapon(); // used to fix scroll issue for banned weapons
	if ((bShoot || bShoot2 || prevNewWeaponVal) && pPlayer->weaponQav == qavGetCorrectID(kQAVVDIDLE2)) pPlayer->weaponTimer = 0;
	if (pPlayer->qavLoop && pPlayer->GetActor()->xspr.health > 0)
	{
		if (bShoot && CheckAmmo(pPlayer, pPlayer->weaponAmmo, 1))
		{
			auto pQAV = getQAV(pPlayer->weaponQav);
			while (pPlayer->weaponTimer <= 0)
			{
				pPlayer->weaponTimer += pQAV->duration;
				pPlayer->qavTimer += pQAV->duration;
			}
		}
		else
		{
			pPlayer->weaponTimer = 0;
			pPlayer->qavTimer = 0;
			pPlayer->qavLoop = 0;
		}
		return;
	}
	pPlayer->weaponTimer = ClipLow(pPlayer->weaponTimer, 0);
	switch (pPlayer->curWeapon)
	{
	case kWeapSpraycan:
		if (processSprayCan(pPlayer))
			return;
		break;
	case kWeapDynamite:
		if (processTNT(pPlayer))
			return;
		break;
	case kWeapProximity:
		if (processProxy(pPlayer))
			return;
		break;
	case kWeapRemote:
		if (processRemote(pPlayer))
			return;
		break;
	}
	if (pPlayer->weaponTimer > 0)
		return;
	if (pPlayer->GetActor()->xspr.health == 0 || pPlayer->curWeapon == kWeapNone)
		pPlayer->weaponQav = kQAVNone;
	switch (pPlayer->curWeapon)
	{
	case kWeapLifeLeech:
		if (processLeech(pPlayer))
			return;
		break;
	case kWeapTeslaCannon:
		if (processTesla(pPlayer))
			return;
		break;
	}
	if (VanillaMode())
	{
		if (pPlayer->nextWeapon)
		{
			sfxKill3DSound(pPlayer->GetActor(), -1, 441);
			pPlayer->weaponState = 0;
			pPlayer->newWeapon = pPlayer->nextWeapon;
			pPlayer->nextWeapon = kWeapNone;
		}
	}
	if (pPlayer->cmd.ucmd.getNewWeapon() == WeaponSel_Next)
	{
		pPlayer->cmd.ucmd.setNewWeapon(kWeapNone);
		if (VanillaMode())
		{
			pPlayer->weaponState = 0;
		}
		pPlayer->nextWeapon = kWeapNone;
		int t;
		int weapon = WeaponFindNext(pPlayer, &t, 1);
		pPlayer->weaponMode[weapon] = t;
		if (VanillaMode())
		{
			if (pPlayer->curWeapon)
			{
				WeaponLower(pPlayer);
				pPlayer->nextWeapon = weapon;
				return;
			}
		}
		pPlayer->newWeapon = weapon;
	}
	else if (pPlayer->cmd.ucmd.getNewWeapon() == WeaponSel_Prev)
	{
		pPlayer->cmd.ucmd.setNewWeapon(kWeapNone);
		if (VanillaMode())
		{
			pPlayer->weaponState = 0;
		}
		pPlayer->nextWeapon = kWeapNone;
		int t;
		int weapon = WeaponFindNext(pPlayer, &t, 0);
		pPlayer->weaponMode[weapon] = t;
		if (VanillaMode())
		{
			if (pPlayer->curWeapon)
			{
				WeaponLower(pPlayer);
				pPlayer->nextWeapon = weapon;
				return;
			}
		}
		pPlayer->newWeapon = weapon;
	}
	else if (pPlayer->cmd.ucmd.getNewWeapon() == WeaponSel_Alt)
	{
		int weapon;

		switch (pPlayer->curWeapon)
		{
		case kWeapDynamite:
			weapon = kWeapProximity;
			break;
		case kWeapProximity:
			weapon = kWeapRemote;
			break;
		case kWeapRemote:
			weapon = kWeapDynamite;
			break;
		default:
			return;
		}

		pPlayer->cmd.ucmd.setNewWeapon(kWeapNone);
		pPlayer->weaponState = 0;
		pPlayer->nextWeapon = kWeapNone;
		int t = 0;
		pPlayer->weaponMode[weapon] = t;
		if (pPlayer->curWeapon)
		{
			WeaponLower(pPlayer);
			pPlayer->nextWeapon = weapon;
			return;
		}
		pPlayer->newWeapon = weapon;
	}
	if (!VanillaMode())
	{
		if (pPlayer->nextWeapon)
		{
			sfxKill3DSound(pPlayer->GetActor(), -1, 441);
			pPlayer->newWeapon = pPlayer->nextWeapon;
			pPlayer->nextWeapon = kWeapNone;
		}
	}
	if (pPlayer->weaponState == -1)
	{
		pPlayer->weaponState = 0;
		int t;
		int weapon = WeaponFindLoaded(pPlayer, &t);
		pPlayer->weaponMode[weapon] = t;
		if (pPlayer->curWeapon)
		{
			WeaponLower(pPlayer);
			pPlayer->nextWeapon = weapon;
			return;
		}
		pPlayer->newWeapon = weapon;
	}
	if (pPlayer->newWeapon)
	{
		if (pPlayer->isUnderwater && BannedUnderwater(pPlayer->newWeapon) && !checkLitSprayOrTNT(pPlayer) && !VanillaMode()) // skip banned weapons when underwater and using next/prev weapon key inputs
		{
			if (prevNewWeaponVal == WeaponSel_Next || prevNewWeaponVal == WeaponSel_Prev) // if player switched weapons
			{
				int saveweapon = pPlayer->curWeapon;
				pPlayer->curWeapon = pPlayer->newWeapon; // set current banned weapon to curweapon so WeaponFindNext() can find the next weapon
				for (int i = 0; i < 3; i++) // attempt twice to find a new weapon
				{
					pPlayer->curWeapon = WeaponFindNext(pPlayer, NULL, (char)(prevNewWeaponVal == WeaponSel_Next));
					if (!BannedUnderwater(pPlayer->curWeapon)) // if new weapon is not a banned weapon, set to new current weapon
					{
						pPlayer->newWeapon = pPlayer->curWeapon;
						pPlayer->weaponMode[pPlayer->newWeapon] = 0;
						break;
					}
				}
				pPlayer->curWeapon = saveweapon;
			}
		}
		if (pPlayer->newWeapon == kWeapDynamite)
		{
			if (pPlayer->curWeapon == kWeapDynamite)
			{
				if (checkAmmo2(pPlayer, 10, 1))
					pPlayer->newWeapon = kWeapProximity;
				else if (checkAmmo2(pPlayer, 11, 1))
					pPlayer->newWeapon = kWeapRemote;
			}
			else if (pPlayer->curWeapon == kWeapProximity)
			{
				if (checkAmmo2(pPlayer, 11, 1))
					pPlayer->newWeapon = kWeapRemote;
				else if (checkAmmo2(pPlayer, 5, 1) && pPlayer->isUnderwater == 0)
					pPlayer->newWeapon = kWeapDynamite;
			}
			else if (pPlayer->curWeapon == kWeapRemote)
			{
				if (checkAmmo2(pPlayer, 5, 1) && pPlayer->isUnderwater == 0)
					pPlayer->newWeapon = kWeapDynamite;
				else if (checkAmmo2(pPlayer, 10, 1))
					pPlayer->newWeapon = kWeapProximity;
			}
			else
			{
				if (checkAmmo2(pPlayer, 5, 1) && pPlayer->isUnderwater == 0)
					pPlayer->newWeapon = kWeapDynamite;
				else if (checkAmmo2(pPlayer, 10, 1))
					pPlayer->newWeapon = kWeapProximity;
				else if (checkAmmo2(pPlayer, 11, 1))
					pPlayer->newWeapon = kWeapRemote;
			}
		}
		else if ((pPlayer->newWeapon == kWeapSpraycan) && !VanillaMode())
		{
			if ((pPlayer->curWeapon == kWeapSpraycan) && (pPlayer->weaponState == 2)) // fix spray can state glitch when switching from spray to tnt and back quickly
			{
				pPlayer->weaponState = 1;
				pPlayer->newWeapon = kWeapNone;
				return;
			}
		}
		if (pPlayer->GetActor()->xspr.health == 0 || pPlayer->hasWeapon[pPlayer->newWeapon] == 0)
		{
			pPlayer->newWeapon = kWeapNone;
			return;
		}
		if (pPlayer->isUnderwater && BannedUnderwater(pPlayer->newWeapon) && !checkLitSprayOrTNT(pPlayer))
		{
			pPlayer->newWeapon = kWeapNone;
			return;
		}
		int nWeapon = pPlayer->newWeapon;
		int v4c = weaponModes[nWeapon].update;
		if (!pPlayer->curWeapon)
		{
			int nAmmoType = weaponModes[nWeapon].ammoType;
			if (v4c > 1)
			{
				if (CheckAmmo(pPlayer, nAmmoType, 1) || nAmmoType == 11)
					WeaponRaise(pPlayer);
				pPlayer->newWeapon = kWeapNone;
			}
			else
			{
				if (CheckWeaponAmmo(pPlayer, nWeapon, nAmmoType, 1))
					WeaponRaise(pPlayer);
				else
				{
					pPlayer->weaponState = 0;
					int t;
					int weapon = WeaponFindLoaded(pPlayer, &t);
					pPlayer->weaponMode[weapon] = t;
					if (pPlayer->curWeapon)
					{
						WeaponLower(pPlayer);
						pPlayer->nextWeapon = weapon;
						return;
					}
					pPlayer->newWeapon = weapon;
				}
			}
			return;
		}
		if (nWeapon == pPlayer->curWeapon && v4c <= 1)
		{
			pPlayer->newWeapon = kWeapNone;
			return;
		}
		int i = 0;
		if (nWeapon == pPlayer->curWeapon)
			i = 1;
		for (; i <= v4c; i++)
		{
			int v6c = (pPlayer->weaponMode[nWeapon] + i) % v4c;
			if (CheckWeaponAmmo(pPlayer, nWeapon, weaponModes[nWeapon].ammoType, 1))
			{
				WeaponLower(pPlayer);
				pPlayer->weaponMode[nWeapon] = v6c;
				return;
			}
		}
		pPlayer->newWeapon = kWeapNone;
		return;
	}
	if (pPlayer->curWeapon && !CheckAmmo(pPlayer, pPlayer->weaponAmmo, 1) && pPlayer->weaponAmmo != 11)
	{
		pPlayer->weaponState = -1;
		return;
	}
	if (bShoot)
	{
		switch (pPlayer->curWeapon)
		{
		case kWeapPitchFork:
			StartQAV(pPlayer, kQAVPFORK, nClientFirePitchfork);
			return;
		case kWeapSpraycan:
			switch (pPlayer->weaponState)
			{
			case 3:
				pPlayer->weaponState = 4;
				StartQAV(pPlayer, kQAVCANFIRE, nClientFireSpray, 1);
				return;
			}
			break;
		case kWeapDynamite:
			switch (pPlayer->weaponState)
			{
			case 3:
				pPlayer->weaponState = 6;
				pPlayer->fuseTime = -1;
				pPlayer->throwTime = PlayClock;
				StartQAV(pPlayer, kQAVBUNFUSE, nClientExplodeBundle);
				return;
			}
			break;
		case kWeapProximity:
			switch (pPlayer->weaponState)
			{
			case 7:
				SetQAV(pPlayer, kQAVPROXIDLE);
				pPlayer->weaponState = 9;
				pPlayer->throwTime = PlayClock;
				return;
			}
			break;
		case kWeapRemote:
			switch (pPlayer->weaponState)
			{
			case 10:
				SetQAV(pPlayer, kQAVREMIDLE1);
				pPlayer->weaponState = 13;
				pPlayer->throwTime = PlayClock;
				return;
			case 11:
				pPlayer->weaponState = 12;
				StartQAV(pPlayer, kQAVREMFIRE, nClientFireRemote);
				return;
			}
			break;
		case kWeapShotgun:
			switch (pPlayer->weaponState)
			{
			case 7:
				pPlayer->weaponState = 6;
				StartQAV(pPlayer, kQAV2SHOTF2, nClientFireShotgun);
				return;
			case 3:
				pPlayer->weaponState = 2;
				StartQAV(pPlayer, kQAVSHOTF1, nClientFireShotgun);
				return;
			case 2:
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVSHOTF2, nClientFireShotgun);
				return;
			}
			break;
		case kWeapTommyGun:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 3, 2))
				StartQAV(pPlayer, kQAV2TOMFIRE, nClientFireTommy, 1);
			else
				StartQAV(pPlayer, kQAVTOMFIRE, nClientFireTommy, 1);
			return;
		case kWeapFlareGun:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 1, 2))
				StartQAV(pPlayer, kQAVFLAR2FIR, nClientFireFlare);
			else
				StartQAV(pPlayer, kQAVFLARFIR2, nClientFireFlare);
			return;
		case kWeapVoodooDoll:
		{
			static int nChance[] = { 0xa000, 0xc000, 0xe000, 0x10000 };
			int nRand = wrand() * 2;
			int i;
			for (i = 0; nChance[i] < nRand; i++)
			{
			}
			pPlayer->voodooTarget = pPlayer->aimTarget;
			if (pPlayer->voodooTarget == nullptr || pPlayer->voodooTarget->spr.statnum != kStatDude)
				i = 4;
			StartQAV(pPlayer, kQAVVDFIRE1 + i, nClientFireVoodoo);
			return;
		}
		case kWeapTeslaCannon:
			switch (pPlayer->weaponState)
			{
			case 2:
				pPlayer->weaponState = 4;
				if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
					StartQAV(pPlayer, kQAV2SGUNFIR, nClientFireTesla);
				else
					StartQAV(pPlayer, kQAVSGUNFIR1, nClientFireTesla);
				return;
			case 5:
				if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
					StartQAV(pPlayer, kQAV2SGUNFIR, nClientFireTesla);
				else
					StartQAV(pPlayer, kQAVSGUNFIR1, nClientFireTesla);
				return;
			}
			break;
		case kWeapNapalm:
			if (powerupCheck(pPlayer, kPwUpTwoGuns))
				StartQAV(pPlayer, kQAV2NAPFIRE, nClientFireNapalm);
			else
				StartQAV(pPlayer, kQAVNAPFIRE, nClientFireNapalm);
			return;
		case kWeapLifeLeech:
			sfxPlay3DSound(pPlayer->GetActor(), 494, 2, 0);
			StartQAV(pPlayer, kQAVSTAFIRE4, nClientFireLifeLeech);
			return;
		case kWeapBeast:
			StartQAV(pPlayer, kQAVBSTATAK1 + Random(4), nClientFireBeast);
			return;
		}
	}
	if (bShoot2)
	{
		switch (pPlayer->curWeapon)
		{
		case kWeapPitchFork:
			StartQAV(pPlayer, kQAVPFORK, nClientFirePitchfork);
			return;
		case kWeapSpraycan:
			switch (pPlayer->weaponState)
			{
			case 3:
				pPlayer->weaponState = 5;
				StartQAV(pPlayer, kQAVCANFIRE2, nClientExplodeCan);
				return;
			}
			break;
		case kWeapDynamite:
			switch (pPlayer->weaponState)
			{
			case 3:
				pPlayer->weaponState = 4;
				StartQAV(pPlayer, kQAVBUNFUSE, nClientExplodeBundle);
				return;
			case 7:
				pPlayer->weaponState = 8;
				StartQAV(pPlayer, kQAVPROXDROP, nClientDropProx);
				return;
			case 10:
				pPlayer->weaponState = 11;
				StartQAV(pPlayer, kQAVREMDROP, nClientDropRemote);
				return;
			case 11:
				if (pPlayer->ammoCount[11] > 0)
				{
					pPlayer->weaponState = 10;
					StartQAV(pPlayer, kQAVREMUP1);
				}
				return;
			}
			break;
		case kWeapProximity:
			switch (pPlayer->weaponState)
			{
			case 7:
				pPlayer->weaponState = 8;
				StartQAV(pPlayer, kQAVPROXDROP, nClientDropProx);
				return;
			}
			break;
		case kWeapRemote:
			switch (pPlayer->weaponState)
			{
			case 10:
				pPlayer->weaponState = 11;
				StartQAV(pPlayer, kQAVREMDROP, nClientDropRemote);
				return;
			case 11:
				if (pPlayer->ammoCount[11] > 0)
				{
					pPlayer->weaponState = 10;
					StartQAV(pPlayer, kQAVREMUP1);
				}
				return;
			}
			break;
		case kWeapShotgun:
			switch (pPlayer->weaponState)
			{
			case 7:
				pPlayer->weaponState = 6;
				StartQAV(pPlayer, kQAV2SHOTFIR, nClientFireShotgun);
				return;
			case 3:
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVSHOTF3, nClientFireShotgun);
				return;
			case 2:
				pPlayer->weaponState = 1;
				StartQAV(pPlayer, kQAVSHOTF2, nClientFireShotgun);
				return;
			}
			break;
		case kWeapTommyGun:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 3, 2))
				StartQAV(pPlayer, kQAV2TOMALT, nClientAltFireSpread2);
			else
				StartQAV(pPlayer, kQAVTOMSPRED, nClientAltFireSpread2);
			return;
		case kWeapVoodooDoll:
			sfxPlay3DSound(pPlayer->GetActor(), 461, 2, 0);
			StartQAV(pPlayer, kQAVVDSPEL1, nClientAltFireVoodoo);
			return;
#if 0
		case kWeapFlareGun:
			if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 1, 2))
				StartQAV(pPlayer, kQAVFLAR2FIR, nClientFireFlare, 0);
			else
				StartQAV(pPlayer, kQAVFLARFIR2, nClientFireFlare, 0);
			return;
#endif
		case kWeapTeslaCannon:
			if (checkAmmo2(pPlayer, 7, 35))
			{
				if (checkAmmo2(pPlayer, 7, 70) && powerupCheck(pPlayer, kPwUpTwoGuns))
					StartQAV(pPlayer, kQAV2SGUNALT, nClientFireTesla);
				else
					StartQAV(pPlayer, kQAVSGUNFIR4, nClientFireTesla);
			}
			else
			{
				if (checkAmmo2(pPlayer, 7, 10) && powerupCheck(pPlayer, kPwUpTwoGuns))
					StartQAV(pPlayer, kQAV2SGUNFIR, nClientFireTesla);
				else
					StartQAV(pPlayer, kQAVSGUNFIR1, nClientFireTesla);
			}
			return;
		case kWeapNapalm:
			if (powerupCheck(pPlayer, kPwUpTwoGuns))
				// by NoOne: allow napalm launcher alt fire act like in v1.0x versions
				if (gGameOptions.weaponsV10x && !VanillaMode()) StartQAV(pPlayer, kQAV2NAPFIR2, nClientFireNapalm2);
				else StartQAV(pPlayer, kQAV2NAPFIRE, nClientAltFireNapalm);
			else
				StartQAV(pPlayer, kQAVNAPFIRE, (gGameOptions.weaponsV10x && !VanillaMode()) ? nClientFireNapalm : nClientAltFireNapalm);
			return;
		case kWeapFlareGun:
			if (CheckAmmo(pPlayer, 1, 8))
			{
				if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 1, 16))
					StartQAV(pPlayer, kQAVFLAR2FIR, nClientAltFireFlare);
				else
					StartQAV(pPlayer, kQAVFLARFIR2, nClientAltFireFlare);
			}
			else
			{
				if (powerupCheck(pPlayer, kPwUpTwoGuns) && checkAmmo2(pPlayer, 1, 2))
					StartQAV(pPlayer, kQAVFLAR2FIR, nClientFireFlare);
				else
					StartQAV(pPlayer, kQAVFLARFIR2, nClientFireFlare);
			}
			return;
		case kWeapLifeLeech:
			if (gGameOptions.nGameType <= 1 && !checkAmmo2(pPlayer, 8, 1) && pPlayer->GetActor()->xspr.health < (25 << 4))
			{
				sfxPlay3DSound(pPlayer->GetActor(), 494, 2, 0);
				StartQAV(pPlayer, kQAVSTAFIRE4, nClientFireLifeLeech);
			}
			else
			{
				StartQAV(pPlayer, kQAVSTAFDOWN);
				AltFireLifeLeech(1, pPlayer);
				pPlayer->weaponState = -1;
			}
			return;
		}
	}
	WeaponUpdateState(pPlayer);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void teslaHit(DBloodActor* missileactor, int a2)
{
	auto mpos = missileactor->spr.pos;
	int nDist = 300;
	auto pSector = missileactor->sector();
	auto owneractor = missileactor->GetOwner();
	const bool newSectCheckMethod = !cl_bloodvanillaexplosions && !VanillaMode(); // use new sector checking logic
	auto sectorMap = GetClosestSpriteSectors(pSector, mpos.XY(), nDist, nullptr, newSectCheckMethod);
	bool v4 = true;
	DBloodActor* actor = nullptr;
	actHitcodeToData(a2, &gHitInfo, &actor);
	if (a2 == 3 && actor && actor->spr.statnum == kStatDude)
		v4 = false;
	BloodStatIterator it(kStatDude);
	while (auto hitactor = it.Next())
	{
		if (hitactor != owneractor || v4)
		{
			if (hitactor->spr.flags & 32)
				continue;
			if (CheckSector(sectorMap, hitactor) && CheckProximity(hitactor, mpos, pSector, nDist))
			{
				int length = int((missileactor->spr.pos.XY() - hitactor->spr.pos.XY()).Length());
				int nDamage = ClipLow((nDist - length + 20) >> 1, 10);
				if (hitactor == owneractor)
					nDamage /= 2;
				actDamageSprite(owneractor, hitactor, kDamageTesla, nDamage << 4);
			}
		}
	}
	it.Reset(kStatThing);
	while (auto hitactor = it.Next())
	{
		if (hitactor->spr.flags & 32)
			continue;
		if (CheckSector(sectorMap, hitactor) && CheckProximity(hitactor, mpos, pSector, nDist))
		{
			if (!hitactor->xspr.locked)
			{
				int length = int((missileactor->spr.pos.XY() - hitactor->spr.pos.XY()).Length());
				int nDamage = ClipLow(nDist - length + 20, 20);
				actDamageSprite(owneractor, hitactor, kDamageTesla, nDamage << 4);
			}
		}
	}
}

END_BLD_NS
