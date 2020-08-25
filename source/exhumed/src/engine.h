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

#ifndef __engine_h__
#define __engine_h__

#include "compat.h"
#include "build.h"
#include "pragmas.h"

BEGIN_PS_NS

#define kMaxSprites 4096
#define kMaxSectors 1024
#define kMaxWalls   8192
#define kMaxVoxels	4096

enum
{
	kStatIgnited = 404
};


#define kMaxPalookups 256
#define kMaxStatus   1024
//#define MAXPSKYTILES 256


int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, unsigned int clipmask);
void precache();
void resettiming();

// cd

bool playCDtrack(int nTrack, bool bLoop);
int StepFadeCDaudio();
bool CDplaying();
void StopCD();

// init
#define kMap20	20

enum {
    kSectUnderwater = 0x2000,
    kSectLava = 0x4000,
};

extern int ogameclock;
extern int initx;
extern int inity;
extern int initz;
extern short inita;
extern short initsect;

extern short nCurChunkNum;
extern short nBodyGunSprite[50];
extern int movefifoend;
extern int movefifopos;
extern short nCurBodyGunNum;

void SnapSectors(short nSectorA, short nSectorB, short b);

extern short SectSound[];
extern short SectDamage[];
extern short SectSpeed[];
extern int SectBelow[];
extern short SectFlag[];
extern int SectDepth[];
extern short SectSoundSect[];
extern int SectAbove[];

uint8_t LoadLevel(int nMap);
void LoadObjects();

// light

int LoadPaletteLookups();
void SetGreenPal();
void RestoreGreenPal();
void FixPalette();
int HavePLURemap();
uint8_t RemapPLU(uint8_t pal);

//extern unsigned char kenpal[];
extern short overscanindex;

extern char *origpalookup[];

extern short nPalDiff;

// map

extern short bShowTowers;
extern int ldMapZoom;
extern int lMapZoom;

void InitMap();
void GrabMap();
void UpdateMap();
void DrawMap();

// network

extern short nNetMoveFrames;

// random

void InitRandom();
int RandomBit();
char RandomByte();
uint16_t RandomWord();
int RandomLong();
int RandomSize(int nSize);

// record

// save

// trigdat

#define kAngleMask	0x7FF

int GetMyAngle(int x, int y);

int AngleDiff(short a, short b);
int AngleDelta(int a, int b, int c);

inline int Sin(int angle)
{
    return sintable[angle & kAngleMask];
}

inline double FSin(double angle)
{
    return calcSinTableValue(fmod(angle, kAngleMask + 1));
}

inline int Cos(int angle)
{
    return sintable[(angle + 512) & kAngleMask];
}

END_PS_NS

#endif
