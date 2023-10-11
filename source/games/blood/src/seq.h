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
#pragma once

#include "misc.h"
#include "common_game.h"
BEGIN_BLD_NS


struct SeqFrame
{
	FVector2 scale;
	FTextureID texture;
	uint8_t palette;
	int8_t shade;
	int8_t soundRange;
	unsigned int transparent : 1;
	unsigned int transparent2 : 1;
	unsigned int blockable : 1;
	unsigned int hittable : 1;
	unsigned int trigger : 1;
	unsigned int smoke : 1;
	unsigned int aiming : 1;
	unsigned int pushable : 1;
	unsigned int playsound : 1;
	unsigned int invisible : 1;
	unsigned int flipx : 1;
	unsigned int flipy : 1;
	unsigned int surfaceSound : 1; // (by NoOne) trigger surface sound when moving / touching
};

struct Seq
{
	TArrayView<SeqFrame> frames;
	int ticksPerFrame;
	FSoundID soundId;
	int soundResId;	// still needed for the soundRange feature
	int flags;

	bool isLooping()
	{
		return (flags & 1) != 0;
	}

	bool isRemovable()
	{
		return (flags & 2) != 0;
	}

	FSoundID getSound(int frame)
	{
		if (!frames[frame].playsound) return NO_SOUND;
		int range = frames[frame].soundRange;
		if (VanillaMode() || range <= 0) return soundId;
		return soundEngine->FindSoundByResID(soundResId + Random(max(2, range)));
	}

};

class DBloodActor;
struct SEQINST
{
	Seq* pSequence;
	EventObject target;
	VMFunction* callback;

	int type;
	int nSeqID;	// only one of these two may be set
	FName nName;
	int16_t timeCounter;
	uint16_t frameIndex;
	void Update();
};

void seqPrecacheId(FName name, int id, int palette);

inline void seqPrecacheId(int id, int palette)
{
	seqPrecacheId(NAME_None, id, palette);
}

SEQINST* GetInstance(int a1, EventObject& a2);
SEQINST* GetInstance(DBloodActor* actor);

void seqSpawn(int a1, int ty, walltype* a2, VMFunction* a4 = nullptr);
void seqSpawn(int a1, int ty, sectortype* a2, VMFunction* a4 = nullptr);
void seqSpawn(int a1, DBloodActor* actor, VMFunction* a4 = nullptr);

void seqSpawn(FName name, int nSeqID, DBloodActor* actor, VMFunction* callback = nullptr);
void seqSpawn(FName name, int nSeqID, int type, sectortype* sect, VMFunction* callback = nullptr);
void seqSpawn(FName name, int nSeqID, int type, walltype* wal, VMFunction* callback = nullptr);

void seqKill(int a1, walltype* a2);
void seqKill(int a1, sectortype* a2);
void seqKill(DBloodActor* actor);
void seqKillAll(void);
int seqGetStatus(int a1, walltype* a2);
int seqGetStatus(int a1, sectortype* a2);
int seqGetStatus(DBloodActor*);
int seqGetID(int a1, walltype* a2);
int seqGetID(int a1, sectortype* a2);
int seqGetID(DBloodActor*);
void seqProcess(int a1);

Seq* getSequence(FName res_name, int res_id);


END_BLD_NS
