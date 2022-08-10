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
#pragma once
#include "callback.h"

BEGIN_BLD_NS

// Object storage helper that packs the value into 8 bytes. This depends on pointer alignment being a multiple of 8 bytes for actors
class EventObject
{
	enum EType
	{
		Actor = 0,
		Sector = 1,
		Wall = 2
	};
	union
	{
		DBloodActor* ActorP;
		uint64_t index;
	};

public:
	EventObject() = default;
	explicit EventObject(std::nullptr_t) { index = -1; }
	explicit EventObject(DBloodActor* actor_) { ActorP = actor_; assert(isActor()); }
	explicit EventObject(sectortype* sect) { index = (sectnum(sect) << 8) | Sector; }
	explicit EventObject(walltype* wall) { index = (wallnum(wall) << 8) | Wall; }

	bool isActor() const { return (index & 7) == Actor; }
	bool isSector() const { return (index & 7) == Sector; }
	bool isWall() const { return (index & 7) == Wall; }

	DBloodActor* actor() { assert(isActor()); return GC::ReadBarrier(ActorP); }
	sectortype* sector() { assert(isSector()); return &::sector[index >> 8]; }
	walltype* wall() { assert(isWall()); return &::wall[index >> 8]; }
	int rawindex() { return index >> 8; }

	bool operator==(const EventObject& other) const { return index == other.index; }
	bool operator!=(const EventObject& other) const { return index != other.index; }

	FString description() const;
	void Mark()
	{
		if (isActor()) GC::Mark(ActorP);
	}

};


enum {
	kChannelZero                        = 0,
	kChannelSetTotalSecrets             = 1,
	kChannelSecretFound                 = 2,
	kChannelTextOver                    = 3,
	kChannelLevelExitNormal             = 4,
	kChannelLevelExitSecret             = 5,
	kChannelModernEndLevelCustom        = 6, // custom level number end (gModernMap)
	kChannelLevelStart                  = 7,
	kChannelLevelStartMatch             = 8, // DM and TEAMS
	kChannelLevelStartCoop              = 9,
	kChannelLevelStartTeamsOnly         = 10,
	kChannelPlayerDeathTeamA            = 15,
	kChannelPlayerDeathTeamB            = 16,
	/////////////////////////////
	// level start channels for specific ports
	kChannelLevelStartNBLOOD            = 17, // *NBlood only* must trigger it at level start (gModernMap)
	kChannelLevelStartRAZE              = 18, // *Raze only* must trigger it at level start (gModernMap)
	// channels of players to send commands on
	kChannelAllPlayers                  = 29,
	kChannelPlayer0                     = 30,
	kChannelPlayer1,
	kChannelPlayer2,
	kChannelPlayer3,
	kChannelPlayer4,
	kChannelPlayer5,
	kChannelPlayer6,
	kChannelPlayer7,

	// channel of event causer
	kChannelEventCauser = 50,
	// map requires modern features to work properly
	kChannelMapModernize = 60,
	/////////////////////////////
	kChannelTeamAFlagCaptured = 80,
	kChannelTeamBFlagCaptured,
	kChannelRemoteBomb0 = 90,
	kChannelRemoteBomb1,
	kChannelRemoteBomb2,
	kChannelRemoteBomb3,
	kChannelRemoteBomb4,
	kChannelRemoteBomb5,
	kChannelRemoteBomb6,
	kChannelRemoteBomb7,
	kChannelUser = 100,
	kChannelUserMax = 1024,
	kChannelMax = 4096,
};

extern EventObject rxBucket[];
extern unsigned short bucketHead[];

enum COMMAND_ID {
	kCmdOff = 0,
	kCmdOn = 1,
	kCmdState = 2,
	kCmdToggle = 3,
	kCmdNotState = 4,
	kCmdLink = 5,
	kCmdLock = 6,
	kCmdUnlock = 7,
	kCmdToggleLock = 8,
	kCmdStopOff = 9,
	kCmdStopOn = 10,
	kCmdStopNext = 11,
	kCmdCounterSector = 12,
	kCmdCallback = 20,
	kCmdRepeat = 21,


	kCmdSpritePush = 30,
	kCmdSpriteImpact = 31,
	kCmdSpritePickup = 32,
	kCmdSpriteTouch = 33,
	kCmdSpriteSight = 34,
	kCmdSpriteProximity = 35,
	kCmdSpriteExplode = 36,

	kCmdSectorPush = 40,
	kCmdSectorImpact = 41,
	kCmdSectorEnter = 42,
	kCmdSectorExit = 43,

	kCmdWallPush = 50,
	kCmdWallImpact = 51,
	kCmdWallTouch = 52,
#ifdef NOONE_EXTENSIONS
	kCmdSectorMotionPause = 13,   // stops motion of the sector
	kCmdSectorMotionContinue = 14,   // continues motion of the sector
	kCmdDudeFlagsSet = 15,   // copy dudeFlags from sprite to dude
	kCmdEventKillFull = 16,   // immediately kill the pending object events
	kCmdModernUse = 53,   // used by most of modern types
#endif

	kCmdNumberic = 64, // 64: 0, 65: 1 and so on up to 255
	kCmdModernFeaturesEnable = 100, // must be in object with kChannelMapModernize RX / TX
	kCmdModernFeaturesDisable = 200, // must be in object with kChannelMapModernize RX / TX
	kCmdNumbericMax = 255,
};

enum SSType
{
	SS_WALL = 0,
	SS_CEILING = 1,
	SS_FLOOR = 2,
	SS_SPRITE = 3,
	SS_MASKED = 4,

	SS_SECTOR = 6,
};

inline bool playerRXRngIsFine(int rx) {
	return (rx >= kChannelPlayer0 && rx < kChannelPlayer7);
}

inline bool channelRangeIsFine(int channel) {
	return (channel >= kChannelUser && channel < kChannelUserMax);
}

struct EVENT
{
	EventObject target;
	int8_t cmd;
	int16_t funcID;
	int priority;
	TObjPtr<DBloodActor*> initiator;

	bool operator<(const EVENT& other) const
	{
		return priority < other.priority;
	}

	bool event_isObject(const EventObject& obj) const
	{
		return (this->target == obj);
	}

	bool isActor() const
	{
		return target.isActor();
	}

	bool isSector() const
	{
		return target.isSector();
	}

	bool isWall() const
	{
		return target.isWall();
	}

	DBloodActor* getActor()
	{
		assert(isActor());
		return target.actor();
	}

	sectortype* getSector()
	{
		assert(isSector());
		return target.sector();
	}

	walltype* getWall()
	{
		assert(isWall());
		return target.wall();
	}
};

void evInit(TArray<DBloodActor*>& actors);
void evPostActor(DBloodActor*, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator);
void evPostActor(DBloodActor*, unsigned int nDelta, CALLBACK_ID callback);

void evPostSector(sectortype* index, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator);
void evPostSector(sectortype* index, unsigned int nDelta, CALLBACK_ID callback);

void evPostWall(walltype* index, unsigned int nDelta, COMMAND_ID command, DBloodActor* initiator);

void evProcess(unsigned int nTime);
void evKillActor(DBloodActor*);
void evKillActor(DBloodActor*, DBloodActor* initiator);
void evKillActor(DBloodActor*, CALLBACK_ID a3);

END_BLD_NS
