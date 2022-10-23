//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020 Christoph Oelckers

This file is part of Raze.

Raze is free software; you can redistribute it and/or
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

#include "ns.h"
#include <set>

#include "printf.h"
#include "blood.h"
#include "secrets.h"
#include "serializer.h"
#include "bloodactor.h"

BEGIN_BLD_NS


const int kMaxID = 1024;
EventObject rxBucket[kChannelMax];
unsigned short bucketHead[kMaxID + 1];
static int bucketCount;
static std::multiset<EVENT> queue;


FString EventObject::description() const
{
	if (isActor()) return FStringf("actor %d", ActorP->GetIndex()); // do not add a read barrier here!
	if (isSector()) return FStringf("sector %d", int(index >> 8));
	if (isWall()) return FStringf("wall %d", int(index >> 8));
	return FString("invalid object");
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int GetBucketChannel(EventObject* pBucket)
{
	if (pBucket->isSector())
	{
		auto pSector = pBucket->sector();
		assert(pSector->hasX());
		return pSector->xs().rxID;
	}
	if (pBucket->isWall())
	{
		auto pWall = pBucket->wall();
		assert(pWall->hasX());
		return pWall->xw().rxID;
	}
	if (pBucket->isActor())
	{
		auto pActor = pBucket->actor();
		return pActor ? pActor->xspr.rxID : 0;
	}

	Printf(PRINT_HIGH, "Unexpected rxBucket %s", pBucket->description().GetChars());
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int CompareChannels(const void* p1, const void* p2)
{
	return GetBucketChannel((EventObject*)p1) - GetBucketChannel((EventObject*)p2);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void createBucketHeads()
{
	int i, j;
	// create the list of header indices
	for (i = 0, j = 0; i < kMaxID; i++)
	{
		bucketHead[i] = (short)j;
		while (j < bucketCount && GetBucketChannel(&rxBucket[j]) == i)
		{
			j++;
		}
	}
	bucketHead[i] = (short)j;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evInit(TArray<DBloodActor*>& actors)
{
	int nCount = 0;

	queue.clear();
	memset(rxBucket, 0, sizeof(rxBucket));

	// add all the tags to the bucket array
	for (auto& sect : sector)
	{
		if (sect.hasX() && sect.xs().rxID > 0)
		{
			assert(nCount < kChannelMax);
			rxBucket[nCount] = EventObject(&sect);
			nCount++;
		}
	}

	for (auto& wal : wall)
	{
		if (wal.hasX() && wal.xw().rxID > 0)
		{
			assert(nCount < kChannelMax);
			rxBucket[nCount] = EventObject(&wal);
			nCount++;
		}
	}

	for (auto actor : actors)
	{
		if (actor->exists() && actor->hasX() && actor->xspr.rxID > 0)
		{
			assert(nCount < kChannelMax);
			rxBucket[nCount] = EventObject(actor);
			nCount++;
		}
	}
	qsort(rxBucket, nCount, sizeof(EventObject), CompareChannels);
	bucketCount = nCount;
	createBucketHeads();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool evGetSourceState(EventObject& eob)
{
	if (eob.isSector())
	{
		auto pSect = eob.sector();
		return  pSect->hasX() && pSect->xs().state != 0;
	}
	else if (eob.isWall())
	{
		auto pWall = eob.wall();
		return pWall->hasX() && pWall->xw().state != 0;
	}
	else if (eob.isActor())
	{
		auto actor = eob.actor();
		if (actor && actor->hasX())
			return actor->xspr.state != 0;
	}

	// shouldn't reach this point
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evSend(EventObject& eob, int rxId, COMMAND_ID command, DBloodActor* initiator)
{
	switch (command) {
	case kCmdState:
		command = evGetSourceState(eob) ? kCmdOn : kCmdOff;
		break;
	case kCmdNotState:
		command = evGetSourceState(eob) ? kCmdOff : kCmdOn;
		break;
	default:
		break;
	}

	EVENT event;
	event.target = eob;
	event.cmd = command;
	event.initiator = gModernMap? initiator : nullptr;

	switch (rxId) {
	case kChannelTextOver:
		if (command >= kCmdNumberic) trTextOver(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid TextOver command by %s", eob.description().GetChars());
		return;
	case kChannelLevelExitNormal:
		levelEndLevel(0);
		return;
	case kChannelLevelExitSecret:
		levelEndLevel(1);
		return;
#ifdef NOONE_EXTENSIONS
		// finished level and load custom level ¹ via numbered command.
	case kChannelModernEndLevelCustom:
		if (command >= kCmdNumberic) levelEndLevelCustom(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Level-Exit# command by %s", eob.description().GetChars());
		return;
#endif
	case kChannelSetTotalSecrets:
		if (command >= kCmdNumberic) Level.setSecrets(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Total-Secrets command by %s", eob.description().GetChars());
		break;
	case kChannelSecretFound:
	{
		int nIndex = -1;
		int nType = -1;
		if (eob.isActor() && eob.actor()) nIndex = eob.actor()->GetIndex(), nType = Secret_Sprite;
		else if (eob.isSector()) nIndex = eob.rawindex(), nType = Secret_Sector;
		else if (eob.isWall()) nIndex = eob.rawindex(), nType = Secret_Wall;
		if (SECRET_Trigger(nIndex, nType)) // if the hint system knows this secret it's a retrigger - skip that.
		{
			if (command >= kCmdNumberic)
			{
				int nType = command - kCmdNumberic;
				if (nType < 0)
					Printf(PRINT_HIGH | PRINT_NOTIFY, "Invalid secret type %d triggered.\n", nType);
				else
				{
					if (nType == 0)
						Level.addSecret(-1);
					else
						Level.addSuperSecret(-1);

					if (gGameOptions.nGameType == 0)
					{
						viewSetMessage(GStrings(FStringf("TXTB_SECRET%d", Random(2))), nullptr, MESSAGE_PRIORITY_SECRET);
					}
				}
			}
			else viewSetSystemMessage("Invalid Trigger-Secret command by %s", eob.description().GetChars());
		}
		break;
	}
	case kChannelRemoteBomb0:
	case kChannelRemoteBomb1:
	case kChannelRemoteBomb2:
	case kChannelRemoteBomb3:
	case kChannelRemoteBomb4:
	case kChannelRemoteBomb5:
	case kChannelRemoteBomb6:
	case kChannelRemoteBomb7:
	{
		BloodStatIterator it(kStatThing);
		while (auto actor = it.Next())
		{
			if (actor->spr.flags & 32)
				continue;
			if (actor->hasX())
			{
				if (actor->xspr.rxID == rxId)
					trMessageSprite(actor, event);
			}
		}
		return;
	}
	case kChannelTeamAFlagCaptured:
	case kChannelTeamBFlagCaptured:
	{
		BloodStatIterator it(kStatItem);
		while (auto actor = it.Next())
		{
			if (actor->spr.flags & 32)
				continue;
			if (actor->hasX())
			{
				if (actor->xspr.rxID == rxId)
					trMessageSprite(actor, event);
			}
		}
		return;
	}
	default:
		break;
	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		// allow to send commands on player sprites
		DBloodPlayer* pPlayer = NULL;
		if (playerRXRngIsFine(rxId))
		{
			if ((pPlayer = getPlayerById((rxId - kChannelPlayer7) + kMaxPlayers)) != nullptr)
			{
				if (command == kCmdEventKillFull)
					evKillActor(pPlayer->GetActor());
				else
					trMessageSprite(pPlayer->GetActor(), event);
			}
		}
		else if (rxId == kChannelAllPlayers)
		{
			for (int i = 0; i < kMaxPlayers; i++)
			{
				if ((pPlayer = getPlayerById(i)) != nullptr)
				{
					if (command == kCmdEventKillFull)
						evKillActor(pPlayer->GetActor());
					else
						trMessageSprite(pPlayer->GetActor(), event);
				}
			}
			return;
		}
		// send command on sprite which created the event sequence
		else if (rxId == kChannelEventCauser && event.initiator != nullptr)
		{
			DBloodActor* einitiator = event.initiator;
			if (!(einitiator->spr.flags & kHitagFree) && !(einitiator->spr.flags & kHitagRespawn))
			{
				if (command == kCmdEventKillFull)
					evKillActor(einitiator);
				else
					trMessageSprite(einitiator, event);
			}

			return;
		}
		else if (command == kCmdEventKillFull)
		{
			killEvents(rxId, command);
			return;
		}

	}
#endif
	for (int i = bucketHead[rxId]; i < bucketHead[rxId + 1]; i++)
	{
		auto eo = rxBucket[i];
		if (!event.event_isObject(eo))
		{
			if (eo.isSector())
			{
				trMessageSector(eo.sector(), event);
			}
			else if (eo.isWall())
			{
				trMessageWall(eo.wall(), event);
			}
			else if (eo.isActor())
			{
				auto actor = eo.actor();

				if (actor && actor->hasX() && !(actor->spr.flags & 32))
				{
					if (actor->xspr.rxID > 0)
						trMessageSprite(actor, event);
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

void evPost_(EventObject& eob, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator)
{
	assert(command != kCmdCallback);
	if (command == kCmdState) command = evGetSourceState(eob) ? kCmdOn : kCmdOff;
	else if (command == kCmdNotState) command = evGetSourceState(eob) ? kCmdOff : kCmdOn;
	EVENT evn = { eob, (int8_t)command, 0, PlayClock + (int)nDelta, MakeObjPtr(gModernMap ? initiator : nullptr) };
	queue.insert(evn);
}

void evPost_(const EventObject& eob, unsigned int nDelta, VMFunction* callback)
{
	EVENT evn = { eob, kCmdCallback, callback, PlayClock + (int)nDelta };
	queue.insert(evn);
}


void evPostActor(DBloodActor* actor, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator)
{
	auto ev = EventObject(actor);
	evPost_(ev, nDelta, command, initiator);
}

void evPostActor(DBloodActor* actor, unsigned int nDelta, VMFunction* callback)
{
	evPost_(EventObject(actor), nDelta, callback);
}

void evPostSector(sectortype* sect, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator)
{
	auto ev = EventObject(sect);
	evPost_(ev, nDelta, command, initiator);
}

void evPostWall(walltype* wal, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator)
{
	auto ev = EventObject(wal);
	evPost_(ev, nDelta, command, initiator);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evKill_(const EventObject& eob)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->event_isObject(eob)) ev = queue.erase(ev);
		else ev++;
	}
}

void evKill_(const EventObject& eob, DBloodActor* initiator)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->event_isObject(eob) && ev->initiator.ForceGet() == initiator) ev = queue.erase(ev);
		else ev++;
	}
}

void evKill_(const EventObject& eob, VMFunction* cb)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->event_isObject(eob) && ev->funcID == cb) ev = queue.erase(ev);
		else ev++;
	}
}

void evKillActor(DBloodActor* actor)
{
	evKill_(EventObject(actor));
}

void evKillActor(DBloodActor* actor, DBloodActor* initiator)
{
	if (!gModernMap)
		evKill_(EventObject(actor));
	else
		evKill_(EventObject(actor), initiator);
}

void evKillActor(DBloodActor* actor, VMFunction* cb)
{
	evKill_(EventObject(actor));
}

void evKillWall(walltype* wal)
{
	evKill_(EventObject(wal));
}

void evKillSector(sectortype* sec)
{
	evKill_(EventObject(sec));
}

void evKillWall(walltype* wal, DBloodActor* initiator)
{
	evKill_(EventObject(wal), initiator);
}

void evKillSector(sectortype* sec, DBloodActor* initiator)
{
	evKill_(EventObject(sec), initiator);
}

// these have no target.
void evSendGame(int rxId, COMMAND_ID command, DBloodActor* initiator = nullptr)
{
	auto ev = EventObject(nullptr);
	evSend(ev, rxId, command, initiator);
}

void evSendActor(DBloodActor* actor, int rxId, COMMAND_ID command, DBloodActor* initiator = nullptr)
{
	auto ev = EventObject(actor);
	evSend(ev, rxId, command, initiator);
}

void evSendSector(sectortype* sect, int rxId, COMMAND_ID command, DBloodActor* initiator = nullptr)
{
	auto ev = EventObject(sect);
	evSend(ev, rxId, command, initiator);
}

void evSendWall(walltype* wal, int rxId, COMMAND_ID command, DBloodActor* initiator = nullptr)
{
	auto ev = EventObject(wal);
	evSend(ev, rxId, command, initiator);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evProcess(unsigned int time)
{
	while (queue.size() > 0 && (int)time >= queue.begin()->priority)
	{
		EVENT event = *queue.begin();
		queue.erase(queue.begin());
		if (event.target.isActor())
		{
			// Don't call events on destroyed actors. Seems to happen occasionally.
			if (!event.target.actor() || event.target.actor()->spr.statnum == kStatFree) continue;
		}

		if (event.cmd == kCmdCallback)
		{
			// Except for CounterCheck all other callbacks are for actors only.
			if (event.target.isActor())
			{
				callActorFunction(event.funcID, event.target.actor());
			}
			// this is the only one, so no further checks for now.
			else if (event.target.isSector()) CounterCheck(event.target.sector());
			// no case for walls defined here.
		}
		else
		{
			if (event.target.isActor()) trMessageSprite(event.target.actor(), event);
			else if (event.target.isSector()) trMessageSector(event.target.sector(), event);
			else if (event.target.isWall()) trMessageWall(event.target.wall(), event);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, EventObject& w, EventObject* def)
{
	if (arc.BeginObject(keyname))
	{
		int type = w.isActor() ? 0 : w.isSector() ? 1 : 2;
		arc("type", type);
		switch (type)
		{
		case 0:
		{
			DBloodActor* a = arc.isWriting() ? w.actor() : nullptr;
			arc("actor", a);
			if (arc.isReading()) w = EventObject(a);
			break;
		}
		case 1:
		{
			auto s = arc.isWriting() ? w.sector() : nullptr;
			arc("sector", s);
			if (arc.isReading()) w = EventObject(s);
			break;
		}
		case 2:
		{
			auto s = arc.isWriting() ? w.wall() : nullptr;
			arc("wall", s);
			if (arc.isReading()) w = EventObject(s);
			break;
		}
		}
		arc.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, EVENT& w, EVENT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("target", w.target)
			("command", w.cmd)
			("func", w.funcID)
			("prio", w.priority)
			("initiator",w.initiator)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeEvents(FSerializer& arc)
{
	if (arc.BeginObject("events"))
	{
		arc("bucketcount", bucketCount)
			.Array("buckets", rxBucket, bucketCount)
			.Array("buckethead", bucketHead, countof(bucketHead));

		int numEvents = (int)queue.size();
		arc("eventcount", numEvents);
		if (arc.BeginArray("events"))
		{
			if (arc.isReading())
			{
				queue.clear();
				EVENT ev;
				for (int i = 0; i < numEvents; i++)
				{
					arc(nullptr, ev);
					queue.insert(ev);
				}
			}
			else
			{
				for (auto item : queue)
				{
					arc(nullptr, item);
				}
			}
			arc.EndArray();
		}
		arc.EndObject();
	}
}

END_BLD_NS
