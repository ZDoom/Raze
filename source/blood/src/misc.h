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

#include "common.h"
#include "filesystem.h"

BEGIN_BLD_NS

void playlogos();
void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes);
unsigned int qrand(void);
int wrand(void);
void wsrand(int);
void ChangeExtension(char *pzFile, const char *pzExt);
void SplitPath(const char *pzPath, char *pzDirectory, char *pzFile, char *pzType);
void FireInit(void);
void FireProcess(void);
void UpdateNetworkMenus(void); 
void InitMirrors(void);
void sub_5571C(char mode);
void sub_557C4(int x, int y, int interpolation);
void DrawMirrors(int x, int y, int z, fix16_t a, fix16_t horiz, int smooth, int viewPlayer);
int32_t registerosdcommands(void); 
int qanimateoffs(int a1, int a2);
void qloadpalette();
int32_t qgetpalookup(int32_t a1, int32_t a2);
void HookReplaceFunctions();

struct QAV;
struct PLAYER;
extern QAV* weaponQAV[];

void WeaponInit(void);
void WeaponDraw(PLAYER *pPlayer, int a2, int a3, int a4, int a5, int basepal);
void WeaponRaise(PLAYER *pPlayer);
void WeaponLower(PLAYER *pPlayer);
char WeaponUpgrade(PLAYER *pPlayer, char newWeapon);
void WeaponProcess(PLAYER *pPlayer);
void WeaponUpdateState(PLAYER* pPlayer);
void sub_51340(spritetype *pMissile, int a2);
void StartQAV(PLAYER* pPlayer, int nWeaponQAV, int a3 = -1, char a4 = 0);
void WeaponPrecache(void);

struct ZONE {
    int x, y, z;
    short sectnum, ang;
};
extern ZONE gStartZone[8];

void warpInit(void);
int CheckLink(spritetype *pSprite);
int CheckLink(int *x, int *y, int *z, int *nSector);

extern int costable[2048];

int GetOctant(int x, int y);
void RotateVector(int *dx, int *dy, int nAngle);
void RotatePoint(int *x, int *y, int nAngle, int ox, int oy);
void trigInit();

inline int Sin(int ang)
{
    return costable[(ang - 512) & 2047];
}

inline int Cos(int ang)
{
    return costable[ang & 2047];
} 

enum SurfaceType {
    kSurfNone = 0,
    kSurfStone,
    kSurfMetal,
    kSurfWood,
    kSurfFlesh,
    kSurfWater,
    kSurfDirt,
    kSurfClay,
    kSurfSnow,
    kSurfIce,
    kSurfLeaves,
    kSurfCloth,
    kSurfPlant,
    kSurfGoo,
    kSurfLava,
    kSurfMax
};

extern char surfType[MAXTILES];
extern signed char tileShade[MAXTILES];
extern short voxelIndex[MAXTILES];

extern int nPrecacheCount;
extern char precachehightile[2][(MAXTILES+7)>>3];

int tileInit(char a1, const char *a2);
void tileProcessGLVoxels(void);
const uint8_t * tileLoadTile(int nTile);
uint8_t * tileAllocTile(int nTile, int x, int y);
void tilePreloadTile(int nTile);
void tilePrecacheTile(int nTile, int nType = 1);
char tileGetSurfType(int hit);


END_BLD_NS
