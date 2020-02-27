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

#ifndef __object_h__
#define __object_h__

BEGIN_PS_NS

#define kMaxPoints	1024
#define kMaxSlides	128
#define kMaxElevs	1024

enum kStatus
{
    kStatDestructibleSprite = 97,
    kStatAnubisDrum,
    kStatExplodeTrigger = 141,
    kStatExplodeTarget = 152
};

extern short nSmokeSparks;
extern short nDronePitch;
extern int lFinaleStart;
extern short nFinaleSpr;

void InitObjects();
void InitElev();
void InitPoint();
void InitSlide();
void InitWallFace();
void DoDrips();
void DoMovingSects();
void DoFinale();
void PostProcess();

void FuncElev(int, int, int);
void FuncWallFace(int, int, int);
void FuncSlide(int, int, int);
void FuncObject(int, int, int);
void FuncTrap(int, int, int);
void FuncEnergyBlock(int, int, int);
void FuncSpark(int, int, int);

void SnapBobs(short nSectorA, short nSectorB);

short FindWallSprites(short nSector);

void AddMovingSector(int nSector, int edx, int ebx, int ecx);

int BuildWallSprite(int nSector);

void ProcessTrailSprite(int nSprite, int nLotag, int nHitag);

void AddSectorBob(int nSector, int nHitag, int bx);

int BuildObject(short nSprite, int nOjectType, int nHitag);

int BuildArrow(int nSprite, int nVal);

int BuildFireBall(int nSprite, int a, int b);

void BuildDrip(int nSprite);

int BuildEnergyBlock(short nSector);

int BuildElevC(int arg1, int nChannel, int nSector, int nWallSprite, int arg5, int arg6, int nCount, ...);
int BuildElevF(int nChannel, int nSector, int nWallSprite, int arg_4, int arg_5, int nCount, ...);

int BuildWallFace(short nChannel, short nWall, int nCount, ...);

int BuildSlide(int nChannel, int edx, int ebx, int ecx, int arg1, int arg2, int arg3);

END_PS_NS

#endif
