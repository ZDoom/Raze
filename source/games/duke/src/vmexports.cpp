//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2023 - Christoph Oelckers

This file is part of Raze

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "vm.h"
#include "ns.h"
#include "buildtiles.h"
#include "global.h"
#include "funct.h"
BEGIN_DUKE_NS

void resetswitch(int tag);

//---------------------------------------------------------------------------
//
// global exports
//
//---------------------------------------------------------------------------

DDukePlayer* duke_getviewplayer()
{
	return getPlayer(screenpeek);
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, getviewplayer, duke_getviewplayer)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(duke_getviewplayer());
}

DDukePlayer* duke_getlocalplayer()
{
	return getPlayer(myconnectindex);
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, getlocalplayer, duke_getlocalplayer)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(duke_getlocalplayer());
}

DEFINE_ACTION_FUNCTION(_Duke, MaxAmmoAmount)
{
	PARAM_PROLOGUE;
	PARAM_INT(weap);
	int max = weap < 0 || weap >= MAX_WEAPONS ? 0 : gs.max_ammo_amount[weap];
	ACTION_RETURN_INT(max);
}

void S_PlaySpecialMusic(unsigned int m);

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, PlaySpecialMusic, S_PlaySpecialMusic)
{
	PARAM_PROLOGUE;
	PARAM_INT(song);
	S_PlaySpecialMusic(song);
	return 0;
}

static int duke_PlaySound(int num, int chan, int flags, double vol)
{
	return S_PlaySound(FSoundID::fromInt(num), chan, EChanFlags::FromInt(flags), float(vol));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, PlaySound, duke_PlaySound)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	PARAM_INT(chan);
	PARAM_INT(flags);
	PARAM_FLOAT(vol);
	ACTION_RETURN_INT(duke_PlaySound(snd, chan, flags, vol));
}
static void StopSound(int num)
{
	S_StopSound(num);
}

void duke_StopSound(int snd)
{
	S_StopSound(FSoundID::fromInt(snd));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, StopSound, duke_StopSound)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	duke_StopSound(snd);
	return 0;
}

int duke_CheckSoundPlaying(int snd)
{
	return S_CheckSoundPlaying(FSoundID::fromInt(snd));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, CheckSoundPlaying, duke_CheckSoundPlaying)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	ACTION_RETURN_INT(duke_CheckSoundPlaying(snd));
}

DDukePlayer* duke_checkcursectnums(sectortype* sector)
{
	if (!sector) return nullptr;
	int pp = checkcursectnums(sector);
	return pp >= 0 ? getPlayer(pp) : nullptr;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, checkcursectnums, duke_checkcursectnums)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_POINTER(duke_checkcursectnums(sect));
}

int duke_global_random()
{
	return global_random;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, global_random, duke_global_random)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_INT(global_random);
}

int duke_GetSoundFlags(int sndid)
{
	return S_GetUserFlags(FSoundID::fromInt(sndid));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, GetSoundFlags, duke_GetSoundFlags)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	ACTION_RETURN_INT(duke_GetSoundFlags(snd));
}

static int Duke_badguypic(int spawnno)
{
	auto clstype = GetSpawnType(spawnno);
	if (clstype && clstype->IsDescendantOf(RUNTIME_CLASS(DDukeActor)))
		return badguy(static_cast<DDukeActor*>(GetDefaultByType(clstype)));
	return false;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, badguyID, Duke_badguypic)
{
	PARAM_PROLOGUE;
	PARAM_INT(p);
	ACTION_RETURN_INT(Duke_badguypic(p));
}

void updatepindisplay(int tag, int pins);

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, updatepindisplay, updatepindisplay)
{
	PARAM_PROLOGUE;
	PARAM_INT(tag);
	PARAM_INT(mask);
	updatepindisplay(tag, mask);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, StartCommentary, StartCommentary)
{
	PARAM_PROLOGUE;
	PARAM_INT(tag);
	PARAM_POINTER(act, DDukeActor);
	ACTION_RETURN_BOOL(StartCommentary(tag, act));
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, StopCommentary, StopCommentary)
{
	PARAM_PROLOGUE;
	StopCommentary();
	return 0;
}

int getPlayerIndex(DDukePlayer* p)
{
	if (!p) return -1;
	return p->pnum;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, getPlayerIndex, getPlayerIndex)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(p, DDukePlayer);
	ACTION_RETURN_INT(getPlayerIndex(p));
}

int Duke_isaccessswitch(int texint)
{
	return isaccessswitch(FSetTextureID(texint));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, isaccessswitch, Duke_isaccessswitch)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	ACTION_RETURN_BOOL(Duke_isaccessswitch(v));
	return 0;
}

int Duke_isshootableswitch(int texint)
{
	return isshootableswitch(FSetTextureID(texint));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, isshootableswitch, Duke_isshootableswitch)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	ACTION_RETURN_BOOL(Duke_isshootableswitch(v));
	return 0;
}

int Duke_checksprite(PClassActor* self)
{
	auto texid = GetDefaultByType(self)->spr.spritetexture();
	auto tex = TexMan.GetGameTexture(texid);
	return tex && tex->isValid();
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, checksprite, Duke_checksprite)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(v, PClassActor);
	ACTION_RETURN_BOOL(Duke_checksprite(v));
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, setnextmap, setnextmap)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	ACTION_RETURN_BOOL(setnextmap(v));
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, rnd, rnd)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	ACTION_RETURN_BOOL(rnd(v));
	return 0;
}

DEFINE_GLOBAL_UNSIZED(dlevel)
DEFINE_GLOBAL(camsprite)

//---------------------------------------------------------------------------
//
// DukeActor
//
//---------------------------------------------------------------------------

DEFINE_FIELD(DDukeActor, ownerActor)
DEFINE_FIELD(DDukeActor, attackertype)
DEFINE_FIELD(DDukeActor, hitOwnerActor)
DEFINE_FIELD(DDukeActor, cgg)
DEFINE_FIELD(DDukeActor, spriteextra)
DEFINE_FIELD(DDukeActor, hitang)
DEFINE_FIELD(DDukeActor, hitextra)
DEFINE_FIELD(DDukeActor, movflag)
DEFINE_FIELD(DDukeActor, tempval)
DEFINE_FIELD(DDukeActor, timetosleep)
DEFINE_FIELD(DDukeActor, mapSpawned)
DEFINE_FIELD(DDukeActor, floorz)
DEFINE_FIELD(DDukeActor, ceilingz)
DEFINE_FIELD(DDukeActor, saved_ammo)
DEFINE_FIELD(DDukeActor, palvals)
DEFINE_FIELD(DDukeActor, counter)
DEFINE_FIELD(DDukeActor, temp_data)
DEFINE_FIELD(DDukeActor, temp_actor)
DEFINE_FIELD(DDukeActor, seek_actor)
DEFINE_FIELD(DDukeActor, flags1)
DEFINE_FIELD(DDukeActor, flags2)
DEFINE_FIELD(DDukeActor, flags3)
DEFINE_FIELD(DDukeActor, spritesetindex)
DEFINE_FIELD(DDukeActor, temp_walls)
DEFINE_FIELD(DDukeActor, temp_sect)
DEFINE_FIELD(DDukeActor, actorstayput)
DEFINE_FIELD(DDukeActor, temp_pos)
DEFINE_FIELD(DDukeActor, temp_pos2)
DEFINE_FIELD(DDukeActor, temp_angle)
DEFINE_FIELD(DDukeActor, curAction)
DEFINE_FIELD(DDukeActor, curMove)
DEFINE_FIELD(DDukeActor, curAI)
DEFINE_FIELD(DDukeActor, actioncounter)

void TickActor(DDukeActor*);
DEFINE_ACTION_FUNCTION(DDukeActor, Tick)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	TickActor(self);
	return 0;
}
void setSpritesetImage(DDukeActor* self, unsigned int index)
{
	auto& spriteset = static_cast<PClassActor*>(self->GetClass())->ActorInfo()->SpriteSet;

	if (index >= spriteset.Size())
	{
		ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Bad sprite set index %d (max. allowed is %d", index, spriteset.Size() - 1);
	}
	self->spritesetindex = index;
	self->spr.setspritetexture(spriteset[index]);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, SetSpritesetImage, setSpritesetImage)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_UINT(index);
	setSpritesetImage(self, index);
	return 0;
}

static int getSpritesetSize(DDukeActor* self)
{
	auto& spriteset = static_cast<PClassActor*>(self->GetClass())->ActorInfo()->SpriteSet;
	return spriteset.Size();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, GetSpritesetSize, getSpritesetSize)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(getSpritesetSize(self));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, getglobalz, getglobalz)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	getglobalz(self);
	return 0;
}

DDukePlayer* DukeActor_findplayer(DDukeActor* self, double* dist)
{
	double a;
	return getPlayer(findplayer(self, dist? dist : &a));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, findplayer, DukeActor_findplayer)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	double d;
	auto p = DukeActor_findplayer(self, &d);
	if (numret > 0) ret[0].SetPointer(p);
	if (numret > 1) ret[1].SetFloat(d);
	return min(numret, 2);
}

DDukePlayer* DukeActor_getplayer(DDukeActor* self)
{
	return self->isPlayer() ? getPlayer(self->PlayerIndex()) : nullptr;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, getplayer, DukeActor_getplayer)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_POINTER(DukeActor_getplayer(self));
}

int DukeActor_ifhitbyweapon(DDukeActor* self)
{
	return fi.ifhitbyweapon(self);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, ifhitbyweapon, DukeActor_ifhitbyweapon)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(DukeActor_ifhitbyweapon(self));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, domove, ssp)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(clipmask);
	ACTION_RETURN_INT(ssp(self, clipmask));
}

int DukeActor_PlayActorSound(DDukeActor* self, int snd, int chan, int flags)
{
	return S_PlayActorSound(FSoundID::fromInt(snd), self, chan, EChanFlags::FromInt(flags));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, PlayActorSound, DukeActor_PlayActorSound)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(snd);
	PARAM_INT(chan);
	PARAM_INT(flags);
	ACTION_RETURN_INT(DukeActor_PlayActorSound(self, snd, chan, flags));
}

int DukeActor_IsSoundPlaying(DDukeActor* self, int snd, int chan)
{
	return S_CheckActorSoundPlaying(self, FSoundID::fromInt(snd), chan);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, CheckSoundPlaying, DukeActor_IsSoundPlaying)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(snd);
	PARAM_INT(chan);
	ACTION_RETURN_INT(DukeActor_IsSoundPlaying(self, snd, chan));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, CheckAnyActorSoundPlaying, S_CheckAnyActorSoundPlaying)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(S_CheckAnyActorSoundPlaying(self));
}

void DukeActor_StopSound(DDukeActor* self, int snd, int flags)
{
	S_StopSound(FSoundID::fromInt(snd), self, flags);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, StopSound, DukeActor_StopSound)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(snd);
	PARAM_INT(chan);
	DukeActor_StopSound(self, snd, chan);
	return 0;
}

DDukeActor* DukeActor_Spawn(DDukeActor* origin, PClassActor* cls)
{
	return spawn(origin, cls);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spawn, DukeActor_Spawn)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	ACTION_RETURN_POINTER(DukeActor_Spawn(self, type));
}

void DukeActor_Lotsofstuff(DDukeActor* actor, PClassActor* intname, int count)
{
	lotsofstuff(actor, count, intname);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, lotsofstuff, DukeActor_Lotsofstuff)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	PARAM_INT(count);
	lotsofstuff(self, count, type);
	return 0;
}

int DukeActor_movesprite(DDukeActor* actor, double velx, double vely, double velz, int clipmask)
{
	Collision coll;
	return movesprite_ex(actor, DVector3(velx, vely, velz), clipmask, coll);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, movesprite, DukeActor_movesprite)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_FLOAT(velx);
	PARAM_FLOAT(vely);
	PARAM_FLOAT(velz);
	PARAM_INT(clipmask);
	ACTION_RETURN_INT(DukeActor_movesprite(self, velx, vely, velz, clipmask));
}

int DukeActor_movesprite_ex(DDukeActor* actor, double velx, double vely, double velz, int clipmask, Collision* coll)
{
	return movesprite_ex(actor, DVector3(velx, vely, velz), clipmask, *coll);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, movesprite_ex, DukeActor_movesprite_ex)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_FLOAT(velx);
	PARAM_FLOAT(vely);
	PARAM_FLOAT(velz);
	PARAM_INT(clipmask);
	PARAM_POINTER(coll, Collision);
	ACTION_RETURN_INT(DukeActor_movesprite_ex(self, velx, vely, velz, clipmask, coll));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spawnsprite, spawnsprite)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(type);
	ACTION_RETURN_POINTER(spawnsprite(self, type));
}

void DukeActor_Lotsofglass(DDukeActor* origin, int count, walltype* wal)
{
	lotsofglass(origin, wal, count);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, lotsofglass, DukeActor_Lotsofglass)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(count);
	PARAM_POINTER(wall, walltype);
	DukeActor_Lotsofglass(self, count, wall);
	return 0;
}

void DukeActor_Lotsofcolourglass(DDukeActor* origin, int count, walltype* wal)
{
	lotsofcolourglass(origin, wal, count);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, lotsofcolourglass, DukeActor_Lotsofcolourglass)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(count);
	PARAM_POINTER(wall, walltype);
	DukeActor_Lotsofcolourglass(self, count, wall);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, makeitfall, makeitfall)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	makeitfall(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, RandomScrap, RANDOMSCRAP)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	RANDOMSCRAP(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, hitradius, hitradius)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(r);
	PARAM_INT(h1);
	PARAM_INT(h2);
	PARAM_INT(h3);
	PARAM_INT(h4);
	hitradius(self, r, h1, h2, h3, h4);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, hitasprite, hitasprite)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	DDukeActor* p;
	double d = hitasprite(self, &p);
	if (numret > 0) ret[0].SetFloat(d);
	if (numret > 1) ret[1].SetPointer(p);
	return min(numret, 2);
}

void DukeActor_detonate(DDukeActor* origin, PClassActor* type)
{
	detonate(origin, type);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, detonate, DukeActor_detonate)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	DukeActor_detonate(self, type);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, checkhitdefault, checkhitdefault)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_OBJECT(proj, DDukeActor);
	checkhitdefault(self, proj);
	return 0;
}

void DukeActor_operatesectors(DDukeActor* origin, sectortype* sector)
{
	operatesectors(sector, origin);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, operatesectors, DukeActor_operatesectors)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(sec, sectortype);
	operatesectors(sec, self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, badguy, badguy)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(badguy(self));
}

int duke_scripted(const DDukeActor* act)
{
	return act->conInfo() != nullptr || (act->flags4 & SFLAG4_CONOVERRIDE);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, scripted, duke_scripted)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(duke_scripted(self));
}

int duke_isplayer(DDukeActor* act)
{
	return act->isPlayer();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, isplayer, duke_isplayer)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(duke_isplayer(self));
}

int duke_spw(DDukeActor* act)
{
	auto tex = TexMan.GetGameTexture(act->spr.spritetexture());
	return (int)tex->GetDisplayWidth();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spritewidth, duke_spw)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(duke_spw(self));
}

int duke_sph(DDukeActor* act)
{
	auto tex = TexMan.GetGameTexture(act->spr.spritetexture());
	return (int)tex->GetDisplayHeight();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spriteheight, duke_sph)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(duke_sph(self));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, shoot, shoot)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	shoot(self, type);
	return 0;
}

void DukeActor_setclipDistFromTile(DDukeActor* a)
{
	a->setClipDistFromTile();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, setclipDistFromTile, DukeActor_setclipDistFromTile)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	DukeActor_setclipDistFromTile(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, insertspriteq, insertspriteq)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	insertspriteq(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, operateforcefields, operateforcefields)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(tag);
	operateforcefields(self, tag);
	return 0;
}

void DukeActor_restoreloc(DDukeActor* self)
{
	self->restoreloc();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, restoreloc, DukeActor_restoreloc)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(tag);
	DukeActor_restoreloc(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, fakebubbaspawn, fakebubbaspawn)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	fakebubbaspawn(self, p);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, aim, aim_)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(weapon, DDukeActor);
	PARAM_FLOAT(aimangle);
	bool b = false;
	auto result = aim_(self, weapon, aimangle, &b);
	if (numret > 0) ret[0].SetPointer(result);
	if (numret > 1) ret[1].SetInt(b);
	return min(numret, 2);
}

void Duke_SetAction(DDukeActor* self, int intname)
{
	int ndx = LookupAction(self->GetClass(), FName(ENamedName(intname)));
	self->curAction = &actions[ndx];
	self->actioncounter = self->curframe = 0;

}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, SetAction, Duke_SetAction)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(n);
	Duke_SetAction(self, n);
	return 0;
}

void Duke_SetMove(DDukeActor* self, int intname, int flags)
{
	int ndx = LookupMove(self->GetClass(), FName(ENamedName(intname)));
	self->curMove = &moves[ndx];
	self->spr.hitag = flags;
	self->counter = 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, SetMove, Duke_SetMove)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(n);
	PARAM_INT(f);
	Duke_SetMove(self, n, f);
	return 0;
}

void Duke_SetAI(DDukeActor* self, int intname)
{
	int ndx = LookupAI(self->GetClass(), FName(ENamedName(intname)));
	auto ai = &ais[ndx];
	assert(!(ai->move & 0x80000000));
	assert(!(ai->action & 0x80000000));
	self->curMove = &moves[ai->move];
	self->curAction = &actions[ai->action];
	self->spr.hitag = ai->moveflags;
	self->curAI = ai->name;
	self->actioncounter = self->curframe = 0;
	self->counter = 0;
	if (self->spr.hitag & random_angle)
		self->spr.Angles.Yaw = randomAngle();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, SetAI, Duke_SetAI)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(n);
	Duke_SetAI(self, n);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, checkp, checkp)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	PARAM_INT(n);
	ACTION_RETURN_INT(checkp(self, p, n));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, cansee, ifcansee)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	PARAM_INT(n);
	ACTION_RETURN_INT(ifcansee(self, p));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, actoroperate, actoroperate)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	actoroperate(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, ifsquished, ifsquished)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	ACTION_RETURN_INT(ifcansee(self, p));
}

void Duke_ChangeType(DDukeActor* self, PClassActor* type)
{
	self->ChangeType(type);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, ChangeType, Duke_ChangeType)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	self->ChangeType(type);
	return 0;
}

void Duke_fall(DDukeActor* self, DDukePlayer* p)
{
	fall(self, p->GetPlayerNum());
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, fall, Duke_fall)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	Duke_fall(self, p);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, actorsizeto, actorsizeto)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	actorsizeto(self, x, y);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, dodge, dodge)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(dodge(self));
}

int Duke_ifcanshoottarget(DDukeActor* self, DDukePlayer* p, double dist)
{
	return ifcanshoottarget(self, p->GetPlayerNum(), int(dist * worldtoint));
}
DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, ifcanshoottarget, Duke_ifcanshoottarget)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(p, DDukePlayer);
	PARAM_FLOAT(x);
	ACTION_RETURN_INT(Duke_ifcanshoottarget(self, p, x));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spriteglass, spriteglass)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(x);
	spriteglass(self, x);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spawndebris, spawndebris)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(x);
	PARAM_INT(y);
	spawndebris(self, x, y);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, respawnhitag, respawnhitag)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	respawnhitag(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, destroyit, destroyit)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	destroyit(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, mamaspawn, mamaspawn)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	mamaspawn(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, garybanjo, garybanjo)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	garybanjo(self);
	return 0;
}

int duke_GetambientSound(DDukeActor* actor)
{
	return ambienttags.SafeGet(actor->spr.detail, {}).lo;
}
DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, GetAmbientSound, duke_GetambientSound)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_INT(duke_GetambientSound(self));
}

double duke_GetambientDist(DDukeActor* actor)
{
	return ambienttags.SafeGet(actor->spr.detail, {}).hi * maptoworld;
}
DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, GetAmbientDist, duke_GetambientDist)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_FLOAT(duke_GetambientDist(self));
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, addkill, addkill)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	addkill(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, subkill, subkill)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	subkill(self);
	return 0;
}

DEFINE_ACTION_FUNCTION(DDukeActor, killit)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	if (self->flags4 & SFLAG4_INRUNSTATE)
	{
		// this has to be done the hard way. The original CON interpreter stops running commands once an actor is set to be killed.
		// The only safe way to handle this in native code while exiting all nested functions is throwing an exception.
		throw CDukeKillEvent(1);
	}
	else
	{
		// if it is something else than the currently ticked actor we can outright destroy it.
		self->Destroy();
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// DukePlayer
//
//---------------------------------------------------------------------------

DEFINE_FIELD(DDukePlayer, actor)
DEFINE_FIELD(DDukePlayer, gotweapon)
DEFINE_FIELD(DDukePlayer, pals)
DEFINE_FIELD(DDukePlayer, weapon_sway)
DEFINE_FIELD(DDukePlayer, oweapon_sway)
DEFINE_FIELD(DDukePlayer, weapon_pos)
DEFINE_FIELD(DDukePlayer, kickback_pic)
DEFINE_FIELD(DDukePlayer, random_club_frame)
DEFINE_FIELD(DDukePlayer, oweapon_pos)
DEFINE_FIELD(DDukePlayer, okickback_pic)
DEFINE_FIELD(DDukePlayer, orandom_club_frame)
DEFINE_FIELD(DDukePlayer, hard_landing)
DEFINE_FIELD(DDukePlayer, ohard_landing)
DEFINE_FIELD(DDukePlayer, psectlotag)
DEFINE_FIELD(DDukePlayer, loogie)
DEFINE_FIELD(DDukePlayer, numloogs)
DEFINE_FIELD(DDukePlayer, loogcnt)
DEFINE_FIELD(DDukePlayer, invdisptime)
//DEFINE_FIELD(DukePlayer, exitx)
//DEFINE_FIELD(DukePlayer, exity)
//DEFINE_FIELD(DukePlayer, bobposx)
//DEFINE_FIELD(DukePlayer, bobposy)
DEFINE_FIELD(DDukePlayer, pyoff)
DEFINE_FIELD(DDukePlayer, opyoff)
DEFINE_FIELD(DDukePlayer, vel)
DEFINE_FIELD(DDukePlayer, last_pissed_time)
DEFINE_FIELD(DDukePlayer, truefz)
DEFINE_FIELD(DDukePlayer, truecz)
DEFINE_FIELD(DDukePlayer, player_par)
DEFINE_FIELD(DDukePlayer, visibility)
DEFINE_FIELD(DDukePlayer, bobcounter)
DEFINE_FIELD(DDukePlayer, randomflamex)
DEFINE_FIELD(DDukePlayer, crack_time)
DEFINE_FIELD(DDukePlayer, aim_mode)
DEFINE_FIELD(DDukePlayer, ftt)
DEFINE_FIELD(DDukePlayer, cursector)
DEFINE_FIELD(DDukePlayer, last_extra)
DEFINE_FIELD(DDukePlayer, subweapon)
DEFINE_FIELD(DDukePlayer, ammo_amount)
DEFINE_FIELD(DDukePlayer, frag)
DEFINE_FIELD(DDukePlayer, fraggedself)
DEFINE_FIELD(DDukePlayer, curr_weapon)
DEFINE_FIELD(DDukePlayer, last_weapon)
DEFINE_FIELD(DDukePlayer, tipincs)
DEFINE_FIELD(DDukePlayer, wantweaponfire)
DEFINE_FIELD(DDukePlayer, holoduke_amount)
DEFINE_FIELD(DDukePlayer, hurt_delay)
DEFINE_FIELD(DDukePlayer, hbomb_hold_delay)
DEFINE_FIELD(DDukePlayer, jumping_counter)
DEFINE_FIELD(DDukePlayer, airleft)
DEFINE_FIELD(DDukePlayer, knee_incs)
DEFINE_FIELD(DDukePlayer, access_incs)
DEFINE_FIELD(DDukePlayer, ftq)
DEFINE_FIELD(DDukePlayer, access_wall)
DEFINE_FIELD(DDukePlayer, got_access)
DEFINE_FIELD(DDukePlayer, weapon_ang)
DEFINE_FIELD(DDukePlayer, firstaid_amount)
DEFINE_FIELD(DDukePlayer, one_parallax_sectnum)
DEFINE_FIELD(DDukePlayer, over_shoulder_on)
DEFINE_FIELD(DDukePlayer, fist_incs)
DEFINE_FIELD(DDukePlayer, cheat_phase)
DEFINE_FIELD(DDukePlayer, extra_extra8)
DEFINE_FIELD(DDukePlayer, quick_kick)
DEFINE_FIELD(DDukePlayer, last_quick_kick)
DEFINE_FIELD(DDukePlayer, heat_amount)
DEFINE_FIELD(DDukePlayer, timebeforeexit)
DEFINE_FIELD(DDukePlayer, customexitsound)
DEFINE_FIELD(DDukePlayer, interface_toggle_flag)
DEFINE_FIELD(DDukePlayer, dead_flag)
DEFINE_FIELD(DDukePlayer, show_empty_weapon)
DEFINE_FIELD(DDukePlayer, scuba_amount)
DEFINE_FIELD(DDukePlayer, jetpack_amount)
DEFINE_FIELD(DDukePlayer, steroids_amount)
DEFINE_FIELD(DDukePlayer, shield_amount)
DEFINE_FIELD(DDukePlayer, pycount)
DEFINE_FIELD(DDukePlayer, frag_ps)
DEFINE_FIELD(DDukePlayer, transporter_hold)
DEFINE_FIELD(DDukePlayer, last_full_weapon)
DEFINE_FIELD(DDukePlayer, footprintshade)
DEFINE_FIELD(DDukePlayer, boot_amount)
DEFINE_FIELD(DDukePlayer, on_warping_sector)
DEFINE_FIELD(DDukePlayer, footprintcount)
DEFINE_FIELD(DDukePlayer, hbomb_on)
DEFINE_FIELD(DDukePlayer, jumping_toggle)
DEFINE_FIELD(DDukePlayer, rapid_fire_hold)
DEFINE_FIELD(DDukePlayer, on_ground)
DEFINE_FIELD(DDukePlayer, inven_icon)
DEFINE_FIELD(DDukePlayer, buttonpalette)
DEFINE_FIELD(DDukePlayer, jetpack_on)
DEFINE_FIELD(DDukePlayer, spritebridge)
DEFINE_FIELD(DDukePlayer, lastrandomspot)
DEFINE_FIELD(DDukePlayer, scuba_on)
DEFINE_FIELD(DDukePlayer, footprintpal)
DEFINE_FIELD(DDukePlayer, heat_on)
DEFINE_FIELD(DDukePlayer, holster_weapon)
DEFINE_FIELD(DDukePlayer, falling_counter)
DEFINE_FIELD(DDukePlayer, refresh_inventory)
DEFINE_FIELD(DDukePlayer, toggle_key_flag)
DEFINE_FIELD(DDukePlayer, knuckle_incs)
DEFINE_FIELD(DDukePlayer, walking_snd_toggle)
DEFINE_FIELD(DDukePlayer, palookup)
DEFINE_FIELD(DDukePlayer, quick_kick_msg)
DEFINE_FIELD(DDukePlayer, stairs)
DEFINE_FIELD(DDukePlayer, detonate_count)
//DEFINE_FIELD(DukePlayer, noise.X)
//DEFINE_FIELD(DukePlayer, noise.Y)
DEFINE_FIELD(DDukePlayer, noise_radius)
DEFINE_FIELD(DDukePlayer, drink_timer)
DEFINE_FIELD(DDukePlayer, eat_timer)
DEFINE_FIELD(DDukePlayer, SlotWin)
DEFINE_FIELD(DDukePlayer, recoil)
DEFINE_FIELD(DDukePlayer, detonate_time)
DEFINE_FIELD(DDukePlayer, yehaa_timer)
DEFINE_FIELD(DDukePlayer, drink_amt)
DEFINE_FIELD(DDukePlayer, eat)
DEFINE_FIELD(DDukePlayer, drunkang)
DEFINE_FIELD(DDukePlayer, eatang)
DEFINE_FIELD(DDukePlayer, shotgun_state)
DEFINE_FIELD(DDukePlayer, donoise)
DEFINE_FIELD(DDukePlayer, keys)
DEFINE_FIELD(DDukePlayer, drug_aspect)
DEFINE_FIELD(DDukePlayer, drug_timer)
DEFINE_FIELD(DDukePlayer, SeaSick)
DEFINE_FIELD(DDukePlayer, MamaEnd)
DEFINE_FIELD(DDukePlayer, moto_drink)
DEFINE_FIELD(DDukePlayer, TiltStatus)
DEFINE_FIELD(DDukePlayer, oTiltStatus)
DEFINE_FIELD(DDukePlayer, VBumpNow)
DEFINE_FIELD(DDukePlayer, VBumpTarget)
DEFINE_FIELD(DDukePlayer, TurbCount)
DEFINE_FIELD(DDukePlayer, drug_stat)
DEFINE_FIELD(DDukePlayer, DrugMode)
DEFINE_FIELD(DDukePlayer, lotag800kill)
DEFINE_FIELD(DDukePlayer, sea_sick_stat)
DEFINE_FIELD(DDukePlayer, hurt_delay2)
DEFINE_FIELD(DDukePlayer, nocheat)
DEFINE_FIELD(DDukePlayer, OnMotorcycle)
DEFINE_FIELD(DDukePlayer, OnBoat)
DEFINE_FIELD(DDukePlayer, moto_underwater)
DEFINE_FIELD(DDukePlayer, NotOnWater)
DEFINE_FIELD(DDukePlayer, MotoOnGround)
DEFINE_FIELD(DDukePlayer, moto_do_bump)
DEFINE_FIELD(DDukePlayer, moto_bump_fast)
DEFINE_FIELD(DDukePlayer, moto_on_oil)
DEFINE_FIELD(DDukePlayer, moto_on_mud)
DEFINE_FIELD(DDukePlayer, MotoSpeed)
DEFINE_FIELD(DDukePlayer, holoduke_on)
DEFINE_FIELD(DDukePlayer, actorsqu)
DEFINE_FIELD(DDukePlayer, wackedbyactor)
DEFINE_FIELD(DDukePlayer, on_crane)
DEFINE_FIELD(DDukePlayer, somethingonplayer)
DEFINE_FIELD(DDukePlayer, access_spritenum)
DEFINE_FIELD(DDukePlayer, dummyplayersprite)
DEFINE_FIELD(DDukePlayer, newOwner)
DEFINE_FIELD(DDukePlayer, fric)

DEFINE_ACTION_FUNCTION(_DukePlayer, IsFrozen)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	ACTION_RETURN_BOOL(self->GetActor()->spr.pal == 1 && self->last_extra < 2);
}

DEFINE_ACTION_FUNCTION(_DukePlayer, GetGameVar)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_STRING(name);
	PARAM_INT(def);
	ACTION_RETURN_INT(GetGameVar(name, def, self->GetActor(), self->GetPlayerNum()).safeValue());
}

void dukeplayer_backuppos(DDukePlayer* self)
{
	self->backuppos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, backuppos, dukeplayer_backuppos)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	dukeplayer_backuppos(self);
	return 0;
}

void dukeplayer_backupxyz(DDukePlayer* self)
{
	self->GetActor()->backuppos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, backupxyz, dukeplayer_backupxyz)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	dukeplayer_backupxyz(self);
	return 0;
}

void dukeplayer_setpos(DDukePlayer* self, double x, double y, double z)
{
	self->GetActor()->spr.pos = { x, y, z + self->GetActor()->viewzoffset };
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, setpos, dukeplayer_setpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	dukeplayer_setpos(self, x, y, z);
	return 0;
}

void dukeplayer_addpos(DDukePlayer* self, double x, double y, double z)
{
	self->GetActor()->spr.pos += { x, y, z };
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addpos, dukeplayer_addpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	dukeplayer_addpos(self, x, y, z);
	return 0;
}

void dukeplayer_centerview(DDukePlayer* self)
{
	self->cmd.ucmd.actions |= SB_CENTERVIEW;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, centerview, dukeplayer_centerview)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	self->cmd.ucmd.actions |= SB_CENTERVIEW;
	return 0;
}

inline int DukePlayer_PlayerInput(DDukePlayer* pl, int bit)
{
	return (!!((pl->cmd.ucmd.actions) & ESyncBits::FromInt(bit)));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerinput, DukePlayer_PlayerInput)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(bit);
	ACTION_RETURN_INT(DukePlayer_PlayerInput(self, bit));
}

void dukeplayer_settargetangle(DDukePlayer* self, double a, int backup)
{
	self->GetActor()->spr.Angles.Yaw = DAngle::fromDeg(a);
	if (backup) self->GetActor()->PrevAngles.Yaw = self->GetActor()->spr.Angles.Yaw;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, settargetangle, dukeplayer_settargetangle)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_FLOAT(a);
	PARAM_BOOL(bak);
	dukeplayer_settargetangle(self, a, bak);
	return 0;
}

double dukeplayer_angle(DDukePlayer* self)
{
	return self->GetActor()->spr.Angles.Yaw.Degrees();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, angle, dukeplayer_angle)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	ACTION_RETURN_FLOAT(dukeplayer_angle(self));
}

void dukeplayer_addpitch(DDukePlayer* self, double a)
{
	self->GetActor()->spr.Angles.Pitch += DAngle::fromDeg(a);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addpitch, dukeplayer_addpitch)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_FLOAT(a);
	dukeplayer_addpitch(self, a);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, clearcameras, clearcameras)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	clearcameras(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, quickkill, quickkill)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	quickkill(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, CheckWeapRec, CheckWeapRec)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(ac, DDukeActor);
	PARAM_BOOL(test);
	ACTION_RETURN_INT(CheckWeapRec(self, ac, test));
}

void DukePlayer_addammo(DDukePlayer* p, int ammo, int amount)
{
	if ((unsigned)ammo >= MAX_WEAPONS) ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Ammo number out of range");
	addammo(ammo, p, amount);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addammo, DukePlayer_addammo)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(type);
	PARAM_INT(amount);
	DukePlayer_addammo(self, type, amount);
	return 0;
}

void DukePlayer_addweapon(DDukePlayer* p, int wpn, int switchit)
{
	if ((unsigned)wpn >= MAX_WEAPONS) ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Weapon number out of range");
	fi.addweapon(p, wpn, switchit);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addweapon, DukePlayer_addweapon)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(type);
	PARAM_INT(switchit);
	DukePlayer_addweapon(self, type, switchit);
	return 0;

}

DEFINE_ACTION_FUNCTION(_DukePlayer, hitablockingwall)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	walltype* pwal;
	hitawall(self, &pwal);
	ACTION_RETURN_BOOL(pwal && pwal->overtexture.isValid());
}

inline double DukePlayer_GetPitchwithView(DDukePlayer* pl)
{
	return pl->Angles.getPitchWithView().Degrees();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, GetPitchwithView, DukePlayer_GetPitchwithView)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	ACTION_RETURN_FLOAT(DukePlayer_GetPitchwithView(self));
}

inline void DukePlayer_setbobpos(DDukePlayer* pl)
{
	return pl->setbobpos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, setbobpos, DukePlayer_setbobpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	self->setbobpos();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, StartMotorcycle, OnMotorcycle)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	OnMotorcycle(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, StartBoat, OnBoat)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	OnBoat(self);
	return 0;
}

void pl_checkhitswitch(DDukePlayer* p, walltype* wal, DDukeActor* act)
{
	checkhitswitch(p, wal, act);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, checkhitswitch, pl_checkhitswitch)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(wal, walltype);
	PARAM_POINTER(act, DDukeActor);
	pl_checkhitswitch(self, wal, act);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerkick, playerkick)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(act, DDukeActor);
	playerkick(self, act);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerstomp, playerstomp)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(act, DDukeActor);
	playerstomp(self, act);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerreset, playerreset)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(act, DDukeActor);
	playerreset(self, act);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addphealth, addphealth)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(amt);
	PARAM_INT(big);
	addphealth(self, amt, big);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, wackplayer, wackplayer)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	wackplayer(self);
	return 0;
}

static void duke_checkweapons(DDukePlayer* p)
{
	fi.checkweapons(p);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, checkweapons, duke_checkweapons)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	duke_checkweapons(self);
	return 0;
}

static void msg(DDukePlayer* p, int num)
{
	FTA(num, p);
}
DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, FTA, msg)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(num);
	FTA(num, self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playercheckinventory, playercheckinventory)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(act, DDukeActor);
	PARAM_INT(num);
	PARAM_INT(amt);
	ACTION_RETURN_BOOL(playercheckinventory(self, act, num, amt));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playeraddinventory, playeraddinventory)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_POINTER(act, DDukeActor);
	PARAM_INT(num);
	PARAM_INT(amt);
	playeraddinventory(self, act, num, amt);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playeraddweapon, playeraddweapon)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(num);
	PARAM_INT(amt);
	ACTION_RETURN_BOOL(playeraddweapon(self, num, amt));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playeraddammo, playeraddammo)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(num);
	PARAM_INT(amt);
	ACTION_RETURN_BOOL(playeraddammo(self, num, amt));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, forceplayerangle, forceplayerangle)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	forceplayerangle(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playereat, playereat)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(amt);
	PARAM_BOOL(big);
	ACTION_RETURN_BOOL(playereat(self, amt, big));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerdrink, playerdrink)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukePlayer);
	PARAM_INT(amt);
	playerdrink(self, amt);
	return 0;
}

static DDukeActor* duke_firstStat(DukeStatIterator* it, int statnum)
{
	it->Reset(statnum);
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeStatIterator, First, duke_firstStat)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeStatIterator);
	PARAM_INT(Sect);
	ACTION_RETURN_POINTER(duke_firstStat(self, Sect));
}

static DDukeActor* duke_nextStat(DukeStatIterator* it)
{
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeStatIterator, Next, duke_nextStat)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeStatIterator);
	ACTION_RETURN_POINTER(duke_nextStat(self));
}

static DDukeActor* duke_firstSect(DukeSectIterator* it, sectortype* sect)
{
	if (sect == nullptr) return nullptr;
	it->Reset(sect);
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeSectIterator, First, duke_firstSect)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeSectIterator);
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_POINTER(duke_firstSect(self, sect));
}

static DDukeActor* duke_nextSect(DukeSectIterator* it)
{
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeSectIterator, Next, duke_nextSect)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeSectIterator);
	ACTION_RETURN_POINTER(duke_nextSect(self));
}

static DDukeActor* duke_firstSprite(DukeSpriteIterator* it)
{
	it->Reset();
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeSpriteIterator, First, duke_firstSprite)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeSpriteIterator);
	ACTION_RETURN_POINTER(duke_firstSprite(self));
}

static DDukeActor* duke_nextSprite(DukeSpriteIterator* it)
{
	return it->Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeSpriteIterator, Next, duke_nextSprite)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeSpriteIterator);
	ACTION_RETURN_POINTER(duke_nextSprite(self));
}

DDukeActor* DukeLevel_SpawnActor(DukeLevel* self, sectortype* sect, double x, double y, double z, PClassActor* type, int shade, double scalex, double scaley, double angle, double vel, double zvel, DDukeActor* owner, int stat)
{
	return SpawnActor(sect, DVector3(x, y, z), type, shade, DVector2(scalex, scaley), DAngle::fromDeg(angle), vel, zvel, owner, stat);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, SpawnActor, DukeLevel_SpawnActor)
{
	PARAM_SELF_STRUCT_PROLOGUE(DukeLevel);
	PARAM_POINTER(sect, sectortype);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_CLASS(type, DDukeActor);
	PARAM_INT(shade);
	PARAM_FLOAT(scalex);
	PARAM_FLOAT(scaley);
	PARAM_FLOAT(angle);
	PARAM_FLOAT(vel);
	PARAM_FLOAT(zvel);
	PARAM_OBJECT(owner, DDukeActor);
	PARAM_INT(stat);

	ACTION_RETURN_POINTER(DukeLevel_SpawnActor(self, sect, x, y, z, static_cast<PClassActor*>(type), shade, scalex, scaley, angle, vel, zvel, owner, stat));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, check_activator_motion, check_activator_motion)
{
	PARAM_PROLOGUE;
	PARAM_INT(lotag);
	ACTION_RETURN_INT(check_activator_motion(lotag));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, operatemasterswitches, operatemasterswitches)
{
	PARAM_PROLOGUE;
	PARAM_INT(lotag);
	operatemasterswitches(lotag);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, operateactivators, operateactivators)
{
	PARAM_PROLOGUE;
	PARAM_INT(lotag);
	PARAM_POINTER(p, DDukePlayer);
	operateactivators(lotag, p);
	return 0;
}

int duke_floorsurface(sectortype* sector)
{
	return tilesurface(sector->floortexture);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, floorsurface, duke_floorsurface)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_INT(duke_floorsurface(sect));
}

int duke_ceilingsurface(sectortype* sector)
{
	return tilesurface(sector->ceilingtexture);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, ceilingsurface, duke_ceilingsurface)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_INT(duke_ceilingsurface(sect));
}

int duke_ismirror(walltype* wal)
{
	return wal->walltexture == mirrortex || wal->overtexture == mirrortex;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, ismirror, duke_ismirror)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(wal, walltype);
	ACTION_RETURN_BOOL(duke_ismirror(wal));
}

void duke_checkhitwall(walltype* wal, DDukeActor * actor, double x, double y, double z)
{
	checkhitwall(actor, wal, DVector3(x, y, z));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, checkhitwall, duke_checkhitwall)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(wal, walltype);
	PARAM_POINTER(act, DDukeActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	duke_checkhitwall(wal, act, x, y, z);
	return 0;
}

void duke_checkhitceiling(sectortype* sect, DDukeActor* actor)
{
	checkhitceiling(sect); // actor is currently unused, this may change.
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, checkhitceiling, duke_checkhitceiling)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(wal, sectortype);
	PARAM_POINTER(act, DDukeActor);
	checkhitceiling(wal);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, addcycler, addcycler)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	PARAM_INT(lotag);
	PARAM_INT(shade);
	PARAM_INT(shade2);
	PARAM_INT(hitag);
	PARAM_INT(state);
	addcycler(sect, lotag, shade, shade2, hitag, state);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, addtorch, addtorch)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	PARAM_INT(shade);
	PARAM_INT(lotag);
	addtorch(sect, shade, lotag);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, addlightning, addlightning)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	PARAM_INT(shade);
	addlightning(sect, shade);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, addambient, addambient)
{
	PARAM_PROLOGUE;
	PARAM_INT(hitag);
	PARAM_INT(lotag);
	ACTION_RETURN_INT(addambient(hitag, lotag));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, resetswitch, resetswitch)
{
	PARAM_PROLOGUE;
	PARAM_INT(tag);
	resetswitch(tag);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, LocateTheLocator, LocateTheLocator)
{
	PARAM_PROLOGUE;
	PARAM_INT(tag);
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_POINTER(LocateTheLocator(tag, sect));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, getanimationindex, getanimationindex)
{
	PARAM_PROLOGUE;
	PARAM_INT(tag);
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_INT(getanimationindex(tag, sect));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, setanimation, static_cast<int(*)(sectortype*, int, sectortype*, double, double)>(setanimation))
{
	PARAM_PROLOGUE;
	PARAM_POINTER(asect, sectortype);
	PARAM_INT(tag);
	PARAM_POINTER(sect, sectortype);
	PARAM_FLOAT(dest);
	PARAM_FLOAT(vel);
	ACTION_RETURN_INT(setanimation(asect, tag, sect, dest, vel));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, tearitup, tearitup)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	tearitup(sect);
	return 0;
}


DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, max_ammo_amount);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, playerfriction);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, gravity);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, respawnactortime);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, bouncemineblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, respawnitemtime);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, morterblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, numfreezebounces);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, pipebombblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, rpgblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, seenineblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, shrinkerblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, tripbombblastradius);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, camerashitable);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, max_player_health);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, max_armour_amount);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, lasermode);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, freezerhurtowner);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, impact_damage);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, playerheight);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, gutsscale);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, displayflags);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, tripbombcontrol);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, stickybomb_lifetime);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, stickybomb_lifetime_var);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, grenade_lifetime);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, grenade_lifetime_var);
DEFINE_FIELD_X(DukeGameInfo, DukeGameInfo, weaponsandammosprites);

DEFINE_GLOBAL_UNSIZED(gs)

DEFINE_FIELD_X(DukeUserDefs, user_defs, mapflags);
DEFINE_FIELD_X(DukeUserDefs, user_defs, god);
DEFINE_FIELD_X(DukeUserDefs, user_defs, cashman);
DEFINE_FIELD_X(DukeUserDefs, user_defs, eog);
DEFINE_FIELD_X(DukeUserDefs, user_defs, clipping);
DEFINE_FIELD_X(DukeUserDefs, user_defs, user_pals);
DEFINE_FIELD_X(DukeUserDefs, user_defs, from_bonus);
DEFINE_FIELD_X(DukeUserDefs, user_defs, last_level);
DEFINE_FIELD_X(DukeUserDefs, user_defs, secretlevel);
DEFINE_FIELD_X(DukeUserDefs, user_defs, const_visibility);
DEFINE_FIELD_X(DukeUserDefs, user_defs, coop);
DEFINE_FIELD_X(DukeUserDefs, user_defs, respawn_monsters);
DEFINE_FIELD_X(DukeUserDefs, user_defs, respawn_items);
DEFINE_FIELD_X(DukeUserDefs, user_defs, respawn_inventory);
DEFINE_FIELD_X(DukeUserDefs, user_defs, recstat); 
DEFINE_FIELD_X(DukeUserDefs, user_defs, monsters_off);
DEFINE_FIELD_X(DukeUserDefs, user_defs, brightness);
DEFINE_FIELD_X(DukeUserDefs, user_defs, ffire);
DEFINE_FIELD_X(DukeUserDefs, user_defs, multimode);
DEFINE_FIELD_X(DukeUserDefs, user_defs, pistonsound);
DEFINE_FIELD_X(DukeUserDefs, user_defs, fogactive);
DEFINE_FIELD_X(DukeUserDefs, user_defs, player_skill);
DEFINE_FIELD_X(DukeUserDefs, user_defs, marker);
DEFINE_FIELD_X(DukeUserDefs, user_defs, bomb_tag);
DEFINE_FIELD_X(DukeUserDefs, user_defs, cameraactor);
DEFINE_FIELD_X(DukeUserDefs, user_defs, chickenplant);
DEFINE_FIELD_X(DukeUserDefs, user_defs, earthquaketime);
DEFINE_FIELD_X(DukeUserDefs, user_defs, ufospawnsminion);
DEFINE_FIELD_X(DukeUserDefs, user_defs, joe9000);
DEFINE_GLOBAL_UNSIZED(ud)

DEFINE_FIELD_X(ActorMove, ActorMove, qualifiedName);
DEFINE_FIELD_X(ActorMove, ActorMove, name);
DEFINE_FIELD_X(ActorMove, ActorMove, movex);
DEFINE_FIELD_X(ActorMove, ActorMove, movez);

DEFINE_FIELD_X(ActorAction, ActorAction, qualifiedName);
DEFINE_FIELD_X(ActorAction, ActorAction, name);
DEFINE_FIELD_X(ActorAction, ActorAction, base);
DEFINE_FIELD_X(ActorAction, ActorAction, offset);
DEFINE_FIELD_X(ActorAction, ActorAction, numframes);
DEFINE_FIELD_X(ActorAction, ActorAction, rotationtype);
DEFINE_FIELD_X(ActorAction, ActorAction, increment);
DEFINE_FIELD_X(ActorAction, ActorAction, delay);


void tspritetype_setWeaponOrAmmoSprite(tspritetype* targ, unsigned z)
{
	if (z < 15)
	{
		targ->setspritetexture(GetDefaultByType(gs.weaponsandammosprites[z])->spr.spritetexture());
	}
}

DEFINE_ACTION_FUNCTION_NATIVE(_tspritetype, setWeaponOrAmmoSprite, tspritetype_setWeaponOrAmmoSprite)
{
	PARAM_SELF_STRUCT_PROLOGUE(tspritetype);
	PARAM_INT(z);
	tspritetype_setWeaponOrAmmoSprite(self, z);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_tspritetype, copyfloorpal, copyfloorpal)
{
	PARAM_SELF_STRUCT_PROLOGUE(tspritetype);
	PARAM_POINTER(s, sectortype);
	copyfloorpal(self, s);
	return 0;
}

// this must still work around the lack of proper texture support on the script side and should go away once sprites have proper texture support
DEFINE_ACTION_FUNCTION(DDukeGenericDestructible, SetBroken)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukeActor);
	PARAM_INT(bust);
	FSetTextureID tilenum(self->IntVar(bust ? NAME_brokenstate : NAME_spawnstate));
	if (tilenum.isValid()) self->spr.setspritetexture(tilenum);
	ACTION_RETURN_BOOL(!tilenum.isValid());
}




void spawnguts(DDukeActor* origin, PClass* type, int count)
{
	IFVM(DukeActor, spawnguts)
	{
		VMValue params[] = { (DObject*)origin, type, count };
		VMCall(func, params, 3, nullptr, 0);
	}
}


END_DUKE_NS
