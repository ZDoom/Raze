//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#ifndef __move_h__
#define __move_h__

// 16 bytes
struct BlockInfo
{
    int x;
    int y;
    int field_8;
    short nSprite;
};
extern BlockInfo sBlockInfo[];

extern int hihit;
extern short nChunkSprite[];

signed int lsqrt(int a1);
void MoveThings();
void ResetMoveFifo();
void InitChunks();
void InitPushBlocks();
void Gravity(short nSprite);
short UpdateEnemy(short *nEnemy);
int MoveCreature(short nSprite);
int MoveCreatureWithCaution(int nSprite);
void WheresMyMouth(int nPlayer, int *x, int *y, int *z, short *sectnum);

int GetSpriteHeight(int nSprite);

int GrabBody();

int GrabBodyGunSprite();
void CreatePushBlock(int nSector);

void FuncCreatureChunk(int a, int, int nRun);

int FindPlayer(int nSprite, int nVal);

int BuildCreatureChunk(int nVal, int nPic);

void BuildNear(int x, int y, int walldist, int nSector);
int BelowNear(short nSprite);

int PlotCourseToSprite(int nSprite1, int nSprite2);

void CheckSectorFloor(short nSector, int z, int *a, int *b);

int GetAngleToSprite(int nSprite1, int nSprite2);

int GetWallNormal(short nWall);

int GetUpAngle(short nSprite1, int nVal, short nSprite2, int ecx);
void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel);

int AngleChase(int nSprite, int nSprite2, int ebx, int ecx, int push1);

void SetQuake(short nSprite, int nVal);

#endif
