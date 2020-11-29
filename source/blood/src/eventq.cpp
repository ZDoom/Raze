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
#include "common_game.h"
#include "eventq.h"
#include "db.h"
#include "levels.h"
#include "triggers.h"
#include "view.h"
#include "nnexts.h"
#include "secrets.h"
#include "serializer.h"

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
		nXIndex = sector[pBucket->index].extra;
		assert(nXIndex > 0);
		return xsector[nXIndex].rxID;

	case SS_WALL:
		nXIndex = wall[pBucket->index].extra;
		assert(nXIndex > 0);
		return xwall[nXIndex].rxID;

	case SS_SPRITE:
		nXIndex = sprite[pBucket->index].extra;
		assert(nXIndex > 0);
		return xsprite[nXIndex].rxID;
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
			int vt = ClipHigh(vbx - first, first - pArray);
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&vbx[i - vt], &pArray[i]);
			}
			vt = ClipHigh(last - v4, v2c - last - 1);
			for (int i = 0; i < vt; i++)
			{
				SortSwap(&v2c[i - vt], &vbx[i]);
			}
			int vvsi = last - v4;
			int vvdi = vbx - first;
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
			rxBucket[nCount].index = i;
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
			rxBucket[nCount].index = i;
			nCount++;
		}
	}

	for (int i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			int nXSprite = sprite[i].extra;
			if (nXSprite > 0 && xsprite[nXSprite].rxID > 0)
			{
				assert(nCount < kChannelMax);
				rxBucket[nCount].type = SS_SPRITE;
				rxBucket[nCount].index = i;
				nCount++;
			}
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

static bool evGetSourceState(int type, int nIndex)
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
		nXIndex = sprite[nIndex].extra;
		assert(nXIndex > 0 && nXIndex < kMaxXSprites);
		return xsprite[nXIndex].state != 0;
	}

	// shouldn't reach this point
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evSend(int nIndex, int nType, int rxId, COMMAND_ID command)
{
	switch (command) {
	case kCmdState:
		command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
		break;
	case kCmdNotState:
		command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
		break;
	default:
		break;
	}

	EVENT event;
	event.index = nIndex;
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
		if (command >= kCmdNumberic) levelSetupSecret(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Total-Secrets command by xobject #%d (object type %d)", nIndex, nType);
		break;
	case kChannelSecretFound:
		SECRET_Trigger(nIndex + 65536 * nType);
		if (command >= kCmdNumberic) levelTriggerSecret(command - kCmdNumberic);
		else viewSetSystemMessage("Invalid Trigger-Secret command by xobject #%d (object type %d)", nIndex, nType);
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
		int nSprite;
		StatIterator it(kStatThing);
		while ((nSprite = it.NextIndex()) >= 0)
		{
			spritetype* pSprite = &sprite[nSprite];
			if (pSprite->flags & 32)
				continue;
			int nXSprite = pSprite->extra;
			if (nXSprite > 0)
			{
				XSPRITE* pXSprite = &xsprite[nXSprite];
				if (pXSprite->rxID == rxId)
					trMessageSprite(nSprite, event);
			}
		}
		return;
	}
	case kChannelTeamAFlagCaptured:
	case kChannelTeamBFlagCaptured:
	{
		int nSprite;
		StatIterator it(kStatItem);
		while ((nSprite = it.NextIndex()) >= 0)
		{
			spritetype* pSprite = &sprite[nSprite];
			if (pSprite->flags & 32)
				continue;
			int nXSprite = pSprite->extra;
			if (nXSprite > 0)
			{
				XSPRITE* pXSprite = &xsprite[nXSprite];
				if (pXSprite->rxID == rxId)
					trMessageSprite(nSprite, event);
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
			if ((pPlayer = getPlayerById((kChannelPlayer0 - kChannelPlayer7) + kMaxPlayers)) != nullptr)
				trMessageSprite(pPlayer->nSprite, event);
		}
		else if (rxId == kChannelAllPlayers) 
		{
			for (int i = 0; i < kMaxPlayers; i++) 
			{
				if ((pPlayer = getPlayerById(i)) != nullptr)
					trMessageSprite(pPlayer->nSprite, event);
			}
		}

	}
#endif
	for (int i = bucketHead[rxId]; i < bucketHead[rxId + 1]; i++) 
	{
		if (event.type != rxBucket[i].type || event.index != rxBucket[i].index) 
		{
			switch (rxBucket[i].type) 
			{
			case 6:
				trMessageSector(rxBucket[i].index, event);
				break;
			case 0:
				trMessageWall(rxBucket[i].index, event);
				break;
			case 3:
			{
				int nSprite = rxBucket[i].index;
				spritetype* pSprite = &sprite[nSprite];
				if (pSprite->flags & 32)
					continue;
				int nXSprite = pSprite->extra;
				if (nXSprite > 0)
				{
					XSPRITE* pXSprite = &xsprite[nXSprite];
					if (pXSprite->rxID > 0)
						trMessageSprite(nSprite, event);
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

void evPost(int nIndex, int nType, unsigned int nDelta, COMMAND_ID command)
{
	assert(command != kCmdCallback);
	if (command == kCmdState) command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
	else if (command == kCmdNotState) command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
	EVENT evn = { (int16_t)nIndex, (int8_t)nType, (int8_t)command, 0, gFrameClock + (int)nDelta };
	queue.insert(evn);
}

void evPost(int nIndex, int nType, unsigned int nDelta, CALLBACK_ID callback)
{
	EVENT evn = { (int16_t)nIndex, (int8_t)nType, kCmdCallback, (int16_t)callback, gFrameClock + (int)nDelta };
	queue.insert(evn);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evKill(int index, int type)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->index == index && ev->type == type) ev = queue.erase(ev);
		else ev++;
	}
}

void evKill(int index, int type, CALLBACK_ID cb)
{
	for (auto ev = queue.begin(); ev != queue.end();)
	{
		if (ev->index == index && ev->type == type && ev->funcID == cb) ev = queue.erase(ev);
		else ev++;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void evProcess(unsigned int time)
{
	while (queue.size() > 0 && time >= queue.begin()->priority)
	{
		EVENT event = *queue.begin();
		queue.erase(queue.begin());

		if (event.cmd == kCmdCallback)
		{
			assert(event.funcID < kCallbackMax);
			assert(gCallback[event.funcID] != nullptr);
			gCallback[event.funcID](event.index);
		}
		else
		{
			switch (event.type)
			{
			case SS_SECTOR:
				trMessageSector(event.index, event);
				break;
			case SS_WALL:
				trMessageWall(event.index, event);
				break;
			case SS_SPRITE:
				trMessageSprite(event.index, event);
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
		arc("index", w.index)
			("type", w.type)
			("command", w.cmd)
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
		arc("index", w.index)
			("type", w.type)
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
