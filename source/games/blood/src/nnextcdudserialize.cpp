/*
 * Copyright (C) 2023 Christoph Oelckers
 *
 * This file is part of Raze
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifdef NOONE_EXTENSIONS
#pragma once
#include "nnextcdud.h"

BEGIN_BLD_NS

// this code writes out the entire custom dude because there is really no good way to partially run the setup code on savegame reload

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_SOUND& w, CUSTOMDUDE_SOUND* def)
{
	if (arc.BeginObject(keyname))
	{
		arc.Array("id", w.id, kCdudeMaxSounds)
			("medium", w.medium)
			("ai", w.ai)
			("interruptable", w.interruptable)
			("once", w.once)
			("volume", w.volume)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, APPEARANCE& w, APPEARANCE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("sound", w.sound)
			.Array("scl", w.scl, 2)
			("available", w.available)
			("soundAvailable", w.soundAvailable)
			("seq", w.seq)
			("pic", w.pic)
			("clb", w.clb)
			("xrp", w.xrp)
			("yrp", w.yrp)
			("xof", w.xof)
			("yof", w.yof)
			("pal", w.pal)
			("shd", w.shd)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_WEAPON::SHOT& w, CUSTOMDUDE_WEAPON::SHOT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("velocity", w._velocity)
			("slope", w._slope)
			("targetfollow", w.targetFollow)
			("clipdist", w.clipdist)
			("impact", w.impact)
			("remtime", w.remTime)
			("appearance", w.appearance)
			("offset", w.offset)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_WEAPON::AMMO& w, CUSTOMDUDE_WEAPON::AMMO* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("cur", w.cur)
		("total", w.total)
		.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_WEAPON::COOLDOWN& w, CUSTOMDUDE_WEAPON::COOLDOWN* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("clock", w.clock)
		("delay", w.delay)
		("usecount", w.useCount)
		("total", w.totalUseCount)
		.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_WEAPON::SHOT_STYLE& w, CUSTOMDUDE_WEAPON::SHOT_STYLE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("available", w.available)
			("angle", w.angle)
			("offset", w.offset)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_WEAPON& w, CUSTOMDUDE_WEAPON* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("type", w.type)
		("numshots", w.numshots)
		("id", w.id)
		("sharedid", w.sharedId)
		("angle", w.angle)
		("medium", w.medium)
		("pickchance", w.pickChance)
		("available", w.available)
		("posture", w.posture)
		("interruptable", w.interruptable)
		("turntotarget", w.turnToTarget)
		("stateid", w.stateID)
		("nextstateid", w.nextStateID)
		("clipmask", w.clipMask)
		("group", w.group)
		.Array("dispersion", w.dispersion, 2)
		.Array("distrange", w.distRange, 2)
		.Array("targethprange", w.targHpRange, 2)
		.Array("dudehprange", w.dudeHpRange, 2)
		("sound", w.sound)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_GIB& w, CUSTOMDUDE_GIB* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("available", w.available)
			("force", w.force)
			("trflags", w.trFlags)
			("physics", w.physics)
			("thingtype", w.thingType)
			("data1", w.data1)
			("data2", w.data2)
			("data3", w.data3)
			("data4", w.data4)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_EFFECT& w, CUSTOMDUDE_EFFECT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			.Array("id", w.id, kCdudeMaxEffects)
			("clock", w.clock)
			("livetime", w.liveTime)
			("velocity", w._velocity)
			("velocityslope", w._velocitySlope)
			("angle", w.angle)
			("posture", w.posture)
			("medium", w.medium)
			("allunique", w.allUnique)
			("srcVelocity", w.srcVelocity)
			("chance", w.chance)
			.Array("delay", w.delay, 2)
			("spr2gib", w.spr2gib)
			("appearance", w.appearance)
			("anims", w.pAnims)
			("frames", w.pFrames)
			("states", w.pStates)
			("offset", w.offset)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DAMAGE& w, CUSTOMDUDE_DAMAGE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			.Array("id", w.id, kDmgMax)
			("ignoreSources", w.ignoreSources)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DODGE::DAMAGE& w, CUSTOMDUDE_DODGE::DAMAGE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("times", w.times)
			("timer", w.timer)
			("chance", w.chance)
			("dmgReq", w.dmgReq)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DODGE::AIMMISS& w, CUSTOMDUDE_DODGE::AIMMISS* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("chance", w.chance)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DODGE& w, CUSTOMDUDE_DODGE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("ondamage", w.onDamage)
			("onaimmiss", w.onAimMiss)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_RECOIL& w, CUSTOMDUDE_RECOIL* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("times", w.times)
			("timer", w.timer)
			("chance", w.chance)
			("dmgReq", w.dmgReq)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_KNOCKOUT& w, CUSTOMDUDE_KNOCKOUT* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
		("times", w.times)
			("timer", w.timer)
			("chance", w.chance)
			("dmgReq", w.dmgReq)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_VELOCITY& w, CUSTOMDUDE_VELOCITY* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			.Array("id", w.id, kParVelocityMax)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DROPITEM::itemdesc& w, CUSTOMDUDE_DROPITEM::itemdesc* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("class", w.cls)
			("percent", w.perc)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, CUSTOMDUDE_DROPITEM& w, CUSTOMDUDE_DROPITEM* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			.Array("items", w.items, kCdudeMaxDropItems)
			("sprDropItemChance", w.sprDropItemChance)
			.EndObject();
	}
	return arc;
}

END_BLD_NS

// this one's ugly because there is no easy way to retrieve the function records, especially later for scripted functions.
// We have to cheat.
// To avoid endless leaking allocations we store what we create for reuse later. 
// Note that we cannot reference the holding array because it can reallocate.
// Once stats can be defined via scripts this will become unnecessary.
static TArray<VMNativeFunction**> vmfuncpointers;
template<>
FSerializer& Serialize(FSerializer& arc, const char* key, VMNativeFunction**& pfunc, VMNativeFunction***)
{
	VMFunction* func = pfunc ? *pfunc : nullptr;
	arc(key, func);
	if (arc.isReading())
	{
		if (func == nullptr) pfunc = nullptr;
		else
		{
			auto pos = vmfuncpointers.FindEx([&](const auto& element) { return *element == func; });
			if (pos < vmfuncpointers.Size()) pfunc = vmfuncpointers[pos];
			else
			{
				// put these in a place where automatic maintenance will get rid of them later.
				VMNativeFunction** fakep = (VMNativeFunction**)ClassDataAllocator.Alloc(sizeof(VMFunction*));
				vmfuncpointers.Push(fakep);
				pfunc = fakep;
			}
		}
	}
	return arc;
}

BEGIN_BLD_NS

FSerializer& Serialize(FSerializer& arc, const char* keyname, AISTATE& w, AISTATE* def)
{
	if (arc.BeginObject(keyname))
	{
		arc
			("stateType", w.stateType)
			("seqId", w.seqId)
			("enterFunc", w.enterFunc)
			("moveFunc", w.moveFunc)
			("thinkFunc", w.thinkFunc)
			("funcId", w.funcId)
			("stateTicks", w.stateTicks)
			("nextState", w.nextState)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, NextDudeInfo& w, NextDudeInfo* def)
{
	if (arc.BeginObject(keyname))
	{
		size_t index = w.index();
		arc("index", index);
		switch (index)
		{
		case 0:
		{
			auto v = *std::get_if<bool>(&w);
			arc("v", v);
			w = v;
			break;
		}
		case 1:
		{
			auto v = *std::get_if<int>(&w);
			arc("v", v);
			w = v;
			break;
		}
		case 2:
		{
			auto v = *std::get_if<PClass*>(&w);
			arc("v", v);
			w = v;
			break;
		}
		case 3:
		{
			auto v = *std::get_if<DBloodActor*>(&w);
			arc("v", v);
			w = v;
			break;
		}
		}
		arc.EndObject();
	}
	return arc;
}

void DCustomDude::Serialize(FSerializer & arc)
{
	Super::Serialize(arc);

	arc
	("version", version)
		("initialized", initialized)
		("numEffects", numEffects)
		("numWeapons", numWeapons)
		("pSpr", pSpr)
		("pSpr", pLeech)
		.Array("weapons", weapons, kCdudeMaxWeapons)
		("damage", damage)
		.Array("velocity", velocity, kCdudePostureMax)
		.Array("sound", sound, kCdudeSndMax)
		("dodge", dodge)
		("recoil", recoil)
		("knockout", knockout)
		("dropItem", dropItem)
		.Array("effects", effects, kCdudeMaxEffectGroups)
		.Array("states", states[0], kCdudeStateMax * kCdudePostureMax)
		("pSlaves", pSlaves)
		("triggerSeqs", triggerSeqs)
		("medium", medium)
		("posture", posture)
		("mass", mass)
		("largestPic", largestPic)
		("prevSector", prevSector)
		("seeDist", _seeDist)
		("hearDist", _hearDist)
		("periphery", periphery)
		("fallHeight", fallHeight)
		("nextDude", nextDude)

		;


	size_t weap = pWeapon ? pWeapon - weapons : ~0u;
	arc("curweapon", weap);
	if (arc.isReading()) pWeapon = weap == ~0u ? nullptr : weapons + weap;
	arc.EndObject();

	if (arc.isReading()) pInfo = getDudeInfo(pSpr->GetType());
}

END_BLD_NS
#endif