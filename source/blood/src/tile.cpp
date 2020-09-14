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

void GameInterface::SetTileProps(int tile, int surf, int vox, int shade)
{
    if (surf != INT_MAX) surfType[tile] = surf;
    if (vox != INT_MAX) voxelIndex[tile] = vox;
    if (shade != INT_MAX) tileShade[tile] = shade;
}

END_BLD_NS
