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

void UpdateCeiling(int nXSector, SEQFRAME* pFrame)
{
	assert(nXSector > 0 && nXSector < kMaxXSectors);
	int nSector = xsector[nXSector].reference;
	assert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSector = &sector[nSector];
	assert(pSector->extra == nXSector);
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

void UpdateFloor(int nXSector, SEQFRAME* pFrame)
{
	assert(nXSector > 0 && nXSector < kMaxXSectors);
	int nSector = xsector[nXSector].reference;
	assert(nSector >= 0 && nSector < kMaxSectors);
	sectortype* pSector = &sector[nSector];
	assert(pSector->extra == nXSector);
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

void UpdateWall(int nXWall, SEQFRAME* pFrame)
{
	assert(nXWall > 0 && nXWall < kMaxXWalls);
	int nWall = xwall[nXWall].reference;
	assert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall = &wall[nWall];
	assert(pWall->extra == nXWall);
	pWall->picnum = seqGetTile(pFrame);
	if (pFrame->palette)
		pWall->pal = pFrame->palette;
	if (pFrame->transparent)
		pWall->cstat |= 128;
	else
		pWall->cstat &= ~128;
	if (pFrame->transparent2)
		pWall->cstat |= 512;
	else
		pWall->cstat &= ~512;
	if (pFrame->blockable)
		pWall->cstat |= 1;
	else
		pWall->cstat &= ~1;
	if (pFrame->hittable)
		pWall->cstat |= 64;
	else
		pWall->cstat &= ~64;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateMasked(int nXWall, SEQFRAME* pFrame)
{
	assert(nXWall > 0 && nXWall < kMaxXWalls);
	int nWall = xwall[nXWall].reference;
	assert(nWall >= 0 && nWall < kMaxWalls);
	walltype* pWall = &wall[nWall];
	assert(pWall->extra == nXWall);
	assert(pWall->nextwall >= 0);
	walltype* pWallNext = &wall[pWall->nextwall];
	pWall->overpicnum = pWallNext->overpicnum = seqGetTile(pFrame);
	if (pFrame->palette)
		pWall->pal = pWallNext->pal = pFrame->palette;
	if (pFrame->transparent)
	{
		pWall->cstat |= 128;
		pWallNext->cstat |= 128;
	}
	else
	{
		pWall->cstat &= ~128;
		pWallNext->cstat &= ~128;
	}
	if (pFrame->transparent2)
	{
		pWall->cstat |= 512;
		pWallNext->cstat |= 512;
	}
	else
	{
		pWall->cstat &= ~512;
		pWallNext->cstat &= ~512;
	}
	if (pFrame->blockable)
	{
		pWall->cstat |= 1;
		pWallNext->cstat |= 1;
	}
	else
	{
		pWall->cstat &= ~1;
		pWallNext->cstat &= ~1;
	}
	if (pFrame->hittable)
	{
		pWall->cstat |= 64;
		pWallNext->cstat |= 64;
	}
	else
	{
		pWall->cstat &= ~64;
		pWallNext->cstat &= ~64;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UpdateSprite(DBloodActor* actor, SEQFRAME* pFrame)
{
	spritetype* pSprite = &actor->s();
	assert(actor->hasX());
	if (pSprite->flags & 2)
	{
		if (tileHeight(pSprite->picnum) != tileHeight(seqGetTile(pFrame)) || tileTopOffset(pSprite->picnum) != tileTopOffset(seqGetTile(pFrame))
			|| (pFrame->yrepeat && pFrame->yrepeat != pSprite->yrepeat))
			pSprite->flags |= 4;
	}
	pSprite->picnum = seqGetTile(pFrame);
	if (pFrame->palette)
		pSprite->pal = pFrame->palette;
	pSprite->shade = pFrame->shade;

	int scale = actor->x().scale; // SEQ size scaling
	if (pFrame->xrepeat) {
		if (scale) pSprite->xrepeat = ClipRange(MulScale(pFrame->xrepeat, scale, 8), 0, 255);
		else pSprite->xrepeat = pFrame->xrepeat;
	}

	if (pFrame->yrepeat) {
		if (scale) pSprite->yrepeat = ClipRange(MulScale(pFrame->yrepeat, scale, 8), 0, 255);
		else pSprite->yrepeat = pFrame->yrepeat;
	}

	if (pFrame->transparent)
		pSprite->cstat |= 2;
	else
		pSprite->cstat &= ~2;
	if (pFrame->transparent2)
		pSprite->cstat |= 512;
	else
		pSprite->cstat &= ~512;
	if (pFrame->blockable)
		pSprite->cstat |= 1;
	else
		pSprite->cstat &= ~1;
	if (pFrame->hittable)
		pSprite->cstat |= 256;
	else
		pSprite->cstat &= ~256;
	if (pFrame->invisible)
		pSprite->cstat |= 32768;
	else
		pSprite->cstat &= (unsigned short)~32768;
	if (pFrame->pushable)
		pSprite->cstat |= 4096;
	else
		pSprite->cstat &= ~4096;
	if (pFrame->smoke)
		pSprite->flags |= 256;
	else
		pSprite->flags &= ~256;
	if (pFrame->aiming)
		pSprite->flags |= 8;
	else
		pSprite->flags &= ~8;
	if (pFrame->flipx)
		pSprite->flags |= 1024;
	else
		pSprite->flags &= ~1024;
	if (pFrame->flipy)
		pSprite->flags |= 2048;
	else
		pSprite->flags &= ~2048;
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
	case 0:
		UpdateWall(seqindex, &pSequence->frames[frameIndex]);
		break;
	case 1:
		UpdateCeiling(seqindex, &pSequence->frames[frameIndex]);
		break;
	case 2:
		UpdateFloor(seqindex, &pSequence->frames[frameIndex]);
		break;
	case 3:
	{
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
		spritetype* pSprite = &actor->s();
		if (!VanillaMode() && pSequence->frames[frameIndex].surfaceSound && actor->zvel == 0 && actor->xvel != 0) {

			if (getUpperLink(pSprite->sectnum)) break; // don't play surface sound for stacked sectors
			int surf = tileGetSurfType(sector[pSprite->sectnum].floorpicnum); 
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
				sfxPlay3DSoundCP(pSprite, sndId, -1, 0, 0, (surfSfxMove[surf][2] != relVol) ? relVol : surfSfxMove[surf][3]);
			}
		}
		break;
	}
	case 4:
		UpdateMasked(seqindex, &pSequence->frames[frameIndex]);
		break;
	}

	// all seq callbacks are for sprites, but there's no sanity checks here that what gets passed is meant to be for a sprite...
	if (pSequence->frames[frameIndex].trigger && callback != -1)
	{
		assert(type == 3);
		if (type == 3) seqClientCallback[callback](type, actor);
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
	int getIndex(int num) { return list[num].seqindex; }
	DBloodActor* getActor(int num) { return list[num].actor; }
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

	SEQINST* get(int type, int index)
	{
		for (auto& n : list)
		{
			if (n.type == type && n.seqindex == index) return &n;
		}
		return nullptr;
	}

	SEQINST* get(DBloodActor* actor)
	{
		for (auto& n : list)
		{
			if (n.type == 3 && n.actor == actor) return &n;
		}
		return nullptr;
	}

	void remove(int type, int index)
	{
		for (unsigned i = 0; i < list.Size(); i++)
		{
			auto& n = list[i];
			if (n.type == type && n.seqindex == index)
			{
				remove(i);
				return;
			}
		}
	}

	void remove(DBloodActor* actor)
	{
		for (unsigned i = 0; i < list.Size(); i++)
		{
			auto& n = list[i];
			if (n.type == 3 && n.actor == actor)
			{
				remove(i);
				return;
			}
		}
	}

};

static ActiveList activeList;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

SEQINST* GetInstance(int type, int nXIndex)
{
	return activeList.get(type, nXIndex);
}

SEQINST* GetInstance(DBloodActor* actor)
{
	return activeList.get(actor);
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

void seqKill(int type, int nXIndex)
{
	assert(type != SS_SPRITE);
	activeList.remove(type, nXIndex);
}

void seqKillAll()
{
	activeList.clear();
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
#if 1//B_BIG_ENDIAN == 1
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
#endif
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

void seqSpawn(int nSeqID, int type, int nXIndex, int callback)
{
	assert(type != SS_SPRITE);
	Seq* pSequence = getSequence(nSeqID);

	if (pSequence == nullptr) return;

	SEQINST* pInst = activeList.get(type, nXIndex);
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
	pInst->seqindex = nXIndex;
	pInst->actor = nullptr;
	pInst->Update();
}

void seqSpawn(int nSeqID, DBloodActor* actor, int callback)
{
	Seq* pSequence = getSequence(nSeqID);

	if (pSequence == nullptr) return;

	SEQINST* pInst = activeList.get(actor);
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
	pInst->type = SS_SPRITE;
	pInst->seqindex = 0;
	pInst->actor = actor;
	pInst->Update();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seqGetStatus(int type, int nXIndex)
{
	SEQINST* pInst = activeList.get(type, nXIndex);
	if (pInst) return pInst->frameIndex;
	return -1;
}

int seqGetID(int type, int nXIndex)
{
	SEQINST* pInst = activeList.get(type, nXIndex);
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
		int index = pInst->seqindex;
		auto actor = pInst->actor;

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
							if (actor)
							{
								evKillActor(actor);
								if ((actor->s().hitag & kAttrRespawn) != 0 && (actor->s().inittype >= kDudeBase && actor->s().inittype < kDudeMax))
								evPostActor(actor, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
								else DeleteSprite(actor);	// safe to not use actPostSprite here
							}
						}

						else if (pInst->type == SS_MASKED)
						{
							int nWall = xwall[index].reference;
							assert(nWall >= 0 && nWall < kMaxWalls);
							wall[nWall].cstat &= ~(8 + 16 + 32);
							if (wall[nWall].nextwall != -1)
								wall[wall[nWall].nextwall].cstat &= ~(8 + 16 + 32);
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
		arc	("type", w.type)
			("callback", w.callback)
			("seqid", w.nSeqID)
			("timecounter", w.timeCounter)
			("frameindex", w.frameIndex);
#ifdef OLD_SAVEGAME
		if (w.type == SS_SPRITE) arc("index", w.actor);
		else arc("index", w.seqindex);
#else
		arc("index", w.seqindex)
			("actor", w.actor);
#endif
			
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
