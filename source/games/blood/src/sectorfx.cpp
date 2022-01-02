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
#include "ns.h"	// Must come before everything else!

#include "build.h"

#include "blood.h"
#include "interpolate.h"

BEGIN_BLD_NS

static const uint8_t flicker1[] = {
	0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0,
	1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1
};

static const uint8_t flicker2[] = {
	1, 2, 4, 2, 3, 4, 3, 2, 0, 0, 1, 2, 4, 3, 2, 0,
	2, 1, 0, 1, 0, 2, 3, 4, 3, 2, 1, 1, 2, 0, 0, 1,
	1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 4, 2, 1, 0, 1,
	0, 0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2
};

static const uint8_t flicker3[] = {
	4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 2, 4, 3, 4, 4,
	4, 4, 2, 1, 3, 3, 3, 4, 3, 4, 4, 4, 4, 4, 2, 4,
	4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 0, 1,
	0, 1, 0, 1, 0, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 4
};

static const uint8_t flicker4[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 0, 0, 3, 0, 1, 0, 1, 0, 4, 4, 4, 4, 4, 2, 0,
	0, 0, 0, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 4, 3, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0, 0, 0 ,0, 0
};

static const uint8_t strobe[] = {
	64, 64, 64, 48, 36, 27, 20, 15, 11, 9, 6, 5, 4, 3, 2, 2,
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GetWaveValue(int a, int b, int c)
{
	b &= 2047;
	switch (a)
	{
	case 0:
		return c;
	case 1:
		return (b >> 10) * c;
	case 2:
		return (abs(128 - (b >> 3)) * c) >> 7;
	case 3:
		return ((b >> 3) * c) >> 8;
	case 4:
		return ((255 - (b >> 3)) * c) >> 8;
	case 5:
		return (c + MulScale(c, Sin(b), 30)) >> 1;
	case 6:
		return flicker1[b >> 5] * c;
	case 7:
		return (flicker2[b >> 5] * c) >> 2;
	case 8:
		return (flicker3[b >> 5] * c) >> 2;
	case 9:
		return (flicker4[b >> 4] * c) >> 2;
	case 10:
		return (strobe[b >> 5] * c) >> 6;
	case 11:
		if (b * 4 > 2048)
			return 0;
		return (c - MulScale(c, Cos(b * 4), 30)) >> 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// These can be fully regenerated after loading a savegame.
//
//---------------------------------------------------------------------------

TArray<sectortype*> shadeList;
TArray<sectortype*> panList;
TArray<walltype*> wallPanList;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSectorLighting(void)
{
	for (auto& pSector : shadeList)
	{
		XSECTOR* pXSector = &pSector->xs();
		if (pXSector->shade)
		{
			int v4 = pXSector->shade;
			if (pXSector->shadeFloor)
			{
				pSector->floorshade -= v4;
				if (pXSector->color)
				{
					int nTemp = pXSector->floorpal;
					pXSector->floorpal = pSector->floorpal;
					pSector->floorpal = nTemp;
				}
			}
			if (pXSector->shadeCeiling)
			{
				pSector->ceilingshade -= v4;
				if (pXSector->color)
				{
					int nTemp = pXSector->ceilpal;
					pXSector->ceilpal = pSector->ceilingpal;
					pSector->ceilingpal = nTemp;
				}
			}
			if (pXSector->shadeWalls)
			{
				for (auto& wal : wallsofsector(pSector))
				{
					wal.shade -= v4;
					if (pXSector->color)
					{
						wal.pal = pSector->floorpal;
					}
				}
			}
			pXSector->shade = 0;
		}
		if (pXSector->shadeAlways || pXSector->busy)
		{
			int t1 = pXSector->wave;
			int t2 = pXSector->amplitude;
			if (!pXSector->shadeAlways && pXSector->busy)
			{
				t2 = MulScale(t2, pXSector->busy, 16);
			}
			int v4 = GetWaveValue(t1, pXSector->phase * 8 + pXSector->freq * PlayClock, t2);
			if (pXSector->shadeFloor)
			{
				pSector->floorshade = ClipRange(pSector->floorshade + v4, -128, 127);
				if (pXSector->color && v4 != 0)
				{
					int nTemp = pXSector->floorpal;
					pXSector->floorpal = pSector->floorpal;
					pSector->floorpal = nTemp;
				}
			}
			if (pXSector->shadeCeiling)
			{
				pSector->ceilingshade = ClipRange(pSector->ceilingshade + v4, -128, 127);
				if (pXSector->color && v4 != 0)
				{
					int nTemp = pXSector->ceilpal;
					pXSector->ceilpal = pSector->ceilingpal;
					pSector->ceilingpal = nTemp;
				}
			}
			if (pXSector->shadeWalls)
			{
				for (auto& wal : wallsofsector(pSector))
				{
					wal.shade = ClipRange(wal.shade + v4, -128, 127);
					if (pXSector->color && v4 != 0)
					{
						wal.pal = pSector->floorpal;
					}
				}
			}
			pXSector->shade = v4;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void UndoSectorLighting(void)
{
	for (auto& sect : sector)
	{
		if (sect.hasX())
		{
			XSECTOR* pXSector = &sect.xs();
			if (pXSector->shade)
			{
				int v4 = pXSector->shade;
				if (pXSector->shadeFloor)
				{
					sect.floorshade -= v4;
					if (pXSector->color)
					{
						int nTemp = pXSector->floorpal;
						pXSector->floorpal = sect.floorpal;
						sect.floorpal = nTemp;
					}
				}
				if (pXSector->shadeCeiling)
				{
					sect.ceilingshade -= v4;
					if (pXSector->color)
					{
						int nTemp = pXSector->ceilpal;
						pXSector->ceilpal = sect.ceilingpal;
						sect.ceilingpal = nTemp;
					}
				}
				if (pXSector->shadeWalls)
				{
					for (auto& wal : wallsofsector(&sect))
					{
						wal.shade -= v4;
						if (pXSector->color)
						{
							wal.pal = sect.floorpal;
						}
					}
				}
				pXSector->shade = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoSectorPanning(void)
{
	for (auto pSector : panList)
	{
		XSECTOR* pXSector = &pSector->xs();
		if (pXSector->panAlways || pXSector->busy)
		{
			int angle = pXSector->panAngle + 1024;
			int speed = pXSector->panVel << 10;
			if (!pXSector->panAlways && (pXSector->busy & 0xffff))
				speed = MulScale(speed, pXSector->busy, 16);

			if (pXSector->panFloor) // Floor
			{
				int nTile = pSector->floorpicnum;
				if (pSector->floorstat & CSTAT_SECTOR_ALIGN)
					angle -= 512;
				int xBits = tileWidth(nTile) >> int((pSector->floorstat & CSTAT_SECTOR_TEXHALF) != 0);
				int px = MulScale(speed << 2, Cos(angle), 30) / xBits;
				int yBits = tileHeight(nTile) >> int((pSector->floorstat & CSTAT_SECTOR_TEXHALF) != 0);
				int py = MulScale(speed << 2, Sin(angle), 30) / yBits;
				pSector->addfloorxpan(px * (1.f / 256));
				pSector->addfloorypan(-py * (1.f / 256));
			}
			if (pXSector->panCeiling) // Ceiling
			{
				int nTile = pSector->ceilingpicnum;
				if (pSector->ceilingstat & CSTAT_SECTOR_ALIGN)
					angle -= 512;
				int xBits = tileWidth(nTile) >> int((pSector->ceilingstat & CSTAT_SECTOR_TEXHALF) != 0);
				int px = MulScale(speed << 2, Cos(angle), 30) / xBits;
				int yBits = tileHeight(nTile) >> int((pSector->ceilingstat & CSTAT_SECTOR_TEXHALF) != 0);
				int py = MulScale(speed << 2, Sin(angle), 30) / yBits;
				pSector->addceilingxpan(px * (1.f / 256));
				pSector->addceilingypan(-py * (1.f / 256));
			}
		}
	}
	for (auto pWall : wallPanList)
	{
		XWALL* pXWall = &pWall->xw();
		if (pXWall->panAlways || pXWall->busy)
		{
			int psx = pXWall->panVel.X << 10;
			int psy = pXWall->panVel.Y << 10;
			if (!pXWall->panAlways && (pXWall->busy & 0xffff))
			{
				psx = MulScale(psx, pXWall->busy, 16);
				psy = MulScale(psy, pXWall->busy, 16);
			}
			int nTile = pWall->picnum;
			int px = (psx << 2) / tileWidth(nTile);
			int py = (psy << 2) / tileHeight(nTile);

			pWall->addxpan(px * (1.f / 256));
			pWall->addypan(py * (1.f / 256));
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void InitSectorFX(void)
{
	shadeList.Clear();
	panList.Clear();
	wallPanList.Clear();
	for (auto& sect : sector)
	{
		if (sect.hasX())
		{
			XSECTOR* pXSector = &sect.xs();
			if (pXSector->amplitude)
				shadeList.Push(&sect);
			if (pXSector->panVel)
			{
				panList.Push(&sect);

				if (pXSector->panCeiling)
				{
					StartInterpolation(&sect, Interp_Sect_CeilingPanX);
					StartInterpolation(&sect, Interp_Sect_CeilingPanY);
				}
				if (pXSector->panFloor)
				{
					StartInterpolation(&sect, Interp_Sect_FloorPanX);
					StartInterpolation(&sect, Interp_Sect_FloorPanY);
				}

			}
		}
	}
	for (auto& wal : wall)
	{
		if (wal.hasX())
		{
			XWALL* pXWall = &wal.xw();
			if (pXWall->panVel.X || pXWall->panVel.Y)
			{
				wallPanList.Push(&wal);
				if (pXWall->panVel.X) StartInterpolation(&wal, Interp_Wall_PanX);
				if (pXWall->panVel.Y) StartInterpolation(&wal, Interp_Wall_PanY);
			}
		}
	}
}


END_BLD_NS
