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


DEF_ANIMATOR(aiGenDudeMoveForward)
DEF_ANIMATOR(aiMoveDodge)
DEF_ANIMATOR(aiMoveForward)
DEF_ANIMATOR(aiMoveTurn)
DEF_ANIMATOR(aiPodChase)
DEF_ANIMATOR(aiPodMove)
DEF_ANIMATOR(aiPodSearch)
DEF_ANIMATOR(aiThinkTarget)
DEF_ANIMATOR(batMoveDodgeDown)
DEF_ANIMATOR(batMoveDodgeUp)
DEF_ANIMATOR(batMoveFly)
DEF_ANIMATOR(batMoveForward)
DEF_ANIMATOR(batMoveSwoop)
DEF_ANIMATOR(batMoveToCeil)
DEF_ANIMATOR(batThinkChase)
DEF_ANIMATOR(batThinkGoto)
DEF_ANIMATOR(batThinkPonder)
DEF_ANIMATOR(batThinkSearch)
DEF_ANIMATOR(batThinkTarget)
DEF_ANIMATOR(beastMoveForward)
DEF_ANIMATOR(beastThinkChase)
DEF_ANIMATOR(beastThinkGoto)
DEF_ANIMATOR(beastThinkSearch)
DEF_ANIMATOR(beastThinkSwimChase)
DEF_ANIMATOR(beastThinkSwimGoto)
DEF_ANIMATOR(burnThinkChase)
DEF_ANIMATOR(burnThinkGoto)
DEF_ANIMATOR(burnThinkSearch)
DEF_ANIMATOR(calebThinkChase)
DEF_ANIMATOR(calebThinkGoto)
DEF_ANIMATOR(calebThinkSearch)
DEF_ANIMATOR(calebThinkSwimChase)
DEF_ANIMATOR(calebThinkSwimGoto)
DEF_ANIMATOR(cerberusThinkChase)
DEF_ANIMATOR(cerberusThinkGoto)
DEF_ANIMATOR(cerberusThinkSearch)
DEF_ANIMATOR(cerberusThinkTarget)
DEF_ANIMATOR(cultThinkChase)
DEF_ANIMATOR(cultThinkGoto)
DEF_ANIMATOR(cultThinkSearch)
DEF_ANIMATOR(eelMoveAscend)
DEF_ANIMATOR(eelMoveDodgeDown)
DEF_ANIMATOR(eelMoveDodgeUp)
DEF_ANIMATOR(eelMoveForward)
DEF_ANIMATOR(eelMoveSwoop)
DEF_ANIMATOR(eelMoveToCeil)
DEF_ANIMATOR(eelThinkChase)
DEF_ANIMATOR(eelThinkGoto)
DEF_ANIMATOR(eelThinkPonder)
DEF_ANIMATOR(eelThinkSearch)
DEF_ANIMATOR(eelThinkTarget)
DEF_ANIMATOR(entryAIdle)
DEF_ANIMATOR(entryEStand)
DEF_ANIMATOR(entryEZombie)
DEF_ANIMATOR(entryFStatue)
DEF_ANIMATOR(entrySStatue)
DEF_ANIMATOR(forcePunch)
DEF_ANIMATOR(gargMoveDodgeDown)
DEF_ANIMATOR(gargMoveDodgeUp)
DEF_ANIMATOR(gargMoveFly)
DEF_ANIMATOR(gargMoveForward)
DEF_ANIMATOR(gargMoveSlow)
DEF_ANIMATOR(gargMoveSwoop)
DEF_ANIMATOR(gargThinkChase)
DEF_ANIMATOR(gargThinkGoto)
DEF_ANIMATOR(gargThinkSearch)
DEF_ANIMATOR(gargThinkTarget)
DEF_ANIMATOR(ghostMoveDodgeDown)
DEF_ANIMATOR(ghostMoveDodgeUp)
DEF_ANIMATOR(ghostMoveFly)
DEF_ANIMATOR(ghostMoveForward)
DEF_ANIMATOR(ghostMoveSlow)
DEF_ANIMATOR(ghostMoveSwoop)
DEF_ANIMATOR(ghostThinkChase)
DEF_ANIMATOR(ghostThinkGoto)
DEF_ANIMATOR(ghostThinkSearch)
DEF_ANIMATOR(ghostThinkTarget)
DEF_ANIMATOR(gillThinkChase)
DEF_ANIMATOR(gillThinkGoto)
DEF_ANIMATOR(gillThinkSearch)
DEF_ANIMATOR(gillThinkSwimChase)
DEF_ANIMATOR(gillThinkSwimGoto)
DEF_ANIMATOR(handThinkChase)
DEF_ANIMATOR(handThinkGoto)
DEF_ANIMATOR(handThinkSearch)
DEF_ANIMATOR(houndThinkChase)
DEF_ANIMATOR(houndThinkGoto)
DEF_ANIMATOR(houndThinkSearch)
DEF_ANIMATOR(innocThinkChase)
DEF_ANIMATOR(innocThinkGoto)
DEF_ANIMATOR(innocThinkSearch)
DEF_ANIMATOR(MorphToBeast)
DEF_ANIMATOR(myThinkSearch)
DEF_ANIMATOR(myThinkTarget)
DEF_ANIMATOR(playStatueBreakSnd)
DEF_ANIMATOR(ratThinkChase)
DEF_ANIMATOR(ratThinkGoto)
DEF_ANIMATOR(ratThinkSearch)
DEF_ANIMATOR(spidThinkChase)
DEF_ANIMATOR(spidThinkGoto)
DEF_ANIMATOR(spidThinkSearch)
DEF_ANIMATOR(sub_628A0)
DEF_ANIMATOR(sub_62AE0)
DEF_ANIMATOR(sub_62D7C)
DEF_ANIMATOR(sub_65D04)
DEF_ANIMATOR(sub_65F44)
DEF_ANIMATOR(sub_661E0)
DEF_ANIMATOR(sub_6CB00)
DEF_ANIMATOR(sub_6CD74)
DEF_ANIMATOR(sub_6D03C)
DEF_ANIMATOR(sub_72580)
DEF_ANIMATOR(sub_725A4)
DEF_ANIMATOR(sub_72850)
DEF_ANIMATOR(tchernobogThinkChase)
DEF_ANIMATOR(unicultThinkChase)
DEF_ANIMATOR(unicultThinkGoto)
DEF_ANIMATOR(unicultThinkSearch)
DEF_ANIMATOR(zombaThinkChase)
DEF_ANIMATOR(zombaThinkGoto)
DEF_ANIMATOR(zombaThinkPonder)
DEF_ANIMATOR(zombaThinkSearch)
DEF_ANIMATOR(zombfThinkChase)
DEF_ANIMATOR(zombfThinkGoto)
DEF_ANIMATOR(zombfThinkSearch)
DEF_ANIMATOR(FireballSeqCallback)
DEF_ANIMATOR(Fx33Callback)
DEF_ANIMATOR(NapalmSeqCallback)
DEF_ANIMATOR(Fx32Callback)
DEF_ANIMATOR(TreeToGibCallback)
DEF_ANIMATOR(DudeToGibCallback1)
DEF_ANIMATOR(DudeToGibCallback2)
DEF_ANIMATOR(batBiteSeqCallback)
DEF_ANIMATOR(SlashSeqCallback)
DEF_ANIMATOR(StompSeqCallback)
DEF_ANIMATOR(eelBiteSeqCallback)
DEF_ANIMATOR(BurnSeqCallback)
DEF_ANIMATOR(SeqAttackCallback)
DEF_ANIMATOR(cerberusBiteSeqCallback)
DEF_ANIMATOR(cerberusBurnSeqCallback)
DEF_ANIMATOR(cerberusBurnSeqCallback2)
DEF_ANIMATOR(TommySeqCallback)
DEF_ANIMATOR(TeslaSeqCallback)
DEF_ANIMATOR(ShotSeqCallback)
DEF_ANIMATOR(cultThrowSeqCallback)
DEF_ANIMATOR(cultThrowSeqCallback2)
DEF_ANIMATOR(cultThrowSeqCallback3)
DEF_ANIMATOR(SlashFSeqCallback)
DEF_ANIMATOR(ThrowFSeqCallback)
DEF_ANIMATOR(BlastSSeqCallback)
DEF_ANIMATOR(ThrowSSeqCallback)
DEF_ANIMATOR(ghostSlashSeqCallback)
DEF_ANIMATOR(ghostThrowSeqCallback)
DEF_ANIMATOR(ghostBlastSeqCallback)
DEF_ANIMATOR(GillBiteSeqCallback)
DEF_ANIMATOR(HandJumpSeqCallback)
DEF_ANIMATOR(houndBiteSeqCallback)
DEF_ANIMATOR(houndBurnSeqCallback)
DEF_ANIMATOR(podPlaySound1)
DEF_ANIMATOR(podPlaySound2)
DEF_ANIMATOR(podAttack)
DEF_ANIMATOR(podExplode)
DEF_ANIMATOR(ratBiteSeqCallback)
DEF_ANIMATOR(SpidBiteSeqCallback)
DEF_ANIMATOR(SpidJumpSeqCallback)
DEF_ANIMATOR(SpidBirthSeqCallback)
DEF_ANIMATOR(tchernobogFire)
DEF_ANIMATOR(tchernobogBurnSeqCallback)
DEF_ANIMATOR(tchernobogBurnSeqCallback2)
DEF_ANIMATOR(genDudeAttack1)
DEF_ANIMATOR(punchCallback)
DEF_ANIMATOR(ThrowCallback1)
DEF_ANIMATOR(ThrowCallback2)
DEF_ANIMATOR(HackSeqCallback)
DEF_ANIMATOR(StandSeqCallback)
DEF_ANIMATOR(zombfHackSeqCallback)
DEF_ANIMATOR(PukeSeqCallback)
DEF_ANIMATOR(ThrowSeqCallback)
DEF_ANIMATOR(PlayerSurvive)
DEF_ANIMATOR(PlayerKneelsOver)
DEF_ANIMATOR(FireballTrapSeqCallback)
DEF_ANIMATOR(MGunFireSeqCallback)
DEF_ANIMATOR(MGunOpenSeqCallback)


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

static void bloodactor_evPostActorCallback(DBloodActor* act, int delta, int id)
{
	evPostActor(act, delta, (CALLBACK_ID)id);
}

DEFINE_ACTION_FUNCTION_NATIVE(DBloodActor, evPostActorCallback, bloodactor_evPostActorCallback)
{
	PARAM_SELF_PROLOGUE(DBloodActor);
	PARAM_INT(d);
	PARAM_INT(id);
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


END_BLD_NS
