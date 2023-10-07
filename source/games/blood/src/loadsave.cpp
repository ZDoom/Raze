//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2022 Christoph Oelckers

This file is part of Raze

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include "build.h"

#include "blood.h"
#include "i_specialpaths.h"
#include "savegamehelp.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "d_net.h"


BEGIN_BLD_NS

void validateLinks();

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, DUDEEXTRA& w, DUDEEXTRA* def)
{
	int empty = 0;
	uint8_t empty2 = 0;
	if (arc.isReading()) w = {};

	if (arc.BeginObject(keyname))
	{
		arc("time", w.time, &empty)
			("teslaHit", w.teslaHit, &empty2)
			("prio", w.prio, &empty)
			("thinkTime", w.thinkTime, &empty)
			("birthCounter", w.birthCounter, &empty)
			("active", w.active, &empty2)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DBloodActor::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc
		("hasx", hasx)
		("basepoint", basePoint);

	// The rest is only relevant if the actor has an xsprite.
	if (hasX())
	{
		arc("xsprite", xspr)
			("dudeslope", dudeSlope)
			("dudeextra", dudeExtra)
			("explosionflag", explosionhackflag)
			("spritehit", hit)
			("owneractor", ownerActor)
			.Array("dmgcontrol", dmgControl, kDamageMax);

	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		arc("spritemass", spriteMass)
			("prevmarker", prevmarker)
			.Array("conditions", condition, 2);


		// GenDudeExtra only contains valid info for kDudeModernCustom and kDudeModernCustomBurning so only save when needed as these are not small.
		if (GetType() == kDudeModernCustom || GetType() == kDudeModernCustomBurning)
		{
			arc("gendudeextra", genDudeExtra);
		}
	}
#endif
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, XWALL& w, XWALL* def)
{
	static XWALL nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}

	if (arc.BeginObject(keyname))
	{
		arc("flags", w.flags, def->flags)
			("busy", w.busy, def->busy)
			("data", w.data, def->data)
			("txid", w.txID, def->txID)
			("rxid", w.rxID, def->rxID)
			("busytime", w.busyTime, def->busyTime)
			("waittime", w.waitTime, def->waitTime)
			("command", w.command, def->command)
			("panxvel", w.panVel.X, def->panVel.X)
			("panyvel", w.panVel.Y, def->panVel.Y)
			("key", w.key, def->key)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, XSECTOR& w, XSECTOR* def)
{
	static XSECTOR nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}

	if (arc.BeginObject(keyname))
	{
		arc("flags", w.flags, def->flags)
			("flags2", w.flags2, def->flags2)
			("busy", w.busy, def->busy)
			("data", w.data, def->data)
			("txid", w.txID, def->txID)
			("rxid", w.rxID, def->rxID)
			("offceilz", w.offCeilZ, def->offCeilZ)
			("onceilz", w.onCeilZ, def->onCeilZ)
			("offloorz", w.offFloorZ, def->offFloorZ)
			("onloorz", w.onFloorZ, def->onFloorZ)
			("windvel", w.windVel, def->windVel)
			("busytimea", w.busyTimeA, def->busyTimeA)
			("busytimeb", w.busyTimeB, def->busyTimeB)
			("waittimea", w.waitTimeA, def->waitTimeA)
			("waittimeb", w.waitTimeB, def->waitTimeB)
			("panangle", w.panAngle, def->panAngle)
			("marker0", w.marker0, def->marker0)
			("marker1", w.marker1, def->marker1)
			("basepath", w.basePath, def->basePath)
			("actordata", w.actordata, def->actordata)
			("windang", w.windAng, def->windAng)
			("bobtheta", w.bobTheta, def->bobTheta)
			("bobspeed", w.bobSpeed, def->bobSpeed)
			("busywavea", w.busyWaveA, def->busyWaveA)
			("busywaveb", w.busyWaveB, def->busyWaveB)
			("command", w.command, def->command)
			("amplitude", w.amplitude, def->amplitude)
			("freq", w.freq, def->freq)
			("phase", w.phase, def->phase)
			("wave", w.wave, def->wave)
			("shade", w.shade, def->shade)
			("panvel", w.panVel, def->panVel)
			("depth", w.Depth, def->Depth)
			("key", w.Key, def->Key)
			("ceilpal", w.ceilpal, def->ceilpal)
			("floorpal", w.floorpal, def->floorpal)
			("damagetype", w.damageType, def->damageType)
			("bobzrange", w.bobZRange, def->bobZRange)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, XSPRITE& w, XSPRITE* def)
{
	static XSPRITE nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("flags", w.flags, def->flags)
			//("aistate", w.aiState, def->aiState) disabled until we can transition to engine states.
			("busy", w.busy, def->busy)
			("txid", w.txID, def->txID)
			("rxid", w.rxID, def->rxID)
			("command", w.command, def->command)
			("data1", w.data1, def->data1)
			("data2", w.data2, def->data2)
			("data3", w.data3, def->data3)
			("data4", w.data4, def->data4)
			("targetX", w.TargetPos.X, def->TargetPos.X)
			("targetY", w.TargetPos.Y, def->TargetPos.Y)
			("targetZ", w.TargetPos.Z, def->TargetPos.Z)
			("target", w.target, def->target)
			("sysdata1", w.sysData1, def->sysData1)
			("sysdata2", w.sysData2, def->sysData2)
			("scale", w.scale, def->scale)
			("physattr", w.physAttr, def->physAttr)
			("health", w.health, def->health)
			("burnsource", w.burnSource, def->burnSource)
			("busytime", w.busyTime, def->busyTime)
			("waittime", w.waitTime, def->waitTime)
			("goalang", w.goalAng, def->goalAng)
			("burntime", w.burnTime, def->burnTime)
			("height", w.height, def->height)
			("statetimer", w.stateTimer, def->stateTimer)
			("respawnpending", w.respawnPending, def->respawnPending)
			("dropmsg", w.dropMsg, def->dropMsg)
			("key", w.key, def->key)
			("lskill", w.lSkill, def->lSkill)
			("lockmsg", w.lockMsg, def->lockMsg)
			("dodgedir", w.dodgeDir, def->dodgeDir)
			("wave", w.wave, def->wave)
			("medium", w.medium, def->medium)
			("respawn", w.respawn, def->respawn)
			("modernflags", w.modernFlags, def->modernFlags)
			("sightstuff", w.sightstuff, def->sightstuff)
			("patrolturndelay", w.patrolturndelay, def->patrolturndelay)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, GAMEOPTIONS& w, GAMEOPTIONS* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.nGameType)
			("difficulty", w.nDifficulty)
			("monster", w.nMonsterSettings)
			("flags", w.uGameFlags)
			("netflags", w.uNetGameFlags)
			("weapons", w.nWeaponSettings)
			("items", w.nItemSettings)
			("respawn", w.nRespawnSettings)
			("team", w.nTeamSettings)
			("monsterrespawntime", w.nMonsterRespawnTime)
			("itemrespawntime", w.nItemRespawnTime)
			("specialrespawntime", w.nSpecialRespawnTime)
			("weaponsv10x", w.weaponsV10x)
			("friendlyfire", w.bFriendlyFire)
			("keepkeys", w.bKeepKeysOnRespawn)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeState(FSerializer& arc)
{
	if (arc.isReading())
	{
		cheatReset();
	}
	if (arc.BeginObject("state"))
	{
		arc("visibility", gVisibility)
			("frameclock", PlayClock)
			("framecount", gFrameCount)
			("hitinfo", gHitInfo)
			("fogmode", gFogMode)
#ifdef NOONE_EXTENSIONS
			("modern", gModernMap)
#endif
			("cheating", bPlayerCheated)
			("gameoptions", gGameOptions);

		arc.EndObject();
	}
}


void SerializeEvents(FSerializer& arc);
void SerializeSequences(FSerializer& arc);
void SerializeWarp(FSerializer& arc);
void SerializeTriggers(FSerializer& arc);
void SerializePlayers(FSerializer& arc);
void SerializeView(FSerializer& arc);
void SerializeNNExts(FSerializer& arc);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.isWriting())
	{
	}
	else
	{
		sndKillAllSounds();
		sfxKillAllSounds();
		ambKillAll();
		seqKillAll();
	}
	SerializeState(arc);
	SerializePlayers(arc);
	SerializeEvents(arc);
	SerializeSequences(arc);
	SerializeWarp(arc);
	SerializeTriggers(arc);
	SerializeView(arc);
#ifdef NOONE_EXTENSIONS
	SerializeNNExts(arc);
#endif

	if (arc.isReading())
	{
		InitSectorFX();
		viewInitializePrediction();
		PreloadCache();
		if (!getPlayer(myconnectindex)->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
			sfxSetReverb(0);
		ambInit();
		for (int i = 0; i < gNetPlayers; i++)
			playerSetRace(getPlayer(i), getPlayer(i)->lifeMode);
		viewSetErrorMessage("");
		paused = 0;
		Mus_ResumeSaved();
	}
	validateLinks();
}



END_BLD_NS
