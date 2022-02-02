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
#include "bloodactor.h"

BEGIN_BLD_NS


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlameLick(DBloodActor* actor, sectortype*) // 0
{
	if (!actor) return;
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	for (int i = 0; i < 3; i++)
	{
		int nDist = (actor->spr.xrepeat * (tileWidth(actor->spr.picnum) / 2)) >> 3;
		int nAngle = Random(2048);
		int dx = MulScale(nDist, Cos(nAngle), 30);
		int dy = MulScale(nDist, Sin(nAngle), 30);
		int x = actor->int_pos().X + dx;
		int y = actor->int_pos().Y + dy;
		int z = bottom - Random(bottom - top);
		auto pFX = gFX.fxSpawnActor(FX_32, actor->sector(), x, y, z, 0);
		if (pFX)
		{
			pFX->vel.X = actor->vel.X + Random2(-dx);
			pFX->vel.Y = actor->vel.Y + Random2(-dy);
			pFX->vel.Z = actor->vel.Z - Random(0x1aaaa);
		}
	}
	if (actor->xspr.burnTime > 0)
		evPostActor(actor, 5, kCallbackFXFlameLick);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Remove(DBloodActor* actor, sectortype*) // 1
{
	if (!actor) return;
	evKillActor(actor, kCallbackFXFlareSpark);
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

void FlareBurst(DBloodActor* actor, sectortype*) // 2
{
	if (!actor) return;
	int nAngle = getangle(actor->vel.X, actor->vel.Y);
	int nRadius = 0x55555;
	for (int i = 0; i < 8; i++)
	{
		auto spawnedactor = actSpawnSprite(actor, 5);
		spawnedactor->spr.picnum = 2424;
		spawnedactor->spr.shade = -128;
		spawnedactor->spr.xrepeat = spawnedactor->spr.yrepeat = 32;
		spawnedactor->spr.type = kMissileFlareAlt;
		spawnedactor->spr.clipdist = 2;
		spawnedactor->SetOwner(actor);
		int nAngle2 = (i << 11) / 8;
		int dx = 0;
		int dy = mulscale30r(nRadius, Sin(nAngle2));
		int dz = mulscale30r(nRadius, -Cos(nAngle2));
		if (i & 1)
		{
			dy >>= 1;
			dz >>= 1;
		}
		RotateVector(&dx, &dy, nAngle);
		spawnedactor->vel.X += dx;
		spawnedactor->vel.Y += dy;
		spawnedactor->vel.Z += dz;
		evPostActor(spawnedactor, 960, kCallbackRemove);
	}
	evPostActor(actor, 0, kCallbackRemove);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlareSpark(DBloodActor* actor, sectortype*) // 3
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_28, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random(0x1aaaa);
	}
	evPostActor(actor, 4, kCallbackFXFlareSpark);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxFlareSparkLite(DBloodActor* actor, sectortype*) // 4
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_28, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random(0x1aaaa);
	}
	evPostActor(actor, 12, kCallbackFXFlareSparkLite);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxZombieBloodSpurt(DBloodActor* actor, sectortype*) // 5
{
	if (!actor) return;
	assert(actor->hasX());
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	auto pFX = gFX.fxSpawnActor(FX_27, actor->sector(), actor->int_pos().X, actor->int_pos().Y, top, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2(0x11111);
		pFX->vel.Y = actor->vel.Y + Random2(0x11111);
		pFX->vel.Z = actor->vel.Z - 0x6aaaa;
	}
	if (actor->xspr.data1 > 0)
	{
		evPostActor(actor, 4, kCallbackFXZombieSpurt);
		actor->xspr.data1 -= 4;
	}
	else if (actor->xspr.data2 > 0)
	{
		evPostActor(actor, 60, kCallbackFXZombieSpurt);
		actor->xspr.data1 = 40;
		actor->xspr.data2--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxBloodSpurt(DBloodActor* actor, sectortype*) // 6
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_27, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->spr.ang = 0;
		pFX->vel.X = actor->vel.X >> 8;
		pFX->vel.Y = actor->vel.Y >> 8;
		pFX->vel.Z = actor->vel.Z >> 8;
	}
	evPostActor(actor, 6, kCallbackFXBloodSpurt);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxArcSpark(DBloodActor* actor, sectortype*) // 7
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_15, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2(0x10000);
		pFX->vel.Y = actor->vel.Y + Random2(0x10000);
		pFX->vel.Z = actor->vel.Z - Random(0x1aaaa);
	}
	evPostActor(actor, 3, kCallbackFXArcSpark);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxDynPuff(DBloodActor* actor, sectortype*) // 8
{
	if (!actor) return;
	if (actor->vel.Z)
	{
		int nDist = (actor->spr.xrepeat * (tileWidth(actor->spr.picnum) / 2)) >> 2;
		int x = actor->int_pos().X + MulScale(nDist, Cos(actor->spr.ang - 512), 30);
		int y = actor->int_pos().Y + MulScale(nDist, Sin(actor->spr.ang - 512), 30);
		int z = actor->int_pos().Z;
		auto pFX = gFX.fxSpawnActor(FX_7, actor->sector(), x, y, z, 0);
		if (pFX)
		{
			pFX->vel.X = actor->vel.X;
			pFX->vel.Y = actor->vel.Y;
			pFX->vel.Z = actor->vel.Z;
		}
	}
	evPostActor(actor, 12, kCallbackFXDynPuff);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Respawn(DBloodActor* actor, sectortype*) // 9
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
		evPostActor(actor, nTime, kCallbackRespawn);
		break;
	}
	case 2: {
		int nTime = MulScale(actGetRespawnTime(actor), 0x2000, 16);
		actor->xspr.respawnPending = 3;
		evPostActor(actor, nTime, kCallbackRespawn);
		break;
	}
	case 3: {
		assert(actor->spr.intowner != kStatRespawn);
		assert(actor->spr.intowner >= 0 && actor->spr.intowner < kMaxStatus);
		ChangeActorStat(actor, actor->spr.intowner);
		actor->spr.type = actor->spr.inittype;
		actor->SetOwner(nullptr);
		actor->spr.flags &= ~kHitagRespawn;
		actor->vel.X = actor->vel.Y = actor->vel.Z = 0;
		actor->xspr.respawnPending = 0;
		actor->xspr.burnTime = 0;
		actor->xspr.isTriggered = 0;
		if (actor->IsDudeActor())
		{
			int nType = actor->spr.type - kDudeBase;
			actor->set_int_pos(actor->basePoint);
			actor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1 | CSTAT_SPRITE_BLOCK_ALL;
#ifdef NOONE_EXTENSIONS
			if (!gModernMap || actor->xspr.sysData2 <= 0) actor->xspr.health = dudeInfo[actor->spr.type - kDudeBase].startHealth << 4;
			else actor->xspr.health = ClipRange(actor->xspr.sysData2 << 4, 1, 65535);

			switch (actor->spr.type) {
			default:
				actor->spr.clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
				if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
					seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor, -1);
				break;
			case kDudeModernCustom:
				seqSpawn(genDudeSeqStartId(actor), actor, -1);
				break;
			}

			// return dude to the patrol state
			if (gModernMap && actor->xspr.dudeFlag4) {
				actor->xspr.data3 = 0;
				actor->SetTarget(nullptr);
			}
#else
			actor->spr.clipdist = getDudeInfo(nType + kDudeBase)->clipdist;
			actor->xspr.health = getDudeInfo(nType + kDudeBase)->startHealth << 4;
			if (getSequence(getDudeInfo(nType + kDudeBase)->seqStartID))
				seqSpawn(getDudeInfo(nType + kDudeBase)->seqStartID, actor, -1);
#endif
			aiInitSprite(actor);
			actor->xspr.key = 0;
		}
		else if (actor->spr.type == kThingTNTBarrel) {
			actor->spr.cstat |= CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN;
			actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
		}

		gFX.fxSpawnActor(FX_29, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
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

void PlayerBubble(DBloodActor* actor, sectortype*) // 10
{
	if (!actor) return;
	if (actor->IsPlayerActor())
	{
		PLAYER* pPlayer = &gPlayer[actor->spr.type - kDudePlayer1];
		if (!pPlayer->bubbleTime)
			return;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		for (int i = 0; i < (pPlayer->bubbleTime >> 6); i++)
		{
			int nDist = (actor->spr.xrepeat * (tileWidth(actor->spr.picnum) / 2)) >> 2;
			int nAngle = Random(2048);
			int x = actor->int_pos().X + MulScale(nDist, Cos(nAngle), 30);
			int y = actor->int_pos().Y + MulScale(nDist, Sin(nAngle), 30);
			int z = bottom - Random(bottom - top);
			auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23 + Random(3)), actor->sector(), x, y, z, 0);
			if (pFX)
			{
				pFX->vel.X = actor->vel.X + Random2(0x1aaaa);
				pFX->vel.Y = actor->vel.Y + Random2(0x1aaaa);
				pFX->vel.Z = actor->vel.Z + Random2(0x1aaaa);
			}
		}
		evPostActor(actor, 4, kCallbackPlayerBubble);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void EnemyBubble(DBloodActor* actor, sectortype*) // 11
{
	if (!actor) return;
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	for (int i = 0; i < (abs(actor->vel.Z) >> 18); i++)
	{
		int nDist = (actor->spr.xrepeat * (tileWidth(actor->spr.picnum) / 2)) >> 2;
		int nAngle = Random(2048);
		int x = actor->int_pos().X + MulScale(nDist, Cos(nAngle), 30);
		int y = actor->int_pos().Y + MulScale(nDist, Sin(nAngle), 30);
		int z = bottom - Random(bottom - top);
		auto pFX = gFX.fxSpawnActor((FX_ID)(FX_23 + Random(3)), actor->sector(), x, y, z, 0);
		if (pFX)
		{
			pFX->vel.X = actor->vel.X + Random2(0x1aaaa);
			pFX->vel.Y = actor->vel.Y + Random2(0x1aaaa);
			pFX->vel.Z = actor->vel.Z + Random2(0x1aaaa);
		}
	}
	evPostActor(actor, 4, kCallbackEnemeyBubble);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void CounterCheck(DBloodActor*, sectortype* pSector) // 12
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
		if (actor->spr.type == nType) nCount++;
	}

	if (nCount < nReq) {
		evPostSector(pSector, 5, kCallbackCounterCheck);
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

void FinishHim(DBloodActor* actor, sectortype*) // 13
{
	if (!actor) return;
	if (actor->IsPlayerActor() && playerSeqPlaying(&gPlayer[actor->spr.type - kDudePlayer1], 16) && actor == gMe->actor)
		sndStartSample(3313, -1, 1, 0);
}

void fxBloodBits(DBloodActor* actor, sectortype*) // 14
{
	if (!actor) return;
	int ceilZ, floorZ;
	Collision floorColl, ceilColl;
	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->add_int_z(floorZ - bottom);
	int nAngle = Random(2048);
	int nDist = Random(16) << 4;
	int x = actor->int_pos().X + MulScale(nDist, Cos(nAngle), 28);
	int y = actor->int_pos().Y + MulScale(nDist, Sin(nAngle), 28);
	gFX.fxSpawnActor(FX_48, actor->sector(), x, y, actor->int_pos().Z, 0);
	if (actor->spr.ang == 1024)
	{
		int nChannel = 28 + (actor->GetIndex() & 2);    // this is a little stupid...
		sfxPlay3DSound(actor, 385, nChannel, 1);
	}
	if (Chance(0x5000))
	{
		auto pFX = gFX.fxSpawnActor(FX_36, actor->sector(), x, y, floorZ - 64, 0);
		if (pFX)
			pFX->spr.ang = nAngle;
	}
	gFX.remove(actor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxTeslaAlt(DBloodActor* actor, sectortype*) // 15
{
	if (!actor) return;
	auto pFX = gFX.fxSpawnActor(FX_49, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->vel.X = actor->vel.X + Random2(0x1aaaa);
		pFX->vel.Y = actor->vel.Y + Random2(0x1aaaa);
		pFX->vel.Z = actor->vel.Z - Random(0x1aaaa);
	}
	evPostActor(actor, 3, kCallbackFXTeslaAlt);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const int tommySleeveSnd[] = { 608, 609, 611 }; // unused?
static const int sawedOffSleeveSnd[] = { 610, 612 };

void fxBouncingSleeve(DBloodActor* actor, sectortype*) // 16
{
	if (!actor) return;
	int ceilZ, floorZ;
	Collision floorColl, ceilColl;

	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
	int top, bottom; GetActorExtents(actor, &top, &bottom);
	actor->add_int_z(floorZ - bottom);

	int zv = actor->vel.Z - actor->sector()->velFloor;

	if (actor->vel.Z == 0) sleeveStopBouncing(actor);
	else if (zv > 0) {
		actFloorBounceVector((int*)&actor->vel.X, (int*)&actor->vel.Y, &zv, actor->sector(), 0x9000);
		actor->vel.Z = zv;
		if (actor->sector()->velFloor == 0 && abs(actor->vel.Z) < 0x20000) {
			sleeveStopBouncing(actor);
			return;
		}

		int nChannel = 28 + (actor->GetIndex() & 2);

		// tommy sleeve
		if (actor->spr.type >= FX_37 && actor->spr.type <= FX_39) {
			Random(3);
			sfxPlay3DSound(actor, 608 + Random(2), nChannel, 1);

			// sawed-off sleeve
		}
		else {
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
	actor->vel.X = actor->vel.Y = actor->vel.Z = 0;
	if (actor->hasX()) seqKill(actor);
	sfxKill3DSound(actor, -1, -1);

	switch (actor->spr.type) {
	case FX_37:
	case FX_38:
	case FX_39:
		actor->spr.picnum = 2465;
		break;
	case FX_40:
	case FX_41:
	case FX_42:
		actor->spr.picnum = 2464;
		break;
	}

	actor->spr.type = FX_51;
	actor->spr.xrepeat = actor->spr.yrepeat = 10;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void returnFlagToBase(DBloodActor* actor, sectortype*) // 17
{
	if (!actor) return;
	auto aOwner = actor->GetOwner();
	if (aOwner)
	{
		switch (actor->spr.type)
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
	evPostActor(actor, 0, kCallbackRemove);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxPodBloodSpray(DBloodActor* actor, sectortype*) // 18
{
	if (!actor) return;
	DBloodActor* pFX;
	if (actor->spr.type == 53)
		pFX = gFX.fxSpawnActor(FX_53, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	else
		pFX = gFX.fxSpawnActor(FX_54, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0);
	if (pFX)
	{
		pFX->spr.ang = 0;
		pFX->vel.X = actor->vel.X >> 8;
		pFX->vel.Y = actor->vel.Y >> 8;
		pFX->vel.Z = actor->vel.Z >> 8;
	}
	evPostActor(actor, 6, kCallbackFXPodBloodSpray);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fxPodBloodSplat(DBloodActor* actor, sectortype*) // 19
{
	if (!actor) return;
	int ceilZ, floorZ;
	Collision floorColl, ceilColl;

	GetZRange(actor, &ceilZ, &ceilColl, &floorZ, &floorColl, actor->spr.clipdist, CLIPMASK0);
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->add_int_z(floorZ - bottom);
	int nAngle = Random(2048);
	int nDist = Random(16) << 4;
	int x = actor->int_pos().X + MulScale(nDist, Cos(nAngle), 28);
	int y = actor->int_pos().Y + MulScale(nDist, Sin(nAngle), 28);
	if (actor->spr.ang == 1024 && actor->spr.type == 53)
	{
		int nChannel = 28 + (actor->GetIndex() & 2);
		assert(nChannel < 32);
		sfxPlay3DSound(actor, 385, nChannel, 1);
	}
	DBloodActor* pFX = NULL;
	if (actor->spr.type == 53 || actor->spr.type == kThingPodGreenBall)
	{
		if (Chance(0x500) || actor->spr.type == kThingPodGreenBall)
			pFX = gFX.fxSpawnActor(FX_55, actor->sector(), x, y, floorZ - 64, 0);
		if (pFX)
			pFX->spr.ang = nAngle;
	}
	else
	{
		pFX = gFX.fxSpawnActor(FX_32, actor->sector(), x, y, floorZ - 64, 0);
		if (pFX)
			pFX->spr.ang = nAngle;
	}
	gFX.remove(actor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void LeechStateTimer(DBloodActor* actor, sectortype*) // 20
{
	if (!actor) return;
	if (actor->spr.statnum == kStatThing && !(actor->spr.flags & 32)) {
		switch (actor->spr.type) {
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

void sub_76A08(DBloodActor* actor, DBloodActor* actor2, PLAYER* pPlayer) // ???
{
	int top, bottom;
	GetActorExtents(actor, &top, &bottom);
	actor->set_int_pos({ actor2->int_pos().X, actor2->int_pos().Y, actor2->sector()->int_floorz() - (bottom - actor->int_pos().Z) });
	actor->spr.ang = actor2->spr.ang;
	ChangeActorSect(actor, actor2->sector());
	sfxPlay3DSound(actor2, 201, -1, 0);
	actor->vel.X = actor->vel.Y = actor->vel.Z = 0;
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

void DropVoodooCb(DBloodActor* actor, sectortype*) // unused
{
	if (!actor) return;
	auto Owner = actor->GetOwner();
	if (Owner == nullptr)
	{
		evPostActor(actor, 0, kCallbackRemove);
		return;
	}
	PLAYER* pPlayer;
	if (Owner->IsPlayerActor())
		pPlayer = &gPlayer[Owner->spr.type - kDudePlayer1];
	else
		pPlayer = nullptr;
	if (!pPlayer)
	{
		evPostActor(actor, 0, kCallbackRemove);
		return;
	}
	actor->spr.ang = getangle(Owner->int_pos().X - actor->int_pos().X, Owner->int_pos().Y - actor->int_pos().Y);
	if (actor->hasX())
	{
		if (actor->xspr.data1 == 0)
		{
			evPostActor(actor, 0, kCallbackRemove);
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
				PLAYER* pPlayer2;
				if (actor2->IsPlayerActor())
					pPlayer2 = &gPlayer[actor2->spr.type - kDudePlayer1];
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
							evPostActor(actor, 0, kCallbackRemove);
							return;
						}
					}
					else
					{
						int vd = 0x2666;
						switch (actor2->spr.type)
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
							evPostActor(actor, 0, kCallbackRemove);
							return;
						}
					}
				}
			}
		}
		actor->xspr.data1 = ClipLow(actor->xspr.data1 - 1, 0);
		evPostActor(actor, 0, kCallbackRemove);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void callbackCondition(DBloodActor* actor, sectortype*)
{
	if (actor->xspr.isTriggered) return;

	TRCONDITION const* pCond = &gCondition[actor->xspr.sysData1];
	for (unsigned i = 0; i < pCond->length; i++) {
		EVENT evn;
		evn.target = pCond->obj[i].obj;
		evn.cmd = pCond->obj[i].cmd;
		evn.funcID = kCallbackCondition;
		useCondition(actor, evn);
	}

	evPostActor(actor, actor->xspr.busyTime, kCallbackCondition);
	return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void(*gCallback[kCallbackMax])(DBloodActor*, sectortype*) =
{
	fxFlameLick,
	Remove,
	FlareBurst,
	fxFlareSpark,
	fxFlareSparkLite,
	fxZombieBloodSpurt,
	fxBloodSpurt,
	fxArcSpark,
	fxDynPuff,
	Respawn,
	PlayerBubble,
	EnemyBubble,
	CounterCheck,
	FinishHim,
	fxBloodBits,
	fxTeslaAlt,
	fxBouncingSleeve,
	returnFlagToBase,
	fxPodBloodSpray,
	fxPodBloodSplat,
	LeechStateTimer,
	DropVoodooCb, // unused
	#ifdef NOONE_EXTENSIONS
	callbackUniMissileBurst, // the code is in nnexts.cpp
	callbackMakeMissileBlocking, // the code is in nnexts.cpp
	callbackGenDudeUpdate, // the code is in nnexts.cpp
	callbackCondition,
	#endif
};

END_BLD_NS
