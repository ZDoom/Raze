//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Raze

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "filesystem.h"
#include "tarray.h"

#include "build.h"

#include "blood.h"
#include "files.h"
#include "eventq.h"
#include "callback.h"


BEGIN_BLD_NS

static void (*seqClientCallback[])(int, DBloodActor*) = {
	FireballSeqCallback,
	Fx33Callback,
	NapalmSeqCallback,
	Fx32Callback,
	TreeToGibCallback,
	DudeToGibCallback1,
	DudeToGibCallback2,
	batBiteSeqCallback,
	SlashSeqCallback,
	StompSeqCallback,
	eelBiteSeqCallback,
	BurnSeqCallback,
	SeqAttackCallback,
	cerberusBiteSeqCallback,
	cerberusBurnSeqCallback,
	cerberusBurnSeqCallback2,
	TommySeqCallback,
	TeslaSeqCallback,
	ShotSeqCallback,
	cultThrowSeqCallback,
	sub_68170,
	sub_68230,
	SlashFSeqCallback,
	ThrowFSeqCallback,
	ThrowSSeqCallback,
	BlastSSeqCallback,
	ghostSlashSeqCallback,
	ghostThrowSeqCallback,
	ghostBlastSeqCallback,
	GillBiteSeqCallback,
	HandJumpSeqCallback,
	houndBiteSeqCallback,
	houndBurnSeqCallback,
	podAttack,
	sub_70284,
	sub_6FF08,
	sub_6FF54,
	ratBiteSeqCallback,
	SpidBiteSeqCallback,
	SpidJumpSeqCallback,
	SpidBirthSeqCallback,
	sub_71BD4,
	sub_720AC,
	sub_71A90,
	genDudeAttack1,
	punchCallback,
	ThrowCallback1,
	ThrowCallback2,
	HackSeqCallback,
	StandSeqCallback,
	zombfHackSeqCallback,
	PukeSeqCallback,
	ThrowSeqCallback,
	PlayerSurvive,
	PlayerKneelsOver,
	FireballTrapSeqCallback,
	MGunFireSeqCallback,
	MGunOpenSeqCallback,
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Seq::Precache(int palette)
{
	if (memcmp(signature, "SEQ\x1a", 4) != 0)
		I_Error("Invalid sequence");
	if ((version & 0xff00) != 0x300)
		I_Error("Obsolete sequence version");
	for (int i = 0; i < nFrames; i++)
		tilePrecacheTile(seqGetTile(&frames[i]), -1, palette);
}

void seqPrecacheId(int id, int palette)
{
	auto pSeq = getSequence(id);
	if (pSeq) pSeq->Precache(palette);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateCeiling(sectortype* pSector, SEQFRAME* pFrame)
{
	pSector->ceilingpicnum = seqGetTile(pFrame);
	pSector->ceilingshade = pFrame->shade;
	if (pFrame->palette)
		pSector->ceilingpal = pFrame->palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateFloor(sectortype* pSector, SEQFRAME* pFrame)
{
	pSector->floorpicnum = seqGetTile(pFrame);
	pSector->floorshade = pFrame->shade;
	if (pFrame->palette)
		pSector->floorpal = pFrame->palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateWall(walltype* pWall, SEQFRAME* pFrame)
{
	assert(pWall->hasX());
	pWall->picnum = seqGetTile(pFrame);
	if (pFrame->palette)
		pWall->pal = pFrame->palette;
	if (pFrame->transparent)
		pWall->cstat |= CSTAT_WALL_TRANSLUCENT;
	else
		pWall->cstat &= ~CSTAT_WALL_TRANSLUCENT;
	if (pFrame->transparent2)
		pWall->cstat |= CSTAT_WALL_TRANS_FLIP;
	else
		pWall->cstat &= ~CSTAT_WALL_TRANS_FLIP;
	if (pFrame->blockable)
		pWall->cstat |= CSTAT_WALL_BLOCK;
	else
		pWall->cstat &= ~CSTAT_WALL_BLOCK;
	if (pFrame->hittable)
		pWall->cstat |= CSTAT_WALL_BLOCK_HITSCAN;
	else
		pWall->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateMasked(walltype* pWall, SEQFRAME* pFrame)
{
	assert(pWall->hasX());
	walltype* pWallNext = pWall->nextWall();
	pWall->overpicnum = pWallNext->overpicnum = seqGetTile(pFrame);
	if (pFrame->palette)
		pWall->pal = pWallNext->pal = pFrame->palette;
	if (pFrame->transparent)
	{
		pWall->cstat |= CSTAT_WALL_TRANSLUCENT;
		pWallNext->cstat |= CSTAT_WALL_TRANSLUCENT;
	}
	else
	{
		pWall->cstat &= ~CSTAT_WALL_TRANSLUCENT;
		pWallNext->cstat &= ~CSTAT_WALL_TRANSLUCENT;
	}
	if (pFrame->transparent2)
	{
		pWall->cstat |= CSTAT_WALL_TRANS_FLIP;
		pWallNext->cstat |= CSTAT_WALL_TRANS_FLIP;
	}
	else
	{
		pWall->cstat &= ~CSTAT_WALL_TRANS_FLIP;
		pWallNext->cstat &= ~CSTAT_WALL_TRANS_FLIP;
	}
	if (pFrame->blockable)
	{
		pWall->cstat |= CSTAT_WALL_BLOCK;
		pWallNext->cstat |= CSTAT_WALL_BLOCK;
	}
	else
	{
		pWall->cstat &= ~CSTAT_WALL_BLOCK;
		pWallNext->cstat &= ~CSTAT_WALL_BLOCK;
	}
	if (pFrame->hittable)
	{
		pWall->cstat |= CSTAT_WALL_BLOCK_HITSCAN;
		pWallNext->cstat |= CSTAT_WALL_BLOCK_HITSCAN;
	}
	else
	{
		pWall->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
		pWallNext->cstat &= ~CSTAT_WALL_BLOCK_HITSCAN;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateSprite(DBloodActor* actor, SEQFRAME* pFrame)
{
	assert(actor->hasX());
	if (actor->spr.flags & 2)
	{
		if (tileHeight(actor->spr.picnum) != tileHeight(seqGetTile(pFrame)) || tileTopOffset(actor->spr.picnum) != tileTopOffset(seqGetTile(pFrame))
			|| (pFrame->yrepeat && pFrame->yrepeat != actor->spr.yrepeat))
			actor->spr.flags |= 4;
	}
	actor->spr.picnum = seqGetTile(pFrame);
	if (pFrame->palette)
		actor->spr.pal = pFrame->palette;
	actor->spr.shade = pFrame->shade;

	int scale = actor->xspr.scale; // SEQ size scaling
	if (pFrame->xrepeat) {
		if (scale) actor->spr.xrepeat = ClipRange(MulScale(pFrame->xrepeat, scale, 8), 0, 255);
		else actor->spr.xrepeat = pFrame->xrepeat;
	}

	if (pFrame->yrepeat) {
		if (scale) actor->spr.yrepeat = ClipRange(MulScale(pFrame->yrepeat, scale, 8), 0, 255);
		else actor->spr.yrepeat = pFrame->yrepeat;
	}

	if (pFrame->transparent)
		actor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_TRANSLUCENT;
	if (pFrame->transparent2)
		actor->spr.cstat |= CSTAT_SPRITE_TRANS_FLIP;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_TRANS_FLIP;
	if (pFrame->blockable)
		actor->spr.cstat |= CSTAT_SPRITE_BLOCK;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK;
	if (pFrame->hittable)
		actor->spr.cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_HITSCAN;
	if (pFrame->invisible)
		actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
	if (pFrame->pushable)
		actor->spr.cstat |= CSTAT_SPRITE_BLOOD_BIT1;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_BLOOD_BIT1;
	if (pFrame->smoke)
		actor->spr.flags |= 256;
	else
		actor->spr.flags &= ~256;
	if (pFrame->aiming)
		actor->spr.flags |= 8;
	else
		actor->spr.flags &= ~8;
	if (pFrame->flipx)
		actor->spr.flags |= 1024;
	else
		actor->spr.flags &= ~1024;
	if (pFrame->flipy)
		actor->spr.flags |= 2048;
	else
		actor->spr.flags &= ~2048;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SEQINST::Update()
{
	assert(frameIndex < pSequence->nFrames);
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
		if (pSequence->frames[frameIndex].playsound) {

			int sound = pSequence->soundId;

			// by NoOne: add random sound range feature
			if (!VanillaMode() && pSequence->frames[frameIndex].soundRange > 0)
				sound += Random(((pSequence->frames[frameIndex].soundRange == 1) ? 2 : pSequence->frames[frameIndex].soundRange));

			sfxPlay3DSound(actor, sound, -1, 0);
		}


		// by NoOne: add surfaceSound trigger feature
		if (!VanillaMode() && pSequence->frames[frameIndex].surfaceSound && actor->vel.Z == 0 && actor->vel.X != 0) {

			if (actor->sector()->upperLink) break; // don't play surface sound for stacked sectors
			int surf = tileGetSurfType(actor->sector()->floorpicnum);
			if (!surf) break;
			static int surfSfxMove[15][4] = {
				/* {snd1, snd2, gameVolume, myVolume} */
				{800,801,80,25},
				{802,803,80,25},
				{804,805,80,25},
				{806,807,80,25},
				{808,809,80,25},
				{810,811,80,25},
				{812,813,80,25},
				{814,815,80,25},
				{816,817,80,25},
				{818,819,80,25},
				{820,821,80,25},
				{822,823,80,25},
				{824,825,80,25},
				{826,827,80,25},
				{828,829,80,25},
			};

			int sndId = surfSfxMove[surf][Random(2)];
			auto snd = soundEngine->FindSoundByResID(sndId);
			if (snd > 0)
			{
				auto udata = soundEngine->GetUserData(snd);
				int relVol = udata ? udata[2] : 255;
				sfxPlay3DSoundCP(actor, sndId, -1, 0, 0, (surfSfxMove[surf][2] != relVol) ? relVol : surfSfxMove[surf][3]);
			}
		}
		break;
	}
	case SS_MASKED:
		assert(target.isWall());
		UpdateMasked(target.wall(), &pSequence->frames[frameIndex]);
		break;
	}

	// all seq callbacks are for sprites, but there's no sanity checks here that what gets passed is meant to be for a sprite...
	if (pSequence->frames[frameIndex].trigger && callback != -1)
	{
		assert(type == SS_SPRITE);
		if (target.isActor()) seqClientCallback[callback](type, target.actor());
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

};

static ActiveList activeList;

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

static void ByteSwapSEQ(Seq* pSeq)
{
	pSeq->version = LittleShort(pSeq->version);
	pSeq->nFrames = LittleShort(pSeq->nFrames);
	pSeq->ticksPerFrame = LittleShort(pSeq->ticksPerFrame);
	pSeq->soundId = LittleShort(pSeq->soundId);
	pSeq->flags = LittleLong(pSeq->flags);
	for (int i = 0; i < pSeq->nFrames; i++)
	{
		SEQFRAME* pFrame = &pSeq->frames[i];
		BitReader bitReader((char*)pFrame, sizeof(SEQFRAME));
		SEQFRAME swapFrame;
		swapFrame.tile = bitReader.readUnsigned(12);
		swapFrame.transparent = bitReader.readBit();
		swapFrame.transparent2 = bitReader.readBit();
		swapFrame.blockable = bitReader.readBit();
		swapFrame.hittable = bitReader.readBit();
		swapFrame.xrepeat = bitReader.readUnsigned(8);
		swapFrame.yrepeat = bitReader.readUnsigned(8);
		swapFrame.shade = bitReader.readSigned(8);
		swapFrame.palette = bitReader.readUnsigned(5);
		swapFrame.trigger = bitReader.readBit();
		swapFrame.smoke = bitReader.readBit();
		swapFrame.aiming = bitReader.readBit();
		swapFrame.pushable = bitReader.readBit();
		swapFrame.playsound = bitReader.readBit();
		swapFrame.invisible = bitReader.readBit();
		swapFrame.flipx = bitReader.readBit();
		swapFrame.flipy = bitReader.readBit();
		swapFrame.tile2 = bitReader.readUnsigned(4);
		swapFrame.soundRange = bitReader.readUnsigned(4);
		swapFrame.surfaceSound = bitReader.readBit();
		swapFrame.reserved = bitReader.readUnsigned(2);
		*pFrame = swapFrame;
	}
}

FMemArena seqcache;
static TMap<int, Seq*> sequences;
Seq* getSequence(int res_id)
{
	auto p = sequences.CheckKey(res_id);
	if (p != nullptr) return *p;

	int index = fileSystem.FindResource(res_id, "SEQ");
	if (index < 0)
	{
		return nullptr;
	}
	auto fr = fileSystem.OpenFileReader(index);
	auto seqdata = (Seq*)seqcache.Alloc(fr.GetLength());
	fr.Read(seqdata, fr.GetLength());
	sequences.Insert(res_id, seqdata);
	ByteSwapSEQ(seqdata);
	return seqdata;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seqSpawn_(int nSeqID, int type, const EventObject& eob, int callback)
{
	Seq* pSequence = getSequence(nSeqID);

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

	pInst->pSequence = pSequence;
	pInst->nSeqID = nSeqID;
	pInst->callback = callback;
	pInst->timeCounter = (short)pSequence->ticksPerFrame;
	pInst->frameIndex = 0;
	pInst->type = type;
	pInst->target = eob;
	pInst->Update();
}

void seqSpawn(int nSeqID, DBloodActor* actor, int callback)
{
	seqSpawn_(nSeqID, SS_SPRITE, EventObject(actor), callback);
}

void seqSpawn(int nSeqID, int type, sectortype* sect, int callback)
{
	assert(type == SS_FLOOR || type == SS_CEILING);
	seqSpawn_(nSeqID, type, EventObject(sect), callback);
}

void seqSpawn(int nSeqID, int type, walltype* wal, int callback)
{
	assert(type == SS_WALL || type == SS_MASKED);
	seqSpawn_(nSeqID, type, EventObject(wal), callback);
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

		assert(pInst->frameIndex < pSeq->nFrames);

		pInst->timeCounter -= nTicks;
		while (pInst->timeCounter < 0)
		{
			pInst->timeCounter += pSeq->ticksPerFrame;
			++pInst->frameIndex;

			if (pInst->frameIndex == pSeq->nFrames)
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
								if ((actor->spr.hitag & kAttrRespawn) != 0 && (actor->spr.inittype >= kDudeBase && actor->spr.inittype < kDudeMax))
									evPostActor(actor, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
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
		activeList.list[i].pSequence = getSequence(activeList.list[i].nSeqID);
	}
}

END_BLD_NS
