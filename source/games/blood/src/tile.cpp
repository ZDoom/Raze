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

uint8_t surfType[kMaxTiles];
int8_t tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];

struct TextureProps
{
    uint8_t surfType;
    int8_t tileShade;
    int16_t voxelIndex;
};

TArray<TextureProps> tprops;

#define x(a, b) registerName(#a, b);
static void SetTileNames()
{
    auto registerName = [](const char* name, int index)
    {
        TileFiles.addName(name, index);
    };
#include "namelist.h"
    // Oh Joy! Plasma Pak changes the tile number of the title screen, but we preferably want mods that use the original one to display it.
    // So let's make this remapping depend on the CRC.
    if (tileGetCRC32(2518) == 1170870757 && (tileGetCRC32(2046) != 290208654 || tileWidth(2518) == 0)) registerName("titlescreen", 2046);
    else registerName("titlescreen", 2518);
}
#undef x

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
        if (voxelIndex[i] >= 0 && voxelIndex[i] < MAXVOXELS)
            voxreserve.Set(voxelIndex[i]);
    }
    SetTileNames();
}

void GameInterface::SetupSpecialTextures()
{
    // set up all special tiles here, before we fully hook up with the texture manager.
    tileDelete(504);
    TileFiles.tileCreate(4077, kLensSize, kLensSize);
    TileFiles.tileCreate(4079, 128, 128);
    TileFiles.tileMakeWritable(2342);
    TileFiles.lock();   // from this point on the tile<->texture associations may not change anymore.
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
