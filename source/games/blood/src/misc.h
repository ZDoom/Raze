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

#include "m_fixed.h"
#include "filesystem.h"

BEGIN_BLD_NS

void playlogos();
unsigned int qrand(void);
int wrand(void);
void wsrand(int);
void FireInit(void);
void FireProcess(void);
void UpdateNetworkMenus(void); 
void InitMirrors(void);
void setPortalFlags(char mode);
void processSpritesOnOtherSideOfPortal(int x, int y, int interpolation);
void DrawMirrors(int x, int y, int z, fixed_t a, fixed_t horiz, int smooth, int viewPlayer);
int qanimateoffs(int a1, int a2);
void HookReplaceFunctions();

struct QAV;
struct PLAYER;
extern QAV* weaponQAV[];

void WeaponInit(void);
void WeaponDraw(PLAYER *pPlayer, int a2, double a3, double a4, int a5, double smoothratio);
void WeaponRaise(PLAYER *pPlayer);
void WeaponLower(PLAYER *pPlayer);
int WeaponUpgrade(PLAYER *pPlayer, int newWeapon);
void WeaponProcess(PLAYER *pPlayer);
void WeaponUpdateState(PLAYER* pPlayer);
void teslaHit(spritetype *pMissile, int a2);
void WeaponPrecache();

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

#include "m_fixed.h"

inline int Sin(int ang)
{
    return costable[(ang - 512) & 2047];
}

inline int Cos(int ang)
{
    return costable[ang & 2047];
}

inline int SinScale16(int ang)
{
    return FixedToInt(costable[(ang - 512) & 2047]);
}

inline int CosScale16(int ang)
{
    return FixedToInt(costable[ang & 2047]);
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
extern int8_t tileShade[MAXTILES];
extern short voxelIndex[MAXTILES];

extern int nPrecacheCount;

void tilePrecacheTile(int nTile, int nType, int palette);

char tileGetSurfType(int hit);

END_BLD_NS
