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
	actor->s().statnum = statnum;
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
	actor->s().statnum = statnum;
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
	assert(*prevp == actor);
	assert(*nextp == actor);

	assert(ValidateStatList(actor->link_stat));
	*prevp = next;
	*nextp = prev;
	assert(ValidateStatList(actor->link_stat));

	actor->nextStat = actor->prevStat = nullptr;
	actor->s().statnum = MAXSTATUS;
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
	assert(actor->s().statnum >= 0 && actor->s().statnum < MAXSTATUS);
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
	actor->s().setsector(sect);
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
	actor->s().setsector(sect);
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
	assert(*prevp == actor);
	assert(*nextp == actor);

	assert(ValidateSectList(actor->link_sector));
	*prevp = next;
	*nextp = prev;
	assert(ValidateSectList(actor->link_sector, actor));

	actor->nextSect = actor->prevSect = nullptr;
	actor->s().setsector(nullptr);
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
		actor->s().setsector(nullptr);
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

	spritetype* pSprite = &actor->s();
	pSprite->clear();

	InsertActorStat(actor, stat, tail);
	InsertActorSect(actor, sector, tail);

	Numsprites++;
	actor->s().time = leveltimer++;
	return actor;
}

//==========================================================================
//
//
//
//==========================================================================

void DCoreActor::OnDestroy()
{
	assert(spr.statnum >= 0 && spr.statnum < MAXSTATUS);

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
	for (auto& act : allActors)
	{
		if (!(act->ObjectFlags & OF_EuthanizeMe))
			act->Destroy();
	}
	for (auto& stat : statList)
	{
		stat.firstEntry = stat.lastEntry = nullptr;
	}
	for (auto& sect : sectors())
	{
		sect.firstEntry = sect.lastEntry = nullptr;
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
	actor->s().setpos(*newpos);
	updatesector(newpos->x, newpos->y, &tempsector);

	if (tempsector && tempsector != actor->sector())
		ChangeActorSect(actor, tempsector);
}

void SetActorZ(DCoreActor* actor, const vec3_t* newpos)
{
	auto tempsector = actor->sector();
	actor->s().setpos(*newpos);
	updatesectorz(newpos->x, newpos->y, newpos->z, &tempsector);

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

