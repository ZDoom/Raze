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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build.h"

#include "blood.h"

BEGIN_BLD_NS


int nTileFiles = 0;

int tileStart[256];
int tileEnd[256];
int hTileFile[256];

uint8_t surfType[kMaxTiles];
int8_t tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::LoadGameTextures()
{
    auto hFile = fileSystem.OpenFileReader("SURFACE.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(surfType, sizeof(surfType));
    }
    hFile = fileSystem.OpenFileReader("VOXEL.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(voxelIndex, sizeof(voxelIndex));
#if WORDS_BIGENDIAN
        for (int i = 0; i < kMaxTiles; i++)
            voxelIndex[i] = LittleShort(voxelIndex[i]);
#endif
    }
    hFile = fileSystem.OpenFileReader("SHADE.DAT");
    if (hFile.isOpen())
    {
		hFile.Read(tileShade, sizeof(tileShade));
    }
    for (int i = 0; i < kMaxTiles; i++)
    {
        if (voxelIndex[i] >= 0 && voxelIndex[i] < kMaxVoxels)
            voxreserve.Set(voxelIndex[i]);
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int tileGetSurfType(int hit)
{
    return surfType[hit];
}

int tileGetSurfType(CollisionBase& hit)
{
    switch (hit.type)
    {
    default:
        return 0;
    case kHitSector:
        return surfType[hit.hitSector->floorpicnum];
    case kHitWall:
        return surfType[hit.hitWall->picnum];
    case kHitSprite:
        return surfType[hit.hitActor->spr.picnum];
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::SetTileProps(int tile, int surf, int vox, int shade)
{
    if (surf != INT_MAX) surfType[tile] = surf;
    if (vox != INT_MAX) voxelIndex[tile] = vox;
    if (shade != INT_MAX) tileShade[tile] = shade;
}

END_BLD_NS
