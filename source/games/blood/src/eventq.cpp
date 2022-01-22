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
#include "build.h"
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

static int CompareChannels(EventObject* ref1, EventObject* ref2)
{
	return GetBucketChannel(ref1) - GetBucketChannel(ref2);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static EventObject* SortGetMiddle(EventObject* a1, EventObject* a2, EventObject* a3)
{
	if (CompareChannels(a1, a2) > 0)
	{
		if (CompareChannels(a1, a3) > 0)
		{
			if (CompareChannels(a2, a3) > 0)
				return a2;
			return a3;
		}
		return a1;
	}
	else
	{
		if (CompareChannels(a1, a3) < 0)
		{
			if (CompareChannels(a2, a3) > 0)
				return a3;
			return a2;
		}
		return a1;
	}
}

static void SortSwap(EventObject* a, EventObject* b)
{
	EventObject t = *a;
	*a = *b;
	*b = t;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void SortRXBucket(int nCount)
{
	EventObject* v144[32];
	int vc4[32];
	int v14 = 0;
	EventObject* pArray = rxBucket;
	while (true)
	{
		while (nCount > 1)
		{
			if (nCount < 16)
			{
				for (int nDist = 3; nDist > 0; nDist -= 2)
				{
					for (EventObject* pI = pArray + nDist; pI < pArray + nCount; pI += nDist)
					{
						for (EventObject* pJ = pI; pJ > pArray && CompareChannels(pJ - nDist, pJ) > 0; pJ -= nDist)
						{
							SortSwap(pJ, pJ - nDist);
						}
					}
				}
				break;
			}
			EventObject* middle = pArray + nCount / 2;
			if (nCount > 29)
			{
				EventObject* first = pArray;
				EventObject* last = pArray + nCount - 1;
				if (nCount > 42)
				{
					int eighth = nCount / 8;
					first = SortGetMiddle(first, first + eighth, first + eighth * 2);
					middle = SortGetMiddle(middle - eighth, middle, middle + eighth);
					last = SortGetMiddle(last - eighth * 2, last - eighth, last);
				}
				middle = SortGetMiddle(first, middle, last);
			}
			EventObject pivot = *middle;
			EventObject* first = pArray;
			EventObject* last = pArray + nCount - 1;
			EventObject* vbx = first;
			EventObject* v4 = last;
			while (true)
			{
				while (vbx <= v4)
				{
					int nCmp = CompareChannels(vbx, &pivot);
					if (nCmp > 0)
						break;
					if (nCmp == 0)
					{
						SortSwap(vbx, first);
						first++;
					}
					vbx++;
				}
				while (vbx <= v4)
				{
					int nCmp = CompareChannels(v4, &pivot);
					if (nCmp < 0)
						break;
					if (nCmp == 0)
					{
						SortSwap(v4, last);
						last--;
					}
					v4--;
				}
				if (vbx > v4)
					break;
				SortSwap(vbx, v4);
				v4--;
				vbx++;
			}
			EventObject* v2c = pArray + nCount;
			int vt = int(min(vbx - first, first - pArray));
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&vbx[i - vt], &pArray[i]);
			}
			vt = int(min(last - v4, v2c - last - 1));
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&v2c[i - vt], &vbx[i]);
			}
			int vvsi = int(last - v4);
			int vvdi = int(vbx - first);
			if (vvsi >= vvdi)
			{
				vc4[v14] = vvsi;
				v144[v14] = v2c - vvsi;
				nCount = vvdi;
				v14++;
			}
			else
			{
				vc4[v14] = vvdi;
				v144[v14] = pArray;
				nCount = vvsi;
				pArray = v2c - vvsi;
				v14++;
			}
		}
		if (v14 == 0)
			return;
		v14--;
		pArray = v144[v14];
		nCount = vc4[v14];
	}
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
	SortRXBucket(nCount);
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

void evSend(EventObject& eob, int rxId, COMMAND_ID command)
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
		if (command >= kCmdNumberic) gSecretMgr.SetCount(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Total-Secrets command by %s", eob.description().GetChars());
		break;
	case kChannelSecretFound:
	{
		int nIndex = -1;
		if (eob.isActor() && eob.actor()) nIndex = eob.actor()->GetIndex() + 3 * 65536;	// the hint system needs the sprite index.
		else if (eob.isSector()) nIndex = eob.rawindex() + 6 * 65536;
		else if (eob.isWall()) nIndex = eob.rawindex();
		if (SECRET_Trigger(nIndex)) // if the hint system knows this secret it's a retrigger - skip that.
		{
			if (command >= kCmdNumberic)
			{
				gSecretMgr.Found(command - kCmdNumberic);
				if (gGameOptions.nGameType == 0)
				{
					viewSetMessage(GStrings(FStringf("TXTB_SECRET%d", Random(2))), nullptr, MESSAGE_PRIORITY_SECRET);
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
		PLAYER* pPlayer = NULL;
		if (playerRXRngIsFine(rxId))
		{
			if ((pPlayer = getPlayerById((rxId - kChannelPlayer7) + kMaxPlayers)) != nullptr)
				trMessageSprite(pPlayer->actor, event);
		}
		else if (rxId == kChannelAllPlayers)
		{
			for (int i = 0; i < kMaxPlayers; i++)
			{
				if ((pPlayer = getPlayerById(i)) != nullptr)
					trMessageSprite(pPlayer->actor, event);
			}
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

void evPost_(EventObject& eob, unsigned int nDelta, COMMAND_ID command)
{
	assert(command != kCmdCallback);
	if (command == kCmdState) command = evGetSourceState(eob) ? kCmdOn : kCmdOff;
	else if (command == kCmdNotState) command = evGetSourceState(eob) ? kCmdOff : kCmdOn;
	EVENT evn = { eob, (int8_t)command, 0, PlayClock + (int)nDelta };
	queue.insert(evn);
}

void evPost_(const EventObject& eob, unsigned int nDelta, CALLBACK_ID callback)
{
	EVENT evn = { eob, kCmdCallback, (int16_t)callback, PlayClock + (int)nDelta };
	queue.insert(evn);
}


void evPostActor(DBloodActor* actor, unsigned int nDelta, COMMAND_ID command)
{
	auto ev = EventObject(actor);
	evPost_(ev, nDelta, command);
}

void evPostActor(DBloodActor* actor, unsigned int nDelta, CALLBACK_ID callback)
{
	evPost_(EventObject(actor), nDelta, callback);
}

void evPostSector(sectortype* sect, unsigned int nDelta, COMMAND_ID command)
{
	auto ev = EventObject(sect);
	evPost_(ev, nDelta, command);
}

void evPostSector(sectortype* sect, unsigned int nDelta, CALLBACK_ID callback)
{
	evPost_(EventObject(sect), nDelta, callback);
}

void evPostWall(walltype* wal, unsigned int nDelta, COMMAND_ID command)
{
	auto ev = EventObject(wal);
	evPost_(ev, nDelta, command);
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

void evKill_(const EventObject& eob, CALLBACK_ID cb)
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

void evKillActor(DBloodActor* actor, CALLBACK_ID cb)
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

// these have no target.
void evSendGame(int rxId, COMMAND_ID command)
{
	auto ev = EventObject(nullptr);
	evSend(ev, rxId, command);
}

void evSendActor(DBloodActor* actor, int rxId, COMMAND_ID command)
{
	auto ev = EventObject(actor);
	evSend(ev, rxId, command);
}

void evSendSector(sectortype* sect, int rxId, COMMAND_ID command)
{
	auto ev = EventObject(sect);
	evSend(ev, rxId, command);
}

void evSendWall(walltype* wal, int rxId, COMMAND_ID command)
{
	auto ev = EventObject(wal);
	evSend(ev, rxId, command);
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
			assert(event.funcID < kCallbackMax);
			if (event.target.isActor()) gCallback[event.funcID](event.target.actor(), nullptr);
			else if (event.target.isSector()) gCallback[event.funcID](nullptr, event.target.sector());
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
