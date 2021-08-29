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
RXBUCKET rxBucket[kChannelMax];
unsigned short bucketHead[kMaxID + 1];
static int bucketCount;
static std::multiset<EVENT> queue;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int GetBucketChannel(const RXBUCKET* pBucket)
{
	int nXIndex;
	switch (pBucket->type)
	{
	case SS_SECTOR:
		nXIndex = sector[pBucket->rxindex].extra;
		assert(nXIndex > 0);
		return xsector[nXIndex].rxID;

	case SS_WALL:
		nXIndex = wall[pBucket->rxindex].extra;
		assert(nXIndex > 0);
		return xwall[nXIndex].rxID;

	case SS_SPRITE:
		return pBucket->GetActor()? pBucket->GetActor()->x().rxID : 0;
	}

	Printf(PRINT_HIGH, "Unexpected rxBucket type %d", pBucket->type);
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int CompareChannels(const RXBUCKET* ref1, const RXBUCKET* ref2)
{
	return GetBucketChannel(ref1) - GetBucketChannel(ref2);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static RXBUCKET* SortGetMiddle(RXBUCKET* a1, RXBUCKET* a2, RXBUCKET* a3)
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

static void SortSwap(RXBUCKET* a, RXBUCKET* b)
{
	RXBUCKET t = *a;
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
	RXBUCKET* v144[32];
	int vc4[32];
	int v14 = 0;
	RXBUCKET* pArray = rxBucket;
	while (true)
	{
		while (nCount > 1)
		{
			if (nCount < 16)
			{
				for (int nDist = 3; nDist > 0; nDist -= 2)
				{
					for (RXBUCKET* pI = pArray + nDist; pI < pArray + nCount; pI += nDist)
					{
						for (RXBUCKET* pJ = pI; pJ > pArray && CompareChannels(pJ - nDist, pJ) > 0; pJ -= nDist)
						{
							SortSwap(pJ, pJ - nDist);
						}
					}
				}
				break;
			}
			RXBUCKET * middle = pArray + nCount / 2;
			if (nCount > 29)
			{
				RXBUCKET* first = pArray;
				RXBUCKET* last = pArray + nCount - 1;
				if (nCount > 42)
				{
					int eighth = nCount / 8;
					first = SortGetMiddle(first, first + eighth, first + eighth * 2);
					middle = SortGetMiddle(middle - eighth, middle, middle + eighth);
					last = SortGetMiddle(last - eighth * 2, last - eighth, last);
				}
				middle = SortGetMiddle(first, middle, last);
			}
			RXBUCKET pivot = *middle;
			RXBUCKET* first = pArray;
			RXBUCKET* last = pArray + nCount - 1;
			RXBUCKET* vbx = first;
			RXBUCKET* v4 = last;
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
			RXBUCKET* v2c = pArray + nCount;
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

void evInit()
{
	int nCount = 0;

	queue.clear();
	memset(rxBucket, 0, sizeof(rxBucket));

	// add all the tags to the bucket array
	for (int i = 0; i < numsectors; i++)
	{
		int nXSector = sector[i].extra;
		if (nXSector > 0 && xsector[nXSector].rxID > 0)
		{
			assert(nCount < kChannelMax);
			rxBucket[nCount].type = SS_SECTOR;
			rxBucket[nCount].rxindex = i;
			nCount++;
		}
	}

	for (int i = 0; i < numwalls; i++)
	{
		int nXWall = wall[i].extra;
		if (nXWall > 0 && xwall[nXWall].rxID > 0)
		{
			assert(nCount < kChannelMax);
			rxBucket[nCount].type = SS_WALL;
			rxBucket[nCount].rxindex = i;
			nCount++;
		}
	}

	BloodLinearSpriteIterator it;
	while (auto actor = it.Next())
	{
		if (actor->hasX() && actor->x().rxID > 0)
		{
				assert(nCount < kChannelMax);
				rxBucket[nCount].type = SS_SPRITE;
				rxBucket[nCount].rxindex = 0;
				rxBucket[nCount].actor = actor;
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

static bool evGetSourceState(int type, int nIndex, DBloodActor* actor)
{
	int nXIndex;

	switch (type)
	{
	case SS_SECTOR:
		nXIndex = sector[nIndex].extra;
		assert(nXIndex > 0 && nXIndex < kMaxXSectors);
		return xsector[nXIndex].state != 0;

	case SS_WALL:
		nXIndex = wall[nIndex].extra;
		assert(nXIndex > 0 && nXIndex < kMaxXWalls);
		return xwall[nXIndex].state != 0;

	case SS_SPRITE:
		if (actor->hasX())
			return actor->x().state != 0;
	}

	// shouldn't reach this point
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evSend(DBloodActor* actor, int nIndex, int nType, int rxId, COMMAND_ID command)
{
	switch (command) {
	case kCmdState:
		command = evGetSourceState(nType, nIndex, actor) ? kCmdOn : kCmdOff;
		break;
	case kCmdNotState:
		command = evGetSourceState(nType, nIndex, actor) ? kCmdOff : kCmdOn;
		break;
	default:
		break;
	}

	EVENT event;
	event.actor = actor;
	event.index_ = nIndex;
	event.type = nType;
	event.cmd = command;

	switch (rxId) {
	case kChannelTextOver:
		if (command >= kCmdNumberic) trTextOver(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid TextOver command by xobject #%d (object type %d)", nIndex, nType);
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
		else viewSetSystemMessage("Invalid Level-Exit# command by xobject #%d (object type %d)", nIndex, nType);
		return;
#endif
	case kChannelSetTotalSecrets:
		if (command >= kCmdNumberic) gSecretMgr.SetCount(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Total-Secrets command by xobject #%d (object type %d)", nIndex, nType);
		break;
	case kChannelSecretFound:
		if (actor != nullptr) nIndex = actor->GetIndex();	// the hint system needs the sprite index.
		if (SECRET_Trigger(nIndex + 65536 * nType)) // if the hint system knows this secret it's a retrigger - skip that.
		{
			if (command >= kCmdNumberic)
			{
				gSecretMgr.Found(command - kCmdNumberic);
				if (gGameOptions.nGameType == 0)
				{
					viewSetMessage(GStrings(FStringf("TXTB_SECRET%d", Random(2))), 0, MESSAGE_PRIORITY_SECRET);
				}
			}
			else viewSetSystemMessage("Invalid Trigger-Secret command by xobject #%d (object type %d)", nIndex, nType);
		}
		break;
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
			spritetype* pSprite = &actor->s();
			if (pSprite->flags & 32)
				continue;
			if (actor->hasX())
			{
				XSPRITE* pXSprite = &actor->x();
				if (pXSprite->rxID == rxId)
					trMessageSprite(actor->s().index, event);
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
			spritetype* pSprite = &actor->s();
			if (pSprite->flags & 32)
				continue;
			if (actor->hasX())
			{
				XSPRITE* pXSprite = &actor->x();
				if (pXSprite->rxID == rxId)
					trMessageSprite(actor->s().index, event);
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
				trMessageSprite(pPlayer->nSprite, event);
		}
		else if (rxId == kChannelAllPlayers) 
		{
			for (int i = 0; i < kMaxPlayers; i++) 
			{
				if ((pPlayer = getPlayerById(i)) != nullptr)
					trMessageSprite(pPlayer->nSprite, event);
			}
            return;
		}

	}
#endif
	for (int i = bucketHead[rxId]; i < bucketHead[rxId + 1]; i++) 
	{
		if (!event.isObject(rxBucket[i].type, rxBucket[i].actor, rxBucket[i].rxindex))
		{
			switch (rxBucket[i].type) 
			{
			case 6:
				trMessageSector(rxBucket[i].rxindex, event);
				break;
			case 0:
				trMessageWall(rxBucket[i].rxindex, event);
				break;
			case 3:
			{
				auto actor = rxBucket[i].GetActor();

				if (actor && actor->hasX() && !(actor->s().flags & 32))
				{
					XSPRITE* pXSprite = &actor->x();
					if (actor->x().rxID > 0)
						trMessageSprite(actor->s().index, event);
				}
				break;
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

void evPost_(int nIndex, int nType, unsigned int nDelta, COMMAND_ID command)
{
	assert(command != kCmdCallback);
	if (command == kCmdState) command = evGetSourceState(nType, nIndex, &bloodActors[nIndex]) ? kCmdOn : kCmdOff;
	else if (command == kCmdNotState) command = evGetSourceState(nType, nIndex, &bloodActors[nIndex]) ? kCmdOff : kCmdOn;
	EVENT evn = { &bloodActors[nIndex], (int16_t)nIndex, (int8_t)nType, (int8_t)command, 0, PlayClock + (int)nDelta };
	queue.insert(evn);
}

void evPost_(int nIndex, int nType, unsigned int nDelta, CALLBACK_ID callback)
{
	EVENT evn = {&bloodActors[nIndex], (int16_t)nIndex, (int8_t)nType, kCmdCallback, (int16_t)callback, PlayClock + (int)nDelta };
	queue.insert(evn);
}


void evPostActor(DBloodActor* actor, unsigned int nDelta, COMMAND_ID command)
{
	evPost_(actor->s().index, SS_SPRITE, nDelta, command);
}

void evPostActor(DBloodActor* actor, unsigned int nDelta, CALLBACK_ID callback)
{
	evPost_(actor->s().index, SS_SPRITE, nDelta, callback);
}

void evPostSector(int index, unsigned int nDelta, COMMAND_ID command)
{
	evPost_(index, SS_SECTOR, nDelta, command);
}

void evPostSector(int index, unsigned int nDelta, CALLBACK_ID callback)
{
	evPost_(index, SS_SECTOR, nDelta, callback);
}

void evPostWall(int index, unsigned int nDelta, COMMAND_ID command)
{
	evPost_(index, SS_WALL, nDelta, command);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evKill_(DBloodActor* actor, int index, int type)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->isObject(type, actor, index)) ev = queue.erase(ev);
		else ev++;
	}
}

void evKill_(DBloodActor* actor, int index, int type, CALLBACK_ID cb)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->isObject(type, actor, index) && ev->funcID == cb) ev = queue.erase(ev);
		else ev++;
	}
}

void evKillActor(DBloodActor* actor)
{
	evKill_(actor, 0, SS_SPRITE);
}

void evKillActor(DBloodActor* actor, CALLBACK_ID cb)
{
	evKill_(actor, 0, SS_SPRITE, cb);
}

void evKillWall(int wal)
{
	evKill_(nullptr, wal, SS_WALL);
}

void evKillSector(int sec)
{
	evKill_(nullptr, sec, SS_SECTOR);
}

// these have no target.
void evSendGame(int rxId, COMMAND_ID command)
{
	evSend(nullptr, 0, 0, rxId, command);
}

void evSendActor(DBloodActor* actor, int rxId, COMMAND_ID command)
{
	evSend(actor, 0, SS_SPRITE, rxId, command);
}

void evSendSector(int index, int rxId, COMMAND_ID command)
{
	evSend(nullptr, index, SS_SECTOR, rxId, command);
}

void evSendWall(int index, int rxId, COMMAND_ID command)
{
	evSend(nullptr, index, SS_WALL, rxId, command);
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
		if (event.type == SS_SPRITE)
		{
			// Don't call events on destroyed actors. Seems to happen occasionally.
			if (!event.actor || event.actor->s().statnum == kStatFree) continue;
		}

		if (event.cmd == kCmdCallback)
		{
			assert(event.funcID < kCallbackMax);
			assert(gCallback[event.funcID] != nullptr);
			gCallback[event.funcID](event.actor, event.index_);
		}
		else
		{
			switch (event.type)
			{
			case SS_SECTOR:
				trMessageSector(event.index_, event);
				break;
			case SS_WALL:
				trMessageWall(event.index_, event);
				break;
			case SS_SPRITE:
				trMessageSprite(event.actor->s().index, event);
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

FSerializer& Serialize(FSerializer& arc, const char* keyname, EVENT& w, EVENT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type);
		if (w.type != SS_SPRITE) arc("index", w.index_);
		else arc("index", w.actor);
		arc("command", w.cmd)
			("func", w.funcID)
			("prio", w.priority)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, RXBUCKET& w, RXBUCKET* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type);
		if (w.type != SS_SPRITE) arc("index", w.rxindex);
		else arc("index", w.actor);
		arc.EndObject();
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
