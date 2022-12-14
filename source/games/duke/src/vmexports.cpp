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

player_struct* duke_getviewplayer()
{
	return &ps[screenpeek];
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, getviewplayer, duke_getviewplayer)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(duke_getviewplayer());
}

player_struct* duke_getlocalplayer()
{
	return &ps[myconnectindex];
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

player_struct* duke_checkcursectnums(sectortype* sector)
{
	if (!sector) return nullptr;
	int pp = checkcursectnums(sector);
	return pp >= 0 ? &ps[pp] : nullptr;
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

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, badguyID, badguypic)
{
	PARAM_PROLOGUE;
	PARAM_INT(p);
	ACTION_RETURN_INT(badguypic(p));
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

int getPlayerIndex(player_struct* p)
{
	if (!p) return -1;
	return int(p - ps);
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, getPlayerIndex, getPlayerIndex)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(p, player_struct);
	ACTION_RETURN_INT(getPlayerIndex(p));
}

void setlastvisinc(int v)
{
	lastvisinc = PlayClock + v;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, setlastvisinc, setlastvisinc)
{
	PARAM_PROLOGUE;
	PARAM_INT(v);
	setlastvisinc(v);
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
	self->spr.picnum = spriteset[index];
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

player_struct* DukeActor_findplayer(DDukeActor* self, double* dist)
{
	double a;
	return &ps[findplayer(self, dist? dist : &a)];
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

player_struct* DukeActor_getplayer(DDukeActor* self)
{
	return self->isPlayer() ? &ps[self->PlayerIndex()] : nullptr;
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

double DukeActor_gutsoffset(DDukeActor* self)
{
	return gs.actorinfo[self->spr.picnum].gutsoffset;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, gutsoffset, DukeActor_gutsoffset)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	ACTION_RETURN_FLOAT(DukeActor_gutsoffset(self));
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

DDukeActor* DukeActor_Spawnsprite(DDukeActor* origin, int picnum)
{
	if (picnum >= 0 && picnum < MAXTILES)
	{
		return spawn(origin, picnum);
	}
	return nullptr;
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spawnsprite, DukeActor_Spawnsprite)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(type);
	ACTION_RETURN_POINTER(DukeActor_Spawnsprite(self, type));
}

DDukeActor* DukeActor_spawnweaponorammo(DDukeActor* origin, unsigned intname)
{
	if (intname > 14) return nullptr;
	return spawn(origin, gs.weaponsandammosprites[intname]);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, spawnweaponorammo, DukeActor_spawnweaponorammo)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(type);
	ACTION_RETURN_POINTER(DukeActor_spawnweaponorammo(self, type));
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

void DukeActor_hitradius(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	fi.hitradius(actor, r, hp1, hp2, hp3, hp4);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, hitradius, DukeActor_hitradius)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(r);
	PARAM_INT(h1);
	PARAM_INT(h2);
	PARAM_INT(h3);
	PARAM_INT(h4);
	DukeActor_hitradius(self, r, h1, h2, h3, h4);
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

void DukeActor_checkhitdefault(DDukeActor* origin, DDukeActor* proj)
{
	fi.checkhitdefault(origin, proj);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, checkhitdefault, DukeActor_checkhitdefault)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_OBJECT(proj, DDukeActor);
	DukeActor_checkhitdefault(self, proj);
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

int duke_scripted(DDukeActor* act)
{
	return gs.actorinfo[act->spr.picnum].scriptaddress > 0;
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

void DukeActor_checkhitsprite(DDukeActor* act, DDukeActor* hitter)
{
	fi.checkhitsprite(act, hitter);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, checkhitsprite, DukeActor_checkhitsprite)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(h, DDukeActor);
	DukeActor_checkhitsprite(self, h);
	return 0;
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

void DukeActor_shoot(DDukeActor* act, PClassActor* intname)
{
	fi.shoot(act, -1, intname);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, shoot, DukeActor_shoot)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_POINTER(type, PClassActor);
	DukeActor_shoot(self, type);
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

void DukeActor_operateforcefields(DDukeActor* self, int tag)
{
	operateforcefields(self, tag);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, operateforcefields, DukeActor_operateforcefields)
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


// temporary helpers to hide the fact that these flags are not part of the actor yet.
DEFINE_ACTION_FUNCTION(DDukeActor, actorflag1)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(mask);
	ACTION_RETURN_BOOL(!!actorflag(self, EDukeFlags1::FromInt(mask)));
}

DEFINE_ACTION_FUNCTION(DDukeActor, actorflag2)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(mask);
	ACTION_RETURN_BOOL(!!actorflag(self, EDukeFlags2::FromInt(mask)));
}

DEFINE_ACTION_FUNCTION(DDukeActor, actorflag3)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(mask);
	ACTION_RETURN_BOOL(!!actorflag(self, EDukeFlags3::FromInt(mask)));
}

DEFINE_ACTION_FUNCTION(DDukeActor, attackerflag1)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(mask);
	ACTION_RETURN_BOOL(!!attackerflag(self, EDukeFlags1::FromInt(mask)));
}

DEFINE_ACTION_FUNCTION(DDukeActor, attackerflag2)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_INT(mask);
	ACTION_RETURN_BOOL(!!attackerflag(self, EDukeFlags2::FromInt(mask)));
}


//---------------------------------------------------------------------------
//
// DukePlayer
//
//---------------------------------------------------------------------------

DEFINE_FIELD_X(DukePlayer, player_struct, gotweapon)
DEFINE_FIELD_X(DukePlayer, player_struct, pals)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_sway)
DEFINE_FIELD_X(DukePlayer, player_struct, oweapon_sway)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_pos)
DEFINE_FIELD_X(DukePlayer, player_struct, kickback_pic)
DEFINE_FIELD_X(DukePlayer, player_struct, random_club_frame)
DEFINE_FIELD_X(DukePlayer, player_struct, oweapon_pos)
DEFINE_FIELD_X(DukePlayer, player_struct, okickback_pic)
DEFINE_FIELD_X(DukePlayer, player_struct, orandom_club_frame)
DEFINE_FIELD_X(DukePlayer, player_struct, hard_landing)
DEFINE_FIELD_X(DukePlayer, player_struct, ohard_landing)
DEFINE_FIELD_X(DukePlayer, player_struct, psectlotag)
//DEFINE_FIELD_X(DukePlayer, player_struct, exitx)
//DEFINE_FIELD_X(DukePlayer, player_struct, exity)
DEFINE_FIELD_UNSIZED(DukePlayer, player_struct, loogie)
DEFINE_FIELD_X(DukePlayer, player_struct, numloogs)
DEFINE_FIELD_X(DukePlayer, player_struct, loogcnt)
DEFINE_FIELD_X(DukePlayer, player_struct, invdisptime)
//DEFINE_FIELD_X(DukePlayer, player_struct, bobposx)
//DEFINE_FIELD_X(DukePlayer, player_struct, bobposy)
DEFINE_FIELD_X(DukePlayer, player_struct, pyoff)
DEFINE_FIELD_X(DukePlayer, player_struct, opyoff)
//DEFINE_FIELD_X(DukePlayer, player_struct, posxv)
//DEFINE_FIELD_X(DukePlayer, player_struct, posyv)
//DEFINE_FIELD_X(DukePlayer, player_struct, poszv)
DEFINE_FIELD_X(DukePlayer, player_struct, last_pissed_time)
DEFINE_FIELD_X(DukePlayer, player_struct, truefz)
DEFINE_FIELD_X(DukePlayer, player_struct, truecz)
DEFINE_FIELD_X(DukePlayer, player_struct, player_par)
DEFINE_FIELD_X(DukePlayer, player_struct, visibility)
DEFINE_FIELD_X(DukePlayer, player_struct, bobcounter)
DEFINE_FIELD_X(DukePlayer, player_struct, randomflamex)
DEFINE_FIELD_X(DukePlayer, player_struct, crack_time)
DEFINE_FIELD_X(DukePlayer, player_struct, aim_mode)
DEFINE_FIELD_X(DukePlayer, player_struct, ftt)
DEFINE_FIELD_X(DukePlayer, player_struct, cursector)
DEFINE_FIELD_X(DukePlayer, player_struct, last_extra)
DEFINE_FIELD_X(DukePlayer, player_struct, subweapon)
DEFINE_FIELD_X(DukePlayer, player_struct, ammo_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, frag)
DEFINE_FIELD_X(DukePlayer, player_struct, fraggedself)
DEFINE_FIELD_X(DukePlayer, player_struct, curr_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, last_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, tipincs)
DEFINE_FIELD_X(DukePlayer, player_struct, wantweaponfire)
DEFINE_FIELD_X(DukePlayer, player_struct, holoduke_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, hurt_delay)
DEFINE_FIELD_X(DukePlayer, player_struct, hbomb_hold_delay)
DEFINE_FIELD_X(DukePlayer, player_struct, jumping_counter)
DEFINE_FIELD_X(DukePlayer, player_struct, airleft)
DEFINE_FIELD_X(DukePlayer, player_struct, knee_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, access_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, ftq)
DEFINE_FIELD_X(DukePlayer, player_struct, access_wall)
DEFINE_FIELD_X(DukePlayer, player_struct, got_access)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_ang)
DEFINE_FIELD_X(DukePlayer, player_struct, firstaid_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, actor)
DEFINE_FIELD_X(DukePlayer, player_struct, one_parallax_sectnum)
DEFINE_FIELD_X(DukePlayer, player_struct, over_shoulder_on)
DEFINE_FIELD_X(DukePlayer, player_struct, fist_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, cheat_phase)
DEFINE_FIELD_X(DukePlayer, player_struct, extra_extra8)
DEFINE_FIELD_X(DukePlayer, player_struct, quick_kick)
DEFINE_FIELD_X(DukePlayer, player_struct, last_quick_kick)
DEFINE_FIELD_X(DukePlayer, player_struct, heat_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, timebeforeexit)
DEFINE_FIELD_X(DukePlayer, player_struct, customexitsound)
DEFINE_FIELD_X(DukePlayer, player_struct, interface_toggle_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, dead_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, show_empty_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, scuba_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, jetpack_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, steroids_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, shield_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, pycount)
DEFINE_FIELD_X(DukePlayer, player_struct, frag_ps)
DEFINE_FIELD_X(DukePlayer, player_struct, transporter_hold)
DEFINE_FIELD_X(DukePlayer, player_struct, last_full_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintshade)
DEFINE_FIELD_X(DukePlayer, player_struct, boot_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, on_warping_sector)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintcount)
DEFINE_FIELD_X(DukePlayer, player_struct, hbomb_on)
DEFINE_FIELD_X(DukePlayer, player_struct, jumping_toggle)
DEFINE_FIELD_X(DukePlayer, player_struct, rapid_fire_hold)
DEFINE_FIELD_X(DukePlayer, player_struct, on_ground)
DEFINE_FIELD_X(DukePlayer, player_struct, inven_icon)
DEFINE_FIELD_X(DukePlayer, player_struct, buttonpalette)
DEFINE_FIELD_X(DukePlayer, player_struct, jetpack_on)
DEFINE_FIELD_X(DukePlayer, player_struct, spritebridge)
DEFINE_FIELD_X(DukePlayer, player_struct, lastrandomspot)
DEFINE_FIELD_X(DukePlayer, player_struct, scuba_on)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintpal)
DEFINE_FIELD_X(DukePlayer, player_struct, heat_on)
DEFINE_FIELD_X(DukePlayer, player_struct, holster_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, falling_counter)
DEFINE_FIELD_X(DukePlayer, player_struct, refresh_inventory)
DEFINE_FIELD_X(DukePlayer, player_struct, toggle_key_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, knuckle_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, walking_snd_toggle)
DEFINE_FIELD_X(DukePlayer, player_struct, palookup)
DEFINE_FIELD_X(DukePlayer, player_struct, quick_kick_msg)
DEFINE_FIELD_X(DukePlayer, player_struct, max_secret_rooms)
DEFINE_FIELD_X(DukePlayer, player_struct, secret_rooms)
DEFINE_FIELD_X(DukePlayer, player_struct, stairs)
DEFINE_FIELD_X(DukePlayer, player_struct, detonate_count)
//DEFINE_FIELD_X(DukePlayer, player_struct, noise.X)
//DEFINE_FIELD_X(DukePlayer, player_struct, noise.Y)
DEFINE_FIELD_X(DukePlayer, player_struct, noise_radius)
DEFINE_FIELD_X(DukePlayer, player_struct, drink_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, eat_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, SlotWin)
DEFINE_FIELD_X(DukePlayer, player_struct, recoil)
DEFINE_FIELD_X(DukePlayer, player_struct, detonate_time)
DEFINE_FIELD_X(DukePlayer, player_struct, yehaa_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, drink_amt)
DEFINE_FIELD_X(DukePlayer, player_struct, eat)
DEFINE_FIELD_X(DukePlayer, player_struct, drunkang)
DEFINE_FIELD_X(DukePlayer, player_struct, eatang)
DEFINE_FIELD_X(DukePlayer, player_struct, shotgun_state)
DEFINE_FIELD_X(DukePlayer, player_struct, donoise)
DEFINE_FIELD_X(DukePlayer, player_struct, keys)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_aspect)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, SeaSick)
DEFINE_FIELD_X(DukePlayer, player_struct, MamaEnd)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_drink)
DEFINE_FIELD_X(DukePlayer, player_struct, TiltStatus)
DEFINE_FIELD_X(DukePlayer, player_struct, oTiltStatus)
DEFINE_FIELD_X(DukePlayer, player_struct, VBumpNow)
DEFINE_FIELD_X(DukePlayer, player_struct, VBumpTarget)
DEFINE_FIELD_X(DukePlayer, player_struct, TurbCount)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_stat)
DEFINE_FIELD_X(DukePlayer, player_struct, DrugMode)
DEFINE_FIELD_X(DukePlayer, player_struct, lotag800kill)
DEFINE_FIELD_X(DukePlayer, player_struct, sea_sick_stat)
DEFINE_FIELD_X(DukePlayer, player_struct, hurt_delay2)
DEFINE_FIELD_X(DukePlayer, player_struct, nocheat)
DEFINE_FIELD_X(DukePlayer, player_struct, OnMotorcycle)
DEFINE_FIELD_X(DukePlayer, player_struct, OnBoat)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_underwater)
DEFINE_FIELD_X(DukePlayer, player_struct, NotOnWater)
DEFINE_FIELD_X(DukePlayer, player_struct, MotoOnGround)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_do_bump)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_bump_fast)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_on_oil)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_on_mud)
DEFINE_FIELD_X(DukePlayer, player_struct, MotoSpeed)
DEFINE_FIELD_X(DukePlayer, player_struct, holoduke_on)
DEFINE_FIELD_X(DukePlayer, player_struct, actorsqu)
DEFINE_FIELD_X(DukePlayer, player_struct, wackedbyactor)
DEFINE_FIELD_X(DukePlayer, player_struct, on_crane)
DEFINE_FIELD_X(DukePlayer, player_struct, somethingonplayer)
DEFINE_FIELD_X(DukePlayer, player_struct, access_spritenum)
DEFINE_FIELD_X(DukePlayer, player_struct, dummyplayersprite)
DEFINE_FIELD_X(DukePlayer, player_struct, newOwner)
DEFINE_FIELD_X(DukePlayer, player_struct, fric)

DEFINE_ACTION_FUNCTION(_DukePlayer, IsFrozen)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	ACTION_RETURN_BOOL(self->GetActor()->spr.pal == 1 && self->last_extra < 2);
}

DEFINE_ACTION_FUNCTION(_DukePlayer, GetGameVar)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_STRING(name);
	PARAM_INT(def);
	ACTION_RETURN_INT(GetGameVar(name, def, self->GetActor(), self->GetPlayerNum()).safeValue());
}

void dukeplayer_backuppos(player_struct* self)
{
	self->backuppos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, backuppos, dukeplayer_backuppos)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	dukeplayer_backuppos(self);
	return 0;
}

void dukeplayer_backupxyz(player_struct* self)
{
	self->GetActor()->backuppos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, backupxyz, dukeplayer_backupxyz)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	dukeplayer_backupxyz(self);
	return 0;
}

void dukeplayer_setpos(player_struct* self, double x, double y, double z)
{
	self->GetActor()->spr.pos = { x, y, z + self->GetActor()->viewzoffset };
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, setpos, dukeplayer_setpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	dukeplayer_setpos(self, x, y, z);
	return 0;
}

void dukeplayer_addpos(player_struct* self, double x, double y, double z)
{
	self->GetActor()->spr.pos += { x, y, z };
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addpos, dukeplayer_addpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	dukeplayer_addpos(self, x, y, z);
	return 0;
}

void dukeplayer_centerview(player_struct* self)
{
	self->sync.actions |= SB_CENTERVIEW;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, centerview, dukeplayer_centerview)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	self->sync.actions |= SB_CENTERVIEW;
	return 0;
}

inline int DukePlayer_PlayerInput(player_struct* pl, int bit)
{
	return (!!((pl->sync.actions) & ESyncBits::FromInt(bit)));
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, playerinput, DukePlayer_PlayerInput)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_INT(bit);
	ACTION_RETURN_INT(DukePlayer_PlayerInput(self, bit));
}

void dukeplayer_settargetangle(player_struct* self, double a, int backup)
{
	self->GetActor()->spr.Angles.Yaw = DAngle::fromDeg(a);
	if (backup) self->GetActor()->PrevAngles.Yaw = self->GetActor()->spr.Angles.Yaw;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, settargetangle, dukeplayer_settargetangle)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_FLOAT(a);
	PARAM_BOOL(bak);
	dukeplayer_settargetangle(self, a, bak);
	return 0;
}

double dukeplayer_angle(player_struct* self)
{
	return self->GetActor()->spr.Angles.Yaw.Degrees();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, angle, dukeplayer_angle)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	ACTION_RETURN_FLOAT(dukeplayer_angle(self));
}

void dukeplayer_addpitch(player_struct* self, double a)
{
	self->GetActor()->spr.Angles.Pitch += DAngle::fromDeg(a);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addpitch, dukeplayer_addpitch)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_FLOAT(a);
	dukeplayer_addpitch(self, a);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, clearcameras, clearcameras)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	clearcameras(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, quickkill, quickkill)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	quickkill(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, CheckWeapRec, CheckWeapRec)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_POINTER(ac, DDukeActor);
	PARAM_BOOL(test);
	ACTION_RETURN_INT(CheckWeapRec(self, ac, test));
}

void DukePlayer_addammo(player_struct* p, int ammo, int amount)
{
	if ((unsigned)ammo >= MAX_WEAPONS) ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Ammo number out of range");
	addammo(ammo, p, amount);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addammo, DukePlayer_addammo)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_INT(type);
	PARAM_INT(amount);
	DukePlayer_addammo(self, type, amount);
	return 0;
}

void DukePlayer_addweapon(player_struct* p, int wpn, int switchit)
{
	if ((unsigned)wpn >= MAX_WEAPONS) ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Weapon number out of range");
	fi.addweapon(p, wpn, switchit);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, addweapon, DukePlayer_addweapon)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_INT(type);
	PARAM_INT(switchit);
	DukePlayer_addweapon(self, type, switchit);
	return 0;

}

DEFINE_ACTION_FUNCTION(_DukePlayer, hitablockingwall)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	walltype* pwal;
	hitawall(self, &pwal);
	ACTION_RETURN_BOOL(pwal && pwal->overtexture.isValid());
}

inline double DukePlayer_GetPitchwithView(player_struct* pl)
{
	return pl->Angles.getPitchWithView().Degrees();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, GetPitchwithView, DukePlayer_GetPitchwithView)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	ACTION_RETURN_FLOAT(DukePlayer_GetPitchwithView(self));
}

inline void DukePlayer_setbobpos(player_struct* pl)
{
	return pl->setbobpos();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, setbobpos, DukePlayer_setbobpos)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	self->setbobpos();
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, StartMotorcycle, OnMotorcycle)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	OnMotorcycle(self);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukePlayer, StartBoat, OnBoat)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	OnBoat(self);
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
	PARAM_POINTER(p, player_struct);
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

int duke_floorflags(sectortype* sector)
{
	return tileflags(sector->floortexture);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, floorflags, duke_floorflags)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_INT(duke_floorflags(sect));
}

int duke_ceilingflags(sectortype* sector)
{
	return tileflags(sector->ceilingtexture);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, ceilingflags, duke_ceilingflags)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, sectortype);
	ACTION_RETURN_INT(duke_ceilingflags(sect));
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

int duke_wallflags(walltype* wal, int which)
{
	return tileflags(which? wal->overtexture : wal->walltexture);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, wallflags, duke_wallflags)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(sect, walltype);
	PARAM_INT(which);
	ACTION_RETURN_INT(duke_wallflags(sect, which));
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
DEFINE_GLOBAL_UNSIZED(gs)

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


// this is only a temporary helper until weaponsandammosprites can be migrated to real class types. We absolutely do not want any access to tile numbers in the scripts - even now.
void tspritetype_setWeaponOrAmmoSprite(tspritetype* targ, unsigned z)
{
	if (z < 15)
	{
		targ->picnum = gs.weaponsandammosprites[z];
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

// this must still work around the lack of proper texture support on the script side.
DEFINE_ACTION_FUNCTION(DDukeGenericDestructible, SetBroken)
{
	PARAM_SELF_STRUCT_PROLOGUE(DDukeActor);
	PARAM_INT(bust);
	int tilenum = self->IntVar(bust ? NAME_brokenstate : NAME_spawnstate);
	if (tilenum >= 0) self->spr.picnum = tilenum;
	ACTION_RETURN_BOOL(tilenum < 0);
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
