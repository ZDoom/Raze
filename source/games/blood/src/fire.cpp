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

int fireSize = 128;
int gDamping = 6;

uint8_t CoolTable[1024];

void CellularFrame(uint8_t *pFrame, int sizeX, int sizeY);

static uint8_t FrameBuffer[17280];
static uint8_t SeedBuffer[16][128];
static TArray<uint8_t> gCLU;

void InitSeedBuffers(void)
{
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < fireSize; j += 2)
            SeedBuffer[i][j] = SeedBuffer[i][j+1] = wrand();
}

void BuildCoolTable(void)
{
    for (int i = 0; i < 1024; i++)
        CoolTable[i] = ClipLow((i-gDamping) / 4, 0);
}

void DoFireFrame(void)
{
    int nRand = qrand()&15;
    for (int i = 0; i < 3; i++)
    {
        memcpy(FrameBuffer+16896+i*128, SeedBuffer[nRand], 128);
    }
    CellularFrame(FrameBuffer, 128, 132);	
	auto pData = TileFiles.tileMakeWritable(2342);
    uint8_t *pSource = FrameBuffer;
    int x = fireSize;
    do
    {
        int y = fireSize;
        auto pDataBak = pData;
        do
        {
            *pData = gCLU[*pSource];
            pSource++;
            pData += fireSize;
        } while (--y);
        pData = pDataBak + 1;
    } while (--x);
}

void FireInit(void)
{
    memset(FrameBuffer, 0, sizeof(FrameBuffer));
    BuildCoolTable();
    InitSeedBuffers();
    auto fr = fileSystem.OpenFileReader("rfire.clu");
    if (!fr.isOpen())
        I_Error("RFIRE.CLU not found");
    gCLU = fr.Read();
    for (int i = 0; i < 100; i++)
        DoFireFrame();
}

void FireProcess(void)
{
	// This assumes a smooth high frame rate. Ugh...
    static int lastUpdate;
	int clock = I_GetBuildTime()/ 2;
    if (clock < lastUpdate || lastUpdate + 2 < clock)
    {
        DoFireFrame();
        lastUpdate = clock;
        TileFiles.InvalidateTile(2342);
    }
}

void CellularFrame(uint8_t *pFrame, int sizeX, int sizeY)
{
    int nSquare = sizeX * sizeY;
    uint8_t *pPtr1 = (uint8_t*)pFrame;
    while (nSquare--)
    {
        uint8_t *pPtr2 = pPtr1+sizeX;
        int sum = *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
        if (*(pPtr2+sizeX) > 96)
        {
            pPtr2 += sizeX;
            sum += *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
            sum >>= 1;
        }
        *pPtr1 = CoolTable[sum];
        pPtr1++;
    }
}

END_BLD_NS
