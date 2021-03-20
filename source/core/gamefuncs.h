#pragma once

#include "gamecontrol.h"
#include "buildtypes.h"
#include "binaryangle.h"

extern int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, short *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio);
bool spriteIsModelOrVoxel(const spritetype* tspr);
void PlanesAtPoint(const sectortype* sec, float dax, float day, float* ceilz, float* florz);
void setWallSectors();
void GetWallSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out);
void GetFlatSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out);

// y is negated so that the orientation is the same as in GZDoom, in order to use its utilities.
// The render code should NOT use Build coordinates for anything!

inline double WallStartX(int wallnum)
{
    return wall[wallnum].x * (1 / 16.);
}

inline double WallStartY(int wallnum)
{
    return wall[wallnum].y * (1 / -16.);
}

inline double WallEndX(int wallnum)
{
    return wall[wall[wallnum].point2].x * (1 / 16.);
}

inline double WallEndY(int wallnum)
{
    return wall[wall[wallnum].point2].y * (1 / -16.);
}

inline double WallStartX(const walltype* wallnum)
{
    return wallnum->x * (1 / 16.);
}

inline double WallStartY(const walltype* wallnum)
{
    return wallnum->y * (1 / -16.);
}

inline double WallEndX(const walltype* wallnum)
{
    return wall[wallnum->point2].x * (1 / 16.);
}

inline double WallEndY(const walltype* wallnum)
{
    return wall[wallnum->point2].y * (1 / -16.);
}

inline double SpriteX(int wallnum)
{
    return sprite[wallnum].x * (1 / 16.);
}

inline double SpriteY(int wallnum)
{
    return sprite[wallnum].y * (1 / -16.);
}

inline double PointOnLineSide(double x, double y, double linex, double liney, double deltax, double deltay)
{
    return (x - linex) * deltay - (y - liney) * deltax;
}

inline int sectorofwall(int wallNum)
{
    if ((unsigned)wallNum < (unsigned)numwalls) return wall[wallNum].sector;
    return -1;
}


