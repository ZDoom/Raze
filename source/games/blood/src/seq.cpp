/*
 * Copyright (C) 2018, 2022 nukeykt
 * Copyright (C) 2020-2023 Christoph Oelckers
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


#include "ns.h"
#include "filesystem.h"
#include "tarray.h"
#include "files.h"

#include "build.h"

#include "blood.h"
#include "eventq.h"


BEGIN_BLD_NS

FMemArena seqcache; // also used by QAVs.
static TMap<int64_t, Seq*> sequences;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqPrecacheId(FName name, int id, int palette)
{
	if (auto pSeq = getSequence(name, id))
	{
		for (const auto& frame : pSeq->frames)
			tilePrecacheTile(frame.texture, -1, palette);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

template<class T, class U> void SetFlag(T& flagvar, U bit, bool condition)
{
	if (condition) flagvar |= bit;
	else flagvar &= ~bit;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateSprite(DBloodActor* actor, SeqFrame* pFrame)
{
	assert(actor->hasX());
	if (actor->spr.flags & kPhysGravity)
	{
		auto atex = TexMan.GetGameTexture(actor->spr.spritetexture());
		auto stex = TexMan.GetGameTexture(pFrame->texture);
		if (atex->GetDisplayHeight() != stex->GetDisplayHeight() || atex->GetDisplayTopOffset() != stex->GetDisplayTopOffset()
			|| (pFrame->scale.Y && pFrame->scale.Y != actor->spr.scale.Y))
			actor->spr.flags |= kPhysFalling;
	}
	actor->spr.setspritetexture(pFrame->texture);

	if (pFrame->palette) actor->spr.pal = pFrame->palette;
	actor->spr.shade = pFrame->shade;

	double scale = VanillaMode() ? 0 : actor->xspr.scale / 256.; // SEQ size scaling (nnext feature)
	if (pFrame->scale.X)
	{
		double s = pFrame->scale.X;
		if (scale) s = clamp(s * scale, 0., 4.); // Q: Do we need the clamp to 4 here?
		actor->spr.scale.X = s;
	}

	if (pFrame->scale.Y)
	{
		double s = pFrame->scale.Y * REPEAT_SCALE;
		if (scale) s = clamp(s * scale, 0., 4.);
		actor->spr.scale.Y = s;
	}

	SetFlag(actor->spr.cstat, CSTAT_SPRITE_TRANSLUCENT, pFrame->transparent);

	SetFlag(actor->spr.cstat, CSTAT_SPRITE_TRANS_FLIP, pFrame->transparent2);
	SetFlag(actor->spr.cstat, CSTAT_SPRITE_BLOCK, pFrame->blockable);
	SetFlag(actor->spr.cstat, CSTAT_SPRITE_BLOCK_HITSCAN, pFrame->hittable);
	SetFlag(actor->spr.cstat, CSTAT_SPRITE_INVISIBLE, pFrame->invisible);
	SetFlag(actor->spr.cstat, CSTAT_SPRITE_BLOOD_PUSHABLE, pFrame->pushable);

	SetFlag(actor->spr.flags, kHitagSmoke, pFrame->smoke);
	SetFlag(actor->spr.flags, kHitagAutoAim, pFrame->aiming);
	SetFlag(actor->spr.flags, kHitagFlipX, pFrame->flipx);
	SetFlag(actor->spr.flags, kHitagFlipY, pFrame->flipy);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateWall(walltype* pWall, SeqFrame* pFrame)
{
	assert(pWall->hasX());
	pWall->setwalltexture(pFrame->texture);
	if (pFrame->palette) pWall->pal = pFrame->palette;

	SetFlag(pWall->cstat, CSTAT_WALL_TRANSLUCENT, pFrame->transparent);
	SetFlag(pWall->cstat, CSTAT_WALL_TRANS_FLIP, pFrame->transparent2);
	SetFlag(pWall->cstat, CSTAT_WALL_BLOCK, pFrame->blockable);
	SetFlag(pWall->cstat, CSTAT_WALL_BLOCK_HITSCAN, pFrame->hittable);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateFloor(sectortype* pSector, SeqFrame* pFrame)
{
	pSector->setfloortexture(pFrame->texture);
	pSector->floorshade = pFrame->shade;
	if (pFrame->palette) pSector->floorpal = pFrame->palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateCeiling(sectortype* pSector, SeqFrame* pFrame)
{
	pSector->setceilingtexture(pFrame->texture);
	pSector->ceilingshade = pFrame->shade;
	if (pFrame->palette) pSector->ceilingpal = pFrame->palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateMasked(walltype* pWall, SeqFrame* pFrame)
{
	assert(pWall->hasX());
	walltype* pWallNext = pWall->nextWall();
	auto texid = pFrame->texture;
	pWall->setovertexture(texid);
	pWallNext->setovertexture(texid);
	if (pFrame->palette)
		pWall->pal = pWallNext->pal = pFrame->palette;

	SetFlag(pWall->cstat, CSTAT_WALL_TRANSLUCENT, pFrame->transparent);
	SetFlag(pWallNext->cstat, CSTAT_WALL_TRANSLUCENT, pFrame->transparent);
	SetFlag(pWall->cstat, CSTAT_WALL_TRANS_FLIP, pFrame->transparent2);
	SetFlag(pWallNext->cstat, CSTAT_WALL_TRANS_FLIP, pFrame->transparent2);
	SetFlag(pWall->cstat, CSTAT_WALL_BLOCK, pFrame->blockable);
	SetFlag(pWallNext->cstat, CSTAT_WALL_BLOCK, pFrame->blockable);
	SetFlag(pWall->cstat, CSTAT_WALL_BLOCK_HITSCAN, pFrame->hittable);
	SetFlag(pWallNext->cstat, CSTAT_WALL_BLOCK_HITSCAN, pFrame->hittable);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SEQINST::Update()
{
	assert(frameIndex < pSequence->frames.Size());
	switch (type)
	{
	case SS_WALL:
		assert(target.isWall());
		UpdateWall(target.wall(), &pSequence->frames[frameIndex]);
		break;
	case SS_CEILING:
		assert(target.isSector());
		UpdateCeiling(target.sector(), &pSequence->frames[frameIndex]);
		break;
	case SS_FLOOR:
		assert(target.isSector());
		UpdateFloor(target.sector(), &pSequence->frames[frameIndex]);
		break;
	case SS_SPRITE:
	{
		assert(target.isActor());
		auto actor = target.actor();
		if (!actor) break;
		UpdateSprite(actor, &pSequence->frames[frameIndex]);
		sfxPlay3DSound(actor, pSequence->getSound(frameIndex), -1, 0);

		// NBlood's surfaceSound trigger feature - needs to be reviewed because this never checks if the actor touches the floor!
		if (!VanillaMode() && pSequence->frames[frameIndex].surfaceSound && actor->vel.Z == 0 && actor->vel.X != 0) 
		{
			if (actor->sector()->upperLink) break; // don't play surface sound for stacked sectors
			int surf = tilesurface(actor->sector()->floortexture);
			if (surf <= kSurfNone || surf >= kSurfMax) break;

			int sndId = 800 + surf * 2 + Random(2);
			auto snd = soundEngine->FindSoundByResID(sndId);
			if (snd.isvalid())
			{
				auto udata = soundEngine->GetSfx(snd);
				int relVol = udata ? udata->UserVal : 80;
				sfxPlay3DSoundVolume(actor, snd, -1, 0, 0, (relVol != 80) ? relVol : 25); // some weird shit logic here with the volume...
			}
		}
		break;
	}
	case SS_MASKED:
		assert(target.isWall());
		UpdateMasked(target.wall(), &pSequence->frames[frameIndex]);
		break;
	}

	// all seq callbacks are for sprites, so skip for other types.
	if (type == SS_SPRITE && pSequence->frames[frameIndex].trigger && callback != nullptr)
	{
		if (target.isActor()) callActorFunction(callback, target.actor());
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

struct ActiveList
{
	TArray<SEQINST> list;

	void clear() { list.Clear(); }
	int getSize() { return list.Size(); }
	SEQINST* getInst(int num) { return &list[num]; }
	EventObject getElement(int num) { return list[num].target; }
	//DBloodActor* getActor(int num) { return list[num].actor; }
	int getType(int num) { return list[num].type; }

	void remove(int num)
	{
		list[num] = list.Last();
		list.Pop();
	}

	SEQINST* getNew()
	{
		list.Reserve(1);
		return &list.Last();
	}

	SEQINST* get(int type, const EventObject& eob)
	{
		for (auto& n : list)
		{
			if (n.type == type && n.target == eob) return &n;
		}
		return nullptr;
	}

	SEQINST* get(DBloodActor* actor)
	{
		return get(SS_SPRITE, EventObject(actor));
	}

	void remove(int type, const EventObject& eob)
	{
		for (unsigned i = 0; i < list.Size(); i++)
		{
			auto& n = list[i];
			if (n.type == type && n.target == eob)
			{
				remove(i);
				return;
			}
		}
	}

	void remove(DBloodActor* actor)
	{
		remove(SS_SPRITE, EventObject(actor));
	}

	void Mark()
	{
		for (auto& seqinst : list) seqinst.target.Mark();
	}

};

static ActiveList activeList;

void MarkSeq()
{
	activeList.Mark();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

SEQINST* GetInstance(int type, const EventObject& eob)
{
	return activeList.get(type, eob);
}

SEQINST* GetInstance(DBloodActor* actor)
{
	return activeList.get(actor);
}

void seqKill(int type, const EventObject& nIndex)
{
	activeList.remove(type, nIndex);
}

void seqKillAll()
{
	activeList.clear();
}

void seqKill(int type, sectortype* actor)
{
	activeList.remove(type, EventObject(actor));
}

void seqKill(int type, walltype* actor)
{
	activeList.remove(type, EventObject(actor));
}

void seqKill(DBloodActor* actor)
{
	activeList.remove(actor);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static Seq* ReadSEQ(int index)
{
	auto fr = fileSystem.OpenFileReader(index);
	if (!fr.isOpen())
	{
		Printf("%s: unable to open", fileSystem.GetFileFullName(index));
		return nullptr;
	}
	char header[4];
	fr.Read(header, 4);
	if (memcmp(&header[0], "SEQ\x1a", 4) != 0)
	{
		Printf("%s: Invalid sequence", fileSystem.GetFileFullName(index));
		return nullptr;
	}
	int version = fr.ReadInt16();
	if ((version & 0xff00) != 0x300)
	{
		Printf("%s: Obsolete sequence version", fileSystem.GetFileFullName(index));
		return nullptr;
	}

	unsigned int frames = fr.ReadUInt16();
	int ticks = fr.ReadUInt16();
	int soundid = fr.ReadUInt16();
	int flags = fr.ReadInt32();
	// allocate both buffers off our memory arena.
	auto seqdata = (Seq*)seqcache.Alloc(sizeof(Seq));
	seqdata->frames.Set((SeqFrame*)seqcache.Alloc(sizeof(SeqFrame) * frames), frames);
	seqdata->flags = flags;
	seqdata->ticksPerFrame = ticks;
	seqdata->soundResId = soundid;
	seqdata->soundId = soundEngine->FindSoundByResID(soundid);


	for (auto& frame : seqdata->frames)
	{
		uint8_t framebuf[8];
		fr.Read(framebuf, 8);
		BitReader bitReader(framebuf, sizeof(framebuf));
		int tile = bitReader.getBits(12);
		frame.transparent = bitReader.getBit();
		frame.transparent2 = bitReader.getBit();
		frame.blockable = bitReader.getBit();
		frame.hittable = bitReader.getBit();
		frame.scale.X = bitReader.getBits(8) * REPEAT_SCALEF;
		frame.scale.Y = bitReader.getBits(8) * REPEAT_SCALEF;
		frame.shade = bitReader.getBitsSigned(8);
		frame.palette = bitReader.getBits(5);
		frame.trigger = bitReader.getBit();
		frame.smoke = bitReader.getBit();
		frame.aiming = bitReader.getBit();
		frame.pushable = bitReader.getBit();
		frame.playsound = bitReader.getBit();
		frame.invisible = bitReader.getBit();
		frame.flipx = bitReader.getBit();
		frame.flipy = bitReader.getBit();
		tile += bitReader.getBits(4) << 12;;
		frame.soundRange = bitReader.getBits(4);
		frame.surfaceSound = bitReader.getBit();
		frame.texture = tileGetTextureID(tile);
	}
	return seqdata;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

Seq* getSequence(FName res_name, int res_id)
{
	int64_t key = ((int64_t)res_name.GetIndex() << 32) | res_id;
	auto p = sequences.CheckKey(res_id);
	if (p != nullptr) return *p;

	int index;
	if (res_name == NAME_None)
	{
		index = fileSystem.FindResource(res_id, "SEQ");
	}
	else
	{
		// named sequences are only read from the seq folder.
		FString frame;
		if (res_id > 0) frame.Format("@%d", res_id);
		FStringf path("seq/%s%s.seq", res_name.GetChars(), frame.GetChars());
		index = fileSystem.CheckNumForFullName(path.GetChars());
	}

	if (index < 0)
	{
		if (res_name != NAME_None) Printf("%s: sequence not found", res_name.GetChars());
		sequences.Insert(key, nullptr);	// even store null entries to avoid repeated lookup for them.
		return nullptr;
	}

	auto seqdata = ReadSEQ(index);
	sequences.Insert(key, seqdata);
	return seqdata;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqSpawn_(FName name, int nSeqID, int type, const EventObject& eob, VMFunction* callback)
{
	Seq* pSequence = getSequence(name, nSeqID);

	if (pSequence == nullptr) return;

	SEQINST* pInst = activeList.get(type, eob);
	if (!pInst)
	{
		pInst = activeList.getNew();
	}
	else
	{
		// already playing this sequence?
		if (pInst->nSeqID == nSeqID)
			return;
	}

	pInst->nSeqID = nSeqID;
	pInst->pSequence = pSequence;
	pInst->nName = name;
	pInst->callback = callback;
	pInst->timeCounter = pSequence->ticksPerFrame;
	pInst->frameIndex = 0;
	pInst->type = type;
	pInst->target = eob;
	pInst->Update();
}

void seqSpawn(int nSeqID, DBloodActor* actor, VMFunction* callback)
{
	seqSpawn_(NAME_None, nSeqID, SS_SPRITE, EventObject(actor), callback);
}

void seqSpawn(int nSeqID, int type, sectortype* sect, VMFunction* callback)
{
	assert(type == SS_FLOOR || type == SS_CEILING);
	seqSpawn_(NAME_None, nSeqID, type, EventObject(sect), callback);
}

void seqSpawn(int nSeqID, int type, walltype* wal, VMFunction* callback)
{
	assert(type == SS_WALL || type == SS_MASKED);
	seqSpawn_(NAME_None, nSeqID, type, EventObject(wal), callback);
}

void seqSpawn(FName name, int nSeqID, DBloodActor* actor, VMFunction* callback)
{
	seqSpawn_(name, nSeqID, SS_SPRITE, EventObject(actor), callback);
}

void seqSpawn(FName name, int nSeqID, int type, sectortype* sect, VMFunction* callback)
{
	assert(type == SS_FLOOR || type == SS_CEILING);
	seqSpawn_(name, nSeqID, type, EventObject(sect), callback);
}

void seqSpawn(FName name, int nSeqID, int type, walltype* wal, VMFunction* callback)
{
	assert(type == SS_WALL || type == SS_MASKED);
	seqSpawn_(name, nSeqID, type, EventObject(wal), callback);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seqGetStatus(int type, sectortype* nIndex)
{
	SEQINST* pInst = activeList.get(type, EventObject(nIndex));
	if (pInst) return pInst->frameIndex;
	return -1;
}

int seqGetID(int type, sectortype* nIndex)
{
	SEQINST* pInst = activeList.get(type, EventObject(nIndex));
	if (pInst) return pInst->nSeqID;
	return -1;
}

int seqGetStatus(int type, walltype* nIndex)
{
	SEQINST* pInst = activeList.get(type, EventObject(nIndex));
	if (pInst) return pInst->frameIndex;
	return -1;
}

int seqGetID(int type, walltype* nIndex)
{
	SEQINST* pInst = activeList.get(type, EventObject(nIndex));
	if (pInst) return pInst->nSeqID;
	return -1;
}

int seqGetStatus(DBloodActor* actor)
{
	SEQINST* pInst = activeList.get(actor);
	if (pInst) return pInst->frameIndex;
	return -1;
}

int seqGetID(DBloodActor* actor)
{
	SEQINST* pInst = activeList.get(actor);
	if (pInst) return pInst->nSeqID;
	return -1;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqProcess(int nTicks)
{
	for (int i = 0; i < activeList.getSize(); i++)
	{
		SEQINST* pInst = activeList.getInst(i);
		Seq* pSeq = pInst->pSequence;
		auto target = pInst->target;

		assert(pInst->frameIndex < pSeq->frames.Size());

		pInst->timeCounter -= nTicks;
		while (pInst->timeCounter < 0)
		{
			pInst->timeCounter += pSeq->ticksPerFrame;
			++pInst->frameIndex;

			if (pInst->frameIndex == pSeq->frames.Size())
			{
				if (!pSeq->isLooping())
				{
					if (pSeq->isRemovable())
					{
						if (pInst->type == SS_SPRITE)
						{
							assert(target.isActor());
							auto actor = target.actor();
							if (actor)
							{
								evKillActor(actor);
								if ((actor->spr.hitag & kHitagRespawn) != 0 && actor->WasDudeActor())
									evPostActor(actor, gGameOptions.nMonsterRespawnTime, AF(Respawn));
								else DeleteSprite(actor);	// safe to not use actPostSprite here
							}
						}

						else if (pInst->type == SS_MASKED)
						{
							assert(target.isWall());
							auto pWall = target.wall();
							pWall->cstat &= ~(CSTAT_WALL_XFLIP | CSTAT_WALL_MASKED | CSTAT_WALL_1WAY);
							if (pWall->twoSided())
								pWall->nextWall()->cstat &= ~(CSTAT_WALL_XFLIP | CSTAT_WALL_MASKED | CSTAT_WALL_1WAY);
						}
					}

					// remove it from the active list
					activeList.remove(i--);
					break;
				}
				else pInst->frameIndex = 0;
			}

			pInst->Update();
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SEQINST& w, SEQINST* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type)
			("callback", w.callback)
			("seqid", w.nSeqID)
			("name", w.nName)
			("timecounter", w.timeCounter)
			("frameindex", w.frameIndex)
			("target", w.target);

		arc.EndObject();
	}
	return arc;
}

void SerializeSequences(FSerializer& arc)
{
	arc("sequences", activeList.list);
	if (arc.isReading()) for (unsigned i = 0; i < activeList.list.Size(); i++)
	{
		activeList.list[i].pSequence = getSequence(activeList.list[i].nName, activeList.list[i].nSeqID);
	}
}

END_BLD_NS
