#pragma once

#include "gamecontrol.h"
#include "binaryangle.h"
#include "build.h"

extern int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, short *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio);
void PlanesAtPoint(const sectortype* sec, float dax, float day, float* ceilz, float* florz);
void setWallSectors();
void GetWallSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out, bool render = false);
void GetFlatSpritePosition(const spritetype* spr, vec2_t pos, vec2_t* out, bool render = false);
void checkRotatedWalls();

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

inline DVector2 WallStart(const walltype* wallnum)
{
    return { WallStartX(wallnum), WallStartY(wallnum) };
}

inline double WallEndX(const walltype* wallnum)
{
    return wall[wallnum->point2].x * (1 / 16.);
}

inline double WallEndY(const walltype* wallnum)
{
    return wall[wallnum->point2].y * (1 / -16.);
}

inline DVector2 WallEnd(const walltype* wallnum)
{
    return { WallEndX(wallnum), WallEndY(wallnum) };
}

inline DVector2 WallDelta(const walltype* wallnum)
{
    return WallEnd(wallnum) - WallStart(wallnum);
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

inline double PointOnLineSide(const DVector2 &pos, const walltype *line)
{
    return (pos.X - WallStartX(line)) * WallDelta(line).Y - (pos.Y - WallStartY(line)) * WallDelta(line).X;
}

template<class T>
inline double PointOnLineSide(const TVector2<T>& pos, const TVector2<T>& linestart, const TVector2<T>& lineend)
{
    return (pos.X - linestart.X) * (lineend.Y - linestart.Y) - (pos.Y - linestart.Y) * (lineend.X - linestart.X);
}

inline int sectorofwall(int wallNum)
{
    if ((unsigned)wallNum < (unsigned)numwalls) return wall[wallNum].sector;
    return -1;
}

extern int numshades;

// Return type is int because this gets passed to variadic functions where structs may produce undefined behavior.
inline int shadeToLight(int shade)
{
    shade = clamp(shade, 0, numshades - 1);
    int light = Scale(numshades - 1 - shade, 255, numshades - 1);
    return PalEntry(255, light, light, light);
}


