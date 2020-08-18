//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "aistuff.h"
#include "player.h"
#include "engine.h"
#include "exhumed.h"
#include "sound.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

#define kMaxFlashes			2000
#define kMaxFlickerMask		25
#define kMaxGlows			50
#define kMaxFlickers		100
#define kMaxFlows			375

struct Flash
{
    char field_0;
    short field_1;
    int8_t shade;
};

struct Glow
{
    short field_0;
    short field_2;
    short nSector;
    short field_6;
};

struct Flicker
{
    short field_0;
    short nSector;
    unsigned int field_4;
};

struct Flow
{
    short field_0;
    short field_2;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
};

Flash sFlash[kMaxFlashes];

Glow sGlow[kMaxGlows];
short nNextFlash[kMaxFlashes];
Flicker sFlicker[kMaxFlickers];
short nFreeFlash[kMaxFlashes];
Flow sFlowInfo[kMaxFlows];
int flickermask[kMaxFlickerMask];

short bTorch = 0;
short nFirstFlash = -1;
short nLastFlash = -1;
short nFlashDepth = 2;
short nFlashes;
short nFlowCount;
short nFlickerCount;
short nGlowCount;

int bDoFlicks = 0;
int bDoGlows = 0;


static SavegameHelper sgh("lightning",
    SA(sFlash),
    SA(sGlow),
    SA(nNextFlash),
    SA(sFlicker),
    SA(nFreeFlash),
    SA(sFlowInfo),
    SA(flickermask),
    SV(bTorch),
    SV(nFirstFlash),
    SV(nLastFlash),
    SV(nFlashDepth),
    SV(nFlashes),
    SV(nFlowCount),
    SV(nFlickerCount),
    SV(nGlowCount),
    SV(bDoFlicks),
    SV(bDoGlows),
    nullptr);


// done
int GrabFlash()
{
    if (nFlashes >= kMaxFlashes) {
        return -1;
    }

    short nFlash = nFreeFlash[nFlashes];
    nNextFlash[nFlash] = -1;

    nFlashes++;

    if (nLastFlash <= -1)
    {
        nFirstFlash = nFlash;
    }
    else
    {
        nNextFlash[nLastFlash] = nFlash;
    }

    nLastFlash = nFlash;

    return nLastFlash;
}

void InitLights()
{
    int i;
    nFlickerCount = 0;

    for (i = 0; i < kMaxFlickerMask; i++) {
        flickermask[i] = RandomSize(0x1F) * 2;
    }

    nGlowCount = 0;
    nFlowCount = 0;
    nFlashes  = 0;
    bDoFlicks = false;
    bDoGlows  = false;

    for (i = 0; i < kMaxFlashes; i++) {
        nFreeFlash[i] = i;
    }

    nFirstFlash = -1;
    nLastFlash  = -1;
}

void AddFlash(short nSector, int x, int y, int z, int val)
{
    assert(nSector >= 0 && nSector < kMaxSectors);

    int var_28 = 0;
    unsigned int var_1C = val >> 8;

    if (var_1C >= nFlashDepth) {
        return;
    }

    unsigned int var_20 = val & 0x80;
    unsigned int var_18 = val & 0x40;

    val = ((var_1C + 1) << 8) | char(val);

    int var_14 = 0;

    short startwall = sector[nSector].wallptr;
    short endwall = sector[nSector].wallptr + sector[nSector].wallnum;

    for (int i = startwall; i < endwall; i++)
    {
        short wall2 = wall[i].point2;

        int xAverage = (wall[i].x + wall[wall2].x) / 2;
        int yAverage = (wall[i].y + wall[wall2].y) / 2;

        sectortype *pNextSector = NULL;
        if (wall[i].nextsector > -1) {
            pNextSector = &sector[wall[i].nextsector];
        }

        int ebx = -255;

        if (!var_18)
        {
            int x2 = x - xAverage;
            if (x2 < 0) {
                x2 = -x2;
            }

            ebx = x2;

            int y2 = y - yAverage;
            if (y2 < 0) {
                y2 = -y2;
            }

            ebx = ((y2 + ebx) >> 4) - 255;
        }

        if (ebx < 0)
        {
            var_14++;
            var_28 += ebx;

            if (wall[i].pal < 5)
            {
                if (!pNextSector || pNextSector->floorz < sector[nSector].floorz)
                {
                    short nFlash = GrabFlash();
                    if (nFlash < 0) {
                        return;
                    }

                    sFlash[nFlash].field_0 = var_20 | 2;
                    sFlash[nFlash].shade = wall[i].shade;
                    sFlash[nFlash].field_1 = i;

                    wall[i].pal += 7;

                    ebx += wall[i].shade;
                    int eax = ebx;

                    if (ebx < -127) {
                        eax = -127;
                    }

                    wall[i].shade = eax;

                    if (!var_1C && !wall[i].overpicnum && pNextSector)
                    {
                        AddFlash(wall[i].nextsector, x, y, z, val);
                    }
                }
            }
        }
    }

    if (var_14 && sector[nSector].floorpal < 4)
    {
        short nFlash = GrabFlash();
        if (nFlash < 0) {
            return;
        }

        sFlash[nFlash].field_0 = var_20 | 1;
        sFlash[nFlash].field_1 = nSector;
        sFlash[nFlash].shade = sector[nSector].floorshade;

        sector[nSector].floorpal += 7;

        int edx = sector[nSector].floorshade + var_28;
        int eax = edx;

        if (edx < -127) {
            eax = -127;
        }

        sector[nSector].floorshade = eax;

        if (!(sector[nSector].ceilingstat & 1))
        {
            if (sector[nSector].ceilingpal < 4)
            {
                short nFlash2 = GrabFlash();
                if (nFlash2 >= 0)
                {
                    sFlash[nFlash2].field_0 = var_20 | 3;
                    sFlash[nFlash2].field_1 = nSector;
                    sFlash[nFlash2].shade = sector[nSector].ceilingshade;

                    sector[nSector].ceilingpal += 7;

                    int edx = sector[nSector].ceilingshade + var_28;
                    int eax = edx;

                    if (edx < -127) {
                        eax = -127;
                    }

                    sector[nSector].ceilingshade = eax;
                }
            }
        }

        for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            if (sprite[nSprite].pal < 4)
            {
                short nFlash3 = GrabFlash();
                if (nFlash3 >= 0)
                {
                    sFlash[nFlash3].field_0 = var_20 | 4;
                    sFlash[nFlash3].shade = sprite[nSprite].shade;
                    sFlash[nFlash3].field_1 = nSprite;

                    sprite[nSprite].pal += 7;

                    int eax = -255;

                    if (!var_18)
                    {
                        int xDiff = x - sprite[nSprite].x;
                        if (xDiff < 0) {
                            xDiff = -xDiff;
                        }

                        int yDiff = y - sprite[nSprite].y;
                        if (yDiff < 0) {
                            yDiff = -yDiff;
                        }

                        eax = ((xDiff + yDiff) >> 4) - 255;
                    }

                    if (eax < 0)
                    {
                        short shade = sprite[nSprite].shade + eax;
                        if (shade < -127) {
                            shade = -127;
                        }

                        sprite[nSprite].shade = shade;
                    }
                }
            }
        }
    }
}

void UndoFlashes()
{
    if (!nFlashes) {
        return;
    }

    int var_24 = 0; // CHECKME - Watcom error "initializer for variable var_24 may not execute

    int edi = -1;

    for (short nFlash = nFirstFlash; nFlash >= 0; nFlash = nNextFlash[nFlash])
    {
        assert(nFlash < 2000 && nFlash >= 0);

        uint8_t var_28 = sFlash[nFlash].field_0 & 0x3F;
        short nIndex = sFlash[nFlash].field_1;

        if (sFlash[nFlash].field_0 & 0x80)
        {
            int var_20 = var_28 - 1;
            assert(var_20 >= 0);

            int8_t *pShade = NULL;

            switch (var_20)
            {
                case 0:
                {
                    assert(nIndex >= 0 && nIndex < kMaxSectors);

                    pShade = &sector[nIndex].floorshade;
                    break;
                }

                case 1:
                {
                    assert(nIndex >= 0 && nIndex < kMaxWalls);

                    pShade = &wall[nIndex].shade;
                    break;
                }

                case 2:
                {
                    assert(nIndex >= 0 && nIndex < kMaxSectors);

                    pShade = &sector[nIndex].ceilingshade;
                    break;
                }

                case 3:
                {
                    assert(nIndex >= 0 && nIndex < kMaxSprites);

                    if (sprite[nIndex].pal >= 7)
                    {
                        pShade = &sprite[nIndex].shade;
                    }
                    else {
                        goto loc_1868A;
                    }

                    break;
                }

                default:
                    break;
            }

            assert(pShade != NULL);

            short var_2C = (*pShade) + 6;
            int var_30 = sFlash[nFlash].shade;

            if (var_2C < var_30)
            {
                *pShade = var_2C;
                edi = nFlash;
                continue;
            }
        }

        // loc_185FE
        var_24 = var_28 - 1; // CHECKME - Watcom error "initializer for variable var_24 may not execute
        assert(var_24 >= 0);

        switch (var_24)
        {
            default:
                break;

            case 0:
            {
                sector[nIndex].floorpal -= 7;
                sector[nIndex].floorshade = sFlash[nFlash].shade;
                break;
            }

            case 1:
            {
                wall[nIndex].pal -= 7;
                wall[nIndex].shade = sFlash[nFlash].shade;
                break;
            }

            case 2:
            {
                sector[nIndex].ceilingpal -= 7;
                sector[nIndex].ceilingshade = sFlash[nFlash].shade;
                break;
            }

            case 3:
            {
                if (sprite[nIndex].pal >= 7)
                {
                    sprite[nIndex].pal -= 7;
                    sprite[nIndex].shade = sFlash[nFlash].shade;
                }

                break;
            }
        }

loc_1868A:

        nFlashes--;
        assert(nFlashes >= 0);

        nFreeFlash[nFlashes] = nFlash;

        if (edi != -1)
        {
            nNextFlash[edi] = nNextFlash[nFlash];
        }

        if (nFlash == nFirstFlash)
        {
            nFirstFlash = nNextFlash[nFirstFlash];
        }

        if (nFlash == nLastFlash)
        {
            nLastFlash = edi;
        }
    }
}

void AddGlow(short nSector, int nVal)
{
    if (nGlowCount >= kMaxGlows) {
        return;
    }

    sGlow[nGlowCount].field_6 = nVal;
    sGlow[nGlowCount].nSector = nSector;
    sGlow[nGlowCount].field_0 = -1;
    sGlow[nGlowCount].field_2 = 0;

    nGlowCount++;
}

// ok
void AddFlicker(short nSector, int nVal)
{
    if (nFlickerCount >= kMaxFlickers) {
        return;
    }

    sFlicker[nFlickerCount].field_0 = nVal;
    sFlicker[nFlickerCount].nSector = nSector;

    if (nVal >= 25) {
        nVal = 24;
    }

    sFlicker[nFlickerCount].field_4 = flickermask[nVal];

    nFlickerCount++;
}

void DoGlows()
{
    bDoGlows++;

    if (bDoGlows < 3) {
        return;
    }

    bDoGlows = 0;

    for (int i = 0; i < nGlowCount; i++)
    {
        sGlow[i].field_2++;

        short nSector = sGlow[i].nSector;
        short nShade = sGlow[i].field_0;

        if (sGlow[i].field_2 >= sGlow[i].field_6)
        {
            sGlow[i].field_2 = 0;
            sGlow[i].field_0 = -sGlow[i].field_0;
        }

        sector[nSector].ceilingshade += nShade;
        sector[nSector].floorshade   += nShade;

        int startwall = sector[nSector].wallptr;
        int endwall = startwall + sector[nSector].wallnum - 1;

        for (int nWall = startwall; nWall <= endwall; nWall++)
        {
            wall[nWall].shade += nShade;

            // CHECKME - ASM has edx decreasing here. why?
        }
    }
}

void DoFlickers()
{
    bDoFlicks ^= 1;
    if (!bDoFlicks) {
        return;
    }

    for (int i = 0; i < nFlickerCount; i++)
    {
        short nSector = sFlicker[i].nSector;

        unsigned int eax = (sFlicker[i].field_4 & 1);
        unsigned int edx = (sFlicker[i].field_4 & 1) << 31;
        unsigned int ebp = sFlicker[i].field_4 >> 1;

        ebp |= edx;
        edx = ebp & 1;

        sFlicker[i].field_4 = ebp;

        if (edx ^ eax)
        {
            short shade;

            if (eax)
            {
                shade = sFlicker[i].field_0;
            }
            else
            {
                shade = -sFlicker[i].field_0;
            }

            sector[nSector].ceilingshade += shade;
            sector[nSector].floorshade += shade;

            int startwall = sector[nSector].wallptr;
            int endwall = startwall + sector[nSector].wallnum - 1;

            for (int nWall = endwall; nWall >= startwall; nWall--)
            {
                wall[nWall].shade += shade;

                // CHECKME - ASM has edx decreasing here. why?
            }
        }
    }
}

// nWall can also be passed in here via nSprite parameter - TODO - rename nSprite parameter :)
void AddFlow(int nSprite, int nSpeed, int b)
{
    if (nFlowCount >= kMaxFlows)
        return;

    short nFlow = nFlowCount;
    nFlowCount++;

    short var_18;

    if (b < 2)
    {
        var_18 = sprite[nSprite].sectnum;
        short nPic = sector[var_18].floorpicnum;
        short nAngle = sprite[nSprite].ang;

        sFlowInfo[nFlow].field_14 = (tilesiz[nPic].x << 14) - 1;
        sFlowInfo[nFlow].field_18 = (tilesiz[nPic].y << 14) - 1;
        sFlowInfo[nFlow].field_C  = -Cos(nAngle) * nSpeed;
        sFlowInfo[nFlow].field_10 = Sin(nAngle) * nSpeed;
    }
    else
    {
        short nAngle;

        if (b == 2) {
            nAngle = 512;
        }
        else {
            nAngle = 1536;
        }

        var_18 = nSprite;
        short nPic = wall[var_18].picnum;

        sFlowInfo[nFlow].field_14 = (tilesiz[nPic].x * wall[var_18].xrepeat) << 8;
        sFlowInfo[nFlow].field_18 = (tilesiz[nPic].y * wall[var_18].yrepeat) << 8;
        sFlowInfo[nFlow].field_C = -Cos(nAngle) * nSpeed;
        sFlowInfo[nFlow].field_10 = Sin(nAngle) * nSpeed;
    }

    sFlowInfo[nFlow].field_8 = 0;
    sFlowInfo[nFlow].field_4 = 0;
    sFlowInfo[nFlow].field_0 = var_18;
    sFlowInfo[nFlow].field_2 = b;
}

void DoFlows()
{
    for (int i = 0; i < nFlowCount; i++)
    {
        sFlowInfo[i].field_4 += sFlowInfo[i].field_C;
        sFlowInfo[i].field_8 += sFlowInfo[i].field_10;

        switch (sFlowInfo[i].field_2)
        {
            case 0:
            {
                sFlowInfo[i].field_4 &= sFlowInfo[i].field_14;
                sFlowInfo[i].field_8 &= sFlowInfo[i].field_18;

                short nSector = sFlowInfo[i].field_0;
                sector[nSector].floorxpanning = sFlowInfo[i].field_4 >> 14;
                sector[nSector].floorypanning = sFlowInfo[i].field_8 >> 14;
                break;
            }

            case 1:
            {
                short nSector = sFlowInfo[i].field_0;

                sector[nSector].ceilingxpanning = sFlowInfo[i].field_4 >> 14;
                sector[nSector].ceilingypanning = sFlowInfo[i].field_8 >> 14;

                sFlowInfo[i].field_4 &= sFlowInfo[i].field_14;
                sFlowInfo[i].field_8 &= sFlowInfo[i].field_18;
                break;
            }

            case 2:
            {
                short nWall = sFlowInfo[i].field_0;

                wall[nWall].xpanning = sFlowInfo[i].field_4 >> 14;
                wall[nWall].ypanning = sFlowInfo[i].field_8 >> 14;

                if (sFlowInfo[i].field_4 < 0)
                {
                    sFlowInfo[i].field_4 += sFlowInfo[i].field_14;
                }

                if (sFlowInfo[i].field_8 < 0)
                {
                    sFlowInfo[i].field_8 += sFlowInfo[i].field_18;
                }

                break;
            }

            case 3:
            {
                short nWall = sFlowInfo[i].field_0;

                wall[nWall].xpanning = sFlowInfo[i].field_4 >> 14;
                wall[nWall].ypanning = sFlowInfo[i].field_8 >> 14;

                if (sFlowInfo[i].field_4 >= sFlowInfo[i].field_14)
                {
                    sFlowInfo[i].field_4 -= sFlowInfo[i].field_14;
                }

                if (sFlowInfo[i].field_8 >= sFlowInfo[i].field_18)
                {
                    sFlowInfo[i].field_8 -= sFlowInfo[i].field_18;
                }

                break;
            }
        }
    }
}

void DoLights()
{
    DoFlickers();
    DoGlows();
    DoFlows();
}

void SetTorch(int nPlayer, int bTorchOnOff)
{
    char buf[40];

    if (bTorchOnOff == bTorch) {
        return;
    }

    if (nPlayer != nLocalPlayer) {
        return;
    }

    if (bTorchOnOff == 2) {
        bTorch = !bTorch;
    }
    else {
        bTorch = bTorchOnOff;
    }

    if (bTorch) {
        PlayLocalSound(StaticSound[kSoundTorchOn], 0);
    }

    strcpy(buf, "TORCH IS ");

    if (bTorch) {
        strcat(buf, "LIT");
    }
    else {
        strcat(buf, "OUT");
    }

    StatusMessage(150, buf);
}

void BuildFlash(short nPlayer, short UNUSED(nSector), int nVal)
{
    if (nPlayer == nLocalPlayer)
    {
        flash = nVal;
        flash = -nVal; // ???
    }
}

END_PS_NS
