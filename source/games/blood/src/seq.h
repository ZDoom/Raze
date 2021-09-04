//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#pragma once

#include "misc.h"
BEGIN_BLD_NS


struct SEQFRAME 
{
	unsigned int tile : 12;
	unsigned int transparent : 1;
	unsigned int transparent2 : 1;
	unsigned int blockable : 1;
	unsigned int hittable : 1;
	unsigned int xrepeat : 8;
	unsigned int yrepeat : 8;
	signed int shade : 8;
	unsigned int palette : 5;
	unsigned int trigger : 1;
	unsigned int smoke : 1;
	unsigned int aiming : 1;
	unsigned int pushable : 1;
	unsigned int playsound : 1;
	unsigned int invisible : 1;// invisible
	unsigned int flipx : 1;
	unsigned int flipy : 1;
	unsigned int tile2 : 4;
	unsigned soundRange : 4; // (by NoOne) random sound range relative to global SEQ sound
	unsigned surfaceSound : 1; // (by NoOne) trigger surface sound when moving / touching
	unsigned reserved : 2;
};

struct Seq {
	char signature[4];
	short version;
	short nFrames;
	short ticksPerFrame;
	short soundId;
	int flags;
	SEQFRAME frames[1];
	void Precache(int palette);

	bool isLooping()
	{
		return (flags & 1) != 0;
	}

	bool isRemovable()
	{
		return (flags & 2) != 0;
	}
};

class DBloodActor;
struct SEQINST
{
	Seq* pSequence;
	DBloodActor* actor;
	int seqindex, type;

	int nSeqID;
	int callback;
	short timeCounter;
	uint8_t frameIndex;
	void Update();
};

inline int seqGetTile(SEQFRAME* pFrame)
{
	return pFrame->tile + (pFrame->tile2 << 12);
}

int seqRegisterClient(void(*pClient)(int, int));
void seqPrecacheId(int id, int palette);
SEQINST* GetInstance(int a1, int a2);
SEQINST* GetInstance(DBloodActor* actor);
void UnlockInstance(SEQINST* pInst);
void seqSpawn(int a1, int a2, int a3, int a4 = -1);
void seqSpawn(int a1, DBloodActor* actor, int a4 = -1);

void seqKill(int a1, int a2);
void seqKill(DBloodActor* actor);
void seqKillAll(void);
int seqGetStatus(int a1, int a2);
int seqGetStatus(DBloodActor*);
int seqGetID(int a1, int a2);
int seqGetID(DBloodActor*);
void seqProcess(int a1);

Seq* getSequence(int res_id);


END_BLD_NS
