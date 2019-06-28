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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "build.h"
#include "common.h"
#include "common_game.h"

#include "blood.h"
#include "config.h"
#include "globals.h"
#include "resource.h"
#include "tile.h"

void qloadvoxel(int32_t nVoxel)
{
    static int nLastVoxel = 0;
    dassert(nVoxel >= 0 && nVoxel < kMaxVoxels);
    DICTNODE *hVox = gSysRes.Lookup(nVoxel, "KVX");
    if (!hVox)
        ThrowError("Missing voxel #%d", nVoxel);
    if (!hVox->lockCount)
        voxoff[nLastVoxel][0] = 0;
    nLastVoxel = nVoxel;
    char *pVox = (char*)gSysRes.Lock(hVox);
    for (int i = 0; i < MAXVOXMIPS; i++)
    {
        int nSize = *((int*)pVox);
#if B_BIG_ENDIAN == 1
        nSize = B_LITTLE32(nSize);
#endif
        pVox += 4;
        voxoff[nVoxel][i] = (intptr_t)pVox;
        pVox += nSize;
    }
}

void CalcPicsiz(int a1, int a2, int a3)
{
    int nP = 0;
    for (int i = 2; i <= a2; i<<= 1)
        nP++;
    for (int i = 2; i <= a3; i<<= 1)
        nP+=1<<4;
    picsiz[a1] = nP;
}

CACHENODE tileNode[kMaxTiles];

bool artLoaded = false;
int nTileFiles = 0;

int tileStart[256];
int tileEnd[256];
int hTileFile[256];

char surfType[kMaxTiles];
signed char tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];

const char *pzBaseFileName = "TILES%03i.ART"; //"TILES%03i.ART";

int32_t MAXCACHE1DSIZE = (96*1024*1024);

int tileInit(char a1, const char *a2)
{
    UNREFERENCED_PARAMETER(a1);
    if (artLoaded)
        return 1;
    artLoadFiles(a2 ? a2 : pzBaseFileName, MAXCACHE1DSIZE);
    for (int i = 0; i < kMaxTiles; i++)
        voxelIndex[i] = 0;

    int hFile = kopen4loadfrommod("SURFACE.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, surfType, sizeof(surfType));
        kclose(hFile);
    }
    hFile = kopen4loadfrommod("VOXEL.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, voxelIndex, sizeof(voxelIndex));
#if B_BIG_ENDIAN == 1
        for (int i = 0; i < kMaxTiles; i++)
            voxelIndex[i] = B_LITTLE16(voxelIndex[i]);
#endif
        kclose(hFile);
    }
    hFile = kopen4loadfrommod("SHADE.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, tileShade, sizeof(tileShade));
        kclose(hFile);
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
        DICTNODE *hVox = gSysRes.Lookup(i, "KVX");
        if (!hVox)
            continue;
        char *pVox = (char*)gSysRes.Load(hVox);
        voxmodels[i] = loadkvxfrombuf(pVox, hVox->size);
    }
}
#endif

char * tileLoadTile(int nTile)
{
    if (!waloff[nTile]) tileLoad(nTile);
    return (char*)waloff[nTile];
}

char * tileAllocTile(int nTile, int x, int y, int ox, int oy)
{
    dassert(nTile >= 0 && nTile < kMaxTiles);
    char *p = (char*)tileCreate(nTile, x, y);
    dassert(p != NULL);
    picanm[nTile].xofs = ClipRange(ox, -127, 127);
    picanm[nTile].yofs = ClipRange(oy, -127, 127);
    return (char*)waloff[nTile];
}

void tilePreloadTile(int nTile)
{
    int n = 0;
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
        else
            qloadvoxel(voxelIndex[nTile]);
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
    int n = 0;
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
