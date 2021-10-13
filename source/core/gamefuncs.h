#pragma once

#include "gamecontrol.h"
#include "binaryangle.h"
#include "build.h"

extern TArray<int> GlobalSectorList;

extern int cameradist, cameraclock;

void loaddefinitionsfile(const char* fn, bool cumulative = false, bool maingrp = false);

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, int *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio);

void PlanesAtPoint(const sectortype* sec, int dax, int day, float* ceilz, float* florz);
inline void PlanesAtPoint(const sectortype* sec, float dax, float day, float* ceilz, float* florz) // this is just for warning evasion.
{
    PlanesAtPoint(sec, int(dax), int(day), ceilz, florz);
}
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

inline double SpriteX(spritetype* spr)
{
    return spr->x * (1 / 16.);
}

inline double SpriteY(spritetype* spr)
{
    return spr->y * (1 / -16.);
}

inline DVector2 SpritePos(spritetype* spr)
{
    return { SpriteX(spr), SpriteY(spr) };
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

inline void copyfloorpal(spritetype* spr, const sectortype* sect)
{
    if (!lookups.noFloorPal(sect->floorpal)) spr->pal = sect->floorpal;
}

inline void spriteSetSlope(spritetype* spr, int heinum)
{
    int cstat = spr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK;
    if (spr->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
    {
        spr->xoffset = heinum & 255;
        spr->yoffset = (heinum >> 8) & 255;
        spr->cstat = (spr->cstat & ~CSTAT_SPRITE_ALIGNMENT_MASK) | (heinum != 0 ? CSTAT_SPRITE_ALIGNMENT_SLOPE : CSTAT_SPRITE_ALIGNMENT_FLOOR);
    }
}

inline int spriteGetSlope(spritetype* spr)
{
    return ((spr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLOPE) ? 0 : uint8_t(spr->xoffset) + (uint8_t(spr->yoffset) << 8);
}

inline int I_GetBuildTime()
{
    return I_GetTime(120);
}

inline int32_t getangle(walltype* wal)
{
    return getangle(
        wall[wal->point2].x - wal->x,
        wall[wal->point2].y - wal->y);
}

inline TArrayView<sectortype> sectors()
{
    return TArrayView<sectortype>(sector, numsectors);
}

inline TArrayView<walltype> walls()
{
    return TArrayView<walltype>(wall, numwalls);
}

inline TArrayView<walltype> wallsofsector(sectortype* sec)
{
    return TArrayView<walltype>(sec->firstWall(), sec->wallnum);
}

inline TArrayView<walltype> wallsofsector(int sec)
{
    return wallsofsector(&sector[sec]);
}
