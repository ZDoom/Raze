//-------------------------------------------------------------------------
/*
Copyright (C) 2020 - Christoph Oelckers

This file is part of Raze

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
of the License, or (at your option) any later version.

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

#include <stdio.h>
#include "build.h"
#include "compat.h"

#include "blood.h"
#include "i_specialpaths.h"
#include "savegamehelp.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "d_net.h"


BEGIN_BLD_NS

FixedBitArray<MAXSPRITES> activeXSprites;

// All AI states for assigning an index.
static AISTATE* allAIStates[] =
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
	&beast138FB4,
	&beast138FD0,
	&beast138FEC,
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
	&genDudeBurnIdle,
	&genDudeBurnChase,
	&genDudeBurnGoto,
	&genDudeBurnSearch,
	&genDudeBurnAttack,
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
	&tinycaleb139660,
	&tinycaleb13967C,
	&tinycaleb139698,
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
	&cerberus139890,
	&cerberus1398AC,
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
	&cultist139A78,
	&cultist139A94,
	&cultist139AB0,
	&cultist139ACC,
	&cultist139AE8,
	&cultistSearch,
	&cultistSFire,
	&cultistTFire,
	&cultistTsFire,
	&cultistSProneFire,
	&cultistTProneFire,
	&cultistTsProneFire,
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
	&gillBeast13A138,
	&gillBeast13A154,
	&gillBeast13A170,
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
	&tcherno13A9B8,
	&tcherno13A9D4,
	&tcherno13A9F0,
	&tcherno13AA0C,
	&tcherno13AA28,
	&genDudeIdleL,
	&genDudeIdleW,
	&genDudeSearchL,
	&genDudeSearchW,
	&genDudeSearchShortL,
	&genDudeSearchShortW,
	&genDudeSearchNoWalkL,
	&genDudeSearchNoWalkW,
	&genDudeGotoL,
	&genDudeGotoW,
	&genDudeDodgeL,
	&genDudeDodgeD,
	&genDudeDodgeW,
	&genDudeDodgeShortL,
	&genDudeDodgeShortD,
	&genDudeDodgeShortW,
	&genDudeDodgeShorterL,
	&genDudeDodgeShorterD,
	&genDudeDodgeShorterW,
	&genDudeChaseL,
	&genDudeChaseD,
	&genDudeChaseW,
	&genDudeChaseNoWalkL,
	&genDudeChaseNoWalkD,
	&genDudeChaseNoWalkW,
	&genDudeFireL,
	&genDudeFireD,
	&genDudeFireW,
	&genDudeRecoilL,
	&genDudeRecoilD,
	&genDudeRecoilW,
	&genDudeRecoilTesla,
	&genDudeThrow,
	&genDudeThrow2,
	&genDudePunch,
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
	&zombie13AC2C,
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
		if (w >= genPatrolStates && w < genPatrolStates + kPatrolStateSize)
		{
			i = int(w - genPatrolStates) + 1000;
			arc(keyname, i);
		}
	}
	else
	{
		arc(keyname, i);
		if (i < countof(allAIStates))
		{
			w = allAIStates[i];
		}
		else if (i >= 1000 && i < 1000 + kPatrolStateSize)
		{
			w = genPatrolStates + (i - 1000);
		}
		else
		{
			w = nullptr;
		}
	}
	return arc;
}

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
#ifdef OLD_SAVEGAME
		// Note: birthCounter/thinkTime are a union and share the same value (this is used for savefile backwards compatibility - see correct implementation below)
		arc("time", w.time, &empty)
			("recoil", w.teslaHit, &empty2)
			("prio", w.prio, &empty)
			("x1", w.stats.birthCounter, &empty)
			("x2", w.stats.thinkTime, &empty)
			("x3", w.stats.active, &empty2)
			.EndObject();
#else
		arc("time", w.time, &empty)
			("teslaHit", w.teslaHit, &empty2)
			("prio", w.prio, &empty)
			("thinkTime", w.stats.thinkTime, &empty)
			("active", w.stats.active, &empty2)
#endif
	}
	return arc;
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, DBloodActor& w, DBloodActor* def)
{
	static DBloodActor nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w.Clear();
	}

	if (arc.BeginObject(keyname))
	{
#ifndef OLD_SAVEGAME
		arc("xvel", w.xvel, def->xvel)
			("yvel", w.yvel, def->yvel)
			("zvel", w.zvel, def->zvel);
#endif

		// The rest is only relevant if the actor has an xsprite.
		if (w.hasX())
		{
			arc("dudeslope", w.dudeSlope, def->dudeSlope)
				("dudeextra", w.dudeExtra, def->dudeExtra)
				("explosionflag", w.explosionhackflag, def->explosionhackflag)
				("spritehit", w.hit, def->hit);
#ifndef OLD_SAVEGAME
			arc("basepoint", w.basePoint, def->basePoint);
#endif

			if (gModernMap)
			{
				arc("spritemass", w.spriteMass, def->spriteMass); // no treatment for old savegames. If this gets lost it is not critical
#ifndef OLD_SAVEGAME
					("prevmarker", w.prevmarker, def->prevmarker);

				// GenDudeExtra only contains valid info for kDudeModernCustom and kDudeModernCustomBurning so only save when needed as these are not small.
				if (w.s().type == kDudeModernCustom || w.s().time == kDudeModernCustomBurning)
				{
					arc("gendudeextra", w.genDudeExtra);
				}
#endif
			}
		}
		arc.EndObject();
	}
	return arc;
}

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
			("reference", w.reference, def->reference)
			("data", w.data, def->data)
			("txid", w.txID, def->txID)
			("rxid", w.rxID, def->rxID)
			("busytime", w.busyTime, def->busyTime)
			("waittime", w.waitTime, def->waitTime)
			("command", w.command, def->command)
			("panxvel", w.panXVel, def->panXVel)
			("panyvel", w.panYVel, def->panYVel)
			("key", w.key, def->key)
			.EndObject();
	}
	return arc;
}

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
			("busy", w.busy, def->busy)
			("reference", w.reference, def->reference)
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
			("reference", w.reference, def->reference)
			("txid", w.txID, def->txID)
			("rxid", w.rxID, def->rxID)
			("command", w.command, def->command)
			("data1", w.data1, def->data1)
			("data2", w.data2, def->data2)
			("data3", w.data3, def->data3)
			("data4", w.data4, def->data4)
			("targetX", w.targetX, def->targetX)
			("targetY", w.targetY, def->targetY)
			("targetZ", w.targetZ, def->targetZ)
			("target", w.target_i, def->target_i)
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
			("modernflags", w.unused1, def->unused1)
			("sightstuff", w.unused3, def->unused3)
			("patrolturndelay", w.unused4, def->unused4)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, HITINFO& w, HITINFO* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("sect", w.hitsect)
			("sprite", w.hitactor)
			("wall", w.hitwall)
			("x", w.hitx)
			("y", w.hity)
			("z", w.hitz)
			.EndObject();
	}
	return arc;
}

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

void SerializeState(FSerializer& arc)
{
	if (arc.isReading())
	{
		cheatReset();
	}
	if (arc.BeginObject("state"))
	{
		psky_t* pSky = tileSetupSky(DEFAULTPSKY);

		arc.Array("sector_filler", qsector_filler, numsectors)
			("visibility", gVisibility)
			("frameclock", PlayClock)
			("framecount", gFrameCount)
			.Array("basewall", baseWall, numwalls)
			.Array("basefloor", baseFloor, numsectors)
			.Array("baseceil", baseCeil, numsectors)
			.Array("velfloor", velFloor, numsectors)
			.Array("velceil", velCeil, numsectors)
			("hitinfo", gHitInfo)
			.Array("statcount", gStatCount, kMaxStatus + 1)
			("xwallsused", XWallsUsed)
			("xsectorsused", XSectorsUsed)
			("fogmode", gFogMode)
#ifdef NOONE_EXTENSIONS
			("modern", gModernMap)
#endif
			("cheating", bPlayerCheated)
			("skyhoriz", pSky->horizfrac)
			("skyy", pSky->yoffs)
			("skyy2", pSky->yoffs2)
			("scale", pSky->yscale)
			.Array("tileofs", pSky->tileofs, countof(pSky->tileofs))
			("numtiles", pSky->lognumtiles)
			("gameoptions", gGameOptions)

			.Array("xwall", xwall, XWallsUsed)  // todo
			.Array("xsector", xsector, XSectorsUsed)
			.SparseArray("actors", bloodActors, kMaxSprites, activeSprites)
			.SparseArray("xsprite", xsprite, kMaxXSprites, activeXSprites);

#ifdef OLD_SAVEGAME
		POINT3D baseSprite[kMaxSprites];
		int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];
		for (int i = 0; i < kMaxSprites; i++)
		{
			baseSprite[i] = bloodActors[i].basePoint;
			xvel[i] = bloodActors[i].xvel;
			yvel[i] = bloodActors[i].yvel;
			zvel[i] = bloodActors[i].zvel;
		}
		arc.SparseArray("basesprite", baseSprite, kMaxSprites, activeSprites)
			.SparseArray("xvel", xvel, kMaxSprites, activeSprites)
			.SparseArray("yvel", yvel, kMaxSprites, activeSprites)
			.SparseArray("zvel", zvel, kMaxSprites, activeSprites);
		if (arc.isReading()) for (int i = 0; i < kMaxSprites; i++) if (activeSprites[i])
		{
			bloodActors[i].basePoint = baseSprite[i];
			bloodActors[i].xvel = xvel[i];
			bloodActors[i].yvel = yvel[i];
			bloodActors[i].zvel = zvel[i];
		}
#endif
		arc.EndObject();
	}
}


void SerializeEvents(FSerializer& arc);
void SerializeSequences(FSerializer& arc);
void SerializeWarp(FSerializer& arc);
void SerializeTriggers(FSerializer& arc);
void SerializeActor(FSerializer& arc);
void SerializeGameStats(FSerializer& arc);
void SerializePlayers(FSerializer& arc);
void SerializeView(FSerializer& arc);
void SerializeNNExts(FSerializer& arc);
void SerializeMirrors(FSerializer& arc);

void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.isWriting())
	{
		activeXSprites.Zero();
		for (int i = 0; i < kMaxSprites; i++)
		{
			if (activeSprites[i] && sprite[i].extra > 0) activeXSprites.Set(sprite[i].extra);
		}
	}
	else
	{
		sndKillAllSounds();
		sfxKillAllSounds();
		ambKillAll();
		seqKillAll();
		if (gamestate != GS_LEVEL)
		{
			memset(xsprite, 0, sizeof(xsprite));
		}
	}
	arc.SerializeMemory("activexsprites", activeXSprites.Storage(), activeXSprites.StorageSize());
	SerializeState(arc);
	InitFreeList(nextXSprite, kMaxXSprites, activeXSprites);
	SerializeActor(arc);
	SerializePlayers(arc);
	SerializeEvents(arc);
	SerializeGameStats(arc);
	SerializeSequences(arc);
	SerializeMirrors(arc);
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
		if (!gMe->packSlots[1].isActive) // if diving suit is not active, turn off reverb sound effect
			sfxSetReverb(0);
		ambInit();
		for (int i = 0; i < gNetPlayers; i++)
			playerSetRace(&gPlayer[i], gPlayer[i].lifeMode);
		viewSetErrorMessage("");
		Net_ClearFifo();
		paused = 0;
		Polymost::Polymost_prepare_loadboard();
		Mus_ResumeSaved();
	}
}



END_BLD_NS
