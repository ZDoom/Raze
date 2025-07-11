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

#include <random>

#include "build.h"

#include "blood.h"
#include "misc.h"
#include "d_net.h"

BEGIN_BLD_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

double GetWaveValue(unsigned int nPhase, int nType)
{
	switch (nType)
	{
	case 0:
		return 0.5 - 0.5 * BobVal((nPhase / 64.) + 512);
	case 1:
		return FixedToFloat(nPhase);
	case 2:
		return 1.0 - BobVal((nPhase / 128.) + 512);
	case 3:
		return BobVal(nPhase / 128.);
	}
	return nPhase;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SetSpriteState(DBloodActor* actor, int nState, DBloodActor* initiator)
{
	if ((actor->xspr.busy & 0xffff) == 0 && actor->xspr.state == nState)
		return 0;
	actor->xspr.busy = IntToFixed(nState);
	actor->xspr.state = nState;
	evKillActor(actor, initiator);
	if ((actor->spr.flags & kHitagRespawn) != 0 && actor->spr.inittype >= kDudeBase && actor->spr.inittype < kDudeMax)
	{
		actor->xspr.respawnPending = 3;
		evPostActor(actor, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
		return 1;
	}
	if (actor->xspr.restState != nState && actor->xspr.waitTime > 0)
		evPostActor(actor, (actor->xspr.waitTime * 120) / 10, actor->xspr.restState ? kCmdOn : kCmdOff, initiator);
	if (actor->xspr.txID)
	{
		if (actor->xspr.command != kCmdLink && actor->xspr.triggerOn && actor->xspr.state)
			evSendActor(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
		if (actor->xspr.command != kCmdLink && actor->xspr.triggerOff && !actor->xspr.state)
			evSendActor(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
	}
	return 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SetWallState(walltype* pWall, int nState, DBloodActor* initiator)
{
	auto pXWall = &pWall->xw();
	if ((pXWall->busy & 0xffff) == 0 && pXWall->state == nState)
		return 0;
	pXWall->busy = IntToFixed(nState);
	pXWall->state = nState;
	evKillWall(pWall, initiator);
	if (pXWall->restState != nState && pXWall->waitTime > 0)
		evPostWall(pWall, (pXWall->waitTime * 120) / 10, pXWall->restState ? kCmdOn : kCmdOff, initiator);
	if (pXWall->txID)
	{
		if (pXWall->command != kCmdLink && pXWall->triggerOn && pXWall->state)
			evSendWall(pWall, pXWall->txID, (COMMAND_ID)pXWall->command, initiator);
		if (pXWall->command != kCmdLink && pXWall->triggerOff && !pXWall->state)
			evSendWall(pWall, pXWall->txID, (COMMAND_ID)pXWall->command, initiator);
	}
	return 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SetSectorState(sectortype* pSector, int nState, DBloodActor* initiator)
{
	assert(pSector->hasX());
	auto pXSector = &pSector->xs();
	if ((pXSector->busy & 0xffff) == 0 && pXSector->state == nState)
		return 0;
	pXSector->busy = IntToFixed(nState);
	pXSector->state = nState;
	evKillSector(pSector, initiator);
	if (nState == 1)
	{
		if (pXSector->command != kCmdLink && pXSector->triggerOn && pXSector->txID)
			evSendSector(pSector, pXSector->txID, (COMMAND_ID)pXSector->command, initiator);
		if (pXSector->stopOn)
		{
			pXSector->stopOn = 0;
			pXSector->stopOff = 0;
		}
		else if (pXSector->reTriggerA)
			evPostSector(pSector, (pXSector->waitTimeA * 120) / 10, kCmdOff, initiator);
	}
	else
	{
		if (pXSector->command != kCmdLink && pXSector->triggerOff && pXSector->txID)
			evSendSector(pSector, pXSector->txID, (COMMAND_ID)pXSector->command, initiator);
		if (pXSector->stopOff)
		{
			pXSector->stopOn = 0;
			pXSector->stopOff = 0;
		}
		else if (pXSector->reTriggerB)
			evPostSector(pSector, (pXSector->waitTimeB * 120) / 10, kCmdOn, initiator);
	}
	return 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

TArray<BUSY> gBusy;

void AddBusy(sectortype* pSector, BUSYID a2, int nDelta)
{
	assert(nDelta != 0);
	for (auto& b : gBusy)
	{
		if (b.sect == pSector && b.type == a2)
		{
			b.delta = nDelta;
			return;
		}
	}
	if (VanillaMode() && gBusy.Size() == 128) return;
	BUSY b = { pSector, nDelta, nDelta > 0 ? 0 : 65536, a2 };
	gBusy.Push(b);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ReverseBusy(sectortype* pSector, BUSYID a2)
{
	for (auto& b : gBusy)
	{
		if (b.sect == pSector && b.type == a2)
		{
			b.delta = -b.delta;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

unsigned int GetSourceBusy(EVENT& a1)
{
	if (a1.isSector())
	{
		auto sect = a1.getSector();
		return sect->hasX() ? sect->xs().busy : 0;
	}
	else if (a1.isWall())
	{
		auto wal = a1.getWall();
		return wal->hasX() ? wal->xw().busy : 0;
	}
	else if (a1.isActor())
	{
		auto pActor = a1.getActor();
		return pActor && pActor->hasX() ? pActor->xspr.busy : false;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void LifeLeechOperate(DBloodActor* actor, EVENT event)
{
	switch (event.cmd) 
	{
	case kCmdSpritePush:
	{
		int nPlayer = actor->xspr.data4;
		if (nPlayer >= 0 && nPlayer < kMaxPlayers && playeringame[nPlayer])
		{
			DBloodPlayer* pPlayer = getPlayer(nPlayer);
			if (pPlayer->GetActor()->xspr.health > 0)
			{
				evKillActor(actor);
				pPlayer->ammoCount[8] = ClipHigh(pPlayer->ammoCount[8] + actor->xspr.data3, gAmmoInfo[8].max);
				pPlayer->hasWeapon[kWeapLifeLeech] = 1;
				if (pPlayer->curWeapon != kWeapLifeLeech)
				{
					if (!VanillaMode() && checkLitSprayOrTNT(pPlayer)) // if tnt/spray is actively used, do not switch weapon
						break;
					pPlayer->weaponState = 0;
					pPlayer->nextWeapon = kWeapLifeLeech;
				}
			}
		}
		break;
	}
	case kCmdSpriteProximity:
	{
		auto target = actor->GetTarget();
		if (target)
		{
			if (!actor->xspr.stateTimer)
			{
				if (target->spr.statnum == kStatDude && !(target->spr.flags & 32) && target->hasX())
				{
					double top, bottom;
					GetActorExtents(actor, &top, &bottom);
					DUDEINFO* pDudeInfo = getDudeInfo(target->spr.type);
					auto pos = target->spr.pos;
					auto nDist = (pos.XY() - actor->spr.pos.XY()).Length();
					if (nDist != 0 && cansee(DVector3(actor->spr.pos.XY(), top), actor->sector(), pos, target->sector()))
					{
						pos.XY() += target->vel.XY() * nDist * (65536. / 0x1aaaaa);
						auto angBak = actor->spr.Angles.Yaw;
						actor->spr.Angles.Yaw = (pos.XY() - actor->spr.pos.XY()).Angle();
						double tz = target->spr.pos.Z - (target->spr.scale.Y * pDudeInfo->aimHeight);
						auto dvec = DVector3(actor->spr.Angles.Yaw.ToVector(), ((tz - top - 1) / nDist) * (1. / 16.));
						int nMissileType = kMissileLifeLeechAltNormal + (actor->xspr.data3 ? 1 : 0);
						if (auto missile = actFireMissile(actor, 0, (top - actor->spr.pos.Z) - 1, dvec, nMissileType))
						{
							missile->SetOwner(actor);
							actor->xspr.stateTimer = 1;
							evPostActor(actor, (!actor->xspr.data3 ? 120 : 3 * 120) / 10, kCallbackLeechStateTimer);
							actor->xspr.data3 = ClipLow(actor->xspr.data3 - 1, 0);
							if (!VanillaMode()) // disable collisions so lifeleech doesn't do that weird bobbing
								missile->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
						}
						actor->spr.Angles.Yaw = angBak;
					}
				}
			}
		}
		return;
	}
	}
	actPostSprite(actor, kStatFree);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ActivateGenerator(DBloodActor*);

void OperateSprite(DBloodActor* actor, EVENT event)
{
	DBloodActor* initiator = event.initiator;
#ifdef NOONE_EXTENSIONS
	if (gModernMap && modernTypeOperateSprite(actor, event))
		return;
#endif

	switch (event.cmd) {
	case kCmdLock:
		actor->xspr.locked = 1;
		return;
	case kCmdUnlock:
		actor->xspr.locked = 0;
		return;
	case kCmdToggleLock:
		actor->xspr.locked = actor->xspr.locked ^ 1;
		return;
	}

	if (actor->spr.statnum == kStatDude && actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax) {

		switch (event.cmd) {
		case kCmdOff:
			SetSpriteState(actor, 0, initiator);
			break;
		case kCmdSpriteProximity:
			if (actor->xspr.state) break;
			[[fallthrough]];
		case kCmdOn:
		case kCmdSpritePush:
		case kCmdSpriteTouch:
			if (!actor->xspr.state) SetSpriteState(actor, 1, initiator);
			aiActivateDude(actor);
			break;
		}

		return;
	}


	switch (actor->spr.type) {
	case kTrapMachinegun:
		if (actor->xspr.health <= 0) break;
		switch (event.cmd) {
		case kCmdOff:
			if (!SetSpriteState(actor, 0, initiator)) break;
			seqSpawn(40, actor, -1);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			seqSpawn(38, actor, nMGunOpenClient);
			if (actor->xspr.data1 > 0)
				actor->xspr.data2 = actor->xspr.data1;
			break;
		}
		break;
	case kThingFallingRock:
		if (SetSpriteState(actor, 1, initiator))
			actor->spr.flags |= 7;
		break;
	case kThingWallCrack:
		if (SetSpriteState(actor, 0, initiator))
			actPostSprite(actor, kStatFree);
		break;
	case kThingCrateFace:
		if (SetSpriteState(actor, 0, initiator))
			actPostSprite(actor, kStatFree);
		break;
	case kTrapZapSwitchable:
		switch (event.cmd) {
		case kCmdOff:
			actor->xspr.state = 0;
			actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
			break;
		case kCmdOn:
			actor->xspr.state = 1;
			actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
			actor->spr.cstat |= CSTAT_SPRITE_BLOCK;
			break;
		case kCmdToggle:
			actor->xspr.state ^= 1;
			actor->spr.cstat ^= CSTAT_SPRITE_INVISIBLE;
			actor->spr.cstat ^= CSTAT_SPRITE_BLOCK;
			break;
		}
		break;
	case kTrapFlame:
		switch (event.cmd) {
		case kCmdOff:
			if (!SetSpriteState(actor, 0, initiator)) break;
			seqSpawn(40, actor, -1);
			sfxKill3DSound(actor, 0, -1);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			seqSpawn(38, actor, -1);
			sfxPlay3DSound(actor, 441, 0, 0);
			break;
		}
		break;
	case kSwitchPadlock:
		switch (event.cmd) {
		case kCmdOff:
			SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			seqSpawn(37, actor, -1);
			break;
		default:
			SetSpriteState(actor, actor->xspr.state ^ 1, initiator);
			if (actor->xspr.state) seqSpawn(37, actor, -1);
			break;
		}
		break;
	case kSwitchToggle:
		switch (event.cmd) {
		case kCmdOff:
			if (!SetSpriteState(actor, 0, initiator)) break;
			sfxPlay3DSound(actor, actor->xspr.data2, 0, 0);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			sfxPlay3DSound(actor, actor->xspr.data1, 0, 0);
			break;
		default:
			if (!SetSpriteState(actor, actor->xspr.state ^ 1, initiator)) break;
			if (actor->xspr.state) sfxPlay3DSound(actor, actor->xspr.data1, 0, 0);
			else sfxPlay3DSound(actor, actor->xspr.data2, 0, 0);
			break;
		}
		break;
	case kSwitchOneWay:
		switch (event.cmd) {
		case kCmdOff:
			if (!SetSpriteState(actor, 0, initiator)) break;
			sfxPlay3DSound(actor, actor->xspr.data2, 0, 0);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			sfxPlay3DSound(actor, actor->xspr.data1, 0, 0);
			break;
		default:
			if (!SetSpriteState(actor, actor->xspr.restState ^ 1, initiator)) break;
			if (actor->xspr.state) sfxPlay3DSound(actor, actor->xspr.data1, 0, 0);
			else sfxPlay3DSound(actor, actor->xspr.data2, 0, 0);
			break;
		}
		break;
	case kSwitchCombo:
		switch (event.cmd) {
		case kCmdOff:
			actor->xspr.data1--;
			if (actor->xspr.data1 < 0)
				actor->xspr.data1 += actor->xspr.data3;
			break;
		default:
			actor->xspr.data1++;
			if (actor->xspr.data1 >= actor->xspr.data3)
				actor->xspr.data1 -= actor->xspr.data3;
			break;
		}

		sfxPlay3DSound(actor, actor->xspr.data4, -1, 0);

		if (actor->xspr.command == kCmdLink && actor->xspr.txID > 0)
			evSendActor(actor, actor->xspr.txID, kCmdLink, initiator);

		if (actor->xspr.data1 == actor->xspr.data2)
			SetSpriteState(actor, 1, initiator);
		else
			SetSpriteState(actor, 0, initiator);

		break;
	case kMarkerDudeSpawn:
		if (gGameOptions.nMonsterSettings && actor->xspr.data1 >= kDudeBase && actor->xspr.data1 < kDudeMax)
		{
			auto spawned = actSpawnDude(actor, actor->xspr.data1, -1);
			if (spawned) {
				if (AllowedKillType(spawned)) Level.addKillCount();
				switch (actor->xspr.data1) {
				case kDudeBurningInnocent:
				case kDudeBurningCultist:
				case kDudeBurningZombieAxe:
				case kDudeBurningZombieButcher:
				case kDudeBurningTinyCaleb:
				case kDudeBurningBeast: {
					spawned->xspr.health = getDudeInfo(actor->xspr.data1)->startHealth << 4;
					spawned->xspr.burnTime = 10;
					spawned->SetTarget(nullptr);
					aiActivateDude(spawned);
					break;
				default:
					break;
				}
				}
			}
		}
		break;
	case kMarkerEarthQuake:
		actor->xspr.triggerOn = 0;
		actor->xspr.isTriggered = 1;
		SetSpriteState(actor, 1, initiator);
		for (int p = connecthead; p >= 0; p = connectpoint2[p]) 
		{
			auto vec = actor->spr.pos - getPlayer(p)->GetActor()->spr.pos;
			int nDist = int(vec.LengthSquared()) + 0x40000;
			getPlayer(p)->quakeEffect = DivScale(actor->xspr.data1, nDist, 16);
		}
		break;
	case kThingTNTBarrel:
		if (actor->spr.flags & kHitagRespawn) return;
		[[fallthrough]];
	case kThingArmedTNTStick:
	case kThingArmedTNTBundle:
	case kThingArmedSpray:
		actExplodeSprite(actor);
		break;
	case kTrapExploder:
		switch (event.cmd) {
		case kCmdOn:
			SetSpriteState(actor, 1, initiator);
			break;
		default:
			actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
			actExplodeSprite(actor);
			break;
		}
		break;
	case kThingArmedRemoteBomb:
		if (actor->spr.statnum != kStatRespawn) {
			if (event.cmd != kCmdOn) actExplodeSprite(actor);
			else {
				sfxPlay3DSound(actor, 454, 0, 0);
				evPostActor(actor, 18, kCmdOff, initiator);
			}
		}
		break;
	case kThingArmedProxBomb:
		if (actor->spr.statnum != kStatRespawn) {
			switch (event.cmd) {
			case kCmdSpriteProximity:
				if (actor->xspr.state) break;
				sfxPlay3DSound(actor, 452, 0, 0);
				evPostActor(actor, 30, kCmdOff, initiator);
				actor->xspr.state = 1;
				[[fallthrough]];
			case kCmdOn:
				sfxPlay3DSound(actor, 451, 0, 0);
				actor->xspr.Proximity = 1;
				break;
			default:
				actExplodeSprite(actor);
				break;
			}
		}
		break;
	case kThingDroppedLifeLeech:
		LifeLeechOperate(actor, event);
		break;
	case kGenTrigger:
	case kGenDripWater:
	case kGenDripBlood:
	case kGenMissileFireball:
	case kGenMissileEctoSkull:
	case kGenDart:
	case kGenBubble:
	case kGenBubbleMulti:
	case kGenSound:
		switch (event.cmd) {
		case kCmdOff:
			SetSpriteState(actor, 0, initiator);
			break;
		case kCmdRepeat:
			if (actor->spr.type != kGenTrigger) ActivateGenerator(actor);
			if (actor->xspr.txID) evSendActor(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);
			if (actor->xspr.busyTime > 0) {
				int nRand = Random2(actor->xspr.data1);
				evPostActor(actor, 120 * (nRand + actor->xspr.busyTime) / 10, kCmdRepeat, initiator);
			}
			break;
		default:
			if (!actor->xspr.state) {
				SetSpriteState(actor, 1, initiator);
				evPostActor(actor, 0, kCmdRepeat, initiator);
			}
			break;
		}
		break;
	case kSoundPlayer:
		if (gGameOptions.nGameType == 0)
		{
			DBloodPlayer* pPlayer = getPlayer(myconnectindex);

			if (pPlayer->GetActor()->xspr.health <= 0)
				break;
			pPlayer->restTime = 0;
		}
		sndStartSample(actor->xspr.data1, -1, 1, 0, CHANF_FORCE);
		break;
	case kThingObjectGib:
	case kThingObjectExplode:
	case kThingBloodBits:
	case kThingBloodChunks:
	case kThingZombieHead:
		switch (event.cmd) {
		case kCmdOff:
			if (!SetSpriteState(actor, 0, initiator)) break;
			actActivateGibObject(actor);
			break;
		case kCmdOn:
			if (!SetSpriteState(actor, 1, initiator)) break;
			actActivateGibObject(actor);
			break;
		default:
			if (!SetSpriteState(actor, actor->xspr.state ^ 1, initiator)) break;
			actActivateGibObject(actor);
			break;
		}
		break;
	default:
		switch (event.cmd) {
		case kCmdOff:
			SetSpriteState(actor, 0, initiator);
			break;
		case kCmdOn:
			SetSpriteState(actor, 1, initiator);
			break;
		default:
			SetSpriteState(actor, actor->xspr.state ^ 1, initiator);
			break;
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SetupGibWallState(walltype* pWall, XWALL* pXWall)
{
	walltype* pWall2 = NULL;
	if (pWall->twoSided())
		pWall2 = pWall->nextWall();
	if (pXWall->state)
	{
		pWall->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
		if (pWall2)
		{
			pWall2->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
			pWall->cstat &= ~CSTAT_WALL_MASKED;
			pWall2->cstat &= ~CSTAT_WALL_MASKED;
		}
		return;
	}
	bool bVector = pXWall->triggerVector != 0;
	pWall->cstat |= CSTAT_WALL_BLOCK;
	if (bVector)
		pWall->cstat |= CSTAT_WALL_BLOCK_HITSCAN;
	if (pWall2)
	{
		pWall2->cstat &= ~CSTAT_WALL_BLOCK;
		if (bVector)
			pWall2->cstat |= CSTAT_WALL_BLOCK_HITSCAN;
		pWall->cstat |= CSTAT_WALL_MASKED;
		pWall2->cstat |= CSTAT_WALL_MASKED;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateWall(walltype* pWall, EVENT event) 
{
	DBloodActor* initiator = event.initiator;
	auto pXWall = &pWall->xw();

	switch (event.cmd) {
	case kCmdLock:
		pXWall->locked = 1;
		return;
	case kCmdUnlock:
		pXWall->locked = 0;
		return;
	case kCmdToggleLock:
		pXWall->locked ^= 1;
		return;
	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap && modernTypeOperateWall(pWall, event))
		return;
#endif

	switch (pWall->type) {
	case kWallGib:
		bool bStatus;
		switch (event.cmd) {
		case kCmdOn:
		case kCmdWallImpact:
			bStatus = SetWallState(pWall, 1, initiator);
			break;
		case kCmdOff:
			bStatus = SetWallState(pWall, 0, initiator);
			break;
		default:
			bStatus = SetWallState(pWall, pXWall->state ^ 1, initiator);
			break;
		}

		if (bStatus) {
			SetupGibWallState(pWall, pXWall);
			if (pXWall->state) {
				auto vel = DVector3(100, 100, 250) * (1. / FRACUNIT);
				int nType = ClipRange(pXWall->data, 0, 31);
				if (nType > 0)
					GibWall(pWall, (GIBTYPE)nType, &vel);
			}
		}
		return;
	default:
		switch (event.cmd) {
		case kCmdOff:
			SetWallState(pWall, 0, initiator);
			break;
		case kCmdOn:
			SetWallState(pWall, 1, initiator);
			break;
		default:
			SetWallState(pWall, pXWall->state ^ 1, initiator);
			break;
		}
		return;
	}


}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SectorStartSound(sectortype* pSector, int nState)
{
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDecoration && actor->spr.type == kSoundSector && actor->hasX())
		{
			if (nState)
			{
				if (actor->xspr.data3)
					sfxPlay3DSound(actor, actor->xspr.data3, 0, 0);
			}
			else
			{
				if (actor->xspr.data1)
					sfxPlay3DSound(actor, actor->xspr.data1, 0, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SectorEndSound(sectortype* pSector, int nState)
{
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDecoration && actor->spr.type == kSoundSector && actor->hasX())
		{
			if (nState)
			{
				if (actor->xspr.data2)
					sfxPlay3DSound(actor, actor->xspr.data2, 0, 0);
			}
			else
			{
				if (actor->xspr.data4)
					sfxPlay3DSound(actor, actor->xspr.data4, 0, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void PathSound(sectortype* pSector, int nSound)
{
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDecoration && actor->spr.type == kSoundSector)
			sfxPlay3DSound(actor, nSound, 0, 0);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void TranslateSector(sectortype* pSector, double wave1, double wave2, const DVector2& pivot, const DVector2& pt1, DAngle ang1, const DVector2& pt2, DAngle ang2, bool bAllWalls)
{
	auto pt_w1 = interpolatedvalue(pt1, pt2, wave1);
	auto pt_w2 = interpolatedvalue(pt1, pt2, wave2);
	// the angles here may not be normalized before interpolation. Some setups contain multiple rotations!
	DAngle ptang_w1 = DAngle::fromDeg(interpolatedvalue(ang1.Degrees(), ang2.Degrees(), wave1));
	DAngle ptang_w2 = DAngle::fromDeg(interpolatedvalue(ang1.Degrees(), ang2.Degrees(), wave2));

	XSECTOR* pXSector = &pSector->xs();

	DVector2 offset = pt_w2 - pivot;
	DVector2 position = pt_w2 - pt_w1;
	auto angleofs = ptang_w2 - ptang_w1;

	auto rotatewall = [=](walltype* wal, DAngle angle, const DVector2& offset)
	{
		auto vec = wal->baseWall;
		if (angle.Degrees() != 0) 
			vec = rotatepoint(pivot, vec, angle);
		vec += offset;

		vertexscan(wal, [&](walltype* wal)
			{
				viewInterpolateWall(wal);
				wal->move(vec);
			});
	};
	
	if (bAllWalls)
	{
		for (auto& wal : pSector->walls)
		{
			rotatewall(&wal, ptang_w2, offset);
		}
	}
	else
	{
		for (auto& wal : pSector->walls)
		{
			auto p2Wall = wal.point2Wall();
			if (wal.cstat & CSTAT_WALL_MOVE_FORWARD)
			{
				rotatewall(&wal, ptang_w2, offset);

				if ((p2Wall->cstat & CSTAT_WALL_MOVE_MASK) == 0)
				{
					rotatewall(p2Wall, ptang_w2, offset);
				}
				continue;
			}
			if (wal.cstat & CSTAT_WALL_MOVE_BACKWARD)
			{
				rotatewall(&wal, -ptang_w2, -offset);

				if ((p2Wall->cstat & CSTAT_WALL_MOVE_MASK) == 0)
				{
					rotatewall(p2Wall, -ptang_w2, -offset);
				}
				continue;
			}
		}
	}
	
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		// allow to move markers by sector movements in game if flags 1 is added in editor.
		switch (actor->spr.statnum) {
		case kStatMarker:
		case kStatPathMarker:
#ifdef NOONE_EXTENSIONS
			if (!gModernMap || !(actor->spr.flags & kPhysMove)) continue;
#else
			continue;
#endif
			break;
		}

		if (actor->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD)
		{
			auto spot = rotatepoint(pivot, actor->basePoint.XY(), ptang_w2);
			viewBackupSpriteLoc(actor);
			actor->spr.pos.SetXY(spot + pt_w2 - pivot);
			actor->spr.Angles.Yaw += angleofs;

		}
		else if (actor->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE)
		{
			// fix Y arg in RotatePoint for reverse (green) moving sprites. (Original Blood bug?)
			DVector2 pivotDy(pivot.X, gModernMap ? pivot.Y : pivot.X);

			auto spot = rotatepoint(pivotDy, actor->basePoint.XY(), ptang_w2);
			viewBackupSpriteLoc(actor);
			actor->spr.pos.SetXY(spot - pt_w2 + pivot);
			actor->spr.Angles.Yaw += angleofs;
		}
		else if (pXSector->Drag)
		{
			double top, bottom;
			GetActorExtents(actor, &top, &bottom);
			double floorZ = getflorzofslopeptr(pSector, actor->spr.pos);
			if (!(actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) && floorZ <= bottom)
			{
				viewBackupSpriteLoc(actor);
				if (angleofs != nullAngle)
				{
					actor->spr.pos.SetXY(rotatepoint(pt_w1, actor->spr.pos.XY(), angleofs));
				}
				actor->spr.Angles.Yaw += angleofs;
				actor->spr.pos += position;
			}
		}
	}

#ifdef NOONE_EXTENSIONS
	// translate sprites near outside walls
	////////////////////////////////////////////////////////////

	if (gModernMap)
	{
		auto ptr = gSprNSect.GetSprPtr(sectindex(pSector));
		if (ptr)
		{
			for (auto& ac : *ptr)
			{
				if (ac == nullptr)
					continue;

				if (ac->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD)
				{
					auto spot = rotatepoint(pivot, ac->basePoint.XY(), ptang_w2);
					viewBackupSpriteLoc(ac);
					ac->spr.pos.SetXY(spot + pt_w2 - pivot);
					ac->spr.Angles.Yaw += angleofs;
					if (!VanillaMode() && ac->IsPlayerActor()) getPlayer(ac->spr.type - kDudePlayer1)->GetActor()->spr.Angles.Yaw += angleofs;
				}
				else if (ac->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE)
				{
					auto spot = rotatepoint(pivot, ac->basePoint.XY(), ptang_w2);
					viewBackupSpriteLoc(ac);
					ac->spr.pos.SetXY(spot - pt_w2 + pivot);
					ac->spr.Angles.Yaw += angleofs;
					if (!VanillaMode() && ac->IsPlayerActor()) getPlayer(ac->spr.type - kDudePlayer1)->GetActor()->spr.Angles.Yaw += angleofs;
				}
			}
		}
	}
	/////////////////////
#endif

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ZTranslateSector(sectortype* pSector, XSECTOR* pXSector, int a3, int a4)
{
	viewInterpolateSector(pSector);

	double dfz = pXSector->onFloorZ - pXSector->offFloorZ;
	double dcz = pXSector->onCeilZ - pXSector->offCeilZ;

#ifdef NOONE_EXTENSIONS
	// get pointer to sprites near outside walls before translation
	///////////////////////////////////////////////////////////////
	auto ptr1 = (gModernMap && (dfz || dcz))? gSprNSect.GetSprPtr(sectindex(pSector)) : nullptr;
#endif

	if (dfz != 0)
	{
		double old_Z = pSector->floorz;
		pSector->setfloorz(pXSector->offFloorZ + dfz * GetWaveValue(a3, a4));
		pSector->baseFloor = pSector->floorz;
		pSector->velFloor += (pSector->floorz - old_Z);

		BloodSectIterator it(pSector);
		while (auto actor = it.Next())
		{
			if (actor->spr.statnum == kStatMarker || actor->spr.statnum == kStatPathMarker)
				continue;
			double top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (actor->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD)
			{
				viewBackupSpriteLoc(actor);
				actor->spr.pos.Z += pSector->floorz - old_Z;
			}
			else if (actor->spr.flags & 2)
				actor->spr.flags |= 4;
			else if (old_Z <= bottom && !(actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK))
			{
				viewBackupSpriteLoc(actor);
				actor->spr.pos.Z += pSector->floorz - old_Z;
			}
		}


#ifdef NOONE_EXTENSIONS
		// translate sprites near outside walls (floor)
		////////////////////////////////////////////////////////////
		if (ptr1)
		{
			for(auto& ac : *ptr1)
			{
				if (ac && (ac->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD))
				{
					viewBackupSpriteLoc(ac);
					ac->spr.pos.Z += pSector->floorz - old_Z;
				}
			}
		}
		/////////////////////
#endif

	}

	if (dcz != 0)
	{
		double old_Z = pSector->ceilingz;
		pSector->setceilingz(pXSector->offCeilZ + dcz * GetWaveValue(a3, a4));
		pSector->baseCeil = pSector->ceilingz;
		pSector->velCeil += pSector->ceilingz - old_Z;

		BloodSectIterator it(pSector);
		while (auto actor = it.Next())
		{
			if (actor->spr.statnum == kStatMarker || actor->spr.statnum == kStatPathMarker)
				continue;
			if (actor->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE)
			{
				viewBackupSpriteLoc(actor);
				actor->spr.pos.Z += pSector->ceilingz - old_Z;
			}
		}


#ifdef NOONE_EXTENSIONS
		// translate sprites near outside walls (ceil)
		////////////////////////////////////////////////////////////
		if (ptr1)
		{
			for (auto& ac : *ptr1)
			{
				if (ac && (ac->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE))
				{
					viewBackupSpriteLoc(ac);
					ac->spr.pos.Z += pSector->ceilingz - old_Z;
				}
			}
		}

		/////////////////////
#endif

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* GetHighestSprite(sectortype* pSector, int nStatus, double* z)
{
	*z = pSector->floorz;
	DBloodActor* found = nullptr;

	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == nStatus || nStatus == kStatFree)
		{
			double top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (actor->spr.pos.Z - top > *z)
			{
				*z = actor->spr.pos.Z - top;
				found = actor;
			}
		}
	}
	return found;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DBloodActor* GetCrushedSpriteExtents(sectortype* pSector, double* pzTop, double* pzBot)
{
	assert(pzTop != NULL && pzBot != NULL);
	assert(pSector);
	DBloodActor* found = nullptr;
	double foundz = pSector->ceilingz;

	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDude || actor->spr.statnum == kStatThing)
		{
			double top, bottom;
			GetActorExtents(actor, &top, &bottom);
			if (foundz > top)
			{
				foundz = top;
				*pzTop = top;
				*pzBot = bottom;
				found = actor;
			}
		}
	}
	return found;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int VCrushBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	double dz1 = pXSector->onCeilZ - pXSector->offCeilZ;
	double z1 = pXSector->offCeilZ;
	if (dz1 != 0)
		z1 += dz1 * GetWaveValue(a2, nWave);

	double dz2 = pXSector->onFloorZ - pXSector->offFloorZ;
	double z2 = pXSector->offFloorZ;
	if (dz2 != 0)
		z2 += dz2 * GetWaveValue(a2, nWave);

	double highZ;
	if (GetHighestSprite(pSector, 6, &highZ) && z1 >= highZ)
		return 1;
	viewInterpolateSector(pSector);

	if (dz1 != 0)
		pSector->setceilingz(z1);
	if (dz2 != 0)
		pSector->setfloorz(z2);

	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int VSpriteBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	double dz1 = pXSector->onFloorZ - pXSector->offFloorZ;
	if (dz1 != 0)
	{
		BloodSectIterator it(pSector);
		while (auto actor = it.Next())
		{
			if (actor->spr.cstat & CSTAT_SPRITE_MOVE_FORWARD)
			{
				viewBackupSpriteLoc(actor);
				actor->spr.pos.Z = actor->basePoint.Z + dz1 * GetWaveValue(a2, nWave);
			}
		}
	}
	double dz2 = pXSector->onCeilZ - pXSector->offCeilZ;
	if (dz2 != 0)
	{
		BloodSectIterator it(pSector);
		while (auto actor = it.Next())
		{
			if (actor->spr.cstat & CSTAT_SPRITE_MOVE_REVERSE)
			{
				viewBackupSpriteLoc(actor);
				actor->spr.pos.Z = actor->basePoint.Z + dz2 * GetWaveValue(a2, nWave);
			}
		}
	}
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int VDoorBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	int vbp;
	if (pXSector->state)
		vbp = 65536 / ClipLow((120 * pXSector->busyTimeA) / 10, 1);
	else
		vbp = -65536 / ClipLow((120 * pXSector->busyTimeB) / 10, 1);

	double top, bottom;
	auto actor = GetCrushedSpriteExtents(pSector, &top, &bottom);
	if (actor && a2 > pXSector->busy)
	{
		assert(actor->hasX());
		if (pXSector->onCeilZ > pXSector->offCeilZ || pXSector->onFloorZ < pXSector->offFloorZ)
		{
			if (pXSector->interruptable)
			{
				if (pXSector->Crush)
				{
					if (actor->xspr.health <= 0)
						return 2;
					int nDamage;
					if (pXSector->data == 0)
						nDamage = 500;
					else
						nDamage = pXSector->data;
					actDamageSprite(actor, actor, kDamageFall, nDamage << 4);
				}
				a2 = ClipRange(a2 - (vbp / 2) * 4, 0, 65536);
			}
			else if (pXSector->Crush && actor->xspr.health > 0)
			{
				int nDamage;
				if (pXSector->data == 0)
					nDamage = 500;
				else
					nDamage = pXSector->data;
				actDamageSprite(actor, actor, kDamageFall, nDamage << 4);
				a2 = ClipRange(a2 - (vbp / 2) * 4, 0, 65536);
			}
		}
	}
	else if (actor && a2 < pXSector->busy)
	{
		assert(actor->hasX());
		if (pXSector->offCeilZ > pXSector->onCeilZ || pXSector->offFloorZ < pXSector->onFloorZ)
		{
			if (pXSector->interruptable)
			{
				if (pXSector->Crush)
				{
					if (actor->xspr.health <= 0)
						return 2;
					int nDamage;
					if (pXSector->data == 0)
						nDamage = 500;
					else
						nDamage = pXSector->data;
					actDamageSprite(actor, actor, kDamageFall, nDamage << 4);
				}
				a2 = ClipRange(a2 + (vbp / 2) * 4, 0, 65536);
			}
			else if (pXSector->Crush && actor->xspr.health > 0)
			{
				int nDamage;
				if (pXSector->data == 0)
					nDamage = 500;
				else
					nDamage = pXSector->data;
				actDamageSprite(actor, actor, kDamageFall, nDamage << 4);
				a2 = ClipRange(a2 + (vbp / 2) * 4, 0, 65536);
			}
		}
	}
	const int nWave = (pXSector->busy < a2) ? pXSector->busyWaveA : pXSector->busyWaveB;
	ZTranslateSector(pSector, pXSector, a2, (!VanillaMode() && nWave == 3) ? 0 : nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int HDoorBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	if (!pXSector->marker0 || !pXSector->marker1) return 0;
	auto marker0 = pXSector->marker0;
	auto marker1 = pXSector->marker1;
	TranslateSector(pSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), marker0->spr.pos.XY(), marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, marker1->spr.pos.XY(), marker1->spr.Angles.Yaw, pSector->type == kSectorSlide);
	ZTranslateSector(pSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int RDoorBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	int nWave;
	if (pXSector->busy < a2)
		nWave = pXSector->busyWaveA;
	else
		nWave = pXSector->busyWaveB;
	if (!pXSector->marker0) return 0;
	auto marker0 = pXSector->marker0;
	TranslateSector(pSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), marker0->spr.pos.XY(), marker0->spr.pos.XY(), nullAngle, marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, pSector->type == kSectorRotate);
	ZTranslateSector(pSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int StepRotateBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	if (!pXSector->marker0) return 0;
	auto marker0 = pXSector->marker0;

	auto ang1 = mapangle(pXSector->data);
	DAngle ang2;
	if (pXSector->busy < a2)
	{
		ang2 = ang1 + marker0->spr.Angles.Yaw;
		int nWave = pXSector->busyWaveA;
		TranslateSector(pSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), marker0->spr.pos.XY(), marker0->spr.pos.XY(), ang1, marker0->spr.pos.XY(), ang2, true);
	}
	else
	{
		 ang2 = ang1 - marker0->spr.Angles.Yaw;
		int nWave = pXSector->busyWaveB;
		TranslateSector(pSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), marker0->spr.pos.XY(), marker0->spr.pos.XY(), ang2, marker0->spr.pos.XY(), ang1, true);
	}
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		pXSector->data = ang2.Normalized360().Buildang();
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int GenSectorBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();
	pXSector->busy = a2;
	if (pXSector->command == kCmdLink && pXSector->txID)
		evSendSector(pSector, pXSector->txID, kCmdLink, initiator);
	if ((a2 & 0xffff) == 0)
	{
		SetSectorState(pSector, FixedToInt(a2), initiator);
		SectorEndSound(pSector, FixedToInt(a2));
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int PathBusy(sectortype* pSector, unsigned int a2, DBloodActor* initiator)
{
	assert(pSector && pSector->hasX());
	XSECTOR* pXSector = &pSector->xs();

	auto basepath = pXSector->basePath;
	auto marker0 = pXSector->marker0;
	auto marker1 = pXSector->marker1;
	if (!basepath || !marker0 || !marker1) return 0;

	int nWave = marker0->xspr.wave;
	TranslateSector(pSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), basepath->spr.pos.XY(), marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, marker1->spr.pos.XY(), marker1->spr.Angles.Yaw, true);
	ZTranslateSector(pSector, pXSector, a2, nWave);
	pXSector->busy = a2;
	if ((a2 & 0xffff) == 0)
	{
		evPostSector(pSector, (120 * marker1->xspr.waitTime) / 10, kCmdOn, initiator);
		pXSector->state = 0;
		pXSector->busy = 0;
		if (marker0->xspr.data4)
			PathSound(pSector, marker0->xspr.data4);
		pXSector->marker0 = marker1;
		pXSector->data = marker1->xspr.data1;
		return 3;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateDoor(sectortype* pSector, EVENT event, BUSYID busyWave)
{
	auto pXSector = &pSector->xs();
	switch (event.cmd) {
	case kCmdOff:
		if (!pXSector->busy) break;
		AddBusy(pSector, busyWave, -65536 / ClipLow((pXSector->busyTimeB * 120) / 10, 1));
		SectorStartSound(pSector, 1);
		break;
	case kCmdOn:
		if (pXSector->busy == 0x10000) break;
		AddBusy(pSector, busyWave, 65536 / ClipLow((pXSector->busyTimeA * 120) / 10, 1));
		SectorStartSound(pSector, 0);
		break;
	default:
		if (pXSector->busy & 0xffff) {
			if (pXSector->interruptable) {
				ReverseBusy(pSector, busyWave);
				pXSector->state = !pXSector->state;
			}
		}
		else {
			int nDelta;

			if (!pXSector->state) nDelta = 65536 / ClipLow((pXSector->busyTimeA * 120) / 10, 1);
			else nDelta = -65536 / ClipLow((pXSector->busyTimeB * 120) / 10, 1);

			AddBusy(pSector, busyWave, nDelta);
			SectorStartSound(pSector, pXSector->state);
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool SectorContainsDudes(sectortype* pSector)
{
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDude)
			return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void TeleFrag(DBloodActor* killer, sectortype* pSector)
{
	BloodSectIterator it(pSector);
	while (auto victim = it.Next())
	{
		if (victim->spr.statnum == kStatDude)
			actDamageSprite(killer, victim, kDamageExplode, 4000);
		else if (victim->spr.statnum == kStatThing)
			actDamageSprite(killer, victim, kDamageExplode, 4000);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateTeleport(sectortype* pSector)
{
	assert(pSector);
	auto pXSector = &pSector->xs();
	auto destactor = pXSector->marker0;
	assert(destactor != nullptr);
	assert(destactor->spr.statnum == kStatMarker);
	assert(destactor->spr.type == kMarkerWarpDest);
	assert(destactor->insector());
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		if (actor->spr.statnum == kStatDude)
		{
			DBloodPlayer* pPlayer;
			bool bPlayer = actor->IsPlayerActor();
			if (bPlayer)
				pPlayer = getPlayer(actor->spr.type - kDudePlayer1);
			else
				pPlayer = NULL;
			if (bPlayer || !SectorContainsDudes(destactor->sector()))
			{
				if (!(gGameOptions.uNetGameFlags & 2))
				{
					TeleFrag(pXSector->actordata, destactor->sector());
				}
				actor->spr.pos.SetXY(destactor->spr.pos.XY());
				actor->spr.pos.Z += destactor->sector()->floorz - pSector->floorz;
				actor->spr.Angles.Yaw = destactor->spr.Angles.Yaw;
				ChangeActorSect(actor, destactor->sector());
				sfxPlay3DSound(destactor, 201, -1, 0);
				actor->vel.Zero();
				actor->interpolated = false;
				viewBackupSpriteLoc(actor);
				if (pPlayer)
				{
					playerResetInertia(pPlayer);
					pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
					pPlayer->GetActor()->PrevAngles.Yaw = pPlayer->GetActor()->spr.Angles.Yaw = actor->spr.Angles.Yaw;
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

void OperatePath(sectortype* pSector, EVENT event)
{
	DBloodActor* actor;
	assert(pSector);
	auto pXSector = &pSector->xs();
	if (!pXSector->marker0) return;
	auto marker0 = pXSector->marker0;
	int nId = marker0->xspr.data2;

	BloodStatIterator it(kStatPathMarker);
	while ((actor = it.Next()))
	{
		if (actor->spr.type == kMarkerPath)
		{
			if (actor->xspr.data1 == nId)
				break;
		}
	}

	// trigger marker after it gets reached
#ifdef NOONE_EXTENSIONS
	if (gModernMap && marker0->xspr.state != 1)
		trTriggerSprite(pXSector->marker0, kCmdOn, event.initiator);
#endif

	if (actor == nullptr) {
		viewSetSystemMessage("Unable to find path marker with id #%d for path sector", nId);
		pXSector->state = 0;
		pXSector->busy = 0;
		return;
	}

	pXSector->marker1 = actor;
	pXSector->offFloorZ = marker0->spr.pos.Z;
	pXSector->onFloorZ = actor->spr.pos.Z;
	switch (event.cmd) {
	case kCmdOn:
		pXSector->state = 0;
		pXSector->busy = 0;
		AddBusy(pSector, BUSYID_7, 65536 / ClipLow((120 * marker0->xspr.busyTime) / 10, 1));
		if (marker0->xspr.data3) PathSound(pSector, marker0->xspr.data3);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OperateSector(sectortype* pSector, EVENT event)
{
	if (!pSector->hasX()) return;
	auto pXSector = &pSector->xs();
#ifdef NOONE_EXTENSIONS
	if (gModernMap && modernTypeOperateSector(pSector, event))
		return;
#endif

	switch (event.cmd) {
	case kCmdLock:
		pXSector->locked = 1;
		break;
	case kCmdUnlock:
		pXSector->locked = 0;
		break;
	case kCmdToggleLock:
		pXSector->locked ^= 1;
		break;
	case kCmdStopOff:
		pXSector->stopOn = 0;
		pXSector->stopOff = 1;
		break;
	case kCmdStopOn:
		pXSector->stopOn = 1;
		pXSector->stopOff = 0;
		break;
	case kCmdStopNext:
		pXSector->stopOn = 1;
		pXSector->stopOff = 1;
		break;
	default:
#ifdef NOONE_EXTENSIONS
		if (gModernMap && pXSector->unused1) break;
#endif
		switch (pSector->type) {
		case kSectorZMotionSprite:
			OperateDoor(pSector, event, BUSYID_1);
			break;
		case kSectorZMotion:
			OperateDoor(pSector, event, BUSYID_2);
			break;
		case kSectorSlideMarked:
		case kSectorSlide:
			OperateDoor(pSector, event, BUSYID_3);
			break;
		case kSectorRotateMarked:
		case kSectorRotate:
			OperateDoor(pSector, event, BUSYID_4);
			break;
		case kSectorRotateStep:
			switch (event.cmd) {
			case kCmdOn:
				pXSector->state = 0;
				pXSector->busy = 0;
				AddBusy(pSector, BUSYID_5, 65536 / ClipLow((120 * pXSector->busyTimeA) / 10, 1));
				SectorStartSound(pSector, 0);
				break;
			case kCmdOff:
				pXSector->state = 1;
				pXSector->busy = 65536;
				AddBusy(pSector, BUSYID_5, -65536 / ClipLow((120 * pXSector->busyTimeB) / 10, 1));
				SectorStartSound(pSector, 1);
				break;
			}
			break;
		case kSectorTeleport:
			OperateTeleport(pSector);
			break;
		case kSectorPath:
			OperatePath(pSector, event);
			break;
		default:
			if (!pXSector->busyTimeA && !pXSector->busyTimeB) 
			{
				DBloodActor* initiator = event.initiator;
				switch (event.cmd) {
				case kCmdOff:
					SetSectorState(pSector, 0, initiator);
					break;
				case kCmdOn:
					SetSectorState(pSector, 1, initiator);
					break;
				default:
					SetSectorState(pSector, pXSector->state ^ 1, initiator);
					break;
				}

			}
			else {

				OperateDoor(pSector, event, BUSYID_6);

			}

			break;
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitPath(sectortype* pSector, XSECTOR* pXSector)
{
	DBloodActor* actor = nullptr;
	assert(pSector);
	int nId = pXSector->data;

	BloodStatIterator it(kStatPathMarker);
	while ((actor = it.Next()))
	{
		if (actor->spr.type == kMarkerPath && actor->hasX())
		{
			if (actor->xspr.data1 == nId)
				break;
		}
	}

	if (actor == nullptr) {
		viewSetSystemMessage("Unable to find path marker with id #%d for path sector", nId);
		return;

	}

	pXSector->basePath = pXSector->marker0 = actor;
	if (pXSector->state)
		evPostSector(pSector, 0, kCmdOn, nullptr);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void LinkSector(sectortype* pSector, EVENT event)
{
	DBloodActor* initiator = event.initiator;
	auto pXSector = &pSector->xs();
	int nBusy = GetSourceBusy(event);
	switch (pSector->type) {
	case kSectorZMotionSprite:
		VSpriteBusy(pSector, nBusy, initiator);
		break;
	case kSectorZMotion:
		VDoorBusy(pSector, nBusy, initiator);
		break;
	case kSectorSlideMarked:
	case kSectorSlide:
		HDoorBusy(pSector, nBusy, initiator);
		break;
	case kSectorRotateMarked:
	case kSectorRotate:
		RDoorBusy(pSector, nBusy, initiator);
		break;
	default:
		pXSector->busy = nBusy;
		if ((pXSector->busy & 0xffff) == 0)
			SetSectorState(pSector, FixedToInt(nBusy), initiator);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void LinkSprite(DBloodActor* actor, EVENT event)
{
	DBloodActor* initiator = event.initiator;
	int nBusy = GetSourceBusy(event);

	switch (actor->spr.type) {
	case kSwitchCombo:
	{
		if (event.isActor())
		{
			auto actor2 = event.getActor();

			actor->xspr.data1 = actor2 && actor2->hasX() ? actor2->xspr.data1 : 0;
			if (actor->xspr.data1 == actor->xspr.data2)
				SetSpriteState(actor, 1, initiator);
			else
				SetSpriteState(actor, 0, initiator);
		}
	}
	break;
	default:
	{
		actor->xspr.busy = nBusy;
		if ((actor->xspr.busy & 0xffff) == 0)
			SetSpriteState(actor, FixedToInt(nBusy), initiator);
	}
	break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void LinkWall(walltype* pWall, EVENT& event)
{
	int nBusy = GetSourceBusy(event);
	pWall->xw().busy = nBusy;
	if ((pWall->xw().busy & 0xffff) == 0)
		SetWallState(pWall, FixedToInt(nBusy), event.initiator);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trTriggerSector(sectortype* pSector, int command, DBloodActor* initiator)
{
	auto pXSector = &pSector->xs();
	if (!pXSector->locked && !pXSector->isTriggered) {

		if (pXSector->triggerOnce)
			pXSector->isTriggered = 1;

		if (pXSector->decoupled && pXSector->txID > 0)
			evSendSector(pSector, pXSector->txID, (COMMAND_ID)pXSector->command, initiator);

		else {
			EVENT event;
			event.cmd = command;
			event.initiator = gModernMap? initiator : nullptr;
			OperateSector(pSector, event);
		}

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trTriggerWall(walltype* pWall, int command, DBloodActor* initiator)
{
	if (!pWall->hasX()) return;
	auto pXWall = &pWall->xw();
	if (!pXWall->locked && !pXWall->isTriggered) {

		if (pXWall->triggerOnce)
			pXWall->isTriggered = 1;

		if (pXWall->decoupled && pXWall->txID > 0)
			evSendWall(pWall, pXWall->txID, (COMMAND_ID)pXWall->command, initiator);

		else {
			EVENT event;
			event.cmd = command;
			event.initiator = gModernMap ? initiator : nullptr;
			OperateWall(pWall, event);
		}

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trTriggerSprite(DBloodActor* actor, int command, DBloodActor* initiator)
{
	if (!actor->xspr.locked && !actor->xspr.isTriggered) {

		if (actor->xspr.triggerOnce)
			actor->xspr.isTriggered = 1;

		if (actor->xspr.Decoupled && actor->xspr.txID > 0)
			evSendActor(actor, actor->xspr.txID, (COMMAND_ID)actor->xspr.command, initiator);

		else {
			EVENT event;
			event.cmd = command;
			event.initiator = gModernMap ? initiator : nullptr;
			OperateSprite(actor, event);
		}

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trMessageSector(sectortype* pSector, EVENT event)
{
	if (!pSector->hasX()) return;
	XSECTOR* pXSector = &pSector->xs();
	if (!pXSector->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock)
	{
		switch (event.cmd)
		{
		case kCmdLink:
			LinkSector(pSector, event);
			break;
#ifdef NOONE_EXTENSIONS
		case kCmdModernUse:
			modernTypeTrigger(OBJ_SECTOR, pSector, nullptr, nullptr, event);
			break;
#endif
		default:
			OperateSector(pSector, event);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trMessageWall(walltype* pWall, EVENT& event)
{
	assert(pWall->hasX());

	XWALL* pXWall = &pWall->xw();
	if (!pXWall->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock)
	{
		switch (event.cmd) {
		case kCmdLink:
			LinkWall(pWall, event);
			break;
#ifdef NOONE_EXTENSIONS
		case kCmdModernUse:
			modernTypeTrigger(OBJ_WALL, nullptr, pWall, nullptr, event);
			break;
#endif
		default:
			OperateWall(pWall, event);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trMessageSprite(DBloodActor* actor, EVENT event)
{
	if (actor->spr.statnum != kStatFree) {

		if (!actor->xspr.locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock)
		{
			switch (event.cmd)
			{
			case kCmdLink:
				LinkSprite(actor, event);
				break;
#ifdef NOONE_EXTENSIONS
			case kCmdModernUse:
				modernTypeTrigger(OBJ_SPRITE, 0, 0, actor, event);
				break;
#endif
			default:
				OperateSprite(actor, event);
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ProcessMotion(void)
{
	for (auto& sect : sector)
	{
		sectortype* pSector = &sect;

		if (!pSector->hasX()) continue;
		XSECTOR* pXSector = &pSector->xs();
		if (pXSector->bobSpeed != 0)
		{
			if (pXSector->bobAlways)
				pXSector->bobTheta += pXSector->bobSpeed;
			else if (pXSector->busy == 0)
				continue;
			else
				pXSector->bobTheta += MulScale(pXSector->bobSpeed, pXSector->busy, 16);
			double zoff = BobVal(pXSector->bobTheta) * pXSector->bobZRange;

			BloodSectIterator it(pSector);
			while (auto actor = it.Next())
			{
				if (actor->spr.cstat & CSTAT_SPRITE_MOVE_MASK)
				{
					viewBackupSpriteLoc(actor);
					actor->spr.pos.Z += zoff;
				}
			}
			if (pXSector->bobFloor)
			{
				double floorZ = pSector->floorz;
				viewInterpolateSector(pSector);
				pSector->setfloorz(pSector->baseFloor + zoff);

				BloodSectIterator itr(pSector);
				while (auto actor = itr.Next())
				{
					if (actor->spr.flags & 2)
						actor->spr.flags |= 4;
					else
					{
						double top, bottom;
						GetActorExtents(actor, &top, &bottom);
						if (bottom >= floorZ && (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
						{
							viewBackupSpriteLoc(actor);
							actor->spr.pos.Z += zoff;
						}
					}
				}
			}
			if (pXSector->bobCeiling)
			{
				double ceilZ = pSector->ceilingz;
				viewInterpolateSector(pSector);
				pSector->setceilingz(pSector->baseCeil + zoff);

				BloodSectIterator itr(pSector);
				while (auto actor = itr.Next())
				{
					double top, bottom;
					GetActorExtents(actor, &top, &bottom);
					if (top <= ceilZ && (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
					{
						viewBackupSpriteLoc(actor);
						actor->spr.pos.Z += zoff;
					}
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

void AlignSlopes(void)
{
	for (auto& sect : sector)
	{
		if (sect.slopewallofs > 0)
		{
			walltype* pWall = sect.walls.Data() + sect.slopewallofs;	// we must evade range checks here - some maps try to slope to a wall outside their own sector.
			if (pWall >= wall.Data() && pWall <= &wall.Last() &&  pWall->twoSided())
			{
				auto pNextSector = pWall->nextSector();

				auto pos = pWall->center();
				viewInterpolateSector(&sect);
				alignflorslope(&sect, DVector3(pos, getflorzofslopeptr(pNextSector, pos)));
				alignceilslope(&sect, DVector3(pos, getceilzofslopeptr(pNextSector, pos)));
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int(*gBusyProc[])(sectortype*, unsigned int, DBloodActor*) =
{
	VCrushBusy,
	VSpriteBusy,
	VDoorBusy,
	HDoorBusy,
	RDoorBusy,
	StepRotateBusy,
	GenSectorBusy,
	PathBusy
};

void trProcessBusy(void)
{
	for (auto& sect : sector)
	{
		sect.velCeil = sect.velFloor = 0;
	}
	for (int i = gBusy.Size()-1; i >= 0; i--)
	{
		int nStatus;
		int oldBusy = gBusy[i].busy;
		gBusy[i].busy = ClipRange(oldBusy + gBusy[i].delta * 4, 0, 65536);
#ifdef NOONE_EXTENSIONS
		if (!gModernMap || !gBusy[i].sect->xs().unused1) nStatus = gBusyProc[gBusy[i].type](gBusy[i].sect, gBusy[i].busy, nullptr);
		else nStatus = 3; // allow to pause/continue motion for sectors any time by sending special command
#else
		nStatus = gBusyProc[gBusy[i].type](gBusy[i].at0, gBusy[i].at8);
#endif
		switch (nStatus) {
		case 1:
			gBusy[i].busy = oldBusy;
			break;
		case 2:
			gBusy[i].busy = oldBusy;
			gBusy[i].delta = -gBusy[i].delta;
			break;
		case 3:
			gBusy[i] = gBusy.Last();
			gBusy.Pop();
			break;
		}
	}
	ProcessMotion();
	AlignSlopes();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void UpdateBasePoints(sectortype* pSector)
{
#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		// must set basepoint for outside sprites as well
		auto ptr1 = gSprNSect.GetSprPtr(sectindex(pSector));
		if (ptr1)
		{
			for (auto& ac : *ptr1)
				ac->basePoint = ac->spr.pos;
		}
	}
#endif

	for (auto& wal : pSector->walls)
	{
		wal.baseWall = wal.pos;
	}
	BloodSectIterator it(pSector);
	while (auto actor = it.Next())
	{
		actor->basePoint = actor->spr.pos;
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitGenerator(DBloodActor*);

void trInit(TArray<DBloodActor*>& actors)
{
#ifdef NOONE_EXTENSIONS
	if (gModernMap)
		gSprNSect.Init(); // collect sprites near outside walls
#endif

	gBusy.Clear();
	for (auto actor : actors)
	{
		if (!actor->exists()) continue;
		actor->spr.inittype = actor->spr.type;
		actor->basePoint = actor->spr.pos;
	}
	for (auto& wal : wall)
	{
		wal.baseWall = wal.pos;
		if (wal.hasX())
		{
			XWALL* pXWall = &wal.xw();
			if (pXWall->state)
				pXWall->busy = 65536;
		}
	}

	for (auto& sect : sector)
	{
		sectortype* pSector = &sect;
		pSector->baseFloor = pSector->floorz;
		pSector->baseCeil = pSector->ceilingz;
		if (pSector->hasX())
		{
			XSECTOR* pXSector = &pSector->xs();
			if (pXSector->state)
				pXSector->busy = 65536;
			switch (pSector->type)
			{
			case kSectorCounter:
#ifdef NOONE_EXTENSIONS 
				if (gModernMap)
					pXSector->triggerOff = false;
				else
#endif
					pXSector->triggerOnce = 1;
				evPostSector(pSector, 0, kCallbackCounterCheck);
				break;
			case kSectorZMotion:
			case kSectorZMotionSprite:
				ZTranslateSector(pSector, pXSector, pXSector->busy, 1);
				break;
			case kSectorSlideMarked:
			case kSectorSlide:
			{
				auto marker0 = pXSector->marker0;
				auto marker1 = pXSector->marker1;
				TranslateSector(pSector, 0, -1, marker0->spr.pos.XY(), marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, marker1->spr.pos.XY(), marker1->spr.Angles.Yaw, pSector->type == kSectorSlide);
				UpdateBasePoints(pSector);
				TranslateSector(pSector, 0, FixedToFloat(pXSector->busy), marker0->spr.pos.XY(), marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, marker1->spr.pos.XY(), marker1->spr.Angles.Yaw, pSector->type == kSectorSlide);
				ZTranslateSector(pSector, pXSector, pXSector->busy, 1);
				break;
			}
			case kSectorRotateMarked:
			case kSectorRotate:
			{
				auto marker0 = pXSector->marker0;
				TranslateSector(pSector, 0, -1, marker0->spr.pos.XY(), marker0->spr.pos.XY(), nullAngle, marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, pSector->type == kSectorRotate);
				UpdateBasePoints(pSector);
				TranslateSector(pSector, 0, FixedToFloat(pXSector->busy), marker0->spr.pos.XY(), marker0->spr.pos.XY(), nullAngle, marker0->spr.pos.XY(), marker0->spr.Angles.Yaw, pSector->type == kSectorRotate);
				ZTranslateSector(pSector, pXSector, pXSector->busy, 1);
				break;
			}

			case kSectorPath:
				InitPath(pSector, pXSector);
				break;
			default:
				break;
			}
		}
	}

	for (auto actor : actors)
	{
		if (actor->spr.statnum < kStatFree && actor->hasX())
		{
			if (actor->xspr.state)
				actor->xspr.busy = 65536;
			switch (actor->spr.type) {
			case kSwitchPadlock:
				actor->xspr.triggerOnce = 1;
				break;
#ifdef NOONE_EXTENSIONS
			case kModernRandom:
			case kModernRandom2:

				if (!gModernMap || actor->xspr.state == actor->xspr.restState) break;
				evPostActor(actor, (120 * actor->xspr.busyTime) / 10, kCmdRepeat, actor);
				if (actor->xspr.waitTime > 0)
					evPostActor(actor, (actor->xspr.waitTime * 120) / 10, actor->xspr.restState ? kCmdOn : kCmdOff, actor);
				break;
			case kModernSeqSpawner:
			case kModernObjDataAccumulator:
			case kModernDudeTargetChanger:
			case kModernEffectSpawner:
			case kModernWindGenerator:
				if (actor->xspr.state == actor->xspr.restState) break;
				evPostActor(actor, 0, kCmdRepeat, actor);
				if (actor->xspr.waitTime > 0)
					evPostActor(actor, (actor->xspr.waitTime * 120) / 10, actor->xspr.restState ? kCmdOn : kCmdOff, actor);
				break;
#endif
			case kGenTrigger:
			case kGenDripWater:
			case kGenDripBlood:
			case kGenMissileFireball:
			case kGenDart:
			case kGenBubble:
			case kGenBubbleMulti:
			case kGenMissileEctoSkull:
			case kGenSound:
				InitGenerator(actor);
				break;
			case kThingArmedProxBomb:
				actor->xspr.Proximity = 1;
				break;
			case kThingFallingRock:
				if (actor->xspr.state) actor->spr.flags |= 7;
				else actor->spr.flags &= ~7;
				break;
			}
			if (actor->xspr.Vector) actor->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;
			if (actor->xspr.Push) actor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1;
		}
	}

	evSendGame(kChannelLevelStart, kCmdOn);
#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		evSendGame(kChannelLevelStartRAZE, kCmdOn);
	}
#endif


	switch (gGameOptions.nGameType) {
	case 1:
		evSendGame(kChannelLevelStartCoop, kCmdOn);
		break;
	case 2:
		evSendGame(kChannelLevelStartMatch, kCmdOn);
		break;
	case 3:
		evSendGame(kChannelLevelStartMatch, kCmdOn);
		evSendGame(kChannelLevelStartTeamsOnly, kCmdOn);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void trTextOver(int nId)
{
	const char* pzMessage = currentLevel->GetMessage(nId);
	if (pzMessage)
		viewSetMessage(pzMessage, VanillaMode() ? nullptr : TEXTCOLOR_GOLD, MESSAGE_PRIORITY_INI);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitGenerator(DBloodActor* actor)
{
	assert(actor->hasX());
	switch (actor->spr.type) {
	case kGenTrigger:
		actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
		actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
		break;
	}
	if (actor->xspr.state != actor->xspr.restState && actor->xspr.busyTime > 0)
		evPostActor(actor, (120 * (actor->xspr.busyTime + Random2(actor->xspr.data1))) / 10, kCmdRepeat, actor);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ActivateGenerator(DBloodActor* actor)
{
	assert(actor->hasX());
	switch (actor->spr.type) {
	case kGenDripWater:
	case kGenDripBlood: {
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		actSpawnThing(actor->sector(), DVector3(actor->spr.pos.XY(), bottom), (actor->spr.type == kGenDripWater) ? kThingDripWater : kThingDripBlood);
		break;
	}
	case kGenSound:
		sfxPlay3DSound(actor, actor->xspr.data2, -1, 0);
		break;
	case kGenMissileFireball:
		switch (actor->xspr.data2) {
		case 0:
			FireballTrapSeqCallback(3, actor);
			break;
		case 1:
			seqSpawn(35, actor, nFireballTrapClient);
			break;
		case 2:
			seqSpawn(36, actor, nFireballTrapClient);
			break;
		}
		break;
	case kGenMissileEctoSkull:
		break;
	case kGenBubble:
	case kGenBubbleMulti: {
		double top, bottom;
		GetActorExtents(actor, &top, &bottom);
		gFX.fxSpawnActor((actor->spr.type == kGenBubble) ? FX_23 : FX_26, actor->sector(), DVector3(actor->spr.pos.XY(), top));
		break;
	}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void FireballTrapSeqCallback(int, DBloodActor* actor)
{
	if (actor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
		actFireMissile(actor, 0, 0, DVector3(0, 0, (actor->spr.cstat & CSTAT_SPRITE_YFLIP) ? 1 : -1), kMissileFireball);
	else
		actFireMissile(actor, 0, 0, DVector3(actor->spr.Angles.Yaw.ToVector(), 0), kMissileFireball);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void MGunFireSeqCallback(int, DBloodActor* actor)
{
	if (actor->xspr.data2 > 0 || actor->xspr.data1 == 0)
	{
		if (actor->xspr.data2 > 0)
		{
			actor->xspr.data2--;
			if (actor->xspr.data2 == 0)
				evPostActor(actor, 1, kCmdOff, actor);
		}
		DVector3 dv;
		dv.SetXY(actor->spr.Angles.Yaw.ToVector());
		dv.X += Random2F(1000, 14);
		dv.Y += Random2F(1000, 14);
		dv.Z = Random2F(1000, 14);
		actFireVector(actor, 0, 0, dv, kVectorBullet);
		sfxPlay3DSound(actor, 359, -1, 0);
	}
}

void MGunOpenSeqCallback(int, DBloodActor* actor)
{
	seqSpawn(39, actor, nMGunFireClient);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, BUSY& w, BUSY* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("index", w.sect)
			("type", w.type)
			("delta", w.delta)
			("busy", w.busy)
			.EndObject();
	}
	return arc;
}

void SerializeTriggers(FSerializer& arc)
{
	if (arc.BeginObject("triggers"))
	{
		arc("busy", gBusy)
			.EndObject();
	}
}

END_BLD_NS
