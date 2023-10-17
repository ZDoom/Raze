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


#include "blood.h"
#include "i_specialpaths.h"
#include "savegamehelp.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "d_net.h"


BEGIN_BLD_NS

void validateLinks();

// All AI states for assigning an index.
static AISTATE* const allAIStates[] =
{
	nullptr,
	&genIdle,
	&genRecoil,
	&batIdle,
	&batFlyIdle,
	&batChase,
	&batPonder,
	&batGoto,
	&batBite,
	&batRecoil,
	&batSearch,
	&batSwoop,
	&batFly,
	&batTurn,
	&batHide,
	&batDodgeUp,
	&batDodgeUpRight,
	&batDodgeUpLeft,
	&batDodgeDown,
	&batDodgeDownRight,
	&batDodgeDownLeft,
	&beastIdle,
	&beastChase,
	&beastDodge,
	&beastGoto,
	&beastSlash,
	&beastStomp,
	&beastSearch,
	&beastRecoil,
	&beastTeslaRecoil,
	&beastSwimIdle,
	&beastSwimChase,
	&beastSwimDodge,
	&beastSwimGoto,
	&beastSwimSearch,
	&beastSwimSlash,
	&beastSwimRecoil,
	&beastMorphToBeast,
	&beastMorphFromCultist,
	&beastMoveSwimChaseAlt,
	&beastSwimAttack,
	&beastSwimTurn,
	&eelIdle,
	&eelFlyIdle,
	&eelChase,
	&eelPonder,
	&eelGoto,
	&eelBite,
	&eelRecoil,
	&eelSearch,
	&eelSwoop,
	&eelFly,
	&eelTurn,
	&eelHide,
	&eelDodgeUp,
	&eelDodgeUpRight,
	&eelDodgeUpLeft,
	&eelDodgeDown,
	&eelDodgeDownRight,
	&eelDodgeDownLeft,
	&cultistBurnIdle,
	&cultistBurnChase,
	&cultistBurnGoto,
	&cultistBurnSearch,
	&cultistBurnAttack,
	&zombieABurnChase,
	&zombieABurnGoto,
	&zombieABurnSearch,
	&zombieABurnAttack,
	&zombieFBurnChase,
	&zombieFBurnGoto,
	&zombieFBurnSearch,
	&zombieFBurnAttack,
	&innocentBurnChase,
	&innocentBurnGoto,
	&innocentBurnSearch,
	&innocentBurnAttack,
	&beastBurnChase,
	&beastBurnGoto,
	&beastBurnSearch,
	&beastBurnAttack,
	&tinycalebBurnChase,
	&tinycalebBurnGoto,
	&tinycalebBurnSearch,
	&tinycalebBurnAttack,
	&tinycalebIdle,
	&tinycalebChase,
	&tinycalebDodge,
	&tinycalebGoto,
	&tinycalebAttack,
	&tinycalebSearch,
	&tinycalebRecoil,
	&tinycalebTeslaRecoil,
	&tinycalebSwimIdle,
	&tinycalebSwimChase,
	&tinycalebSwimDodge,
	&tinycalebSwimGoto,
	&tinycalebSwimSearch,
	&tinycalebSwimAttack,
	&tinycalebSwimRecoil,
	&tinycalebSwimUnused,
	&tinycalebSwimMoveIn,
	&tinycalebSwimTurn,
	&cerberusIdle,
	&cerberusSearch,
	&cerberusChase,
	&cerberusRecoil,
	&cerberusTeslaRecoil,
	&cerberusGoto,
	&cerberusBite,
	&cerberusBurn,
	&cerberus3Burn,
	&cerberus2Idle,
	&cerberus2Search,
	&cerberus2Chase,
	&cerberus2Recoil,
	&cerberus2Goto,
	&cerberus2Bite,
	&cerberus2Burn,
	&cerberus4Burn,
	&cerberusTurn1,
	&cerberusTurn2,
	&cultistIdle,
	&cultistProneIdle,
	&fanaticProneIdle,
	&cultistProneIdle3,
	&cultistChase,
	&fanaticChase,
	&cultistDodge,
	&cultistGoto,
	&cultistProneChase,
	&cultistProneDodge,
	&cultistTThrow,
	&cultistSThrow,
	&cultistTsThrow,
	&cultistDThrow,
	&cultistDThrow2,
	&cultistDThrow3C,
	&cultistDThrow3B,
	&cultistDThrow3A,
	&cultistDThrow4,
	&cultistSearch,
	&cultistSFire,
	&cultistTFire,
	&cultistTsFire,
	&cultistSProneFire,
	&cultistTProneFire,
	&cultistTsProneFire,
	&cultistTsProneFireFixed,
	&cultistRecoil,
	&cultistProneRecoil,
	&cultistTeslaRecoil,
	&cultistSwimIdle,
	&cultistSwimChase,
	&cultistSwimDodge,
	&cultistSwimGoto,
	&cultistSwimSearch,
	&cultistSSwimFire,
	&cultistTSwimFire,
	&cultistTsSwimFire,
	&cultistSwimRecoil,
	&gargoyleFIdle,
	&gargoyleStatueIdle,
	&gargoyleFChase,
	&gargoyleFGoto,
	&gargoyleFSlash,
	&gargoyleFThrow,
	&gargoyleSThrow,
	&gargoyleSBlast,
	&gargoyleFRecoil,
	&gargoyleFSearch,
	&gargoyleFMorph2,
	&gargoyleFMorph,
	&gargoyleSMorph2,
	&gargoyleSMorph,
	&gargoyleSwoop,
	&gargoyleFly,
	&gargoyleTurn,
	&gargoyleDodgeUp,
	&gargoyleFDodgeUpRight,
	&gargoyleFDodgeUpLeft,
	&gargoyleDodgeDown,
	&gargoyleFDodgeDownRight,
	&gargoyleFDodgeDownLeft,
	&statueFBreakSEQ,
	&statueSBreakSEQ,
	&ghostIdle,
	&ghostChase,
	&ghostGoto,
	&ghostSlash,
	&ghostThrow,
	&ghostBlast,
	&ghostRecoil,
	&ghostTeslaRecoil,
	&ghostSearch,
	&ghostSwoop,
	&ghostFly,
	&ghostTurn,
	&ghostDodgeUp,
	&ghostDodgeUpRight,
	&ghostDodgeUpLeft,
	&ghostDodgeDown,
	&ghostDodgeDownRight,
	&ghostDodgeDownLeft,
	&gillBeastIdle,
	&gillBeastChase,
	&gillBeastDodge,
	&gillBeastGoto,
	&gillBeastBite,
	&gillBeastSearch,
	&gillBeastRecoil,
	&gillBeastSwimIdle,
	&gillBeastSwimChase,
	&gillBeastSwimDodge,
	&gillBeastSwimGoto,
	&gillBeastSwimSearch,
	&gillBeastSwimBite,
	&gillBeastSwimRecoil,
	&gillBeastSwimUnused,
	&gillBeastSwimMoveIn,
	&gillBeastSwimTurn,
	&handIdle,
	&hand13A3B4,
	&handSearch,
	&handChase,
	&handRecoil,
	&handGoto,
	&handJump,
	&houndIdle,
	&houndSearch,
	&houndChase,
	&houndRecoil,
	&houndTeslaRecoil,
	&houndGoto,
	&houndBite,
	&houndBurn,
	&innocentIdle,
	&innocentSearch,
	&innocentChase,
	&innocentRecoil,
	&innocentTeslaRecoil,
	&innocentGoto,
	&podIdle,
	&podMove,
	&podSearch,
	&podStartChase,
	&podRecoil,
	&podChase,
	&tentacleIdle,
	&tentacle13A6A8,
	&tentacle13A6C4,
	&tentacle13A6E0,
	&tentacle13A6FC,
	&tentacleMove,
	&tentacleSearch,
	&tentacleStartChase,
	&tentacleRecoil,
	&tentacleChase,
	&ratIdle,
	&ratSearch,
	&ratChase,
	&ratDodge,
	&ratRecoil,
	&ratGoto,
	&ratBite,
	&spidIdle,
	&spidChase,
	&spidDodge,
	&spidGoto,
	&spidSearch,
	&spidBite,
	&spidJump,
	&spidBirth,
	&tchernobogIdle,
	&tchernobogSearch,
	&tchernobogChase,
	&tchernobogRecoil,
	&tchernobogGoto,
	&tchernobogBurn1,
	&tchernobogBurn2,
	&tchernobogFireAtk,
	&tchernobogTurn,
	&zombieAIdle,
	&zombieAChase,
	&zombieAPonder,
	&zombieAGoto,
	&zombieAHack,
	&zombieASearch,
	&zombieARecoil,
	&zombieATeslaRecoil,
	&zombieARecoil2,
	&zombieAStand,
	&zombieEIdle,
	&zombieEUp2,
	&zombieEUp,
	&zombie2Idle,
	&zombie2Search,
	&zombieSIdle,
	&zombieEStand,
	&zombieFIdle,
	&zombieFChase,
	&zombieFGoto,
	&zombieFDodge,
	&zombieFHack,
	&zombieFPuke,
	&zombieFThrow,
	&zombieFSearch,
	&zombieFRecoil,
	&zombieFTeslaRecoil,
};

#ifdef NOONE_EXTENSIONS
// Serializing states is a lot more tricky for custom dudes.
// Note that this makes no assumptions about the relationship between actor and dude because at restoration this isn't known.
struct AIStateRep
{
	DCustomDude* owner;
	int index;
};
static AIStateRep makeStateRep(AISTATE* state)
{
	int i = 0;
	if (state == nullptr) return { nullptr, -1 };
	for (auto cstate : allAIStates)
	{
		if (state == cstate)
		{
			
			return { nullptr, i };
		}
		i++;
	}
	if (gModernMap)
	{
		if (state >= genPatrolStates && state < genPatrolStates + kPatrolStateSize)
		{
			return { nullptr, int(state - genPatrolStates) + 1000 };
		}
		// now it gets ugly: the custom dudes each have a copied set of states. Find out which one.
		BloodSpriteIterator it;
		while (auto actor = it.Next())
		{
			if (IsCustomDude(actor))
			{
				auto dude = cdudeGet(actor);
				if (dude)
				{
					auto firststate = (AISTATE*)dude->states;
					auto count = sizeof(dude->states) / sizeof(AISTATE);
					if (state >= firststate && state < firststate + count)
					{
						return { dude, int(state - firststate) };
					}
				}
			}
		}
	}
	Printf(PRINT_HIGH, "Attempt to save invalid state!\n"); // we got no further info here, sorry!
	return { nullptr, -1 };
}

static AISTATE* getStateFromRep(AIStateRep* rep)
{
	if (rep->owner == nullptr)
	{
		int i = rep->index;
		if (i >= 0 && i < countof(allAIStates))
		{
			return allAIStates[i];
		}
		else if (gModernMap && i >= 1000 && i < 1000 + kPatrolStateSize)
		{
			return genPatrolStates + (i - 1000);
		}
		else
		{
			return nullptr;
		}
	}
	else if (gModernMap)
	{
		// at this point the object may not have been deserialized but it definitely has been allocated so we can just take the address.
		AISTATE* state = (AISTATE*)rep->owner->states;
		return state + rep->index;
	}
	else return nullptr;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, AIStateRep& w, AIStateRep* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("object", w.owner)
			("index", w.index)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, AISTATE*& w, AISTATE** def)
{
	unsigned i = 0;
	if (arc.isWriting())
	{
		auto rep = makeStateRep(w);
		arc(keyname, rep);
	}
	else
	{
		AIStateRep rep;
		arc(keyname, rep);
		w = getStateFromRep(&rep);
	}
	return arc;
}

#else
FSerializer& Serialize(FSerializer& arc, const char* keyname, AISTATE*& w, AISTATE** def)
{
	unsigned i = 0;
	if (arc.isWriting())
	{
		if (def && w == *def) return arc;
		for (auto cstate : allAIStates)
		{
			if (w == cstate)
			{
				arc(keyname, i);
				return arc;
			}
			i++;
		}
	}
	else
	{
		arc(keyname, i);
		if (i < countof(allAIStates))
		{
			w = allAIStates[i];
		}
		else
		{
			w = nullptr;
		}
	}
	return arc;
}

#endif


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
			("chasehackflag", chasehackflag)
			("spritehit", hit)
			("owneractor", ownerActor)
			.Array("dmgcontrol", dmgControl, kDamageMax);

	}

#ifdef NOONE_EXTENSIONS
	if (gModernMap)
	{
		arc("prevmarker", prevmarker)
			("customdude", customDude)
			.Array("conditions", condition, 4);

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
			("aistate", w.aiState, def->aiState)
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
			("sysdata3", w.sysData3, def->sysData3)
			("sysdata4", w.sysData4, def->sysData4)
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
