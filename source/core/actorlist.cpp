/*
** actorlist.cpp
** Implements the linked stat/sector actor lists
**
**---------------------------------------------------------------------------
** Copyright 2021 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "build.h"
#include "coreactor.h"
#include "gamefuncs.h"
#include "raze_sound.h"
#include "vm.h"

// Doubly linked ring list of Actors


ActorStatList statList[MAXSTATUS];

//==========================================================================
//
//
//
//==========================================================================

CVAR(Bool, safe_spritelist, false, CVAR_ARCHIVE)

static bool isSafe()
{
	return isBlood() || safe_spritelist;
}

//==========================================================================
//
//
//
//==========================================================================
TArray<DCoreActor*> checked;

static bool ValidateStatList(int statnum)
{
#if 0
	checked.Clear();
	for (auto entry = statList[statnum].firstEntry; entry; entry = entry->nextStat)
	{
		assert(!checked.Contains(entry));
		checked.Push(entry);
		assert(entry->prevStat != nullptr || statList[statnum].firstEntry == entry);
		assert(entry->nextStat != nullptr || statList[statnum].lastEntry == entry);
		assert(entry->prevStat == nullptr || entry->prevStat->nextStat == entry);
		assert(entry->nextStat == nullptr || entry->nextStat->prevStat == entry);
	}
#endif
	return true;
}

//==========================================================================
//
//
//
//==========================================================================

static void AddStatTail(DCoreActor* actor, int statnum)
{
	assert(actor->prevStat == nullptr && actor->nextStat == nullptr);

	auto tail = statList[statnum].lastEntry;
	assert(tail == nullptr || tail->nextStat == nullptr);
	assert(ValidateStatList(statnum));
	actor->prevStat = tail;
	if (tail) tail->nextStat = actor;
	else statList[statnum].firstEntry = actor;
	statList[statnum].lastEntry = actor;
	assert(ValidateStatList(statnum));
	actor->spr.statnum = statnum;
	actor->link_stat = statnum;
	GC::WriteBarrier(actor);
	GC::WriteBarrier(tail);
}

//==========================================================================
//
//
//
//==========================================================================

static void AddStatHead(DCoreActor* actor, int statnum)
{
	assert(actor->prevStat == nullptr && actor->nextStat == nullptr);

	auto head = statList[statnum].firstEntry;
	assert(head == nullptr || head->prevStat == nullptr);
	assert(ValidateStatList(statnum));
	actor->nextStat = head;
	if (head) head->prevStat = actor;
	else statList[statnum].lastEntry = actor;
	assert(ValidateStatList(statnum));
	statList[statnum].firstEntry = actor;
	actor->spr.statnum = statnum;
	actor->link_stat = statnum;
	GC::WriteBarrier(actor);
	GC::WriteBarrier(head);
}


//==========================================================================
//
//
//
//==========================================================================

static void RemoveActorStat(DCoreActor* actor)
{
	DCoreActor* prev = actor->prevStat;
	DCoreActor* next = actor->nextStat;
	auto& firstEntry = statList[actor->link_stat].firstEntry;
	auto& lastEntry = statList[actor->link_stat].lastEntry;

	auto prevp = prev ? &prev->nextStat : &firstEntry;
	auto nextp = next ? &next->prevStat : &lastEntry;
	if (*prevp == nullptr && *nextp == nullptr) return;	// can happen during an aborted savegame load.

	assert(*prevp == actor);
	assert(*nextp == actor);

	assert(ValidateStatList(actor->link_stat));
	*prevp = next;
	*nextp = prev;
	assert(ValidateStatList(actor->link_stat));

	actor->nextStat = actor->prevStat = nullptr;
	actor->spr.statnum = MAXSTATUS;
	actor->link_stat = MAXSTATUS;
	GC::WriteBarrier(prev);
	GC::WriteBarrier(next);
}

//==========================================================================
//
//
//
//==========================================================================

static void InsertActorStat(DCoreActor* actor, int stat, bool tail)
{
	assert(actor->prevStat == nullptr && actor->nextStat == nullptr);
	assert(stat >= 0 && stat <= MAXSTATUS);
	if (isSafe() || tail) AddStatTail(actor, stat);
	else AddStatHead(actor, stat);
}

//==========================================================================
//
//
//
//==========================================================================

int ChangeActorStat(DCoreActor* actor, int statnum, bool tail)
{
	int oldstat = actor->link_stat;
	assert(statnum >= 0 && statnum < MAXSTATUS);
	assert(actor->spr.statnum >= 0 && actor->spr.statnum < MAXSTATUS);
	RemoveActorStat(actor);
	InsertActorStat(actor, statnum, tail);
	return 0;
}

//==========================================================================
//
//
//
//==========================================================================

static bool ValidateSectList(sectortype* sect, DCoreActor *checkme = nullptr)
{
#if 0
	assert(sect);
	checked.Clear();
	assert(sect->firstEntry == nullptr || sect->firstEntry->prevSect == nullptr);
	assert(sect->lastEntry == nullptr || sect->lastEntry->nextSect == nullptr);
	for (auto entry = sect->firstEntry; entry; entry = entry->nextSect)
	{
		assert(entry != checkme);
		assert(!checked.Contains(entry));
		checked.Push(entry);
		assert(entry->prevSect != nullptr || sect->firstEntry == entry);
		assert(entry->nextSect != nullptr || sect->lastEntry == entry);
		assert(entry->prevSect == nullptr || entry->prevSect->nextSect == entry);
		assert(entry->nextSect == nullptr || entry->nextSect->prevSect == entry);
	}
#endif
	return true;
}


//==========================================================================
//
//
//
//==========================================================================

static void AddSectTail(DCoreActor *actor, sectortype* sect)
{
	assert(actor->prevSect == nullptr && actor->nextSect == nullptr);

	auto tail = sect->lastEntry;
	assert(tail == nullptr || tail->nextSect == nullptr);
	assert(ValidateSectList(sect));
	actor->prevSect = tail;
	if (tail) tail->nextSect = actor;
	else sect->firstEntry = actor;
	sect->lastEntry = actor;
	assert(ValidateSectList(sect));
	actor->setsector(sect);
	actor->link_sector = sect;
	GC::WriteBarrier(actor);
	GC::WriteBarrier(tail);
}

//==========================================================================
//
//
//
//==========================================================================

static void AddSectHead(DCoreActor *actor, sectortype* sect)
{
	assert(actor->prevSect == nullptr && actor->nextSect == nullptr);

	auto head = sect->firstEntry;
	assert(head == nullptr || head->prevSect == nullptr);
	assert(ValidateSectList(sect));
	actor->nextSect = head;
	if (head) head->prevSect = actor;
	else sect->lastEntry = actor;
	sect->firstEntry = actor;
	assert(ValidateSectList(sect));
	actor->setsector(sect);
	actor->link_sector = sect;
	GC::WriteBarrier(actor);
	GC::WriteBarrier(head);
}

//==========================================================================
//
//
//
//==========================================================================

static void RemoveActorSect(DCoreActor* actor)
{
	if (actor->link_sector == nullptr)
	{
		assert(actor->prevSect == nullptr && actor->nextSect == nullptr);
		return;
	}
	DCoreActor *prev = actor->prevSect;
	DCoreActor *next = actor->nextSect;

	auto& firstEntry = actor->link_sector->firstEntry;
	auto& lastEntry = actor->link_sector->lastEntry;

	auto prevp = prev ? &prev->nextSect : &firstEntry;
	auto nextp = next ? &next->prevSect : &lastEntry;
	if (*prevp == nullptr && *nextp == nullptr) return;	// can happen during an aborted savegame load.
	assert(*prevp == actor);
	assert(*nextp == actor);

	assert(ValidateSectList(actor->link_sector));
	*prevp = next;
	*nextp = prev;
	assert(ValidateSectList(actor->link_sector, actor));

	actor->nextSect = actor->prevSect = nullptr;
	actor->setsector(nullptr);
	actor->link_sector = nullptr;
	GC::WriteBarrier(prev);
	GC::WriteBarrier(next);
}

//==========================================================================
//
//
//
//==========================================================================

static void InsertActorSect(DCoreActor* actor, sectortype* sector, bool tail)
{
	assert(actor->prevSect == nullptr && actor->nextSect == nullptr);

	if (!sector)
	{
		actor->link_sector = nullptr;
		actor->setsector(nullptr);
		return;
	}
	if (isSafe() || tail) AddSectTail(actor, sector);
	else AddSectHead(actor, sector);
}

//==========================================================================
//
//
//
//==========================================================================

void ChangeActorSect(DCoreActor* actor, sectortype* sect, bool tail)
{
	if (sect == nullptr) return;
	RemoveActorSect(actor);
	InsertActorSect(actor, sect, tail);
}

//==========================================================================
//
//
//
//==========================================================================

DCoreActor* InsertActor(PClass* type, sectortype* sector, int stat, bool tail)
{
	assert(type->IsDescendantOf(RUNTIME_CLASS(DCoreActor)));

	auto actor = static_cast<DCoreActor*>(type->CreateNew());
	GC::WriteBarrier(actor);

	InsertActorStat(actor, stat, tail);
	InsertActorSect(actor, sector, tail);

	Numsprites++;
	actor->time = leveltimer++;
	return actor;
}

//==========================================================================
//
//
//
//==========================================================================

void DCoreActor::OnDestroy()
{
	FVector3 pos = GetSoundPos(int_pos());
	soundEngine->RelinkSound(SOURCE_Actor, this, nullptr, &pos);

	// also scan all other sounds if they have this actor as source. If so, null the source and stop looped sounds.
	soundEngine->EnumerateChannels([=](FSoundChan* chan)
		{
			if (chan->Source == this)
			{
				if (chan->ChanFlags & CHANF_LOOP) soundEngine->StopChannel(chan);
				else chan->Source = nullptr;
			}
			return 0;
		});


	if(link_stat == INT_MAX) return;

	int stat = link_stat;
	RemoveActorStat(this);

	auto sect = link_sector;
	if (sect)
	{
		RemoveActorSect(this);
	}
	else
	{
		assert(prevSect == nullptr && nextSect == nullptr);
	}
	Numsprites--;
	
	if (wallspriteinfo) delete wallspriteinfo;
	wallspriteinfo = nullptr;

}

//==========================================================================
//
// code below will go away or be changed once 
// we can use real DObject life cycle management.
//
//==========================================================================

void InitSpriteLists()
{
	// Do not mass-destroy from the iterator. This may fail if destroying one actor results in further destructions.
	TArray<DCoreActor*> allActors;
	TSpriteIterator<DCoreActor> it;
	while (auto actor = it.Next())
		allActors.Push(actor);

	// clear all lists manually before doing any mass destruction. 
	// This may also be called in error situations where the list has become corrupted, 
	// so we should not depend on its consistency anymore.
	for (auto& stat : statList)
	{
		stat.firstEntry = stat.lastEntry = nullptr;
	}
	for (auto& sect: sector)
	{
		sect.firstEntry = sect.lastEntry = nullptr;
	}

	for (auto& act : allActors)
	{
		if (!(act->ObjectFlags & OF_EuthanizeMe))
		{
			act->link_stat = INT_MAX;
			act->prevStat = act->nextStat = act->prevSect = act->nextSect = nullptr;
			act->Destroy();
		}
	}
	Numsprites = 0;
}

//==========================================================================
//
//
//
//==========================================================================

void SetActor(DCoreActor* actor, const vec3_t* newpos)
{
	auto tempsector = actor->sector();
	actor->set_int_pos(*newpos);
	updatesector(newpos->X, newpos->Y, &tempsector);

	if (tempsector && tempsector != actor->sector())
		ChangeActorSect(actor, tempsector);
}

void SetActorZ(DCoreActor* actor, const vec3_t* newpos)
{
	auto tempsector = actor->sector();
	actor->set_int_pos(*newpos);
	updatesectorz(newpos->X, newpos->Y, newpos->Z, &tempsector);

	if (tempsector && tempsector != actor->sector())
		ChangeActorSect(actor, tempsector);
}


IMPLEMENT_CLASS(DCoreActor, false, false)

size_t DCoreActor::PropagateMark()
{
	GC::Mark(prevStat);
	GC::Mark(nextStat);
	GC::Mark(prevSect);
	GC::Mark(nextSect);
	return Super::PropagateMark();
}


int DCoreActor::GetOffsetAndHeight(int& height)
{
	int yrepeat = spr.yrepeat << 2;
	height = tileHeight(spr.picnum) * yrepeat;
	int zofs = (spr.cstat & CSTAT_SPRITE_YCENTER)? height >> 1 : 0;
	return zofs - tileTopOffset(spr.picnum) * yrepeat;
}



DEFINE_FIELD_NAMED(DCoreActor, spr.sectp, sector)
DEFINE_FIELD_NAMED(DCoreActor, spr.cstat, cstat)
DEFINE_FIELD_NAMED(DCoreActor, spr.cstat2, cstat2)
DEFINE_FIELD_NAMED(DCoreActor, spr.picnum, picnum)
DEFINE_FIELD_NAMED(DCoreActor, spr.statnum, statnum)
DEFINE_FIELD_NAMED(DCoreActor, spr.ang, ang)
DEFINE_FIELD_NAMED(DCoreActor, spr.xvel, xvel)
DEFINE_FIELD_NAMED(DCoreActor, spr.yvel, yvel)
DEFINE_FIELD_NAMED(DCoreActor, spr.zvel, zvel)
DEFINE_FIELD_NAMED(DCoreActor, spr.inittype, inittype)
DEFINE_FIELD_NAMED(DCoreActor, spr.hitag, hitag)
DEFINE_FIELD_NAMED(DCoreActor, spr.lotag, lotag)
DEFINE_FIELD_NAMED(DCoreActor, spr.type, type)
DEFINE_FIELD_NAMED(DCoreActor, spr.flags, flags) // need to be careful with this!
DEFINE_FIELD_NAMED(DCoreActor, spr.extra, extra)
DEFINE_FIELD_NAMED(DCoreActor, spr.detail, detail)
DEFINE_FIELD_NAMED(DCoreActor, spr.shade, shade)
DEFINE_FIELD_NAMED(DCoreActor, spr.pal, pal)
DEFINE_FIELD_NAMED(DCoreActor, spr.clipdist, clipdist)
DEFINE_FIELD_NAMED(DCoreActor, spr.blend, blend)
DEFINE_FIELD_NAMED(DCoreActor, spr.xrepeat, xrepeat)
DEFINE_FIELD_NAMED(DCoreActor, spr.yrepeat, yrepeat)
DEFINE_FIELD_NAMED(DCoreActor, spr.xoffset, xoffset)
DEFINE_FIELD_NAMED(DCoreActor, spr.yoffset, yoffset)
DEFINE_FIELD_NAMED(DCoreActor, spr.intowner, owner)
DEFINE_FIELD_NAMED(DCoreActor, sprext.mdanimtims, mdanimtims)
DEFINE_FIELD_NAMED(DCoreActor, sprext.mdanimcur, mdanimcur)
DEFINE_FIELD_NAMED(DCoreActor, sprext.angoff, angoff)
DEFINE_FIELD_NAMED(DCoreActor, sprext.pitch, pitch)
DEFINE_FIELD_NAMED(DCoreActor, sprext.roll, roll)
DEFINE_FIELD_NAMED(DCoreActor, sprext.renderflags, renderflags)
DEFINE_FIELD_NAMED(DCoreActor, sprext.alpha, alpha)
DEFINE_FIELD_NAMED(DCoreActor, time, spawnindex)
DEFINE_FIELD_NAMED(DCoreActor, spritesetindex, spritesetpic)

DEFINE_ACTION_FUNCTION(DCoreActor, pos)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	ACTION_RETURN_VEC3(self->float_pos());
}

DEFINE_ACTION_FUNCTION(DCoreActor, xy)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	ACTION_RETURN_VEC2(self->float_pos().XY());
}

double coreactor_z(DCoreActor* self)
{
	return self->float_pos().Z;
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, z, coreactor_z)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	ACTION_RETURN_FLOAT(coreactor_z(self));
}

void coreactor_setpos(DCoreActor* self, double x, double y, double z, int relink)
{
	self->spr.pos = { x, y, z };
	// todo: SW needs to call updatesectorz here or have a separate function.
	if (relink) SetActor(self, self->int_pos());
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, setpos, coreactor_setpos)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_BOOL(relink);
	coreactor_setpos(self, x, y, z, relink);
	return 0;
}

void coreactor_copypos(DCoreActor* self, DCoreActor* other, int relink)
{
	if (!other) return;
	self->spr.pos = other->spr.pos;
	// todo: SW needs to call updatesectorz here or have a separate function.
	if (relink) SetActor(self, self->int_pos());
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, copypos, coreactor_setpos)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_POINTER(other, DCoreActor);
	PARAM_BOOL(relink);
	coreactor_copypos(self, other, relink);
	return 0;
}

void coreactor_move(DCoreActor* self, double x, double y, double z, int relink)
{
	self->spr.pos += { x, y, z };
	// todo: SW needs to call updatesectorz here or have a separate function.
	if (relink) SetActor(self, self->int_pos());
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, move, coreactor_move)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_BOOL(relink);
	coreactor_move(self, x, y, z, relink);
	return 0;
}

void coreactor_setz(DCoreActor* self, double z)
{
	self->spr.pos.Z = z;
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, setz, coreactor_setz)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_FLOAT(z);
	coreactor_setz(self, z);
	return 0;
}

void coreactor_addz(DCoreActor* self, double z)
{
	self->spr.pos.Z += z;
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, addz, coreactor_addz)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_FLOAT(z);
	coreactor_addz(self, z);
	return 0;
}

void coreactor_setSpritePic(DCoreActor* self, unsigned z)
{
	auto &spriteset = static_cast<PClassActor*>(self->GetClass())->ActorInfo()->SpriteSet;
	if (z < spriteset.Size())
	{
		self->spritesetindex = z;
		self->spr.picnum = spriteset[z];
	}
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, setSpritePic, coreactor_setSpritePic)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	PARAM_INT(z);
	coreactor_setSpritePic(self, z);
	return 0;
}

void coreactor_backuppos(DCoreActor* self)
{
	self->backuppos();
}

DEFINE_ACTION_FUNCTION_NATIVE(DCoreActor, backuppos, coreactor_backuppos)
{
	PARAM_SELF_PROLOGUE(DCoreActor);
	coreactor_backuppos(self);
	return 0;
}

