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
#include "bloodactor.h"

BEGIN_BLD_NS


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlameLick(DBloodActor* actor) // 0
{
	if (!actor) return;
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
	double nDist = (actor->spr.scale.X * tex->GetDisplayWidth()) * (1. / 4);
	for (int i = 0; i < 3; i++)
	{
		DAngle nAngle = RandomAngle();
		DVector2 dv = nAngle.ToVector() * nDist;
		DVector2 pos = actor->spr.pos.XY() + dv;
		double z = bottom - RandomD(bottom - top, 8);

		auto pFX = gFX.fxSpawnActor(FX_32, actor->sector(), DVector3(pos, z));
		if (pFX)
		{
			pFX->vel.X = actor->vel.X + Random2F(-int(16 * dv.X));
			pFX->vel.Y = actor->vel.Y + Random2F(-int(16 * dv.Y));
			pFX->vel.Z = actor->vel.Z - Random2F(0x1aaaa);
		}
	}
	if (actor->xspr.burnTime > 0)
		evPostActor(actor, 5, AF(fxFlameLick));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void RemoveActor(DBloodActor* actor) // 1
{
	if (!actor) return;
	evKillActor(actor, AF(fxFlareSpark));
	if (actor->hasX())
		seqKill(actor);
	sfxKill3DSound(actor, 0, -1);
	DeleteSprite(actor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FlareBurst(DBloodActor* actor) // 2
{
	if (!actor) return;
	auto nAngVec = actor->vel.XY().Angle().ToVector();
	double nRadius = FixedToFloat(0x55555);
	for (int i = 0; i < 8; i++)
	{
		auto spawnedactor = actSpawnSprite(actor, kStatProjectile);
		spawnedactor->spr.setspritetexture(aTexIds[kTexFLAREBURST]);
		spawnedactor->spr.shade = -128;
		spawnedactor->spr.scale = DVector2(0.5, 0.5);
		spawnedactor->ChangeType(kMissileFlareAlt);
		spawnedactor->clipdist = 0.5;
		spawnedactor->SetOwner(actor);
		auto spAngVec = DAngle::fromBam(i << 29).ToVector().Rotated90CW() * nRadius;
		if (i & 1) spAngVec *= 0.5;
		spawnedactor->vel += DVector3(DVector2(0, spAngVec.X).Rotated(nAngVec.X, nAngVec.Y), spAngVec.Y);
		evPostActor(spawnedactor, 960, AF(RemoveActor));
	}
	evPostActor(actor, 0, AF(RemoveActor));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlareSpark(DBloodActor* actor) // 3
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_28, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random2F(0x1aaaa);
	}
	evPostActor(actor, 4, AF(fxFlareSpark));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlareSparkLite(DBloodActor* actor) // 4
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_28, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random2F(0x1aaaa);
	}
	evPostActor(actor, 12, AF(fxFlareSparkLite));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxZombieBloodSpurt(DBloodActor* actor) // 5
{
	if (!actor) return;
	assert(actor->hasX());
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	auto pFX = gFX.fxSpawnActor(FX_27, actor->sector(), DVector3(actor->spr.pos.XY(), top));
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2F(0x11111);
		pFX->vel.Y = actor->vel.Y + Random2F(0x11111);
		pFX->vel.Z = actor->vel.Z - 6.66666;
	}
	if (actor->xspr.data1 > 0)
	{
		evPostActor(actor, 4, AF(fxZombieBloodSpurt));
		actor->xspr.data1 -= 4;
	}
	else if (actor->xspr.data2 > 0)
	{
		evPostActor(actor, 60, AF(fxZombieBloodSpurt));
		actor->xspr.data1 = 40;
		actor->xspr.data2--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxBloodSpurt(DBloodActor* actor) // 6
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_27, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->spr.Angles.Yaw = nullAngle;
		pFX->vel = actor->vel * (1./256);
	}
	evPostActor(actor, 6, AF(fxBloodSpurt));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxArcSpark(DBloodActor* actor) // 7
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_15, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2F(0x10000);
		pFX->vel.Y = actor->vel.Y + Random2F(0x10000);
		pFX->vel.Z = actor->vel.Z - Random2F(0x1aaaa);
	}
	evPostActor(actor, 3, AF(fxArcSpark));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxDynPuff(DBloodActor* actor) // 8
{
	if (!actor) return;
	if (actor->vel.Z)
	{
		auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
		double nDist = (actor->spr.scale.X * tex->GetDisplayWidth()) * (1. / 2);
		DVector3 pos = actor->spr.pos + (actor->spr.Angles.Yaw - DAngle90).ToVector() * nDist;
		auto pFX = gFX.fxSpawnActor(FX_7, actor->sector(), pos);
		if (pFX)
		{
			pFX->vel = actor->vel;
		}
	}
	evPostActor(actor, 12, AF(fxDynPuff));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Respawn(DBloodActor* actor) // 9
{
	if (!actor) return;
	assert(actor->hasX());

	if (actor->spr.statnum != kStatRespawn && actor->spr.statnum != kStatThing) {
		viewSetSystemMessage("Sprite #%d is not on Respawn or Thing list\n", actor->GetIndex());
		return;
	}
	else if (!(actor->spr.flags & kHitagRespawn)) {
		viewSetSystemMessage("Sprite #%d does not have the respawn attribute\n", actor->GetIndex());
		return;
	}

	switch (actor->xspr.respawnPending) {
	case 1: {
		int nTime = MulScale(actGetRespawnTime(actor), 0x4000, 16);
		actor->xspr.respawnPending = 2;
		evPostActor(actor, nTime, AF(Respawn));
		break;
	}
	case 2: {
		int nTime = MulScale(actGetRespawnTime(actor), 0x2000, 16);
		actor->xspr.respawnPending = 3;
		evPostActor(actor, nTime, AF(Respawn));
		break;
	}
	case 3: {
		assert(actor->spr.intowner != kStatRespawn);
		assert(actor->spr.intowner >= 0 && actor->spr.intowner < kMaxStatus);
		ChangeActorStat(actor, actor->spr.intowner);
		actor->ChangeType(actor->spr.inittype);
		actor->SetOwner(nullptr);
		actor->spr.flags &= ~kHitagRespawn;
		actor->vel.Zero();
		actor->xspr.respawnPending = 0;
		actor->xspr.burnTime = 0;
		actor->xspr.isTriggered = 0;
		if (actor->IsDudeActor())
		{
			int nType = actor->GetType() - kDudeBase;
			actor->spr.pos = actor->basePoint;
			actor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
			if ((currentLevel->featureflags & kFeatureCustomEnemyHealth) && actor->xspr.sysData2 > 0)
				actor->xspr.health = clamp(actor->xspr.sysData2 << 4, 1, 65535);
			else
				actor->xspr.health = getDudeInfo(actor)->startHealth << 4;
#ifdef NOONE_EXTENSIONS

			// return dude to the patrol state
			if (gModernMap && actor->xspr.dudeFlag4) 
			{
				actor->xspr.data3 = 0;
				actor->SetTarget(nullptr);
			}

			switch (actor->GetType()) {
			default:
				actor->clipdist = getDudeInfo(nType + kDudeBase)->fClipdist();
				if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
					seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor);
				break;
			case kDudeModernCustom:
				seqSpawn(genDudeSeqStartId(actor), actor);
				break;
			}

#else
			actor->clipdist = getDudeInfo(nType + kDudeBase)->fClipdist();
			if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
				seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor);
#endif
			aiInitSprite(actor);
			actor->xspr.key = 0;
		}
		else if (actor->GetType() == kThingTNTBarrel) {
			actor->spr.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
		}

		gFX.fxSpawnActor(FX_29, actor->sector(), actor->spr.pos);
		sfxPlay3DSound(actor, 350, -1, 0);
		break;
	}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void PlayerBubble(DBloodActor* actor) // 10
{
	if (!actor) return;
	if (actor->IsPlayerActor())
	{
		auto pPlayer = getPlayer(actor);
		if (!pPlayer->bubbleTime)
			return;
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);

		auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
		double nDist = (actor->spr.scale.X * tex->GetDisplayWidth()) * (1. / 2);

		for (int i = 0; i < (pPlayer->bubbleTime >> 6); i++)
		{
			DVector2 pos = actor->spr.pos.XY() + actor->spr.Angles.Yaw.ToVector() * nDist;
			double z = bottom - RandomD(bottom - top, 8);
			auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23 + Random(3)), actor->sector(), DVector3(pos, z));
			if (pFX)
			{
				pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
				pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
				pFX->vel.Z = actor->vel.Z + Random2F(0x1aaaa);
			}
		}
		evPostActor(actor, 4, AF(PlayerBubble));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void EnemyBubble(DBloodActor* actor) // 11
{
	if (!actor) return;
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	auto tex = TexMan.GetGameTexture(actor->spr.spritetexture());
	double nDist = (actor->spr.scale.X * tex->GetDisplayWidth()) * (1. / 2);
	for (int i = 0; i < int(abs(actor->vel.Z) * 0.25); i++)
	{
		auto nAngle = RandomAngle();
		DVector2 pos = actor->spr.pos.XY() + nAngle.ToVector() * nDist;
		double z = bottom - RandomD(bottom - top, 8);

		auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23 + Random(3)), actor->sector(), DVector3(pos, z));
		if (pFX)
		{
			pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
			pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
			pFX->vel.Z = actor->vel.Z + Random2F(0x1aaaa);
		}
	}
	evPostActor(actor, 4, AF(EnemyBubble));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CounterCheck(sectortype* pSector) // 12
{
	if (!pSector || pSector->type != kSectorCounter) return;
	if (!pSector->hasX()) return;

	XSECTOR* pXSector = &pSector->xs();
	int nReq = pXSector->waitTimeA;
	int nType = pXSector->data;
	int nCount = 0;
	if (!nType || !nReq) return;

	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->GetType() == nType) nCount++;
	}

	if (nCount < nReq) {
		evPostSector(pSector, 5, kCmdCallback, nullptr);
		return;
	}
	else {
		//pXSector->waitTimeA = 0; //do not reset necessary objects counter to zero
		trTriggerSector(pSector, kCmdOn);
		pXSector->locked = 1; //lock sector, so it can be opened again later
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void FinishHim(DBloodActor* actor) // 13
{
	if (!actor) return;
	if (actor->IsPlayerActor() && playerSeqPlaying(getPlayer(actor), 16) && actor == getPlayer(myconnectindex)->GetActor())
		sndStartSample(3313, -1, 1, 0);
}

void fxBloodBits(DBloodActor* actor) // 14
{
	if (!actor) return;
	double ceilZ, floorZ;
	Collision floorColl, ceilColl;
	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->clipdist * 0.25, CLIPMASK0);
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->spr.pos.Z += floorZ - bottom;
	DAngle nAngle = RandomAngle();
	int nDist = Random(16);
	auto pos = actor->spr.pos + nAngle.ToVector() * nDist * 4;
	gFX.fxSpawnActor(FX_48, actor->sector(), DVector3(pos.XY(), actor->spr.pos.Z));
	if (actor->spr.Angles.Yaw == DAngle180)
	{
		int nChannel = 28 + (actor->GetIndex() & 2);    // this is a little stupid...
		sfxPlay3DSound(actor, 385, nChannel, 1);
	}
	if (Chance(0x5000))
	{
		auto pFX = gFX.fxSpawnActor(FX_36, actor->sector(), DVector3(pos.XY(), floorZ - 0.25));
		if (pFX)
			pFX->spr.Angles.Yaw = nAngle;
	}
	gFX.remove(actor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxTeslaAlt(DBloodActor* actor) // 15
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_49, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2F(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2F(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random2F(0x1aaaa);
	}
	evPostActor(actor, 3, AF(fxTeslaAlt));
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const int tommySleeveSnd[] = { 608, 609, 611 }; // unused?
static const int sawedOffSleeveSnd[] = { 610, 612 };

void fxBouncingSleeve(DBloodActor* actor) // 16
{
	if (!actor) return;
	double ceilZ, floorZ;
	Collision floorColl, ceilColl;

	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->clipdist * 0.25, CLIPMASK0);
	double top, bottom; 
	GetActorExtents(actor, &top, &bottom);
	actor->spr.pos.Z += floorZ - bottom;

	double veldiff = actor->vel.Z - actor->sector()->velFloor;

	if (actor->vel.Z == 0) sleeveStopBouncing(actor);
	else if (veldiff > 0)
	{
		auto vec4 = actFloorBounceVector(actor, veldiff, actor->sector(), FixedToFloat(0x9000));
		actor->vel = vec4.XYZ();

		if (actor->sector()->velFloor == 0 && abs(actor->vel.Z) < 2)
		{
			sleeveStopBouncing(actor);
			return;
		}

		int nChannel = 28 + (actor->GetIndex() & 2);

		// tommy sleeve
		if (actor->GetType() >= fxType(FX_37) && actor->GetType() <= fxType(FX_39))
		{
			Random(3);
			sfxPlay3DSound(actor, 608 + Random(2), nChannel, 1);

			// sawed-off sleeve
		}
		else 
		{
			sfxPlay3DSound(actor, sawedOffSleeveSnd[Random(2)], nChannel, 1);
		}
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void sleeveStopBouncing(DBloodActor* actor)
{
	actor->vel.Zero();
	if (actor->hasX()) seqKill(actor);
	sfxKill3DSound(actor, -1, -1);

	switch (actor->GetType()) {
	case fxType(FX_37):
	case fxType(FX_38):
	case fxType(FX_39):
		actor->spr.setspritetexture(aTexIds[kTexBULLETCASE]);
		break;
	case fxType(FX_40):
	case fxType(FX_41):
	case fxType(FX_42):
		actor->spr.setspritetexture(aTexIds[kTexSHELLCASE]);
		break;
	}

	actor->ChangeType(fxType(FX_51));
	actor->spr.scale = DVector2(0.15625, 0.15625);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void returnFlagToBase(DBloodActor* actor) // 17
{
	if (!actor) return;
	auto aOwner = actor->GetOwner();
	if (aOwner)
	{
		switch (actor->GetType())
		{
		case kItemFlagA:
			trTriggerSprite(aOwner, kCmdOn, aOwner);
			sndStartSample(8003, 255, 2, 0);
			gBlueFlagDropped = false;
			viewSetMessage("Blue Flag returned to base.");
			break;
		case kItemFlagB:
			trTriggerSprite(aOwner, kCmdOn, aOwner);
			sndStartSample(8002, 255, 2, 0);
			gRedFlagDropped = false;
			viewSetMessage("Red Flag returned to base.");
			break;
		}
	}
	evPostActor(actor, 0, AF(RemoveActor));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxPodBloodSpray(DBloodActor* actor) // 18
{
	if (!actor) return;
	DBloodActor* pFX;
	if (actor->GetType() == fxType(FX_53))
		pFX = gFX.fxSpawnActor(FX_53, actor->sector(), actor->spr.pos);
	else
		pFX = gFX.fxSpawnActor(FX_54, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->spr.Angles.Yaw = nullAngle;
		pFX->vel = actor->vel * (1./256);
	}
	evPostActor(actor, 6, AF(fxPodBloodSpray));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxPodBloodSplat(DBloodActor* actor) // 19
{
	if (!actor) return;
	double ceilZ, floorZ;
	Collision floorColl, ceilColl;

	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->clipdist * 0.25, CLIPMASK0);
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->spr.pos.Z += floorZ - bottom;
	DAngle nAngle = RandomAngle();
	int nDist = Random(16);
	auto pos = actor->spr.pos.XY() + nAngle.ToVector() * nDist * 4;

	if (actor->spr.Angles.Yaw == DAngle180 && actor->GetType() == fxType(FX_53))
	{
		int nChannel = 28 + (actor->GetIndex() & 2);
		assert(nChannel < 32);
		sfxPlay3DSound(actor, 385, nChannel, 1);
	}
	DBloodActor* pFX = NULL;
	if (actor->GetType() == fxType(FX_53) || actor->GetType() == kThingPodGreenBall)
	{
		if (Chance(0x500) || actor->GetType() == kThingPodGreenBall)
			pFX = gFX.fxSpawnActor(FX_55, actor->sector(), DVector3(pos, floorZ - 0.25));
		if (pFX)
			pFX->spr.Angles.Yaw = nAngle;
	}
	else
	{
		pFX = gFX.fxSpawnActor(FX_32, actor->sector(), DVector3(pos, floorZ - 0.25));
		if (pFX)
			pFX->spr.Angles.Yaw = nAngle;
	}
	gFX.remove(actor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void LeechStateTimer(DBloodActor* actor) // 20
{
	if (!actor) return;
	if (actor->spr.statnum == kStatThing && !(actor->spr.flags & 32)) {
		switch (actor->GetType()) {
		case kThingDroppedLifeLeech:
#ifdef NOONE_EXTENSIONS
		case kModernThingEnemyLifeLeech:
#endif
			actor->xspr.stateTimer = 0;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void sub_76A08(DBloodActor* actor, DBloodActor* actor2, DBloodPlayer* pPlayer) // ???
{
	double top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->spr.pos = actor2->spr.pos.plusZ(-(bottom - actor->spr.pos.Z));
	actor->spr.Angles.Yaw = actor2->spr.Angles.Yaw;
	ChangeActorSect(actor, actor2->sector());
	sfxPlay3DSound(actor2, 201, -1, 0);
	actor->vel.Zero();
	viewBackupSpriteLoc(actor);
	if (pPlayer)
	{
		playerResetInertia(pPlayer);
		pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropVoodooCb(DBloodActor* actor) // unused
{
	if (!actor) return;
	auto Owner = actor->GetOwner();
	if (Owner == nullptr)
	{
		evPostActor(actor, 0, AF(RemoveActor));
		return;
	}
	DBloodPlayer* pPlayer;
	if (Owner->IsPlayerActor())
		pPlayer = getPlayer(Owner);
	else
		pPlayer = nullptr;
	if (!pPlayer)
	{
		evPostActor(actor, 0, AF(RemoveActor));
		return;
	}
	actor->spr.Angles.Yaw = (Owner->spr.pos - actor->spr.pos).Angle();
	if (actor->hasX())
	{
		if (actor->xspr.data1 == 0)
		{
			evPostActor(actor, 0, AF(RemoveActor));
			return;
		}

		BloodStatIterator it(kStatDude);
		while (auto actor2 = it.Next())
		{
			auto nextactor = it.Peek();
			if (Owner == actor2)
				continue;
			if (actor2->hasX())
			{
				DBloodPlayer* pPlayer2;
				if (actor2->IsPlayerActor())
					pPlayer2 = getPlayer(actor2);
				else
					pPlayer2 = nullptr;

				if (actor2->xspr.health > 0 && (pPlayer2 || actor2->xspr.key == 0))
				{
					if (pPlayer2)
					{
						if (gGameOptions.nGameType == 1)
							continue;
						if (gGameOptions.nGameType == 3 && pPlayer->teamId == pPlayer2->teamId)
							continue;
						int t = 0x8000 / ClipLow(gNetPlayers - 1, 1);
						if (!powerupCheck(pPlayer2, kPwUpDeathMask))
							t += ((3200 - pPlayer2->armor[2]) << 15) / 3200;
						if (Chance(t) || nextactor == nullptr)
						{
							int nDmg = actDamageSprite(actor, actor2, kDamageSpirit, actor->xspr.data1 << 4);
							actor->xspr.data1 = ClipLow(actor->xspr.data1 - nDmg, 0);
							sub_76A08(actor2, actor, pPlayer2);
							evPostActor(actor, 0, AF(RemoveActor));
							return;
						}
					}
					else
					{
						int vd = 0x2666;
						switch (actor2->GetType())
						{
						case kDudeBoneEel:
						case kDudeBat:
						case kDudeRat:
						case kDudeTinyCaleb:
						case kDudeBeast:
							vd = 0x147;
							break;
						case kDudeZombieAxeBuried:
						case kDudePodGreen:
						case kDudeTentacleGreen:
						case kDudePodFire:
						case kDudeTentacleFire:
						case kDudePodMother:
						case kDudeTentacleMother:
						case kDudeCerberusTwoHead:
						case kDudeCerberusOneHead:
						case kDudeTchernobog:
						case kDudeBurningInnocent:
						case kDudeBurningCultist:
						case kDudeBurningZombieAxe:
						case kDudeBurningZombieButcher:
						case kDudeCultistReserved:
						case kDudeZombieAxeLaying:
						case kDudeInnocent:
						case kDudeBurningTinyCaleb:
						case kDudeBurningBeast:
							vd = 0;
							break;
						}
						if (vd && (Chance(vd) || nextactor == nullptr))
						{
							sub_76A08(actor2, actor, NULL);
							evPostActor(actor, 0, AF(RemoveActor));
							return;
						}
					}
				}
			}
		}
		actor->xspr.data1 = ClipLow(actor->xspr.data1 - 1, 0);
		evPostActor(actor, 0, AF(RemoveActor));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void callbackMissileBurst(DBloodActor* actor) // 22
{
	if (!actor) return;
	if (actor->spr.statnum != kStatProjectile) return;
	auto nAngVec = actor->vel.XY().Angle().ToVector();
	double nRadius = FixedToFloat(0x55555);

	for (int i = 0; i < 8; i++)
	{
		auto burstactor = actSpawnSprite(actor, kStatProjectile, actor->GetClass(), actor->GetType());
		if (!burstactor) break;

		burstactor->spr.shade = actor->spr.shade;
		burstactor->spr.setspritetexture(actor->spr.spritetexture());


		burstactor->spr.cstat = actor->spr.cstat;
		if ((burstactor->spr.cstat & CSTAT_SPRITE_BLOCK))
		{
			burstactor->spr.cstat &= ~CSTAT_SPRITE_BLOCK; // we don't want missiles impact each other
			evPostActor(burstactor, 100, AF(callbackMakeMissileBlocking)); // so set blocking flag a bit later
		}

		burstactor->spr.pal = actor->spr.pal;
		burstactor->clipdist = actor->clipdist * 0.25;
		burstactor->spr.flags = actor->spr.flags;
		burstactor->spr.scale = actor->spr.scale;
		burstactor->spr.scale *= 0.5;

		burstactor->spr.Angles.Yaw = actor->spr.Angles.Yaw + DAngle::fromDeg(actor->FloatVar("angleofs"));
		burstactor->SetOwner(actor);

		IFVIRTUALPTRNAME(burstactor, NAME_BloodMissileBase, initMissile) // note: delete the name if this get scriptified.
		{
			VMValue p[] = { burstactor, actor };
			VMCall(func, p, 2, nullptr, 0);
		}

		auto spAngVec = DAngle::fromBam(i << 29).ToVector().Rotated90CW() * nRadius;
		if (i & 1) spAngVec *= 0.5;
		burstactor->vel += DVector3(DVector2(0, spAngVec.X).Rotated(nAngVec.X, nAngVec.Y), spAngVec.Y);
		evPostActor(burstactor, 960, AF(RemoveActor));
	}
	evPostActor(actor, 0, AF(RemoveActor));
}


void callbackMakeMissileBlocking(DBloodActor* actor) // 23
{
	if (!actor || actor->spr.statnum != kStatProjectile) return;
	actor->spr.cstat |= CSTAT_SPRITE_BLOCK;
}

void fxPodGreenBloodSpray(DBloodActor* actor) // 24
{
	auto pFX = gFX.fxSpawnActor(FX_53, actor->sector(), actor->spr.pos);
	if (pFX)
	{
		pFX->spr.Angles.Yaw = nullAngle;
		pFX->vel = actor->vel / 256.;
	}

	evPostActor(actor, 6, AF(fxPodGreenBloodSpray));
}


END_BLD_NS
