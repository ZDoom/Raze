//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2023 - Christoph Oelckers

This file is part of Raze

This is free software;you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation;either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "vm.h"
#include "ns.h"
#include "buildtiles.h"
#include "blood.h"
BEGIN_BLD_NS


DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nGameType)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nDifficulty)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nMonsterSettings)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, uGameFlags)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, uNetGameFlags)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nWeaponSettings)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nItemSettings)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nRespawnSettings)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nTeamSettings)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nMonsterRespawnTime)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nWeaponRespawnTime)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nItemRespawnTime)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, nSpecialRespawnTime)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, weaponsV10x)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, bFriendlyFire)
DEFINE_FIELD_X(GAMEOPTIONS, GAMEOPTIONS, bKeepKeysOnRespawn)
DEFINE_GLOBAL_UNSIZED(gGameOptions)
DEFINE_GLOBAL_UNSIZED(gHitInfo)


DEFINE_ACTION_FUNCTION(_Blood, OriginalLoadScreen)
{
	static int bLoadScreenCrcMatch = -1;
	if (bLoadScreenCrcMatch == -1)
	{
		auto gtex = TexMan.FindGameTexture("LOADSCREEN", ETextureType::Any);
		if (gtex)
		{
			auto img = gtex->GetTexture()->GetImage();
			bLoadScreenCrcMatch = tileGetCRC32(img) == kLoadScreenCRC;
		}
		else bLoadScreenCrcMatch = true;	// if the LOADSCREEN texture is invalid, allow the widescreen fallback.
	}
	ACTION_RETURN_INT(bLoadScreenCrcMatch);
}

DEFINE_ACTION_FUNCTION(_Blood, PlayIntroMusic)
{
	Mus_Play("PESTIS.MID", false);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Blood, Random2F, Random2F)
{
	PARAM_PROLOGUE;
	PARAM_INT(val);
	PARAM_INT(scale);
	ACTION_RETURN_FLOAT(Random2F(val));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Blood, Chance, Chance)
{
	PARAM_PROLOGUE;
	PARAM_INT(val);
	ACTION_RETURN_INT(Chance(val));
}

DEFINE_ACTION_FUNCTION(_Blood, sndStartSample)
{
	PARAM_PROLOGUE;
	PARAM_INT(id);
	PARAM_INT(vol);
	PARAM_INT(chan);
	PARAM_BOOL(looped);
	PARAM_INT(chanflags);
	sndStartSample(id, vol, chan, looped, EChanFlags::FromInt(chanflags));
	return 0;
}

DEFINE_ACTION_FUNCTION(_Blood, sndStartSampleNamed)
{
	PARAM_PROLOGUE;
	PARAM_STRING(id);
	PARAM_INT(vol);
	PARAM_INT(chan);
	sndStartSample(id.GetChars(), vol, chan);
	return 0;
}

DEFINE_ACTION_FUNCTION(_Blood, PowerupIcon)
{
	PARAM_PROLOGUE;
	PARAM_INT(pwup);
	FTextureID tile = FNullTextureID();
	if (pwup >= 0 && pwup < (int)countof(gPowerUpInfo))
	{
		tile = gPowerUpInfo[pwup].textureID();
	}
	FGameTexture* tex = TexMan.GetGameTexture(tile);
	ACTION_RETURN_INT(tex ? tex->GetID().GetIndex() : -1);
}

DEFINE_ACTION_FUNCTION(_Blood, GetViewPlayer)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(getPlayer(gViewIndex));
}

DEFINE_ACTION_FUNCTION(_BloodPlayer, GetHealth)
{
	PARAM_SELF_STRUCT_PROLOGUE(DBloodPlayer);
	ACTION_RETURN_INT(self->GetActor()->xspr.health);
}

DEFINE_ACTION_FUNCTION_NATIVE(_BloodPlayer, powerupCheck, powerupCheck)
{
	PARAM_SELF_STRUCT_PROLOGUE(DBloodPlayer);
	PARAM_INT(pwup);
	ACTION_RETURN_INT(powerupCheck(self, pwup));
}

DEFINE_FIELD_X(XSECTOR, XSECTOR, flags)
DEFINE_FIELD_X(XSECTOR, XSECTOR, flags2)
DEFINE_FIELD_X(XSECTOR, XSECTOR, marker0)
DEFINE_FIELD_X(XSECTOR, XSECTOR, marker1)
DEFINE_FIELD_X(XSECTOR, XSECTOR, basePath)
DEFINE_FIELD_X(XSECTOR, XSECTOR, actordata)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busy)
DEFINE_FIELD_X(XSECTOR, XSECTOR, offCeilZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, onCeilZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, offFloorZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, onFloorZ)
DEFINE_FIELD_X(XSECTOR, XSECTOR, windVel)
DEFINE_FIELD_X(XSECTOR, XSECTOR, data)
DEFINE_FIELD_X(XSECTOR, XSECTOR, txID)
DEFINE_FIELD_X(XSECTOR, XSECTOR, rxID)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyTimeA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, waitTimeA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, panAngle)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyTimeB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, waitTimeB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, windAng)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobTheta)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobSpeed)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyWaveA)
DEFINE_FIELD_X(XSECTOR, XSECTOR, busyWaveB)
DEFINE_FIELD_X(XSECTOR, XSECTOR, command)
DEFINE_FIELD_X(XSECTOR, XSECTOR, amplitude)
DEFINE_FIELD_X(XSECTOR, XSECTOR, freq)
DEFINE_FIELD_X(XSECTOR, XSECTOR, phase)
DEFINE_FIELD_X(XSECTOR, XSECTOR, wave)
DEFINE_FIELD_X(XSECTOR, XSECTOR, shade)
DEFINE_FIELD_X(XSECTOR, XSECTOR, panVel)
DEFINE_FIELD_X(XSECTOR, XSECTOR, Depth)
DEFINE_FIELD_X(XSECTOR, XSECTOR, Key)
DEFINE_FIELD_X(XSECTOR, XSECTOR, ceilpal)
DEFINE_FIELD_X(XSECTOR, XSECTOR, damageType)
DEFINE_FIELD_X(XSECTOR, XSECTOR, floorpal)
DEFINE_FIELD_X(XSECTOR, XSECTOR, bobZRange)

DEFINE_FIELD_X(XWALL, XWALL, flags)
DEFINE_FIELD_X(XWALL, XWALL, busy)
DEFINE_FIELD_X(XWALL, XWALL, data)
DEFINE_FIELD_X(XWALL, XWALL, txID)
DEFINE_FIELD_X(XWALL, XWALL, rxID)
DEFINE_FIELD_X(XWALL, XWALL, busyTime)
DEFINE_FIELD_X(XWALL, XWALL, waitTime)
DEFINE_FIELD_X(XWALL, XWALL, command)
DEFINE_FIELD_NAMED_X(XWALL, XWALL, panVel.X, panVelX) // VM does not support int vectors.
DEFINE_FIELD_NAMED_X(XWALL, XWALL, panVel.Y, panVelY)
DEFINE_FIELD_X(XWALL, XWALL, key)

DEFINE_FIELD_X(XSPRITE, XSPRITE, aiState)
DEFINE_FIELD_X(XSPRITE, XSPRITE, flags)
DEFINE_FIELD_X(XSPRITE, XSPRITE, target)
DEFINE_FIELD_X(XSPRITE, XSPRITE, burnSource)
DEFINE_FIELD_X(XSPRITE, XSPRITE, TargetPos)
DEFINE_FIELD_X(XSPRITE, XSPRITE, goalAng)
DEFINE_FIELD_X(XSPRITE, XSPRITE, sysData1)
DEFINE_FIELD_X(XSPRITE, XSPRITE, sysData2)
DEFINE_FIELD_X(XSPRITE, XSPRITE, scale)
DEFINE_FIELD_X(XSPRITE, XSPRITE, physAttr)
DEFINE_FIELD_X(XSPRITE, XSPRITE, health)
DEFINE_FIELD_X(XSPRITE, XSPRITE, busy)
DEFINE_FIELD_X(XSPRITE, XSPRITE, data1)
DEFINE_FIELD_X(XSPRITE, XSPRITE, data2)
DEFINE_FIELD_X(XSPRITE, XSPRITE, data3)
DEFINE_FIELD_X(XSPRITE, XSPRITE, txID)
DEFINE_FIELD_X(XSPRITE, XSPRITE, rxID)
DEFINE_FIELD_X(XSPRITE, XSPRITE, command)
DEFINE_FIELD_X(XSPRITE, XSPRITE, busyTime)
DEFINE_FIELD_X(XSPRITE, XSPRITE, waitTime)
DEFINE_FIELD_X(XSPRITE, XSPRITE, data4)
DEFINE_FIELD_X(XSPRITE, XSPRITE, burnTime)
DEFINE_FIELD_X(XSPRITE, XSPRITE, height)
DEFINE_FIELD_X(XSPRITE, XSPRITE, stateTimer)
DEFINE_FIELD_X(XSPRITE, XSPRITE, respawnPending)
DEFINE_FIELD_X(XSPRITE, XSPRITE, dropMsg)
DEFINE_FIELD_X(XSPRITE, XSPRITE, key)
DEFINE_FIELD_X(XSPRITE, XSPRITE, lSkill)
DEFINE_FIELD_X(XSPRITE, XSPRITE, lockMsg)
DEFINE_FIELD_X(XSPRITE, XSPRITE, dodgeDir)
DEFINE_FIELD_X(XSPRITE, XSPRITE, wave)
DEFINE_FIELD_X(XSPRITE, XSPRITE, medium)
DEFINE_FIELD_X(XSPRITE, XSPRITE, respawn)
DEFINE_FIELD_X(XSPRITE, XSPRITE, modernFlags)
DEFINE_FIELD_X(XSPRITE, XSPRITE, sightstuff)
DEFINE_FIELD_X(XSPRITE, XSPRITE, patrolturndelay)

DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, time)
DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, teslaHit)
DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, active)
DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, prio)
DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, thinkTime)
DEFINE_FIELD_X(DUDEEXTRA, DUDEEXTRA, birthCounter)

DEFINE_FIELD_UNSIZED(SPRITEHIT, SPRITEHIT, hit)
DEFINE_FIELD_UNSIZED(SPRITEHIT, SPRITEHIT, ceilhit)
DEFINE_FIELD_UNSIZED(SPRITEHIT, SPRITEHIT, florhit)

DEFINE_FIELD(DBloodActor, dudeSlope)
DEFINE_FIELD(DBloodActor, hasx)
DEFINE_FIELD(DBloodActor, explosionhackflag)
DEFINE_FIELD_UNSIZED(BloodActor, DBloodActor, hit)
DEFINE_FIELD_UNSIZED(BloodActor, DBloodActor, dudeExtra)
DEFINE_FIELD_UNSIZED(BloodActor, DBloodActor, xspr)
DEFINE_FIELD(DBloodActor, ownerActor)
DEFINE_FIELD(DBloodActor, cumulDamage)
DEFINE_FIELD(DBloodActor, interpolated)


static void Blood_ChangeType(DBloodActor* self, PClassActor* type)
{
	self->ChangeType(type);
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, ChangeType, Blood_ChangeType)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_POINTER(type, PClassActor);
	self->ChangeType(type);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, InsertSprite, InsertSprite)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	PARAM_INT(stat);
	PARAM_POINTER(type, PClassActor);
	ACTION_RETURN_POINTER(InsertSprite(sect, stat, type));
}

static void bloodactor_addX(DBloodActor* a)
{
	a->addX();
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, addX, bloodactor_addX)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	self->addX();
	return 0;
}

static void bloodactor_evPostActorCallback(DBloodActor* act, int delta, VMFunction* id)
{
	evPostActor(act, delta, id);
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, evPostActorCallback, bloodactor_evPostActorCallback)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_INT(d);
	PARAM_POINTER(id, VMFunction);
	bloodactor_evPostActorCallback(self, d, id);
	return 0;
}

double bloodactor_getActorExtents(DBloodActor* act, double* bottom)
{
	double top;
	GetActorExtents(act, &top, bottom);
	return top;
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, getActorExtents, bloodactor_getActorExtents)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	double top, bottom;
	GetActorExtents(self, &top, &bottom);
	if (numret > 0) ret[0].SetFloat(top);
	if (numret > 1) ret[1].SetFloat(bottom);
	return min(numret, 2);
}

int bloodactor_HitScan(DBloodActor* self, double z, double x, double y, double zz, int clipmask, double clipdist)
{
	return HitScan(self, z, DVector3(x, y, zz), clipmask, clipdist);
}
DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, HitScan, bloodactor_HitScan)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_FLOAT(z);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(zz);
	PARAM_INT(clipmask);
	PARAM_FLOAT(clipdist);
	ACTION_RETURN_INT(bloodactor_HitScan(self, z, x, y, zz, clipmask, clipdist));
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, play3DSoundID, sfxPlay3DSound)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_INT(sound);
	PARAM_INT(chan);
	PARAM_INT(flags);
	sfxPlay3DSound(self, sound, chan, flags);
	return 0;
}

DEFINE_ACTION_FUNCTION(DBloodActor, seqSpawnID)	// will be changed later.
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_INT(seqid);
	PARAM_INT(cbid);
	seqSpawn(seqid, self, cbid);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, impactMissile, actImpactMissile)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_INT(hitcode);
	actImpactMissile(self, hitcode);
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actDropObject(DBloodActor* actor, int nType)
{
	IFVM(BloodActor, dropObject)
	{
		PClass* ty = GetSpawnType(nType);
		if (ty == nullptr) ty = RUNTIME_CLASS(DBloodActor);
		DBloodActor* spawned;
		VMReturn ret((void**)&spawned);
		VMValue param[] = { actor, ty };
		VMCall(func, param, 2, &ret, 1);
		return spawned;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int actGetRespawnTime(DBloodActor* actor)
{
	IFVIRTUALPTR(actor, DBloodActor, getRespawnTime)
	{
		int time;
		VMReturn ret(&time);
		VMValue param[] = { actor };
		VMCall(func, param, 1, &ret, 1);
		return time;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DBloodActor* actFireMissile(DBloodActor* actor, double xyoff, double zoff, DVector3 dv, int nType)
{
	IFVM(BloodActor, fireMissile)
	{
		PClass* ty = GetSpawnType(nType);
		DBloodActor* spawned;
		VMReturn ret((void**)&spawned);
		VMValue param[] = { actor, xyoff, zoff, dv.X, dv.Y, dv.Z, ty};
		VMCall(func, param, 7, &ret, 1);
		return spawned;
	}
	return nullptr;
}

void callActorFunction(VMFunction* funcID, DBloodActor* actor)
{
	if (funcID)
	{
		VMValue param[] = { actor };
		VMCall(funcID, param, 1, nullptr, 0);
	}

}


END_BLD_NS
