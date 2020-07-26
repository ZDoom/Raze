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
#include "compat.h"
#include "build.h"
#include "common.h"
#include "common_game.h"

#include "blood.h"
#include "globals.h"
#include "view.h"

BEGIN_BLD_NS


bool artLoaded = false;
int nTileFiles = 0;

int tileStart[256];
int tileEnd[256];
int hTileFile[256];

char surfType[kMaxTiles];
signed char tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];

const char *pzBaseFileName = "TILES%03i.ART"; //"TILES%03i.ART";

int tileInit(char a1, const char *a2)
{
    UNREFERENCED_PARAMETER(a1);
    if (artLoaded)
        return 1;
	TileFiles.artLoadFiles(a2 ? a2 : pzBaseFileName);
    for (int i = 0; i < kMaxTiles; i++)
        voxelIndex[i] = 0;

    auto hFile = fileSystem.OpenFileReader("SURFACE.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(surfType, sizeof(surfType));
    }
    hFile = fileSystem.OpenFileReader("VOXEL.DAT");
    if (hFile.isOpen())
    {
        hFile.Read(voxelIndex, sizeof(voxelIndex));
#if B_BIG_ENDIAN == 1
        for (int i = 0; i < kMaxTiles; i++)
            voxelIndex[i] = B_LITTLE16(voxelIndex[i]);
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
            SetBitString((char*)voxreserve, voxelIndex[i]);
    }

    artLoaded = 1;

    #ifdef USE_OPENGL
    PolymostProcessVoxels_Callback = tileProcessGLVoxels;
    #endif

    return 1;
}

#ifdef USE_OPENGL
void tileProcessGLVoxels(void)
{
    static bool voxInit = false;
    if (voxInit)
        return;
    voxInit = true;
    for (int i = 0; i < kMaxVoxels; i++)
    {
        auto index = fileSystem.FindResource(i, "KVX");
        if (index >= 0)
        {
            auto data = fileSystem.ReadFile(index);
            voxmodels[i] = loadkvxfrombuf((const char*)data.GetMem(), data.GetSize());
        }
    }
}
#endif

const uint8_t * tileLoadTile(int nTile)
{
	tileLoad(nTile);
    return (const uint8_t*)tilePtr(nTile);
}

uint8_t * tileAllocTile(int nTile, int x, int y)
{
    dassert(nTile >= 0 && nTile < kMaxTiles);
    uint8_t *p = TileFiles.tileCreate(nTile, x, y);
    dassert(p != NULL);
    return p;
}

void tilePreloadTile(int nTile)
{
    int n = 1;
    switch (picanm[nTile].extra&7)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    case 6:
    case 7:
        if (voxelIndex[nTile] < 0 || voxelIndex[nTile] >= kMaxVoxels)
        {
            voxelIndex[nTile] = -1;
            picanm[nTile].extra &= ~7;
        }
        break;
    }
    while(n--)
    {
        if (picanm[nTile].sf&PICANM_ANIMTYPE_MASK)
        {
            for (int frame = picanm[nTile].num; frame >= 0; frame--)
            {
                if ((picanm[nTile].sf&PICANM_ANIMTYPE_MASK) == PICANM_ANIMTYPE_BACK)
                    tileLoadTile(nTile-frame);
                else
                    tileLoadTile(nTile+frame);
            }
        }
        else
            tileLoadTile(nTile);
        nTile += 1+picanm[nTile].num;
    }
}

int nPrecacheCount;
char precachehightile[2][(MAXTILES+7)>>3];

void tilePrecacheTile(int nTile, int nType)
{
    int n = 1;
    switch (picanm[nTile].extra&7)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    }
    while(n--)
    {
        if (picanm[nTile].sf&PICANM_ANIMTYPE_MASK)
        {
            for (int frame = picanm[nTile].num; frame >= 0; frame--)
            {
                int tile;
                if ((picanm[nTile].sf&PICANM_ANIMTYPE_MASK) == PICANM_ANIMTYPE_BACK)
                    tile = nTile-frame;
                else
                    tile = nTile+frame;
                if (!TestBitString(gotpic, tile))
                {
                    nPrecacheCount++;
                    SetBitString(gotpic, tile);
                }
                SetBitString(precachehightile[nType], tile);
            }
        }
        else
        {
            if (!TestBitString(gotpic, nTile))
            {
                nPrecacheCount++;
                SetBitString(gotpic, nTile);
            }
            SetBitString(precachehightile[nType], nTile);
        }
        nTile += 1+picanm[nTile].num;
    }
}

char tileGetSurfType(int hit)
{
    int n = hit & 0x3fff;
    switch (hit&0xc000)
    {
    case 0x4000:
        return surfType[sector[n].floorpicnum];
    case 0x6000:
        return surfType[sector[n].ceilingpicnum];
    case 0x8000:
        return surfType[wall[n].picnum];
    case 0xc000:
        return surfType[sprite[n].picnum];
    }
    return 0;
}

END_BLD_NS
